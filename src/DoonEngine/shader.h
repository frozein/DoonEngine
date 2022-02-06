#ifndef SHADER_H
#define SHADER_H

#include "math/common.h"
#include <stdbool.h>

//--------------------------------------------------------------------------------------------------------------------------------//

//Loads, compiles, and links a vertex and fragment shader into a shader program and returns a handle to it. Returns -1 on failure
int shader_load(const char* vertexPath, const char* vertexIncludePath, const char* fragmentPath, const char* fragmentIncludePath);
//Loads, compiles, and links a compute shader into a shader program and returns a handle to it. Will paste the code in includePath to the top of the file if it is not NULL. Returns -1 on failure
int compute_shader_load(const char* path, const char* includePath);
//Deletes a shader, any type
void shader_free(unsigned int id);

//Activates a shader for rendering
void shader_activate(unsigned int id);

//--------------------------------------------------------------------------------------------------------------------------------//

//NOTE: uniform functions DO NOT activate the shader, if the shader is not first activated the uniform will not be set  

//Sets an integer uniform
void shader_uniform_int   (unsigned int id, const char* name, int          val);
//Sets an unsigned integer uniform
void shader_uniform_uint  (unsigned int id, const char* name, unsigned int val);
//Sets a float uniform
void shader_uniform_float (unsigned int id, const char* name, float        val);
//Sets a double uniform
void shader_uniform_double(unsigned int id, const char* name, double       val);

//Sets a vector2 uniform
void shader_uniform_vec2(unsigned int id, const char* name, vec2 val);
//Sets a vector3 uniform
void shader_uniform_vec3(unsigned int id, const char* name, vec3 val);
//Sets a vector4 uniform
void shader_uniform_vec4(unsigned int id, const char* name, vec4 val);

//Sets a 2x2 matrix uniform
void shader_uniform_mat2(unsigned int id, const char* name, mat2 val);
//Sets a 3x3 matrix uniform
void shader_uniform_mat3(unsigned int id, const char* name, mat3 val);
//Sets a 4x4 matrix uniform
void shader_uniform_mat4(unsigned int id, const char* name, mat4 val);

//--------------------------------------------------------------------------------------------------------------------------------//

//NOTE: struct uniform functions DO NOT activate the shader, if the shader is not first activated the uniform will not be set 

//Sets an integer uniform inside a struct
void shader_struct_uniform_int   (unsigned int id, const char* structName, const char* name, int          val);
//Sets an unsigned integer uniform inside a struct
void shader_struct_uniform_uint  (unsigned int id, const char* structName, const char* name, unsigned int val);
//Sets a float uniform inside a struct
void shader_struct_uniform_float (unsigned int id, const char* structName, const char* name, float        val);
//Sets a double uniform inside a struct
void shader_struct_uniform_double(unsigned int id, const char* structName, const char* name, double       val);

//Sets a vector2 uniform inside a struct
void shader_struct_uniform_vec2(unsigned int id, const char* structName, const char* name, vec2 val);
//Sets a vector3 uniform inside a struct
void shader_struct_uniform_vec3(unsigned int id, const char* structName, const char* name, vec3 val);
//Sets a vector4 uniform inside a struct
void shader_struct_uniform_vec4(unsigned int id, const char* structName, const char* name, vec4 val);

//Sets a 2x2 matrix uniform inside a struct
void shader_struct_uniform_mat2(unsigned int id, const char* structName, const char* name, mat2 val);
//Sets a 3x3 matrix uniform inside a struct
void shader_struct_uniform_mat3(unsigned int id, const char* structName, const char* name, mat3 val);
//Sets a 4x4 matrix uniform inside a struct
void shader_struct_uniform_mat4(unsigned int id, const char* structName, const char* name, mat4 val);

#endif