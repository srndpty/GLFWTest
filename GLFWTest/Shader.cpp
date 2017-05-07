#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <string>

#include "Shader.h"



Shader::Shader()
{
}


Shader::~Shader()
{
}

void Shader::SetUp()
{
	//static const int VERTEX_BUFFER_COUNT = 4;
	//GLuint vertexBuffers[VERTEX_BUFFER_COUNT];
	//glGenBuffers(VERTEX_BUFFER_COUNT, vertexBuffers);
	//バーテックスシェーダのコンパイル
	auto vShaderId = glCreateShader(GL_VERTEX_SHADER);
	std::string vertexShader = R"#(
	uniform mat4 MVP;
    attribute vec3 position;
    attribute vec2 uv;
    varying vec2 vuv;
    void main(void){
        gl_Position = MVP * vec4(position, 1.0);
        vuv = uv;
    }
    )#";
	const char* vs = vertexShader.c_str();
	glShaderSource(vShaderId, 1, &vs, NULL);
	glCompileShader(vShaderId);

	//フラグメントシェーダのコンパイル
	GLuint fShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	std::string fragmentShader = R"#(
    varying vec2 vuv;
    uniform sampler2D texture;
    void main(void){
        gl_FragColor = texture2D(texture, vuv);
    }
    )#";
	const char* fs = fragmentShader.c_str();
	glShaderSource(fShaderId, 1, &fs, NULL);
	glCompileShader(fShaderId);

	//プログラムオブジェクトの作成
	GLuint programId = glCreateProgram();
	glAttachShader(programId, vShaderId);
	glAttachShader(programId, fShaderId);

	// リンク
	glLinkProgram(programId);

	glUseProgram(programId);

	mProgramId = programId;
	// 何番目のattribute変数か
	mPositionLocation = glGetAttribLocation(programId, "position");
	mUvLocation       = glGetAttribLocation(programId, "uv");
	mTextureLocation  = glGetUniformLocation(programId, "texture");
	mMvpLocation      = glGetUniformLocation(programId, "MVP");

	// attribute属性を有効にする
	glEnableVertexAttribArray(mPositionLocation);
	glEnableVertexAttribArray(mUvLocation);
	glEnableVertexAttribArray(mMvpLocation);

	// uniform属性を設定する
	glUniform1i(mTextureLocation, 0);

}
