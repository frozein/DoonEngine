#define STB_IMAGE_IMPLEMENTATION

#include "texture.h"
#include "../globals.h"
#include <GLAD/glad.h>
#include <STB/stb_image.h>

//--------------------------------------------------------------------------------------------------------------------------------//

DNtexture DN_texture_load(GLenum type, const char* path, bool mipMap)
{
	stbi_set_flip_vertically_on_load(true);

	int w, h, nChannels;
	unsigned char* data = stbi_load(path, &w, &h, &nChannels, 0);
	if(!data)
	{
		DN_ERROR_LOG("ERROR - FAILED TO LOAD IMAGE AT \"%s\"\n", path);
		return (DNtexture){-1, -1};
	}

	GLenum format;
	switch(nChannels)
	{
	case 1:
		format = GL_RED;
		break;
	case 2:
		format = GL_RG;
		break;
	case 3:
		format = GL_RGB;
		break;
	case 4:
		format = GL_RGBA;
		break;
	}

	DNtexture result = DN_texture_load_raw(type, w, h, format, data, mipMap);
	stbi_image_free(data);
	return result;
}

DNtexture DN_texture_load_raw(GLenum type, int width, int height, GLenum format, unsigned char* data, bool mipMap)
{
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(type, id);
	switch(type)
	{
	case GL_TEXTURE_1D:
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, width, 0, format, GL_UNSIGNED_BYTE, data);
		break;
	case GL_TEXTURE_2D:
	case GL_TEXTURE_2D_MULTISAMPLE:
	case GL_TEXTURE_RECTANGLE:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		break;
	default:
		DN_ERROR_LOG("ERROR - UNSUPPORTED TEXTURE FORMAT\n");
		return (DNtexture){-1, -1};
		break;
	}

	if(mipMap)
		glGenerateMipmap(type);

	return (DNtexture){id, type};
}

void DN_texture_free(DNtexture tex)
{
	glDeleteTextures(1, &tex.id);
}

void DN_texture_bind(DNtexture texture)
{
	glBindTexture(texture.type, texture.id);
}

void DN_texture_activate(DNtexture texture, GLenum pos)
{
	glActiveTexture(GL_TEXTURE0 + pos);
	glBindTexture(texture.type, texture.id);
}

void DN_set_texture_scale(DNtexture texture, GLenum min, GLenum mag)
{
	glBindTexture(texture.type, texture.id);

	if(min != -1)
		glTexParameteri(texture.type, GL_TEXTURE_MIN_FILTER, min);
	if(mag != -1)
		glTexParameteri(texture.type, GL_TEXTURE_MAG_FILTER, mag);
}

void DN_set_texture_wrap (DNtexture texture, GLenum s, GLenum t, GLenum r)
{
	glBindTexture(texture.type, texture.id);

	if(s != -1)
		glTexParameteri(texture.type, GL_TEXTURE_WRAP_S, s);
	if(t != -1)
		glTexParameteri(texture.type, GL_TEXTURE_WRAP_T, t);
	if(r != -1)
		glTexParameteri(texture.type, GL_TEXTURE_WRAP_R, r);
}

void DN_texture_param_float(DNtexture texture, GLenum param, float val)
{
	glBindTexture(texture.type, texture.id);
	glTexParameterf(texture.type, texture.id, val);
}

void DN_texture_param_int(DNtexture texture, GLenum param, int val)
{
	glBindTexture(texture.type, texture.id);
	glTexParameteri(texture.type, texture.id, val);
}

void DN_texture_param_vec4(DNtexture texture, GLenum param, DNvec4 val)
{
	glBindTexture(texture.type, texture.id);
	glTexParameterfv(texture.type, texture.id, (GLfloat*)&val);
}