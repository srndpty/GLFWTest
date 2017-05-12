#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <numeric>
#include <memory>
#include "linmath.h"
#include "Shader.h"

#undef min
#undef max

using namespace std;
struct Vec2
{
	float x, y;

public:
	Vec2() = default;

	Vec2(float a, float b)
		: x(a)
		, y(b)
	{
	}

	Vec2 operator*(float a)
	{
		return{ x * a, y * a };
	}
};

static constexpr float PI = 3.14159265358f;
static Vec2 WINDOW_SIZE = { 640.f, 480.f };
static float ASPECT_RATIO = WINDOW_SIZE.x / WINDOW_SIZE.y;
static Vec2 BAR_SIZE = { 0.1f, 0.5f };
static constexpr int BALL_VERTS_COUNT = 32;
static constexpr int BAR_VERTS_COUNT = 4;

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
Shader shader;

// base class for sprite object
template<int I>
class Sprite
{
public:
	Sprite() = default;
	virtual ~Sprite() = default;

	mat4x4 m, p, mvp;

	void Draw(GLuint texId)
	{
		mat4x4_identity(m);
		mat4x4_translate_in_place(m, pos.x, pos.y, 0);
		mat4x4_ortho(p, -ASPECT_RATIO, ASPECT_RATIO, -1.f, 1.f, 1.f, -1.f);
		mat4x4_mul(mvp, p, m);

		glUniformMatrix4fv(shader.mMvpLocation, 1, false, (const GLfloat*)mvp);
		// attribute属性を登録
		glVertexAttribPointer(shader.mPositionLocation, 2, GL_FLOAT, false, 0, geom);
		glVertexAttribPointer(shader.mUvLocation, 2, GL_FLOAT, false, 0, uv);

		// モデルの描画
		glBindTexture(GL_TEXTURE_2D, texId);
		glDrawArrays(GL_TRIANGLE_FAN, 0, I);
	}

public:
	Vec2 size{};
	Vec2 pos{}; // 座標
	Vec2 vertex[I]{}; // offset
	Vec2 geom[I]{}; // 実際の値
	Vec2 uv[I]{}; // uv
};

template<int VertsCount>
class Ball : public Sprite<VertsCount>
{
public:
	Ball(float aSize, float aDeg, float aSpeed)
		: mDeg(aDeg)
		, mSize(aSize)
		, mSpeed(aSpeed)
		, mMoveVec{ sin(aDeg / 180.0f * PI), cos(aDeg / 180.0f * PI) }
	{
		SetVertex();
		size = { aSize , aSize };
	}

	~Ball()
	{
	}

	void SetVertex()
	{
		for (size_t i = 0; i < VertsCount; i++)
		{
			vertex[i].x = cos((float)i / VertsCount * PI * 2) * mSize;
			vertex[i].y = sin((float)i / VertsCount * PI * 2) * mSize;
			uv[i].x = cos((float)i / VertsCount  * PI * 2) * 0.5f + 0.5f;
			uv[i].y = 1 - sin((float)i / VertsCount  * PI * 2) * 0.5f + 0.5f;
		}
	}

	void Move()
	{
		// 移動
		pos.x += mMoveVec.x * mSpeed;
		pos.y += mMoveVec.y * mSpeed;

		const float XLimit = 0.9f;
		const float YLimit = 0.55f;

		// 反射
		if (pos.x > XLimit - mSize)
		{
			mMoveVec.x *= -1;
		}
		else if (pos.x < -XLimit + mSize)
		{
			mMoveVec.x *= -1;
		}
		if (pos.y > YLimit - mSize)
		{
			mMoveVec.y *= -1;
		}
		else if (pos.y < -YLimit + mSize)
		{
			mMoveVec.y *= -1;
		}

		// 反映
		for (size_t i = 0; i < VertsCount; i++)
		{
			geom[i].x = pos.x + vertex[i].x;
			geom[i].y = pos.y + vertex[i].y;
		}
	}

