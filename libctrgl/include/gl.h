/*
    Boost Software License - Version 1.0 - August 17th, 2003

    Permission is hereby granted, free of charge, to any person or organization
    obtaining a copy of the software and accompanying documentation covered by
    this license (the "Software") to use, reproduce, display, distribute,
    execute, and transmit the Software, and to prepare derivative works of the
    Software, and to permit third-parties to whom the Software is furnished to
    do so, all subject to the following:

    The copyright notices in the Software and this entire statement, including
    the above license grant, this restriction and the following disclaimer,
    must be included in all copies of the Software, in whole or in part, and
    all derivative works of the Software, unless such copies or derivative
    works are solely in the form of machine-executable object code generated by
    a source language processor.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
    SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
    FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

/*
 *  CTRGL - a far-from-complete OpenGL implementation for the POS PICA in the
 *      Nintendo 3DS family of video game consoles.
 *
 *    This project would never have been possible without all the great people
 *    behind 3Dbrew and ctrulib.
 *    Special thanks to Smealum for blessing us with code exec in the first place.
 */

/*
 *  NOTE: Functions such as glInitBufferCTR and glShutdownBufferCTR (also called "direct resource access")
 *      can be used to avoid an additional memory allocation done by their glGen* and glCreate* equivalents.
 *      This makes code a tiny bit more efficient at the cost of diverging from OpenGL, which may or may not
 *      be an acceptable trade-off for your application.
 *      With the correct compiler and linker flags, the allocating variants of the functions, if not used by
 *      your program, will not be linked in at all, thereby slightly reducing code size.
 *
 *      glInit*CTR takes a pointer to a user-allocated data structure and returns a value to be used
 *      with regular GL functions for that class of resource. This value is guaranteed to be the same
 *      as the provided pointer, cast to GLuint.
 *
 *      Similarly, glShutdown*CTR releases all data allocated by the resource, but leaves the actual structure
 *      intact.
 *
 *      If you decide to use these functions, you take on the responsibility to keep you code in sync,
 *      as a breakage may happen anytime. This especially applies to accessing the structure fields directly.
 *      Oh, and as always, no NULL checks are being done, in the ctrGL spirit of "crash early, crash often".
 */

/*
 *  Explanation of GLmemoryTransferModeCTR:
 *    - GL_MEMORY_STATIC_CTR means to use the provided pointer without performing any memory management.
 *        Use this if you can guarantee that the memory block will stay valid for the lifetime of the resource,
 *        for example when the data is directly linked into your program.
 *
 *    - GL_MEMORY_COPY_CTR instructs ctrGL to make a private copy of the provided data and manage it on its own.
 *
 *    - GL_MEMORY_TRANSFER_CTR transfers the ownership of the memory block to ctrGL. The memory block must have
 *        been allocated using 'malloc' and ctrGL will 'free' it when the resource is released.
 *        All pointers to this memory block should be treated as invalid after calling a function with
 *        this argument.
 *
 *    When in doubt, use GL_MEMORY_COPY_CTR.
 */

#ifndef CTRGL_H
#define CTRGL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <3ds.h>
#include <stdint.h>

#define CTRGL_VERSION_STRING "HEAD"

#define glMakeRgba8CTR(r,g,b,a) (\
    (((r) & 0xFF) << 24)\
    | (((g) & 0xFF) << 16)\
    | (((b) & 0xFF) << 8)\
    | ((a) & 0xFF))\

/* type declarations */

typedef void        GLvoid;
typedef char        GLchar;
typedef size_t      GLuint;
typedef int16_t     GLint;
typedef uint8_t     GLubyte;
typedef float       GLclampf;
typedef float       GLfloat;

typedef int         GLboolean;
typedef uint16_t    GLbitfield;
typedef uint16_t    GLenum;
typedef int32_t     GLintptr;
typedef uint32_t    GLsize;
typedef int32_t     GLsizei;
typedef int32_t     GLsizeiptr;

/* enums */

#include "gl_enums.h"
#include "gl_ctr.h"

/* ctrgl control functions */
void ctrglInit(void);
void ctrglExit(void);

void ctrglAllocateCommandBuffers(GLsize size,
        GLuint count            /* must be 1 or 2 */
        );
void ctrglGetCommandBuffers(u32* size, u32** gpuCmd, u32** gpuCmdRight);
void ctrglGetCommandBufferUtilization(u32* size, u32* used);
void ctrglSetTimeout(CTRGLtimeoutType type, u64 nanoseconds);
void ctrglSetTimeoutHandler(CTRGLtimeoutHandler handler);
void ctrglSetVsyncWait(GLboolean enabled);      /* enabled by default */

void ctrglResetGPU(void);
void ctrglBeginRendering(void);
void ctrglFlushState(uint32_t mask);
void ctrglFinishRendering(void);

/* **** STATE **** */
void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glClearColorRgba8CTR(uint32_t rgba);

/* Alpha Test */
void glAlphaFunc(GLenum func, GLclampf ref);

void glAlphaFuncubCTR(GLenum func, GLubyte ref);

/* Blending */
void glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);

void glBlendColorRgba8CTR(uint32_t rgba);

/* Culling */
void glCullFace(GLenum mode);
void glFrontFace(GLenum mode);

