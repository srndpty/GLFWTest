// includes
#include <iostream>
#include "glad/glad.h"

#include <GLFW/glfw3.h>

#include "linmath.h"
#include <complex>

// this line below prevents console window to pop up
//#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\" ")

static constexpr float PI = 3.14159265f;

// structs
struct Extent
{
	float x, y, width, height;
};

struct KeyState
{
	bool pressed;
	bool lastPressed;
};

class Input
{
public:
	static constexpr int KEY_MAX = 512;

	void Update()
	{
		for (size_t i = 0; i < KEY_MAX; i++)
		{
			mKeyStates[i].lastPressed = mKeyStates[i].pressed;
		}
	}

public:
	KeyState mKeyStates[KEY_MAX];
};
Input input;


struct Vertex
{
	float x, y;
	float r, g, b;
};

Vertex sqVerts[4] =
{
	{ -0.1f, +0.1f, 0.f, 1.f, 1.f },
	{ +0.1f, +0.1f, 0.f, 0.f, 1.f },
	{ +0.1f, -0.1f, 1.f, 0.f, 0.f },
	{ -0.1f, -0.1f, 1.f, 1.f, 0.f },
};
static constexpr float BAR_THICKNESS = 0.1f;
static constexpr float BAR_HEIGHT = 0.5f;
static constexpr float BAR_LIMIT = 1.0f - BAR_HEIGHT / 2;
Vertex bar[4] =
{
	{ -BAR_THICKNESS / 2, +BAR_HEIGHT / 2, 0.f, 1.f, 1.f },
	{ +BAR_THICKNESS / 2, +BAR_HEIGHT / 2, 0.f, 0.f, 1.f },
	{ +BAR_THICKNESS / 2, -BAR_HEIGHT / 2, 1.f, 0.f, 0.f },
	{ -BAR_THICKNESS / 2, -BAR_HEIGHT / 2, 1.f, 1.f, 0.f },
};

const int CIRCLE_VERTEX_DIVISION = 36;
const int CIRCLE_VERTEX_COUNT = CIRCLE_VERTEX_DIVISION + 2; // 中心と最後の重複点
Vertex circleVerts[CIRCLE_VERTEX_COUNT];
GLFWwindow* window;

static constexpr float BALL_SPEED = 0.02f;
static constexpr float BALL_RADIUS = 0.1f;
static constexpr float BALL_LIMIT = 1.0f - BALL_RADIUS;
static constexpr float X_LIMIT = 1.4f;
static constexpr float SPEED = 0.02f;
static constexpr float START_DIR = 2 * PI * 0.7f;
Extent bar0;
Extent bar1;
Extent ball;
vec2 dir{ cos(START_DIR), sin(START_DIR) };

// shader
static const char* VERTEX_SHADER_TEXT =
R"delimiter(
uniform mat4 MVP;
attribute vec3 vCol;
attribute vec2 vPos;
varying vec3 color;

void main()
{
	gl_Position = MVP * vec4(vPos, 0.0, 1.0);
	color = vCol;
}
)delimiter";

static const char* FRAGMENT_SHADER_TEXT = 
R"delimiter(
varying vec3 color;

void main()
{
	gl_FragColor = vec4(color, 1.0);
}
)delimiter";

// インデックスに応じて色用の値を取得
float GetColorValue(int index, int maxCount, int offset)
{
	return max((fabs((((index + offset) % maxCount) * 2.0f / maxCount) - 1.0f) * 3.0f) - 1.0f, 0);
}

