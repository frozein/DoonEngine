#include "shader.h"

#include <GLAD/glad.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../globals.h"

//--------------------------------------------------------------------------------------------------------------------------------//

//Adds includeSource to the beginning of baseSource and returns the total string
static char* _DN_add_include_file(char* baseSource, const char* includePath);

//Loads the all contents of a file into a buffer. Allocates but DOES NOT free memory 
static bool _DN_load_into_buffer(const char* path, char** buffer);

int DN_shader_load(GLenum type, const char* path, const char* includePath)
{
	//load raw code into memory:
	char* source = 0;
	if(!_DN_load_into_buffer(path, &source))
		return -1;

	//add included code to original:
	source = _DN_add_include_file(source, includePath);
	if(source == NULL)
		return -1;

	//compile:
	GLshader shader;
	int success;

	shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar * const*)&source, NULL);
	glCompileShader(shader);

	DN_FREE(source);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		GLsizei logLength;
		char message[1024];
		glGetShaderInfoLog(shader, 1024, &logLength, message);

		char messageFinal[1024 + 256];
		sprintf(messageFinal, "failed to compile shader at \"%s\" with the following info log:\n%s", path, message);
		m_DN_message_callback(DN_MESSAGE_SHADER, DN_MESSAGE_ERROR, messageFinal);

		glDeleteShader(shader);
		return -1;
	}

	return shader;
}

void DN_shader_free(GLshader id)
{
	glDeleteShader(id);
}

//--------------------------------------------------------------------------------------------------------------------------------//

int DN_program_load(const char* vertexPath, const char* vertexIncludePath, const char* fragmentPath, const char* fragmentIncludePath)
{
	//load and compile shaders:
	int vertex   = DN_shader_load(GL_VERTEX_SHADER  , vertexPath  , vertexIncludePath  );
	int fragment = DN_shader_load(GL_FRAGMENT_SHADER, fragmentPath, fragmentIncludePath);
	if(vertex < 0 || fragment < 0)
	{
		glDeleteShader(vertex   >= 0 ? vertex   : 0); //only delete if was actually created to avoid extra errors
		glDeleteShader(fragment >= 0 ? fragment : 0);
		return -1;
	}

	//link shaders:
	GLprogram program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);

	int success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if(!success)
	{
		DN_shader_free(vertex);
		DN_shader_free(fragment);
		glDeleteProgram(program);
		return -1;
	}

	//delete shaders:
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return program;
}

int DN_compute_program_load(const char* path, const char* includePath)
{
	int compute = DN_shader_load(GL_COMPUTE_SHADER, path, includePath);
	if(compute < 0)
		return -1;

	//link into program:
	GLprogram program = glCreateProgram();
	glAttachShader(program, compute);
	glLinkProgram(program);

	int success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if(!success)
	{
		DN_shader_free(compute);
		glDeleteProgram(program);
		return -1;
	}

	//delete shaders:
	glDeleteShader(compute);

	return program;
}

void DN_program_free(GLprogram id)
{
	glDeleteProgram(id);
}

void DN_program_activate(GLprogram id)
{
	glUseProgram(id);
}

//--------------------------------------------------------------------------------------------------------------------------------//

void DN_program_uniform_int   (GLprogram id, const char* name, GLint val)
{
	glUniform1i(glGetUniformLocation(id, name), (GLint)val);
}

void DN_program_uniform_uint  (GLprogram id, const char* name, GLuint val)
{
	glUniform1ui(glGetUniformLocation(id, name), (GLuint)val);
}

void DN_program_uniform_float(GLprogram id, const char* name, GLfloat val)
{
	glUniform1f(glGetUniformLocation(id, name), (GLfloat)val);
}

void DN_program_uniform_double(GLprogram id, const char* name, GLdouble val)
{
	glUniform1d(glGetUniformLocation(id, name), (GLdouble)val);
}

