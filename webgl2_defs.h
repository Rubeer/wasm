
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_ARRAY_BUFFER                   0x8892
#define GL_STREAM_DRAW                    0x88E0
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

typedef float GLfloat;
typedef float GLclampf;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLuint;
typedef unsigned int GLbitfield;
typedef unsigned char GLboolean;
typedef unsigned int GLenum;
typedef uintptr_t GLsizeiptr;
typedef void GLvoid;



js_import void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
js_import void glClear(GLbitfield mask);

js_import void glShaderSource(GLuint shader, GLsizei Length, void *Data);
js_import GLuint glCreateShader(GLenum type);
js_import void glCompileShader(GLuint shader);
js_import GLuint glCreateProgram();
js_import void glAttachShader(GLuint program, GLuint shader);
js_import void glLinkProgram (GLuint program);
js_import void glUseProgram (GLuint program);

js_import GLuint glCreateBuffer();
js_import void glBindBuffer (GLenum target, GLuint buffer);
js_import void glBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage);

js_import GLuint glCreateVertexArray();
js_import void glBindVertexArray(GLuint array);
js_import void glEnableVertexAttribArray (GLuint index);
js_import void glVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);

js_import void glDrawArrays(GLenum mode, GLint first, GLsizei count);