	void SwitchX()
	{
		mMoveVec.x *= -1;
	}

private:
	Vec2 mMoveVec;
	float mDeg;
	float mSize;
	float mSpeed;
};

template<int VertsCount>
class Bar : public Sprite<VertsCount>
{
public:
	static constexpr float MOVE_SPEED = 0.015f;
	static constexpr float Y_LIMIT = 0.625f;

public:
	Bar(Vec2 aSize, Vec2 aPos)
	{
		vertex[0] = geom[0] = { -aSize.x / 2, +aSize.y / 2 };
		vertex[1] = geom[1] = { +aSize.x / 2, +aSize.y / 2 };
		vertex[2] = geom[2] = { +aSize.x / 2, -aSize.y / 2 };
		vertex[3] = geom[3] = { -aSize.x / 2, -aSize.y / 2 };
		uv[0] = { 0, 0 };
		uv[1] = { 1, 0 };
		uv[2] = { 1, 1 };
		uv[3] = { 0, 1 };
		pos = aPos;
		size = aSize * 0.5f;
		// 反映
		for (size_t i = 0; i < VertsCount; i++)
		{
			geom[i].x = pos.x + vertex[i].x;
			geom[i].y = pos.y + vertex[i].y;
		}
	}

	~Bar()
	{
	}

	void MoveUp()
	{
		Move(Vec2{ 0, +MOVE_SPEED });
	}

	void MoveDown()
	{
		Move(Vec2{ 0, -MOVE_SPEED });
	}

	void Move(Vec2 aVec)
	{
		pos.x += aVec.x;
		pos.y += aVec.y;

		if (pos.y > Y_LIMIT - vertex[0].y)
		{
			pos.y = Y_LIMIT - vertex[0].y;
		}
		if (pos.y < -Y_LIMIT + vertex[0].y)
		{
			pos.y = -Y_LIMIT + vertex[0].y;
		}

		// 反映
		for (size_t i = 0; i < VertsCount; i++)
		{
			geom[i].x = pos.x + vertex[i].x;
			geom[i].y = pos.y + vertex[i].y;
		}
	}

private:

};

auto ball = std::make_unique<Ball<BALL_VERTS_COUNT>>(0.1f, 50.0f, 0.01f);
auto bar0 = std::make_unique<Bar<BAR_VERTS_COUNT>>(BAR_SIZE, Vec2{ -0.5f, 0.f });
auto bar1 = std::make_unique<Bar<BAR_VERTS_COUNT>>(BAR_SIZE, Vec2{ +0.5f, 0.f });


template<int IA, int IB>
bool IsCollidingSqSq(Sprite<IA> a, Sprite<IB> b)
{
	//if (a.pos.x - a.size.x / 2 > b.pos.x - b.size.x / 2 - a.size.x)
	//{
	//	if (a.pos.x - a.size.x / 2 < b.pos.x + b.size.x / 2)
	//	{
	//		if (a.pos.y - a.size.y / 2 > b.pos.y - b.size.y / 2 - a.size.y)
	//		{
	//			if (a.pos.y - a.size.y / 2 < b.pos.y + b.size.y / 2)
	//			{
	//				return true;
	//			}
	//		}
	//	}
	//}

	//if (a.pos.x - a.size.x / 2 > b.pos.x - b.size.x / 2 - a.size.x)
	//{
	//	if (a.pos.x + a.size.x / 2 < b.pos.x + b.size.x / 2 + a.size.x)
	//	{
	//		if (a.pos.y - a.size.y / 2 > b.pos.y - b.size.y / 2 - a.size.y)
	//		{
	//			if (a.pos.y + a.size.y / 2 < b.pos.y + b.size.y / 2 + a.size.y)
	//			{
	//				return true;
	//			}
	//		}
	//	}
	//}

	if (b.pos.x + b.size.x / 2 > a.pos.x - a.size.x / 2)
	{
		if (b.pos.x - b.size.x / 2 < a.pos.x + a.size.x / 2)
		{
			if (b.pos.y + b.size.y / 2 > a.pos.y - a.size.y / 2)
			{
				if (b.pos.y - b.size.y / 2 < a.pos.y + a.size.y / 2)
				{
					return true;
				}
			}
		}
	}


	return false;
}

