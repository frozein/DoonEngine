#ifndef DN_SHADER_H
#define DN_SHADER_H

#include "../globals.h"
#include "../math/common.h"
#include <stdbool.h>

typedef GLuint GLshader; //a handle to a GL shader
typedef GLuint GLprogram; //a handle to a GL shader program

//--------------------------------------------------------------------------------------------------------------------------------//

//Loads and compiles a shader and returns a handle to it. Returns -1 on failure
int DN_shader_load(GLenum type, const char* path, const char* includePath);
//Deletes a shader, any type
void DN_shader_free(GLshader id);

//Loads, compiles, and links a vertex and fragment shader into a shader program and returns a handle to it. Returns -1 on failure
int DN_program_load(const char* vertexPath, const char* vertexIncludePath, const char* fragmentPath, const char* fragmentIncludePath);
//Loads, compiles, and links a compute shader into a shader program and returns a handle to it. Will paste the code in includePath to the top of the file if it is not NULL. Returns -1 on failure
int DN_compute_program_load(const char* path, const char* includePath);
//Deletes a shader program, any type
void DN_program_free(GLprogram id);
//Activates a shader program for rendering
void DN_program_activate(GLprogram id);

//--------------------------------------------------------------------------------------------------------------------------------//

//NOTE: uniform functions DO NOT activate the shader program, if the shader program is not first activated the uniform will not be set  

//Sets an integer uniform
void DN_program_uniform_int   (GLprogram id, const char* name, GLint    val);
//Sets an unsigned integer uniform
void DN_program_uniform_uint  (GLprogram id, const char* name, GLuint   val);
//Sets a float uniform
void DN_program_uniform_float (GLprogram id, const char* name, GLfloat  val);
//Sets a double uniform
void DN_program_uniform_double(GLprogram id, const char* name, GLdouble val);

//Sets a vector2 uniform
void DN_program_uniform_vec2(GLprogram id, const char* name, DNvec2 val);
//Sets a vector3 uniform
void DN_program_uniform_vec3(GLprogram id, const char* name, DNvec3 val);
//Sets a vector4 uniform
void DN_program_uniform_vec4(GLprogram id, const char* name, DNvec4 val);

//Sets a 2x2 matrix uniform
void DN_program_uniform_mat2(GLprogram id, const char* name, DNmat2 val);
//Sets a 3x3 matrix uniform
void DN_program_uniform_mat3(GLprogram id, const char* name, DNmat3 val);
//Sets a 4x4 matrix uniform
void DN_program_uniform_mat4(GLprogram id, const char* name, DNmat4 val);

#endif