void DN_program_uniform_vec2(GLprogram id, const char* name, DNvec2* val)
{
	glUniform2fv(glGetUniformLocation(id, name), 1, (GLfloat*)val);
}

void DN_program_uniform_vec3(GLprogram id, const char* name, DNvec3* val)
{
	glUniform3fv(glGetUniformLocation(id, name), 1, (GLfloat*)val);
}

void DN_program_uniform_vec4(GLprogram id, const char* name, DNvec4* val)
{
	glUniform4fv(glGetUniformLocation(id, name), 1, (GLfloat*)val);
}

void DN_program_uniform_mat2(GLprogram id, const char* name, DNmat2* val)
{
	glUniformMatrix2fv(glGetUniformLocation(id, name), 1, GL_FALSE, (GLfloat*)&val->m[0][0]);
}

void DN_program_uniform_mat3(GLprogram id, const char* name, DNmat3* val)
{
	glUniformMatrix3fv(glGetUniformLocation(id, name), 1, GL_FALSE, (GLfloat*)&val->m[0][0]);
}

void DN_program_uniform_mat4(GLprogram id, const char* name, DNmat4* val)
{
	glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE, (GLfloat*)&val->m[0][0]);
}

//--------------------------------------------------------------------------------------------------------------------------------//

static char* _DN_add_include_file(char* baseSource, const char* includePath)
{
	if(includePath == NULL)
		return baseSource;

	char* includeSource = 0;
	if(!_DN_load_into_buffer(includePath, &includeSource))
	{
		DN_FREE(baseSource);
		return NULL;
	}

	size_t baseLen    = strlen(baseSource   );
	size_t includeLen = strlen(includeSource);

	char* versionStart = strstr(baseSource, "#version");
	if(versionStart == NULL)
	{
		m_DN_message_callback(DN_MESSAGE_SHADER, DN_MESSAGE_ERROR, "shader source file did not contain a #version, unable to include another shader");

		DN_FREE(baseSource);
		DN_FREE(includeSource);
		return NULL;
	}

	unsigned int i = versionStart - baseSource;
	while(baseSource[i] != '\n')
	{
		i++;
		if(i >= baseLen)
		{
			m_DN_message_callback(DN_MESSAGE_SHADER, DN_MESSAGE_ERROR, "end of shader source file was reached before end of #version was found");
			DN_FREE(baseSource);
			DN_FREE(includeSource);
			return NULL;
		}
	}

	char* combinedSource = DN_MALLOC(sizeof(char) * (baseLen + includeLen + 1));
	memcpy(combinedSource, baseSource, sizeof(char) * i);
	memcpy(&combinedSource[i], includeSource, sizeof(char) * includeLen);
	memcpy(&combinedSource[i + includeLen], &baseSource[i + 1], sizeof(char) * (baseLen - i + 1));

	DN_FREE(baseSource);
	DN_FREE(includeSource);
	return combinedSource;
}

static bool _DN_load_into_buffer(const char* path, char** buffer)
{
	*buffer = 0;
	long length;
	FILE* file = fopen(path, "rb");

	if(file)
	{
		bool result = false;

		fseek(file, 0, SEEK_END);
		length = ftell(file);
		fseek(file, 0, SEEK_SET);
		*buffer = DN_MALLOC(length + 1);

		if(*buffer)
		{
			fread(*buffer, length, 1, file);
			(*buffer)[length] = '\0';
			result = true;
		}
		else
		{
			m_DN_message_callback(DN_MESSAGE_CPU_MEMORY, DN_MESSAGE_ERROR, "failed to allocate memory for shader source code");
			result = false;
		}

		fclose(file);
		return result;
	}
	else
	{
		char message[256];
		sprintf(message, "failed to open file \"%s\" for reading", path);
		m_DN_message_callback(DN_MESSAGE_FILE_IO, DN_MESSAGE_ERROR, message);
		return false;
	}
}
