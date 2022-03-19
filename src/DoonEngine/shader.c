#include "shader.h"

#include <GLAD/glad.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//--------------------------------------------------------------------------------------------------------------------------------//

//Adds includeSource to the beginning of baseSource and returns the total string
static char* add_include_file(char* baseSource, const char* includePath);

//Loads the all contents of a file into a buffer. Allocates but DOES NOT free memory 
static bool load_into_buffer(const char* path, char** buffer);

int shader_load(unsigned int type, const char* path, const char* includePath)
{
	//load raw code into memory:
	char* source = 0;
	if(!load_into_buffer(path, &source))
		return -1;

	//add included code to original:
	source = add_include_file(source, includePath);
	if(source == NULL)
		return -1;

	//compile:
	unsigned int shader;
	int success;

	shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	free(source);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		GLsizei logLength;
		char message[1024];
		glGetShaderInfoLog(shader, 1024, &logLength, message);
		printf("\n\nINFO LOG - %s%s\n\n\n", message, path);

		glDeleteShader(shader);
		return -1;
	}

	return shader;
}

void shader_free(unsigned int id)
{
	glDeleteShader(id);
}

//--------------------------------------------------------------------------------------------------------------------------------//

int shader_program_load(const char* vertexPath, const char* vertexIncludePath, const char* fragmentPath, const char* fragmentIncludePath)
{
	//load and compile shaders:
	int vertex   = shader_load(GL_VERTEX_SHADER  , vertexPath  , vertexIncludePath  );
	int fragment = shader_load(GL_FRAGMENT_SHADER, fragmentPath, fragmentIncludePath);
	if(vertex < 0 || fragment < 0)
	{
		glDeleteShader(vertex   >= 0 ? vertex   : 0); //only delete if was actually created to avoid extra errors
		glDeleteShader(fragment >= 0 ? fragment : 0);
		return -1;
	}

	//link shaders:
	unsigned int program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);

	int success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if(!success)
	{
		shader_free(vertex);
		shader_free(fragment);
		glDeleteProgram(program);
		return -1;
	}

	//delete shaders:
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return program;
}

int compute_shader_program_load(const char* path, const char* includePath)
{
	int compute = shader_load(GL_COMPUTE_SHADER, path, includePath);
	if(compute < 0)
		return -1;

	//link into program:
	unsigned int program = glCreateProgram();
	glAttachShader(program, compute);
	glLinkProgram(program);

	int success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if(!success)
	{
		shader_free(compute);
		glDeleteProgram(program);
		return -1;
	}

	//delete shaders:
	glDeleteShader(compute);

	return program;
}

void shader_program_free(unsigned int id)
{
	glDeleteProgram(id);
}

void shader_program_activate(unsigned int id)
{
	glUseProgram(id);
}

//--------------------------------------------------------------------------------------------------------------------------------//

void shader_uniform_int   (unsigned int id, const char* name, int val)
{
	glUniform1i(glGetUniformLocation(id, name), (GLint)val);
}

void shader_uniform_uint  (unsigned int id, const char* name, unsigned int val)
{
	glUniform1ui(glGetUniformLocation(id, name), (GLuint)val);
}

void shader_uniform_float(unsigned int id, const char* name, float val)
{
	glUniform1f(glGetUniformLocation(id, name), (GLfloat)val);
}

void shader_uniform_double(unsigned int id, const char* name, double val)
{
	glUniform1d(glGetUniformLocation(id, name), (GLdouble)val);
}

void shader_uniform_vec2(unsigned int id, const char* name, vec2 val)
{
	glUniform2fv(glGetUniformLocation(id, name), 1, (GLfloat*)&val);
}

void shader_uniform_vec3(unsigned int id, const char* name, vec3 val)
{
	glUniform3fv(glGetUniformLocation(id, name), 1, (GLfloat*)&val);
}

void shader_uniform_vec4(unsigned int id, const char* name, vec4 val)
{
	glUniform4fv(glGetUniformLocation(id, name), 1, (GLfloat*)&val);
}

void shader_uniform_mat2(unsigned int id, const char* name, mat2 val)
{
	glUniformMatrix2fv(glGetUniformLocation(id, name), 1, GL_FALSE, val.m[0]);
}

void shader_uniform_mat3(unsigned int id, const char* name, mat3 val)
{
	glUniformMatrix3fv(glGetUniformLocation(id, name), 1, GL_FALSE, val.m[0]);
}

void shader_uniform_mat4(unsigned int id, const char* name, mat4 val)
{
	glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE, val.m[0]);
}

//--------------------------------------------------------------------------------------------------------------------------------//

//Returns the total name of a struct uniform
static void get_full_name(char* dest, const char* structName, const char* name);

void shader_struct_uniform_int(unsigned int id, const char* structName, const char* name, int val)
{
	char fullName[256];
	get_full_name(fullName, structName, name);
	glUniform1i(glGetUniformLocation(id, fullName), (GLint)val);
}

