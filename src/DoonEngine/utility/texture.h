#ifndef DN_TEXTURE_H
#define DN_TEXTURE_H

#include "../globals.h"
#include "../math/common.h"
#include <stdbool.h>

//A handle to a texture
typedef struct DNtexture
{
	GLuint id;
	GLenum type;
} DNtexture;

//--------------------------------------------------------------------------------------------------------------------------------//

//Loads a texture from a file and returns the handle to it. ONLY SUPPORTS 1D AND 2D TEXTURES, NOT ARRAYS, CUBEMAPS, OR 3D TEXTURES. Will generate mipmaps if mipMap is set to true. Returns -1 on failure.
DNtexture DN_texture_load(GLenum type, const char* path, bool mipMap);
//Loads a texture from some given data and returns the handle to it. ONLY SUPPORTS 1D AND 2D TEXTURES, NOT ARRAYS, CUBEMAPS, OR 3D TEXTURES. Will generate mipmaps if mipMap is set to true. Returns -1 on failure.
DNtexture DN_texture_load_raw(GLenum type, int width, int height, GLenum format, unsigned char* data, bool mipMap);
//Frees a texture, any type
void DN_texture_free(DNtexture tex);

//Binds a texture for modification/activation
void DN_texture_bind(DNtexture texture);
//Activates a texture at a position for rendering
void DN_texture_activate(DNtexture texture, GLenum pos);

//--------------------------------------------------------------------------------------------------------------------------------//

//Sets the scale parameter of a texture. If -1 is set for any of the dimensions, the scaling method will remain unchanged
void DN_set_texture_scale(DNtexture texture, GLenum min, GLenum mag);
//Sets the wrap parameter of a texture. If -1 is set for any of the dimensions, the wrapping method will remain unchanged
void DN_set_texture_wrap (DNtexture texture, GLenum s, GLenum t, GLenum r);

//Sets the value of a texture's float parameter
void DN_texture_param_float(DNtexture texture, GLenum param, float val);
//Sets the value of a texture's integer parameter
void DN_texture_param_int  (DNtexture texture, GLenum param, int   val);
//Sets the value of a texture's vector4 parameter
void DN_texture_param_vec4 (DNtexture texture, GLenum param, DNvec4  val);

#endif