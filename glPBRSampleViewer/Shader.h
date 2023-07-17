#pragma once
#include <GL/glew.h>
#include <string>

class Shader
{
public:
	Shader(const std::string strVertexSPath, const std::string strFragSPath);
	~Shader();

	GLuint program;

private:
	void ReadShaderFile(const std::string& strFilePath, std::string& strShaderData);
	void CheckErrors(const GLuint& type, const std::string& strType);
};