void shader_struct_uniform_uint(unsigned int id, const char* structName, const char* name, unsigned int val)
{
	char fullName[256];
	get_full_name(fullName, structName, name);
	glUniform1ui(glGetUniformLocation(id, fullName), (GLuint)val);
}

void shader_struct_uniform_float(unsigned int id, const char* structName, const char* name, float val)
{
	char fullName[256];
	get_full_name(fullName, structName, name);
	glUniform1f(glGetUniformLocation(id, fullName), (GLfloat)val);
}

void shader_struct_uniform_double(unsigned int id, const char* structName, const char* name, double val)
{
	char fullName[256];
	get_full_name(fullName, structName, name);
	glUniform1d(glGetUniformLocation(id, fullName), (GLdouble)val);
}

void shader_struct_uniform_vec2(unsigned int id, const char* structName, const char* name, vec2 val)
{
	char fullName[256];
	get_full_name(fullName, structName, name);
	glUniform2fv(glGetUniformLocation(id, fullName), 1, (GLfloat*)&val);
}

void shader_struct_uniform_vec3(unsigned int id, const char* structName, const char* name, vec3 val)
{
	char fullName[256];
	get_full_name(fullName, structName, name);
	glUniform3fv(glGetUniformLocation(id, fullName), 1, (GLfloat*)&val);
}

void shader_struct_uniform_vec4(unsigned int id, const char* structName, const char* name, vec4 val)
{
	char fullName[256];
	get_full_name(fullName, structName, name);
	glUniform4fv(glGetUniformLocation(id, fullName), 1, (GLfloat*)&val);
}

void shader_struct_uniform_mat2(unsigned int id, const char* structName, const char* name, mat2 val)
{
	char fullName[256];
	get_full_name(fullName, structName, name);
	glUniformMatrix2fv(glGetUniformLocation(id, fullName), 1, GL_FALSE, val.m[0]);
}

void shader_struct_uniform_mat3(unsigned int id, const char* structName, const char* name, mat3 val)
{
	char fullName[256];
	get_full_name(fullName, structName, name);
	glUniformMatrix3fv(glGetUniformLocation(id, fullName), 1, GL_FALSE, val.m[0]);
}

void shader_struct_uniform_mat4(unsigned int id, const char* structName, const char* name, mat4 val)
{
	char fullName[256];
	get_full_name(fullName, structName, name);
	glUniformMatrix4fv(glGetUniformLocation(id, fullName), 1, GL_FALSE, val.m[0]);
}

//--------------------------------------------------------------------------------------------------------------------------------//

static char* add_include_file(char* baseSource, const char* includePath)
{
	if(includePath == NULL)
		return baseSource;

	char* includeSource = 0;
	if(!load_into_buffer(includePath, &includeSource))
	{
		free(baseSource);
		return NULL;
	}

	size_t baseLen    = strlen(baseSource   );
	size_t includeLen = strlen(includeSource);

	char* versionStart = strstr(baseSource, "#version");
	if(versionStart == NULL)
	{
		printf("ERROR - SHADER SOURCE FILE DID NOT CONTAIN A #version");
		free(baseSource);
		free(includeSource);
		return NULL;
	}

	unsigned int i = versionStart - baseSource;
	while(baseSource[i] != '\n')
	{
		i++;
		if(i >= baseLen)
		{
			printf("ERROR - END OF SHADER SOURCE FILE REACHED BEFORE END OF #version WAS FOUND");
			free(baseSource);
			free(includeSource);
			return NULL;
		}
	}

	char* combinedSource = malloc(sizeof(char) * (baseLen + includeLen + 1));
	memcpy(combinedSource, baseSource, sizeof(char) * i);
	memcpy(&combinedSource[i], includeSource, sizeof(char) * includeLen);
	memcpy(&combinedSource[i + includeLen], &baseSource[i + 1], sizeof(char) * (baseLen - i + 1));

	free(baseSource);
	free(includeSource);
	return combinedSource;
}

bool load_into_buffer(const char* path, char** buffer)
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
		*buffer = malloc(length + 1);

		if(*buffer)
		{
			if(fread(*buffer, length, 1, file) == 1)
			{
				(*buffer)[length] = '\0';
				result = true;
			}
			else
			{
				printf("ERROR - COULD NOT READ FROM FILE %s\n", path);
				result = false;
				free(*buffer);
			}
		}
		else
		{
			printf("ERROR - COULD NOT ALLOCATE MEMORY FOR SHADER SOURCE CODE");
			result = false;
		}

		fclose(file);
		return result;
	}
	else
	{
		printf("ERROR - COULD NOT OPEN FILE %s\n", path);
		return false;
	}
}

void get_full_name(char* dest, const char* structName, const char* name)
{
	strcpy(dest, structName);
	strcat(dest, ".");
	strcat(dest, name);
}