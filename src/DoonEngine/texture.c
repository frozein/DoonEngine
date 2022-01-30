#define STB_IMAGE_IMPLEMENTATION

#include "texture.h"
#include <GLAD/glad.h>
#include <STB/stb_image.h>

//--------------------------------------------------------------------------------------------------------------------------------//

Texture texture_load(int type, const char* path, bool mipMap)
{
	stbi_set_flip_vertically_on_load(true);

	int w, h, nChannels;
	unsigned char* data = stbi_load(path, &w, &h, &nChannels, 0);
	if(!data)
		return (Texture){-1, -1};

	int format;
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

	Texture result = texture_load_raw(type, w, h, format, data, mipMap);
	stbi_image_free(data);
	return result;
}

Texture texture_load_raw(int type, int width, int height, int format, unsigned char* data, bool mipMap)
{
	unsigned int id;
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
		return (Texture){-1, -1};
		break;
	}

	if(mipMap)
		glGenerateMipmap(type);

	return (Texture){id, type};
}

void texture_free(Texture tex)
{
	glDeleteTextures(1, &tex.id);
}

void texture_bind(Texture texture)
{
	glBindTexture(texture.type, texture.id);
}

void texture_activate(Texture texture, unsigned int pos)
{
	glActiveTexture(GL_TEXTURE0 + pos);
	glBindTexture(texture.type, texture.id);
}

void texture_param_scale(Texture texture, int min, int mag)
{
	glBindTexture(texture.type, texture.id);

	if(min != -1)
		glTexParameteri(texture.type, GL_TEXTURE_MIN_FILTER, min);
	if(mag != -1)
		glTexParameteri(texture.type, GL_TEXTURE_MAG_FILTER, mag);
}

void texture_param_wrap (Texture texture, int s, int t, int r)
{
	glBindTexture(texture.type, texture.id);

	if(s != -1)
		glTexParameteri(texture.type, GL_TEXTURE_WRAP_S, s);
	if(t != -1)
		glTexParameteri(texture.type, GL_TEXTURE_WRAP_T, t);
	if(r != -1)
		glTexParameteri(texture.type, GL_TEXTURE_WRAP_R, r);
}

void texture_param_float(Texture texture, int param, float val)
{
	glBindTexture(texture.type, texture.id);
	glTexParameterf(texture.type, texture.id, val);
}

void texture_param_int(Texture texture, int param, int val)
{
	glBindTexture(texture.type, texture.id);
	glTexParameteri(texture.type, texture.id, val);
}

void texture_param_vec4(Texture texture, int param, vec4 val)
{
	glBindTexture(texture.type, texture.id);
	glTexParameterfv(texture.type, texture.id, (GLfloat*)&val);
}