// 頂点座標の設定
void SetVertices()
{
	// bar 
	bar0.width  = BAR_THICKNESS;
	bar0.height = BAR_HEIGHT;
	bar1.width  = BAR_THICKNESS;
	bar1.height = BAR_HEIGHT;

	// circle
	ball.x = ball.y = 0;
	ball.width = ball.height = BALL_RADIUS * 2;
	circleVerts[0].x = 0;
	circleVerts[0].y = 0;
	circleVerts[0].r = 1.0f;
	circleVerts[0].g = 1.0f;
	circleVerts[0].b = 1.0f;
	for (int i = 0; i <= CIRCLE_VERTEX_DIVISION; i++)
	{
		circleVerts[i + 1].x = cos(PI * 2 / CIRCLE_VERTEX_DIVISION * i) * BALL_RADIUS;
		circleVerts[i + 1].y = sin(PI * 2 / CIRCLE_VERTEX_DIVISION * i) * BALL_RADIUS;
		circleVerts[i + 1].r = GetColorValue(i, CIRCLE_VERTEX_DIVISION, CIRCLE_VERTEX_DIVISION / 3 * 0);
		circleVerts[i + 1].g = GetColorValue(i, CIRCLE_VERTEX_DIVISION, CIRCLE_VERTEX_DIVISION / 3 * 1);
		circleVerts[i + 1].b = GetColorValue(i, CIRCLE_VERTEX_DIVISION, CIRCLE_VERTEX_DIVISION / 3 * 2);
	}
}

// エラーコールバック
void ErrorCallback(int error, const char* description)
{
	std::cerr << "Error Occured code: " << error << " desc: " << description << "\n";
}

// 入力コールバック
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	input.Update();
	if (action == GLFW_PRESS)
	{
		input.mKeyStates[key].pressed = true;
	}
	else if (action == GLFW_RELEASE)
	{
		input.mKeyStates[key].pressed = false;
	}
}

// 左下基準
bool IsCollidingSqSq(const Extent& a, const Extent& b)
{
	if (a.x - a.width / 2 > b.x - b.width / 2 - a.width)
	{
		if (a.x - a.width / 2 < b.x + b.width / 2)
		{
			if (a.y - a.height / 2 > b.y - b.height / 2 - a.height)
			{
				if (a.y - a.height / 2 < b.y + b.height / 2)
				{
					return true;
				}
			}
		}
	}

	return false;
}

// 入力マイフレーム制御
void ProcessInputs()
{
	// 左
	if (input.mKeyStates[GLFW_KEY_W].pressed)
	{
		bar0.y += SPEED;
	}
	else if (input.mKeyStates[GLFW_KEY_S].pressed)
	{
		bar0.y -= SPEED;
	}
	bar0.y = max(-BAR_LIMIT, min(bar0.y, BAR_LIMIT));

	// 右
	if (input.mKeyStates[GLFW_KEY_UP].pressed)
	{
		bar1.y += SPEED;
	}
	else if (input.mKeyStates[GLFW_KEY_DOWN].pressed)
	{
		bar1.y -= SPEED;
	}
	bar1.y = max(-BAR_LIMIT, min(bar1.y, BAR_LIMIT));

	// 終了
	if (input.mKeyStates[GLFW_KEY_ESCAPE].pressed)
	{
		glfwSetWindowShouldClose(window, true);
	}
}

void UpdateBall()
{
	// バーに当たったら反射
	if (IsCollidingSqSq(ball, bar0) && ball.x > bar0.x)
	{
		dir[0] *= -1;
		ball.x = bar0.x + bar0.width / 2 + ball.width / 2;
	}
	else if (IsCollidingSqSq(ball, bar1) && ball.x < bar1.x)
	{
		dir[0] *= -1;
		ball.x = bar1.x - bar0.width / 2 - ball.width / 2;
	}


	// 反射
	else if (ball.x < -X_LIMIT)
	{
		dir[0] *= -1;
	}
	else if (ball.x > X_LIMIT)
	{
		dir[0] *= -1;
	}
	else if (ball.y < -BALL_LIMIT)
	{
		dir[1] *= -1;
	}
	else if (ball.y > BALL_LIMIT)
	{
		dir[1] *= -1;
	}

	// 移動
	ball.x += dir[0] * BALL_SPEED;
	ball.y += dir[1] * BALL_SPEED;
}