GLuint loadTexture(const char* filename)
{
	// テクスチャIDの生成
	GLuint catId;
	glGenTextures(1, &catId);

	// ファイルの読み込み
	std::ifstream fstr(filename, std::ios::binary);
	if (!fstr)
	{
		std::cout << "Failed to load " << filename << "\n";
		return -1;
	}

	const size_t fileSize = static_cast<size_t>(fstr.seekg(0, fstr.end).tellg());
	fstr.seekg(0, fstr.beg);
	if (fileSize >= std::numeric_limits<size_t>::max())
	{
		std::cout << "Failed to get filesize that must be less than size_t max";
	}
	char* textureBuffer = new char[fileSize];
	fstr.read(textureBuffer, fileSize);

	// テクスチャをGPUに転送
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, catId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, textureBuffer);

	// テクスチャの設定
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// テクスチャのアンバインド
	delete[] textureBuffer;
	glBindTexture(GL_TEXTURE_2D, 0);

	return catId;
}

// エラーコールバック
void ErrorCallback2(int error, const char* description)
{
	std::cerr << "Error Occured code: " << error << " desc: " << description << "\n";
}

// 入力コールバック
void KeyCallback2(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		input.mKeyStates[key].pressed = true;
	}
	else if (action == GLFW_RELEASE)
	{
		input.mKeyStates[key].pressed = false;
	}

	// ESCで終了
	if (input.mKeyStates[GLFW_KEY_ESCAPE].pressed)
	{
		glfwSetWindowShouldClose(window, true);
	}
}
#include <direct.h>
#define GetCurrentDir _getcwd
std::string GetCurrentWorkingDir(void)
{
	char buff[FILENAME_MAX];
	GetCurrentDir(buff, FILENAME_MAX);
	std::string current_working_dir(buff);
	return current_working_dir;
}

int main()
{
	std::cout << "current directory is " << GetCurrentWorkingDir().c_str() << "\n";

	if (!glfwInit())
	{
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	GLFWwindow* window = glfwCreateWindow(WINDOW_SIZE.x, WINDOW_SIZE.y, "Pong Game", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwSetErrorCallback(ErrorCallback2);
	glfwSetKeyCallback(window, KeyCallback2);

	// モニタとの同期
	glfwMakeContextCurrent(window);
	auto addr = (GLADloadproc)glfwGetProcAddress;
	gladLoadGLLoader(addr);
	glfwSwapInterval(1);

	//GLuint programId = CreateShader();
	shader.SetUp();

	GLuint catId = loadTexture("cat.raw");
	GLuint dogId = loadTexture("dog.raw");

	// ゲームループ
	while (!glfwWindowShouldClose(window))
	{
		// -- 計算 --
		if (input.mKeyStates[GLFW_KEY_W].pressed)
		{
			bar0->MoveUp();
		}
		else if (input.mKeyStates[GLFW_KEY_S].pressed)
		{
			bar0->MoveDown();
		}

		if (input.mKeyStates[GLFW_KEY_UP].pressed)
		{
			bar1->MoveUp();
		}
		else if (input.mKeyStates[GLFW_KEY_DOWN].pressed)
		{
			bar1->MoveDown();
		}

		ball->Move();
		if (IsCollidingSqSq(*ball, *bar0))
		{
			ball->SwitchX();
		}
		if (IsCollidingSqSq(*ball, *bar1))
		{
			ball->SwitchX();
		}

		// -- 描画 -- 
		// 画面の初期化
		glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearDepth(1.0);

		bar0->Draw(catId);
		bar1->Draw(catId);
		ball->Draw(dogId);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}