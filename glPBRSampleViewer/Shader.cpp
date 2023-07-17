#include "Shader.h"
#include <fstream>
#include <spdlog/spdlog.h>

Shader::Shader(const std::string strVertexSPath, const std::string strFragSPath)
{
	std::string strVSData, strFSData;
	ReadShaderFile(strVertexSPath, strVSData);
	ReadShaderFile(strFragSPath, strFSData);

	const char* szVertexSData = strVSData.c_str();
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &szVertexSData, 0);
	glCompileShader(vertexShader);
	CheckErrors(vertexShader, "VERTEX_S");

	const char* szFragSData = strFSData.c_str();
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragShader, 1, &szFragSData, 0);
	glCompileShader(fragShader);
	CheckErrors(fragShader, "FRAGMENT_S");

	program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragShader);
	glLinkProgram(program);

	glDeleteShader(vertexShader);
	glDeleteShader(fragShader);
}

Shader::~Shader()
{
	glDeleteProgram(program);
}

void Shader::ReadShaderFile(const std::string& strFilePath, std::string& strShaderData)
{
	std::ifstream file(strFilePath);
	if (file.is_open())
	{
		std::stringstream ss;
		ss << file.rdbuf();
		strShaderData = ss.str();

		file.close();
	}
	else
		spdlog::error("Failed to read shader file at : " + strFilePath);
}

void Shader::CheckErrors(const GLuint& type, const std::string& strType)
{
	int success;
	char szInfoLog[1024];
	if (strType == "PROGRAM")
	{
		glGetProgramiv(type, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(program, 1024, NULL, szInfoLog);
			spdlog::error("PROGRAM Linking error: " + std::string(szInfoLog));
		}
	}
	else
	{
		glGetShaderiv(type, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(type, 1024, NULL, szInfoLog);
			spdlog::error(strType + " Shader Compile Error" + std::string(szInfoLog));
		}
	}
}