// ENTRY POINT
int main()
{
	static const int VERTEX_BUFFER_COUNT = 4;
	GLuint vertexBuffers[VERTEX_BUFFER_COUNT];
	GLuint vertexShader, fragmentShader, program;
	GLint mvpLocation, vposLocation, vcolLocation;

	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW\n";
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(640, 480, "sample", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cerr << "Failed to create GLFW window\n";
		glfwTerminate();
		return 1;
	}

	// set vertices
	SetVertices();
	
	// set callbacks
	glfwSetErrorCallback(ErrorCallback);
	glfwSetKeyCallback(window, KeyCallback);

	glfwMakeContextCurrent(window);

	// load
	auto addr = (GLADloadproc)glfwGetProcAddress;
	
	gladLoadGLLoader(addr);
	glfwSwapInterval(1);

	// NOTE: OpenGL error checks has been omitted for brevity
	glGenBuffers(VERTEX_BUFFER_COUNT, vertexBuffers);

	// set shader 
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &VERTEX_SHADER_TEXT, nullptr);
	glCompileShader(vertexShader);

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &FRAGMENT_SHADER_TEXT, nullptr);
	glCompileShader(fragmentShader);

	program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	mvpLocation = glGetUniformLocation(program, "MVP");
	vposLocation = glGetAttribLocation(program, "vPos");
	vcolLocation = glGetAttribLocation(program, "vCol");

	glUseProgram(program);

	bar0.x = -1.0f;
	bar1.x = +1.0f;

	// main loop
	while (!glfwWindowShouldClose(window))
	{
		ProcessInputs();
		UpdateBall();

		float ratio;
		int width, height;
		mat4x4 m, p, mvp;

		glfwGetFramebufferSize(window, &width, &height);
		ratio = static_cast<float>(width) / height;

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.5f, 0.5f, 0.5f, 1);

		// 1 left bar wsad
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(bar), bar, GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(vposLocation);
		glVertexAttribPointer(vposLocation, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(0));
		glEnableVertexAttribArray(vcolLocation);
		glVertexAttribPointer(vcolLocation, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 2));

		mat4x4_identity(m);
		mat4x4_translate_in_place(m, bar0.x, bar0.y, 0);
		//mat4x4_rotate_Z(m, m, (float)glfwGetTime());
		mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
		mat4x4_mul(mvp, p, m);

		glUniformMatrix4fv(mvpLocation, 1, false, (const GLfloat*)mvp);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		// 2 right bar arrows
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(bar), bar, GL_DYNAMIC_DRAW);

		mat4x4_identity(m);
		mat4x4_translate_in_place(m, bar1.x, bar1.y, 0);
		//mat4x4_rotate_Z(m, m, (float)glfwGetTime());
		mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
		mat4x4_mul(mvp, p, m);

		glEnableVertexAttribArray(vposLocation);
		glVertexAttribPointer(vposLocation, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(0));
		glEnableVertexAttribArray(vcolLocation);
		glVertexAttribPointer(vcolLocation, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 2));

		glUseProgram(program);
		glUniformMatrix4fv(mvpLocation, 1, false, (const GLfloat*)mvp);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		// 4 circle
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[3]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(circleVerts), circleVerts, GL_DYNAMIC_DRAW);

		mat4x4_identity(m);
		mat4x4_translate_in_place(m, ball.x, ball.y, 0);
		mat4x4_rotate_Z(m, m, (float)glfwGetTime() * 2);
		mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
		mat4x4_mul(mvp, p, m);

		glEnableVertexAttribArray(vposLocation);
		glVertexAttribPointer(vposLocation, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(0));
		glEnableVertexAttribArray(vcolLocation);
		glVertexAttribPointer(vcolLocation, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 2));

		glUniformMatrix4fv(mvpLocation, 1, false, (const GLfloat*)mvp);
		glDrawArrays(GL_TRIANGLE_FAN, 0, CIRCLE_VERTEX_COUNT);

		// end
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

