/*****************************************************************************
**
** blueberryMSX
** https://github.com/pokebyte/blueberryMSX
**
** An MSX Emulator for Raspberry Pi based on blueMSX
**
** Copyright (C) 2003-2006 Daniel Vik
** Copyright (C) 2014 Akop Karapetyan
**
** GLES code is based on
** https://sourceforge.net/projects/atari800/ and
** https://code.google.com/p/pisnes/
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "Properties.h"
#include "VideoRender.h"

#if defined RASPPI
#include <bcm_host.h>
#include <interface/vchiq_arm/vchiq_if.h>
#include <EGL/egl.h>
//#include <GLES2/gl2.h>
#include <SDL_opengl.h>
#include <SDL.h>
static EGL_DISPMANX_WINDOW_T nativeWindow;
static EGLDisplay display = NULL;
static EGLSurface surface = NULL;
static EGLContext context = NULL;
#else
#include <SDL.h>
#include <SDL_opengl.h>
#include <GL/glext.h>
#endif

typedef	struct ShaderInfo {
	GLuint program;
	GLint a_position;
	GLint a_texcoord;
	GLint u_vp_matrix;
	GLint u_texture;
	GLint scanline;  // Changed from GLboolean to GLint to match shader
	GLint width;  // Changed from GLboolean to GLint to match shader
} ShaderInfo;

#define	TEX_WIDTH  FB_MAX_LINE_WIDTH
#define	TEX_HEIGHT FB_MAX_LINES

#define BIT_DEPTH       16
#define BYTES_PER_PIXEL (BIT_DEPTH >> 3)
#define ZOOM            1
#define	WIDTH           640
#define	HEIGHT          480

#define	minU 0.0f
#define	maxU ((float)WIDTH / TEX_WIDTH)	
#define	minV 0.0f
#define	maxV ((float)HEIGHT / TEX_HEIGHT)

extern Video *video;
extern Properties *properties;

static void drawQuad(const ShaderInfo *sh);
static GLuint createShader(GLenum type, const char *shaderSrc);
static GLuint createProgram(const char *vertexShaderSrc, const char *fragmentShaderSrc);
static void setOrtho(float m[4][4],
	float left, float right, float bottom, float top,
	float near, float far, float scaleX, float scaleY);

uint32_t screenWidth = 0;
uint32_t screenHeight = 0;

static ShaderInfo shader;
static GLuint buffers[3];
static GLuint textures[1];

static SDL_Surface *sdlScreen;

unsigned char *msxScreen = NULL;
int msxScreenPitch;
int height;

#if 0
static const char* vertexShaderSrc =
	"#version 120\n"
	"uniform mat4 u_vp_matrix;\n"
	"uniform int scanline;\n"  // Changed from bool to int to match fragment shader
	"attribute vec4 a_position;\n"
	"attribute vec2 a_texcoord;\n"
	"attribute vec4 in_Colour;\n"
	"varying vec4 v_vColour;\n"
	"varying vec2 v_texcoord;\n"
	"varying vec4 TEX0;\n"
	"void main() {\n"
	"	v_texcoord = a_texcoord;\n"
	"	v_vColour = in_Colour;\n"
	"   if (scanline == 1)\n"  // Changed from boolean to int comparison
	"	{\n"
	// "   	gl_Position = a_position*u_vp_matrix;\n"
	"   	gl_Position = a_position.x*u_vp_matrix[0] + a_position.y*u_vp_matrix[1] + a_position.z*u_vp_matrix[2] + a_position.w*u_vp_matrix[3];\n"
	"   	TEX0.xy = a_position.xy;\n"
	"	} else {"
	"		gl_Position = u_vp_matrix * a_position;\n"
	"	}\n"
	"}\n";
	
// 간단한 스캔라인 셰이더로 수정
static const char* fragmentShaderSrc =
    "#version 120\n"
    "varying vec2 v_texcoord;\n"
    "uniform sampler2D u_texture;\n"
    "uniform int scanline;\n"
    "void main() {\n"
    "   vec3 color = texture2D(u_texture, v_texcoord).rgb;\n"
    "   if (scanline == 1) {\n"
    "       float scanlineEffect = 0.85 + 0.15 * mod(gl_FragCoord.y, 2.0);\n"
    "       color *= scanlineEffect;\n"
    "   }\n"
    "   gl_FragColor = vec4(color, 1.0);\n"
    "}\n";
#else
// static const char* fragmentShaderSrc =
//     "#version 120\n"
//     "varying vec2 v_texcoord;\n"
//     "uniform sampler2D u_texture;\n"
//     "uniform int scanline;\n"
//     "void main() {\n"
//     "   vec4 color = texture2D(u_texture, v_texcoord);\n"
//     "   if (scanline == 1) {\n"
//     "       float scanlineEffect = 1.0;\n"
//     "       if (mod(gl_FragCoord.y, 2.0) < 1.0) {\n"
//     "           scanlineEffect = 0.8;\n"
//     "       }\n"
//     "       color.rgb *= scanlineEffect;\n"
//     "   }\n"
//     "   gl_FragColor = color;\n"
//     "}\n";

// 단순화된 버텍스 셰이더 - 문법 오류 수정 (-0.5.0 → -0.5)
static const char* vertexShaderSrc =
    "#version 120\n"
    "attribute vec4 a_position;\n"
    "attribute vec2 a_texcoord;\n"
    "uniform int scanline;\n"
	"uniform int width;\n"
    "varying vec2 v_texcoord;\n"
    "void main() {\n"
    "   v_texcoord = a_texcoord;\n"
	"   if (scanline == 1) \n"
	"	 	gl_Position = vec4((a_texcoord.x) * 4.7 / width - 1, a_texcoord.y * -4.0 + 1.0, 0.0, 1.0);\n"
	"   else \n"
	"   	gl_Position = a_position;\n"
    "}\n";

// 스캔라인 효과 개선 - 더 뚜렷한 효과를 위해 수정
static const char* fragmentShaderSrc =
    "#version 120\n"
    "varying vec2 v_texcoord;\n"
    "uniform sampler2D u_texture;\n"
    "uniform int scanline;\n"
    "void main() {\n"
    "   vec4 color = texture2D(u_texture, v_texcoord);\n"
    "   if (scanline == 1) {\n"
    "       float scanlineEffect = 1.0;\n"
    "       float line = mod(gl_FragCoord.y, 1.5);\n"
    "       if (line < 1.0) {\n"
    "           scanlineEffect = 0.8; // 더 어두운 효과로 조정\n"
    "       }\n"
    "       color.rgb *= scanlineEffect;\n"
    "   }\n"
    "   gl_FragColor = color;\n"
    "}\n";
#endif
static const GLfloat uvs[] = {
	minU, minV,
	maxU, minV,
	maxU, maxV,
	minU, maxV,
};

static const GLushort indices[] = {
	0, 1, 2,
	0, 2, 3,
};

#define kVertexCount 4
#define kIndexCount 6

static float projection[4][4];

static const GLfloat vertices[] = {
	-0.5, -0.5f, 0.0f,
	+0.5f, -0.5f, 0.0f,
	+0.5f, +0.5f, 0.0f,
	-0.5, +0.5f, 0.0f,
};

SDL_Window* wnd;
SDL_GLContext glc;
SDL_Renderer* rdr;

// #define WindowW 800
// #define WindowH 600
// #define FbW 320
// #define FbH 240
// #define FbTexW 0x200
// #define FbTexH 0x100
static float VertexCoord[] = {0, 0, TEX_WIDTH, 0, 0, TEX_HEIGHT, TEX_WIDTH, TEX_HEIGHT};
static float TexCoord[] = {0, 0, 1, 0, 0, 1, 1, 1};
int width = -1;
int lines = -1;
int interlace = -1;

static void initGL() {
    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_INDEX_ARRAY);

    // Calculate viewport for 4:3 aspect ratio with proper centering
    float target_aspect = 4.0f / 3.0f;
    float screen_aspect = (float)screenWidth / screenHeight;
    int viewport_x, viewport_y, viewport_width, viewport_height;

    if (screen_aspect > target_aspect) {
        // Screen is wider than 4:3
        viewport_height = screenHeight;
        viewport_width = (int)(screenHeight * target_aspect);
        viewport_x = (screenWidth - viewport_width) / 2;
        viewport_y = 0;
    } else {
        // Screen is taller than 4:3
        viewport_width = screenWidth;
        viewport_height = (int)(screenWidth / target_aspect);
        viewport_x = 0;
        viewport_y = (screenHeight - viewport_height) / 2;
    }

    // Set viewport with calculated dimensions
    glViewport(viewport_x, viewport_y, viewport_width, viewport_height);
    
    // Clear entire screen to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Rest of GL initialization
    glOrtho(0, TEX_WIDTH, TEX_HEIGHT, 0, 1, -1);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_WIDTH, TEX_HEIGHT, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);

    // Initialize shader program for scanline effect
    memset(&shader, 0, sizeof(ShaderInfo));
    shader.program = createProgram(vertexShaderSrc, fragmentShaderSrc);
    if (shader.program) {
        shader.a_position = glGetAttribLocation(shader.program, "a_position");
        shader.a_texcoord = glGetAttribLocation(shader.program, "a_texcoord");
        shader.u_vp_matrix = glGetUniformLocation(shader.program, "u_vp_matrix");
        shader.u_texture = glGetUniformLocation(shader.program, "u_texture");
        shader.scanline = glGetUniformLocation(shader.program, "scanline");
        shader.width = glGetUniformLocation(shader.program, "width");
        // Use the shader program
        glUseProgram(shader.program);
        glUniform1i(shader.u_texture, 0);  // Texture unit 0
        
        // Set scanline to 0 (off) by default to avoid crashes
        glUniform1i(shader.scanline, 0);
        
        // Print debug info
        printf("Shader program created successfully. scanline uniform location: %d\n", shader.scanline);
    } else {
        glDisable(GL_VERTEX_PROGRAM_ARB);
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
        printf("Failed to create shader program\n");
    }
}

int piInitVideo()
{
#ifdef RASPPI
	bcm_host_init();
	// get an EGL display connection
	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (display == EGL_NO_DISPLAY) {
		fprintf(stderr, "eglGetDisplay() failed: EGL_NO_DISPLAY\n");
		return 0;
	}

	// initialize the EGL display connection
	EGLBoolean result = eglInitialize(display, NULL, NULL);
	if (result == EGL_FALSE) {
		fprintf(stderr, "eglInitialize() failed: EGL_FALSE\n");
		return 0;
	}

	// get an appropriate EGL frame buffer configuration
	EGLint numConfig;
	EGLConfig config;
	static const EGLint attributeList[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};
	result = eglChooseConfig(display, attributeList, &config, 1, &numConfig);
	if (result == EGL_FALSE) {
		fprintf(stderr, "eglChooseConfig() failed: EGL_FALSE\n");
		return 0;
	}

	result = eglBindAPI(EGL_OPENGL_ES_API);
	if (result == EGL_FALSE) {
		fprintf(stderr, "eglBindAPI() failed: EGL_FALSE\n");
		return 0;
	}

	// create an EGL rendering context
	static const EGLint contextAttributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributes);
	if (context == EGL_NO_CONTEXT) {
		fprintf(stderr, "eglCreateContext() failed: EGL_NO_CONTEXT\n");
		return 0;
	}

	// create an EGL window surface
	int32_t success = graphics_get_display_size(0, &screenWidth, &screenHeight);
	if (result < 0) {
		fprintf(stderr, "graphics_get_display_size() failed: < 0\n");
		return 0;
	}

	printf( "Width/height: %d/%d\n", screenWidth, screenHeight);
	if (screenHeight < 600)
	if (screenHeight < 600 && video)
		video->scanLinesEnable = 0;

	VC_RECT_T dstRect;
	dstRect.x = 0;
	dstRect.y = 0;
	dstRect.width = screenWidth;
	dstRect.height = screenHeight;

	VC_RECT_T srcRect;
	srcRect.x = 0;
	srcRect.y = 0;
	srcRect.width = screenWidth << 16;
	srcRect.height = screenHeight << 16;

	DISPMANX_DISPLAY_HANDLE_T dispManDisplay = vc_dispmanx_display_open(0);
	DISPMANX_UPDATE_HANDLE_T dispmanUpdate = vc_dispmanx_update_start(0);
	DISPMANX_ELEMENT_HANDLE_T dispmanElement = vc_dispmanx_element_add(dispmanUpdate,
		dispManDisplay, 0, &dstRect, 0, &srcRect,
		DISPMANX_PROTECTION_NONE, NULL, NULL, DISPMANX_NO_ROTATE);

	nativeWindow.element = dispmanElement;
	nativeWindow.width = screenWidth;
	nativeWindow.height = screenHeight;
	vc_dispmanx_update_submit_sync(dispmanUpdate);

	fprintf(stderr, "Initializing window surface...\n");

	surface = eglCreateWindowSurface(display, config, &nativeWindow, NULL);
	if (surface == EGL_NO_SURFACE) {
		fprintf(stderr, "eglCreateWindowSurface() failed: EGL_NO_SURFACE\n");
		return 0;
	}

	fprintf(stderr, "Connecting context to surface...\n");

	// connect the context to the surface
	result = eglMakeCurrent(display, surface, surface, context);
	if (result == EGL_FALSE) {
		fprintf(stderr, "eglMakeCurrent() failed: EGL_FALSE\n");
		return 0;
	}
#endif
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_ShowCursor(SDL_DISABLE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Get current display resolution
    SDL_DisplayMode dm;
    if (SDL_GetCurrentDisplayMode(0, &dm) != 0) {
        printf("SDL_GetCurrentDisplayMode failed: %s\n", SDL_GetError());
    } else {
		screenWidth = dm.w;
		screenHeight = dm.h;
	}
    printf("Display resolution: %dx%d\n", screenWidth, screenHeight);
	// screenWidth /= 2;
	// screenHeight /= 2;

    if (screenHeight < 600 && video)
        video->scanLinesEnable = 0;
    fprintf(stderr, "Initializing window surface...\n");

    wnd = SDL_CreateWindow("blueMSX", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        screenWidth, screenHeight, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_INPUT_GRABBED);

	fprintf(stderr, "Connecting context to surface...\n");
	// connect the context to the surface
//	glc = SDL_GL_CreateContext(wnd);
//	assert(glc);
	// SDL_GL_MakeCurrent(wnd, glc);
	// SDL_GL_SetSwapInterval( 1 );
	if( SDL_GL_SetSwapInterval( 1 ) < 0 )
	{
		printf( "Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError() );
	}	
	// return;	
	// rdr = SDL_CreateRenderer(wnd, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	// SDL_SetRenderDrawColor(rdr, 0xff, 0, 0, 0xff);
	initGL();
	return 1;
}

void piDestroyVideo()
{
	if (sdlScreen) {
		SDL_FreeSurface(sdlScreen);
	}
	if (msxScreen) {
		free(msxScreen);
	}

	// Destroy shader resources
	if (shader.program) {
		glDeleteProgram(shader.program);
		// glDeleteBuffers(3, buffers);
	}
	glDeleteTextures(1, textures);
	// Release OpenGL resources
#ifdef RASPPI
	eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(display, surface);
	eglDestroyContext(display, context);
	eglTerminate(display);
	bcm_host_deinit();
#endif
}

static void draw() {
	static char scanline = -1;
    FrameBuffer* frameBuffer = frameBufferFlipViewFrame(properties->emulation.syncMethod == P_EMU_SYNCTOVBLANKASYNC);
    if (frameBuffer == NULL) {
        frameBuffer = frameBufferGetWhiteNoiseFrame();
    }
    if (frameBufferGetDoubleWidth(frameBuffer, 0) != width || height != frameBuffer->lines || (interlace > 0 && frameBuffer->interlace == 0) || (!interlace && frameBuffer->interlace))
    {
        width = frameBufferGetDoubleWidth(frameBuffer, 0);
        height = frameBuffer->lines;
        interlace = frameBuffer->interlace;
        msxScreenPitch = frameBuffer->maxWidth * (width+1);
        VertexCoord[2] = VertexCoord[6] = FB_MAX_LINE_WIDTH * FB_MAX_LINE_WIDTH / msxScreenPitch;
        VertexCoord[5] = VertexCoord[7] = FB_MAX_LINES * FB_MAX_LINES / height;
		TexCoord[0] = 0.0f; TexCoord[1] = 0.0f; 
		TexCoord[2] = 1.0f; TexCoord[3] = 0.0f;  
		TexCoord[4] = 0.0f; TexCoord[5] = 1.f; 
		TexCoord[6] = 1.0f; TexCoord[7] = 1.f;
        printf("width:%d, height=%d\n", msxScreenPitch, height);
		if (shader.program) {
			TexCoord[6] = TexCoord[2] = VertexCoord[2];
			TexCoord[5] = TexCoord[7] = VertexCoord[7];
		// Use shader program for scanlines
			// glUseProgram(shader.program);
			glUniform1i(shader.u_texture, 0);
			glUniform1i(shader.scanline, 1);
			glUniform1i(shader.width, width+1);
			
			// 버텍스 및 텍스처 좌표 설정
			if (shader.a_position != -1) {
				glVertexAttribPointer(shader.a_position, 2, GL_FLOAT, GL_FALSE, 0, VertexCoord);
				glEnableVertexAttribArray(shader.a_position);
			}
			
			if (shader.a_texcoord != -1) {
				glVertexAttribPointer(shader.a_texcoord, 2, GL_FLOAT, GL_FALSE, 0, TexCoord);
				glEnableVertexAttribArray(shader.a_texcoord);
			}
		}
	}

	if (video && (video->scanLinesEnable != scanline || scanline == -1))
	{
		if (video->scanLinesEnable)
		{
			glUseProgram(shader.program);
			// VertexCoord[2] = VertexCoord[6] = FB_MAX_LINE_WIDTH * FB_MAX_LINE_WIDTH / msxScreenPitch;
			// VertexCoord[5] = VertexCoord[7] = FB_MAX_LINES * FB_MAX_LINES / height;
			// glUniform1i(shader.u_texture, 0);
			// glUniform1i(shader.scanline, 1);
			// glUniform1i(shader.width, width+1);
			
			// // 버텍스 및 텍스처 좌표 설정
			// if (shader.a_position != -1) {
			// 	glVertexAttribPointer(shader.a_position, 2, GL_FLOAT, GL_FALSE, 0, VertexCoord);
			// 	glEnableVertexAttribArray(shader.a_position);
			// }
			
			// if (shader.a_texcoord != -1) {
			// 	glVertexAttribPointer(shader.a_texcoord, 2, GL_FLOAT, GL_FALSE, 0, TexCoord);
			// 	glEnableVertexAttribArray(shader.a_texcoord);
			// }
		}
		else
		{
			glUseProgram(0);
			TexCoord[0] = 0.0f; TexCoord[1] = 0.0f; 
			TexCoord[2] = 1.0f; TexCoord[3] = 0.0f;  
			TexCoord[4] = 0.0f; TexCoord[5] = 1.f; 
			TexCoord[6] = 1.0f; TexCoord[7] = 1.f;		
		}
		scanline = video->scanLinesEnable;
	}
    // Check if scanlines are enabled and shader is available
    // Update texture with frame data
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FB_MAX_LINE_WIDTH, height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, frameBuffer->fb);
	// Use fixed function pipeline
	glVertexPointer(2, GL_FLOAT, 0, VertexCoord);
	glTexCoordPointer(2, GL_FLOAT, 0, TexCoord);
    
    // Draw the quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    // OpenGL 오류 확인
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("OpenGL error: %d\n", error);
    }
}


void piUpdateEmuDisplay()
{
	draw();
	SDL_GL_SwapWindow(wnd);
}

static GLuint createProgram(const char *vertexShaderSrc, const char *fragmentShaderSrc)
{
    GLuint vertexShader = createShader(GL_VERTEX_SHADER, vertexShaderSrc);
    if (!vertexShader) {
        fprintf(stderr, "createShader(GL_VERTEX_SHADER) failed\n");
        return 0;
    }

    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    if (!fragmentShader) {
        fprintf(stderr, "createShader(GL_FRAGMENT_SHADER) failed\n");
        glDeleteShader(vertexShader);
        return 0;
    }

    GLuint programObject = glCreateProgram();
    if (!programObject) {
        fprintf(stderr, "glCreateProgram() failed: %d\n", glGetError());
        return 0;
    }

    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);

    // Link the program
    glLinkProgram(programObject);

    // Check the link status
    GLint linked = 0;
    glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint infoLen = 0;
        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = (char *)malloc(infoLen);
            glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
            fprintf(stderr, "Error linking program: %s\n", infoLog);
            free(infoLog);
        }

        glDeleteProgram(programObject);
        return 0;
    }

    // Delete these here because they are attached to the program object.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return programObject;
}

static GLuint createShader(GLenum type, const char *shaderSrc)
{
    GLuint shader = glCreateShader(type);
    if (!shader) {
        fprintf(stderr, "glCreateShader() failed: %d\n", glGetError());
        return 0;
    }

    // Load and compile the shader source
    glShaderSource(shader, 1, &shaderSrc, NULL);
    glCompileShader(shader);

    // Check the compile status
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = (char *)malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            fprintf(stderr, "Error compiling shader:\n%s\n", infoLog);
            free(infoLog);
        }

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}
#if 0
void piUpdateEmuDisplay2()
{
	int w = 0;
	if (!shader.program) {
		fprintf(stderr, "Shader not initialized\n");
		return;
	}
	if (properties->video.force4x3ratio)
		w = (screenWidth - (screenHeight*4/3.0f));
	if (w < 0) w = 0;

	// FrameBuffer* frameBuffer;
	FrameBuffer* frameBuffer = frameBufferFlipViewFrame(properties->emulation.syncMethod == P_EMU_SYNCTOVBLANKASYNC);
	if (frameBuffer == NULL) {
		frameBuffer = frameBufferGetWhiteNoiseFrame();
	}
	if (frameBufferGetDoubleWidth(frameBuffer, 0) != width || height != frameBuffer->lines)
	{
		width = frameBufferGetDoubleWidth(frameBuffer, 0);
		height = frameBuffer->lines;
		msxScreenPitch = frameBuffer->maxWidth * (width+1);//(256+16)*(width+1);
		interlace = frameBuffer->interlace;
		float sx = 1.0f * msxScreenPitch/WIDTH;
		float sy = 1.0f * height / HEIGHT;
		printf("screen = %x, width = %d, height = %d, double = %d, interlaced = %d\n", msxScreen, msxScreenPitch, height, width, interlace);
//		printf("sx=%f,sy=%f\n", sx, sy);
		// fflush(stdin);
		// if (sy == 1.0f)
		// 	setOrtho(projection, -sx/2, sx/2,  sy/2, -sy/2, -0.5f, +0.5f,1,1);		
		// else
		// 	setOrtho(projection, -sx/2, sx/2,    0,   -sy, -0.5f, +0.5f,1,1);		
		// setOrtho(projection, -1, 1,    1,   -1, -0.5f, +0.5f,1,1);		
		glUniformMatrix4fv(shader.u_vp_matrix, 1, GL_FALSE, projection);
	}
	videoRender240(video, 	frameBuffer, BIT_DEPTH, 1, msxScreen, 0, msxScreenPitch*2, -1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, msxScreenPitch, height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, msxScreen);
	glClear(GL_COLOR_BUFFER_BIT);
	glUniform1i(shader.u_texture, 0);
	// glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
	// glVertexAttribPointer(shader.a_position, 3, GL_FLOAT,
	// 	GL_FALSE, 3 * sizeof(GLfloat), NULL);
	// glEnableVertexAttribArray(shader.a_position);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
	glVertexAttribPointer(shader.a_texcoord, 2, GL_FLOAT,
		GL_FALSE, 2 * sizeof(GLfloat), NULL);
	glEnableVertexAttribArray(shader.a_texcoord);
	VertexCoord[2] = VertexCoord[6] = TEX_WIDTH * TEX_WIDTH / msxScreenPitch;
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[2]);
	// glDrawElements(GL_TRIANGLES, kIndexCount, GL_UNSIGNED_SHORT, 0);	
	return;	
}

static GLuint createShader(GLenum type, const char *shaderSrc)
{
	GLuint shader = glCreateShader(type);
	if (!shader) {
		fprintf(stderr, "glCreateShader() failed: %d\n", glGetError());
		return 0;
	}

	// Load and compile the shader source
	glShaderSource(shader, 1, &shaderSrc, NULL);
	glCompileShader(shader);

	// Check the compile status
	GLint compiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLint infoLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char* infoLog = (char *)malloc(sizeof(char) * infoLen);
			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			fprintf(stderr, "Error compiling shader:\n%s\n", infoLog);
			free(infoLog);
		}

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

static GLuint createProgram(const char *vertexShaderSrc, const char *fragmentShaderSrc)
{
	GLuint vertexShader = createShader(GL_VERTEX_SHADER, vertexShaderSrc);
	if (!vertexShader) {
		fprintf(stderr, "createShader(GL_VERTEX_SHADER) failed\n");
		return 0;
	}

	GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
	if (!fragmentShader) {
		fprintf(stderr, "createShader(GL_FRAGMENT_SHADER) failed\n");
		glDeleteShader(fragmentShader);
		return 0;
	}

	GLuint programObject = glCreateProgram();
	if (!programObject) {
		fprintf(stderr, "glCreateProgram() failed: %d\n", glGetError());
		return 0;
	}

	glAttachShader(programObject, vertexShader);
	glAttachShader(programObject, fragmentShader);

	// Link the program
	glLinkProgram(programObject);

	// Check the link status
	GLint linked = 0;
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
	if (!linked) {
		GLint infoLen = 0;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char* infoLog = (char *)malloc(infoLen);
			glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
			fprintf(stderr, "Error linking program: %s\n", infoLog);
			free(infoLog);
		}

		glDeleteProgram(programObject);
		return 0;
	}

	// Delete these here because they are attached to the program object.
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return programObject;
}

static void setOrtho(float m[4][4],
	float left, float right, float bottom, float top,
	float near, float far, float scaleX, float scaleY)
{
	memset(m, 0, 4 * 4 * sizeof(float));
	m[0][0] = 2.0f / (right - left) * scaleX;
	m[1][1] = 2.0f / (top - bottom) * scaleY;
	m[2][2] = 0;//-2.0f / (far - near);
	m[3][0] = -(right + left) / (right - left);
	m[3][1] = -(top + bottom) / (top - bottom);
	m[3][2] = 0;//-(far + near) / (far - near);
	m[3][3] = 1;
}

static void drawQuad(const ShaderInfo *sh)
{
	glUniform1i(sh->u_texture, textures[0]);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
	glVertexAttribPointer(sh->a_position, 3, GL_FLOAT,
		GL_FALSE, 3 * sizeof(GLfloat), NULL);
	glEnableVertexAttribArray(sh->a_position);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
	glVertexAttribPointer(sh->a_texcoord, 2, GL_FLOAT,
		GL_FALSE, 2 * sizeof(GLfloat), NULL);
	glEnableVertexAttribArray(sh->a_texcoord);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[2]);

	glDrawElements(GL_TRIANGLES, kIndexCount, GL_UNSIGNED_SHORT, 0);
}
#endif


