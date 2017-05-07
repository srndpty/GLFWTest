#pragma once

class Shader
{
public:
	Shader();
	~Shader();
	void SetUp();

	int mPositionLocation;
	int mUvLocation;
	int mTextureLocation;
	int mMvpLocation;

private:
	GLuint mProgramId;
};

