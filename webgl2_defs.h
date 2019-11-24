
#define GL_FLOAT                          0x1406
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_UNSIGNED_INT                   0x1405
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
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
#define GL_DEPTH_COMPONENT                0x1902
#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_DEPTH_COMPONENT32              0x81A7
#define GL_DEPTH_COMPONENT32F             0x8CAC
#define GL_MULTISAMPLE                    0x809D
#define GL_TEXTURE0                       0x84C0
#define GL_CULL_FACE                      0x0B44
#define GL_TEXTURE_2D                     0x0DE1
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_NEAREST_MIPMAP_NEAREST         0x2700
#define GL_LINEAR_MIPMAP_NEAREST          0x2701
#define GL_NEAREST_MIPMAP_LINEAR          0x2702
#define GL_LINEAR_MIPMAP_LINEAR           0x2703
#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601
#define GL_R8                             0x8229
#define GL_RED                            0x1903
#define GL_BLEND                          0x0BE2
#define GL_FUNC_ADD                       0x8006
#define GL_FUNC_REVERSE_SUBTRACT          0x800B
#define GL_FUNC_SUBTRACT                  0x800A
#define GL_ZERO                           0
#define GL_ONE                            1

#define GL_FRAMEBUFFER                    0x8D40
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_RGBA                           0x1908
#define GL_RGBA32F                        0x8814
#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9

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

import_from_js void glBlendFunc(GLenum sfactor, GLenum dfactor);
import_from_js void glBlendEquation (GLenum mode);

import_from_js void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
import_from_js void glClear(GLbitfield mask);

import_from_js void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

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
import_from_js void glVertexAttribDivisor (GLuint index, GLuint divisor);
import_from_js GLuint glGetUniformLocation (GLuint Program, u32 NameLength, char *NamePtr);
import_from_js void glUniformMatrix4fv(GLuint Location, GLboolean Transpose, m4x4 const *Data);
import_from_js void glUniform1i (GLuint location, GLint v0);
import_from_js void glUniform3f (GLuint location, f32 x, f32 y, f32 z);

import_from_js void glDrawArrays(GLenum mode, GLint first, GLsizei count);
import_from_js void glDrawArraysInstanced (GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
import_from_js void glDrawElements(GLenum mode, GLsizei count, GLenum type, GLintptr offset);
import_from_js void glDrawElementsInstanced (GLenum mode, GLsizei count, GLenum type, GLuint offset, GLsizei instancecount);

import_from_js void glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, void *DataPtr, size DataSize, size Offset);
import_from_js GLuint glCreateTexture();
import_from_js void glTexParameteri (GLenum target, GLenum pname, GLint param);
import_from_js void glBindTexture (GLenum target, GLuint texture);
import_from_js void glGenerateMipmap (GLenum target);
import_from_js void glActiveTexture (GLenum texture);

import_from_js GLuint glCreateFramebuffer();
import_from_js void glBindFramebuffer (GLenum target, GLuint framebuffer);
import_from_js void glFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
import_from_js void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);


function GLuint glGetUniformLocation(GLuint Program, string Name)
{
    return glGetUniformLocation(Program, Name.Size, Name.Contents);
}

import_from_js GLuint JS_GL_CreateCompileAndLinkProgram(u32 VertLen, char *VertSrc, u32 FragLen, char *FragSrc);
function GLuint JS_GL_CreateCompileAndLinkProgram(string Vert, string Frag)
{
    return JS_GL_CreateCompileAndLinkProgram(Vert.Size, Vert.Contents, Frag.Size, Frag.Contents);
}

function void glUniform3f(GLuint location, v3 V)
{
    glUniform3f(location, V.x, V.y, V.z);
}