/* Depth Test */
void glDepthFunc(GLenum func);
void glDepthMask(GLboolean flag);
void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);

/* Matrices */
/* Because of differences between perspective and orthographic projection,
 * separate functions are needed to cover all cases
 */
void glGetDirectMatrixfCTR(GLenum mode, GLfloat* m);

void glProjectionMatrixfCTR(const GLfloat* m);              /* any projection, no stereo */

void glPerspectiveProjectionMatrixfCTR(const GLfloat* m,    /* stereo-enabled perspective */
        float nearZ,            /* \     used for stereoscopic rendering    */
        float screenZ,          /*  |----  values are ignored otherwise     */
        float scale             /* /    (see the example for explanation)   */
        );
void glOrthoProjectionMatrixfCTR(const GLfloat* m,          /* stereo-enabled orthographic */
        float skew,             /* units of X per unit of Z at interaxial=1.0 */
        float screenZ           /* Z value where views will converge */
        );

void glModelviewMatrixfCTR(const GLfloat* m);

/* Stencil */
void glStencilFunc(GLenum func, GLint ref, GLuint mask);
void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);

/* **** TEXTURES **** */
void glGenTextures(GLsizei n, GLuint* textures);
void glDeleteTextures(GLsizei n, const GLuint* textures);
/*glActiveTexture*/
void glBindTexture(GLenum target, GLuint texture);  /* target must be GL_TEXTURE_2D */
void glTexImage2D(GLenum target,    /* must be GL_TEXTURE_2D */
        GLint level,                /* must be 0 */
        GLint internalFormat,       /* must be GL_BLOCK_RGBA8_CTR */
        GLsizei width, GLsizei height,
        GLint border,               /* must be 0 */
        GLenum format,              /* must be GL_BLOCK_RGBA_CTR */
        GLenum type,                /* must be GL_UNSIGNED_BYTE */
        const GLvoid* data          /* may be NULL */
        );

GLuint glInitTextureCTR(GLtextureCTR* tex);
void glShutdownTextureCTR(GLtextureCTR* tex);
void glNamedTexImage2DCTR(GLuint texture, GLint level, GLint internalFormat,
        GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type,
        const GLvoid* data);        /* arguments: see glTexImage2D */
void glGetNamedTexDataPointerCTR(GLuint texture, GLvoid** data_out);
void glTexEnvubvCTR(GLenum target,  /* must be GL_TEXTURE_ENV */
        GLenum pname,               /* must be GL_TEXTURE_ENV_COLOR */
        const GLubyte* params);
void glTexEnvi(GLenum target,       /* must be GL_TEXTURE_ENV */
        GLenum pname,               /* must be GL_SRCn_RGB */
        GLint param);

/* **** SHADERS **** */
GLuint glCreateProgram(void);
void glDeleteProgram(GLuint program);
void glUseProgram(GLuint program);
GLint glGetUniformLocation(GLuint program, const GLchar* name);

void glUniform4fv(GLint location, GLsizei count, const GLfloat* value);
void glUniformMatrix4fv(GLint location, GLsizei count,
        GLboolean transpose,        /* must be GL_TRUE */
        const GLfloat* value);

GLuint glInitProgramCTR(GLprogramCTR* prog);
void glShutdownProgramCTR(GLprogramCTR* prog);

/* do not call this function more than once per program */
void glLoadProgramBinaryCTR(GLuint program, const void* shbin, GLsize size);
void glLoadProgramBinary2CTR(GLuint program, void* shbin, GLsize size,
        GLmemoryTransferModeCTR shbinMode);

void glGetProgramDvlbCTR(GLuint program, DVLB_s** dvlb_out);

/* **** VERTEX BUFFERS **** */
void glGenBuffers(GLsizei n, GLuint* buffers);
void glDeleteBuffers(GLsizei n, const GLuint* buffers);
void glBindBuffer(GLenum target, GLuint buffer);

void glBufferData(GLenum target, GLsizeiptr size,
        const GLvoid* data,         /* may be NULL */
        GLenum usage);
void glNamedBufferData(GLuint buffer, GLsizei size,
        const void* data,           /* may be NULL */
        GLenum usage);

void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
void glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizei size, const void* data);

void* glMapBuffer(GLenum target, GLenum access);
void* glMapNamedBuffer(GLuint buffer, GLenum access);

void* glMapBufferRange(GLenum target,
    GLintptr offset,
    GLsizeiptr length,
    GLbitfield access);

void* glMapNamedBufferRange(GLuint buffer,
    GLintptr offset,
    GLsizei length,
    GLbitfield access);

GLboolean glUnmapBuffer(GLenum target);
GLboolean glUnmapNamedBuffer(GLuint buffer);

GLuint glInitBufferCTR(GLbufferCTR* buf);
void glShutdownBufferCTR(GLbufferCTR* buf);

/* **** VERTEX ARRAYS **** */
void glVertexFormatCTR(GLuint numAttribs, GLuint vertexSize);
void glVertexAttribCTR(GLuint index, GLint size, GLenum type);

/* **** STEREO **** */
void glStereoDisableCTR(void);
void glStereoEnableCTR(GLfloat interaxial);

/* **** DRAWING **** */
void glDrawArrays(GLenum mode, GLint first, GLsizei count);

#ifdef __cplusplus
}
#endif

#endif
