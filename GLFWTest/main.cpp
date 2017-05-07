// includes
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include "glad/glad.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
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
static Input input;

GLuint texId;

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
int scores[2];


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

GLuint loadBMP_custom(const char * imagepath);


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
	if (IsCollidingSqSq(ball, bar0))
	{
		if (ball.x > bar0.x)
		{
			dir[0] *= -1;
			ball.x = bar0.x + bar0.width / 2 + ball.width / 2;
		}
		else
		{
			ball.x = bar0.x - bar0.width / 2 - ball.width / 2;
		}
	}
	if (IsCollidingSqSq(ball, bar1))
	{
		if (ball.x < bar1.x)
		{
			dir[0] *= -1;
			ball.x = bar1.x - bar1.width / 2 - ball.width / 2;
		}
		else
		{
			ball.x = bar1.x + bar1.width / 2 + ball.width / 2;
		}
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

	// 両端に行ってたら真ん中からリスポーン
	if (ball.x > X_LIMIT)
	{
		ball.x = ball.y = 0;
		++scores[0];
	}
	if (ball.x < -X_LIMIT)
	{
		ball.x = ball.y = 0;
		++scores[1];
	}
}

GLuint LoadTexture(const char* path)
{
	int width, height, comp;
	unsigned char* image = stbi_load(path, &width, &height, &comp, STBI_rgb_alpha);
	if (image == nullptr)
	{
		std::cerr << "failed to load image\n";
		return -1;
	}

	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (comp == 3)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	}
	else if (comp == 4)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	}


	return texID;
}

GLuint InitTexture(const char* path)
{
	glGenTextures(1, &texId);

	static const int TEXHEIGHT = 128;
	static const int TEXWIDTH = 128;
	/* テクスチャの読み込みに使う配列 */
	GLubyte texture[TEXHEIGHT * TEXWIDTH * 3];
	FILE *fp;

	/* テクスチャ画像の読み込み */
	if ((fp = fopen(path, "rb")) != NULL)
	{
		fread(texture, sizeof texture, 1, fp);
		fclose(fp);
	}
	else
	{
		perror(path);
	}

	/* テクスチャ画像はバイト単位に詰め込まれている */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	/* テクスチャの割り当て */
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXWIDTH, TEXHEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);

	/* テクスチャを拡大・縮小する方法の指定 */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	/* 初期設定 */
	glClearColor(0.3, 0.3, 1.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	/* 光源の初期設定 */
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	//glLightfv(GL_LIGHT0, GL_DIFFUSE, lightcol);
	//glLightfv(GL_LIGHT0, GL_SPECULAR, lightcol);
	//glLightfv(GL_LIGHT0, GL_AMBIENT, lightamb);

	return texId;
}

void Scene()
{
	static const GLfloat color[] = { 1.0, 1.0, 1.0, 1.0 };  /* 材質 (色) */
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);/* 材質の設定 */
	static const GLfloat vtx[] = {
		0, 0,
		0, 1,
		1, 0,
		1, 1,
	};
	glVertexPointer(2, GL_FLOAT, 0, vtx);

	// Step5. テクスチャの領域指定
	static const GLfloat texuv[] = {
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
	};
	glTexCoordPointer(2, GL_FLOAT, 0, texuv);

	// Step6. テクスチャの画像指定
	glBindTexture(GL_TEXTURE_2D, texId);

	// Step7. テクスチャの描画
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glDrawArrays(GL_QUADS, 0, 4);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_TEXTURE_2D);

	return;

	/* テクスチャマッピング開始 */
	glEnable(GL_TEXTURE_2D);

	/* １枚の４角形を描く */
	glNormal3d(0.0, 0.0, 1.0);
	glBegin(GL_QUADS);
	glTexCoord2d(0.0, 1.0);
	glVertex3d(-1.0, -1.0, 0.0);
	glTexCoord2d(1.0, 1.0);
	glVertex3d(1.0, -1.0, 0.0);
	glTexCoord2d(1.0, 0.0);
	glVertex3d(1.0, 1.0, 0.0);
	glTexCoord2d(0.0, 0.0);
	glVertex3d(-1.0, 1.0, 0.0);
	glEnd();

	/* テクスチャマッピング終了 */
	glDisable(GL_TEXTURE_2D);
}

// ENTRY POINT
int main_()
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

	//GLuint image = loadBMP_custom("test.bmp");
	InitTexture(R"(C:\Users\Freis\Desktop\GLFWTest\x64\Debug\cat.raw)");

	// main loop
	while (!glfwWindowShouldClose(window))
	{
		Scene();
		//LoadTexture("num.png");
		if(false)
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
		}
		// end
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}


GLuint loadBMP_custom(const char * imagepath)
{
	// Data read from the header of the BMP file
	unsigned char header[54]; // Each BMP file begins by a 54-bytes header
	unsigned int dataPos;     // Position in the file where the actual data begins
	unsigned int width, height;
	unsigned int imageSize;   // = width*height*3
							  // Actual RGB data
	unsigned char * data;

	// Open the file
	FILE * file = fopen(imagepath, "rb");
	if (!file) { printf("Image could not be opened\n"); return 0; }

	if (fread(header, 1, 54, file) != 54)
	{ // If not 54 bytes read : problem
		printf("Not a correct BMP file\n");
		return false;
	}

	if (header[0] != 'B' || header[1] != 'M')
	{
		printf("Not a correct BMP file\n");
		return 0;
	}

	// Read ints from the byte array
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);

	// Some BMP files are misformatted, guess missing information
	if (imageSize == 0)    imageSize = width*height * 3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54; // The BMP header is done that way

										 // Create a buffer
	data = new unsigned char[imageSize];

	// Read the actual data from the file into the buffer
	fread(data, 1, imageSize, file);

	//Everything is in memory now, the file can be closed
	fclose(file);

	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	return textureID;
}

