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
#include <GLES2/gl2.h>
#include <SDL_opengles2.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
//#include <GLES/egl.h>
typedef	struct ShaderInfo {
	GLuint program;
	GLint a_position;
	GLint a_texcoord;
	GLint u_vp_matrix;
	GLint u_texture;
	GLboolean scanline;
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

static const char* vertexShaderSrc =
	"#version 120\n"
	"uniform mat4 u_vp_matrix;\n"
	"uniform bool scanline;\n"
	"attribute vec4 a_position;\n"
	"attribute vec2 a_texcoord;\n"
	"attribute vec4 in_Colour;\n"
	"varying vec4 v_vColour;\n"
	"varying vec2 v_texcoord;\n"
	"varying vec4 TEX0;\n"
	"void main() {\n"
	"	v_texcoord = a_texcoord;\n"
	"	v_vColour = in_Colour;\n"
	"   if (scanline)\n"
	"	{\n"
	// "   	gl_Position = a_position*u_vp_matrix;\n"
	"   	gl_Position = a_position.x*u_vp_matrix[0] + a_position.y*u_vp_matrix[1] + a_position.z*u_vp_matrix[2] + a_position.w*u_vp_matrix[3];\n"
	"   	TEX0.xy = a_position.xy;\n"
	"	} else {"
	"		gl_Position = u_vp_matrix * a_position;\n"
	"	}\n"
	"}\n";
	
static const char* fragmentShaderSrc =
	"#version 120\n"
	"varying  vec2 v_texcoord;\n"
	"uniform bool scanline;\n"
	"uniform sampler2D u_texture;\n"
	"varying vec4 TEX0;\n"
	"uniform vec2 TextureSize;\n"
	"void main() {\n"
	"   if (!scanline)\n"
	"	{\n"
	"  		vec3 col;\n"
	"  		float x = TEX0.x * TextureSize.x;\n"
	"  		float y = floor(gl_FragCoord.y / 3.0) + 0.5;\n"
	"  		float ymod = mod(gl_FragCoord.y, 3.0);\n"
	"  		vec2 f0 = vec2(x, y);\n"
	"  		vec2 uv0 = f0 / TextureSize.xy;\n"
 	"  		vec3 t0 = texture2D(u_texture, v_texcoord).xyz;\n"
	"  		if (ymod > 2.0) {\n"
	"    		vec2 f1 = vec2(x, y + 1.0);\n"
	"    		vec2 uv1 = f1 / TextureSize.xy;\n"
	"    		vec3 t1 = texture2D(u_texture, uv1).xyz * 0.1;\n"
	"    		col = (t0 + t1) / 1.6;\n"
	"  		} else {\n"
	"    		col = t0;\n"
	"  		} \n"
	"  		gl_FragColor = vec4(col, 1.0);\n"
	"	} else {"
	"		gl_FragColor = texture2D(u_texture, v_texcoord);\n"
	"	}\n"
	"}\n";

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
	glVertexPointer(2, GL_FLOAT, 0, VertexCoord);
	glTexCoordPointer(2, GL_FLOAT, 0, TexCoord);
}

