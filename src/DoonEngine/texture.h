#ifndef DN_TEXTURE_H
#define DN_TEXTURE_H

#include "math/common.h"
#include <stdbool.h>

//A handle to a texture
typedef struct Texture
{
	unsigned int id;
	unsigned int type;
} Texture;

//--------------------------------------------------------------------------------------------------------------------------------//

//Loads a texture from a file and returns the handle to it. ONLY SUPPORTS 1D AND 2D TEXTURES, NOT ARRAYS, CUBEMAPS, OR 3D TEXTURES. Will generate mipmaps if mipMap is set to true. Returns -1 on failure.
Texture texture_load(int type, const char* path, bool mipMap);
//Loads a texture from some given data and returns the handle to it. ONLY SUPPORTS 1D AND 2D TEXTURES, NOT ARRAYS, CUBEMAPS, OR 3D TEXTURES. Will generate mipmaps if mipMap is set to true. Returns -1 on failure.
Texture texture_load_raw(int type, int width, int height, int format, unsigned char* data, bool mipMap);
//Frees a texture, any type
void texture_free(Texture tex);

//Binds a texture for modification/activation
void texture_bind(Texture texture);
//Activates a texture at a position for rendering
void texture_activate(Texture texture, unsigned int pos);

//--------------------------------------------------------------------------------------------------------------------------------//

//Sets the scale parameter of a texture. If -1 is set for any of the dimensions, the scaling method will remain unchanged
void texture_param_scale(Texture texture, int min, int mag);
//Sets the wrap parameter of a texture. If -1 is set for any of the dimensions, the wrapping method will remain unchanged
void texture_param_wrap (Texture texture, int s, int t, int r);

//Sets the value of a texture's float parameter
void texture_param_float(Texture texture, int param, float val);
//Sets the value of a texture's integer parameter
void texture_param_int  (Texture texture, int param, int   val);
//Sets the value of a texture's vector4 parameter
void texture_param_vec4 (Texture texture, int param, vec4  val);

#endif