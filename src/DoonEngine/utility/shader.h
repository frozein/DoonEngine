#ifndef DN_SHADER_H
#define DN_SHADER_H

#include "../globals.h"
#include "../math/common.h"
#include <stdbool.h>

typedef GLuint GLshader;  //a handle to a GL shader
typedef GLuint GLprogram; //a handle to a GL shader program

//--------------------------------------------------------------------------------------------------------------------------------//

/* Loads and compiles a shader
 * @param type the type of shader to be compiled. For example, GL_VERTEX_SHADER
 * @param path the path to the shader to be loaded
 * @param includePath the path to the shader to be included, if an include is not needed, set to NULL
 * @returns the handle to the shader, or -1 on failure
 */
int DN_shader_load(GLenum type, const char* path, const char* includePath);
/* Frees a shader
 * @param id the handle to the shader to free
 */
void DN_shader_free(GLshader id);

/* Generates a shader program with a vertex and a fragment shader
 * @param vertexPath the path to the vertex shader to use
 * @param vertexIncludePath the path to the file to be included in the vertex shader, or NULL if none is desired
 * @param fragmentPath the path to the fragment shader to use
 * @param fragmentIncludePath the path to the file to be included in the fragment shader, or NULL if none is desired
 * @returns the handle to the program, or -1 on failure
 */
int DN_program_load(const char* vertexPath, const char* vertexIncludePath, const char* fragmentPath, const char* fragmentIncludePath);
/* Generates a shader program with a compute shader
 * @param path the path to the compute shader to use
 * @param includePath the path to the file to be included in the compute shader, or NULL if none is desired
 * @returns the handle to the program, or -1 on failure
 */
int DN_compute_program_load(const char* path, const char* includePath);
/* Frees a shader program
 * @param id the handle to the shader program to free
 */
void DN_program_free(GLprogram id);
/* Activaes a shader program for drawing
 * @param id the handle to the program to activate
 */
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