int piInitVideo()
{
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

    if (screenHeight < 600 && video)
        video->scanLinesEnable = 0;

    fprintf(stderr, "Initializing window surface...\n");

    wnd = SDL_CreateWindow("blueMSX", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        screenWidth, screenHeight, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_INPUT_GRABBED);

	fprintf(stderr, "Connecting context to surface...\n");
	// connect the context to the surface
	glc = SDL_GL_CreateContext(wnd);
	assert(glc);
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
#if 0
	msxScreen = (char*)calloc(1, BIT_DEPTH / 8 * TEX_WIDTH * TEX_HEIGHT);
	if (!msxScreen) {
		fprintf(stderr, "Error allocating screen texture\n");
		memset(msxScreen, BIT_DEPTH / 8 * TEX_WIDTH * TEX_HEIGHT, 0xf0);
		return 0;
	}
	fprintf(stderr, "Initializing shaders...\n");

	// Init shader resources
	memset(&shader, 0, sizeof(ShaderInfo));
	shader.program = createProgram(vertexShaderSrc, fragmentShaderSrc);
	if (!shader.program) {
		fprintf(stderr, "createProgram() failed\n");
		return 0;
	}

	fprintf(stderr, "Initializing textures/buffers...\n");

	shader.a_position	= glGetAttribLocation(shader.program,	"a_position");
	shader.a_texcoord	= glGetAttribLocation(shader.program,	"a_texcoord");
	shader.u_vp_matrix	= glGetUniformLocation(shader.program,	"u_vp_matrix");
	shader.u_texture	= glGetUniformLocation(shader.program,	"u_texture");
	shader.scanline		= glGetUniformLocation(shader.program,  "scanline");

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, textures);
	// glBindTexture(GL_TEXTURE_2D, textures[0]);
	// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_WIDTH, TEX_HEIGHT, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, msxScreen);
	glUniform1i(shader.scanline, 1);
	// glGenBuffers(3, buffers);
	// glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
	// glBufferData(GL_ARRAY_BUFFER, kVertexCount * sizeof(GLfloat) * 3, vertices, GL_STATIC_DRAW);
	// glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
	// glBufferData(GL_ARRAY_BUFFER, kVertexCount * sizeof(GLfloat) * 2, uvs, GL_STATIC_DRAW);
	// glBindBuffer(GL_ARRAY_BUFFER, 0);
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[2]);
	// glBufferData(GL_ELEMENT_ARRAY_BUFFER, kIndexCount * sizeof(GL_UNSIGNED_SHORT), indices, GL_STATIC_DRAW);
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_DITHER);

//	fprintf(stderr, "Setting up screen...\n");

	fprintf(stderr, "Initializing SDL video...\n");

	// We're doing our own video rendering - this is just so SDL-based keyboard
	// can work
	// sdlScreen = SDL_SetVideoMode(0, 0, 0, 0);//SDL_ASYNCBLIT);
	return 1;
#endif	
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
		// glDeleteProgram(shader.program);
		// glDeleteBuffers(3, buffers);
		glDeleteTextures(1, textures);
	}
	
	// Release OpenGL resources
}

static void draw() {

	FrameBuffer* frameBuffer = frameBufferFlipViewFrame(properties->emulation.syncMethod == P_EMU_SYNCTOVBLANKASYNC);
	if (frameBuffer == NULL) {
		frameBuffer = frameBufferGetWhiteNoiseFrame();
	}
	if (frameBufferGetDoubleWidth(frameBuffer, 0) != width || height != frameBuffer->lines || (interlace > 0 && frameBuffer->interlace == 0) || (!interlace && frameBuffer->interlace))
	{
		width = frameBufferGetDoubleWidth(frameBuffer, 0);
		height = frameBuffer->lines;
		interlace = frameBuffer->interlace;
		msxScreenPitch = frameBuffer->maxWidth * (width+1);//(256+16)*(width+1);
		VertexCoord[2] = VertexCoord[6] = FB_MAX_LINE_WIDTH * FB_MAX_LINE_WIDTH / msxScreenPitch;
		VertexCoord[5] = VertexCoord[7] = FB_MAX_LINES * FB_MAX_LINES / height;
		// printf("width: %d, w:%d, h:%d, interlace: %d\n", width, msxScreenPitch, height, frameBuffer->interlace);
		// setOrtho(projection, -1, 1,    1,   -1, -0.5f, +0.5f,1,1);		
		// glUniformMatrix4fv(shader.u_vp_matrix, 1, GL_FALSE, projection);		
	}	
	// videoRender(video, 	frameBuffer, BIT_DEPTH, 1, msxScreen, 0, msxScreenPitch*2, -1);
	// glUniform1i(shader.u_texture, textures[0]);
	// glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
	// glVertexAttribPointer(shader.a_position, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), NULL);
	// glEnableVertexAttribArray(shader.a_position);
	// glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
	// glVertexAttribPointer(shader.a_texcoord, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);
	// glEnableVertexAttribArray(shader.a_texcoord);	
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FB_MAX_LINE_WIDTH, height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, frameBuffer->fb);
	// glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, msxScreenPitch, height, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, msxScreen);
	// glClear(GL_COLOR_BUFFER_BIT);
	// drawQuad(&shader);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void piUpdateEmuDisplay()
{
	draw();
	SDL_GL_SwapWindow(wnd);
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
