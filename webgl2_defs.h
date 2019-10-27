
#define GL_FLOAT                          0x1406
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_ARRAY_BUFFER                   0x8892
#define GL_STREAM_DRAW                    0x88E0
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_STENCIL_BUFFER_BIT             0x00000400
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_TRIANGLES                      0x0004
#define GL_TRIANGLE_STRIP                 0x0005
#define GL_TRIANGLE_FAN                   0x0006
#define GL_NEVER                          0x0200
#define GL_LESS                           0x0201
#define GL_EQUAL                          0x0202
#define GL_LEQUAL                         0x0203
#define GL_GREATER                        0x0204
#define GL_NOTEQUAL                       0x0205
#define GL_GEQUAL                         0x0206
#define GL_ALWAYS                         0x0207
#define GL_SRC_COLOR                      0x0300
#define GL_ONE_MINUS_SRC_COLOR            0x0301
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_DST_ALPHA                      0x0304
#define GL_ONE_MINUS_DST_ALPHA            0x0305
#define GL_DST_COLOR                      0x0306
#define GL_ONE_MINUS_DST_COLOR            0x0307
#define GL_DEPTH_TEST                     0x0B71
#define GL_TEXTURE0                       0x84C0
#define GL_CULL_FACE                      0x0B44

typedef float GLfloat;
typedef float GLclampf;
typedef int GLint;
typedef unsigned int GLsizei;
typedef unsigned int GLuint;
typedef unsigned int GLbitfield;
typedef unsigned char GLboolean;
typedef unsigned int GLenum;
typedef uintptr_t GLsizeiptr;
typedef intptr_t GLintptr;
typedef void GLvoid;


import_from_js void glDisable (GLenum cap);
import_from_js void glEnable (GLenum cap);

import_from_js void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
import_from_js void glClear(GLbitfield mask);

import_from_js void glShaderSource(GLuint shader, GLsizei Length, void *Data);
import_from_js GLuint glCreateShader(GLenum type);
import_from_js void glCompileShader(GLuint shader);
import_from_js GLuint glCreateProgram();
import_from_js void glAttachShader(GLuint program, GLuint shader);
import_from_js void glLinkProgram (GLuint program);
import_from_js void glUseProgram (GLuint program);
import_from_js GLuint glGetAttribLocation (GLuint Program, u32 NameLength, char *NamePtr);

import_from_js GLuint glCreateBuffer();
import_from_js void glBindBuffer (GLenum target, GLuint buffer);
import_from_js void glBufferData (GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
import_from_js void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
 

import_from_js GLuint glCreateVertexArray();
import_from_js void glBindVertexArray(GLuint array);
import_from_js void glEnableVertexAttribArray (GLuint index);
import_from_js void glVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
import_from_js GLuint glGetUniformLocation (GLuint Program, u32 NameLength, char *NamePtr);
import_from_js void glUniformMatrix4fv(GLuint Location, GLboolean Transpose, m4x4 *Data);

import_from_js void glDrawArrays(GLenum mode, GLint first, GLsizei count);




function GLuint glGetUniformLocation(GLuint Program, string Name)
{
    return glGetUniformLocation(Program, Name.Size, Name.Contents);
}

import_from_js GLuint JS_GL_CreateCompileAndLinkProgram(u32 VertLen, char *VertSrc, u32 FragLen, char *FragSrc);


