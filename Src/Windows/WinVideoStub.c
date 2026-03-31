/*****************************************************************************
**
** blueberryMSX - Windows Port
**
** SDL2 Rendering Pipeline Implementation
**
******************************************************************************
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <SDL.h>
#include "Properties.h"
#include "VideoRender.h"
#include "PiVideo.h"
#include "FrameBuffer.h"

extern Video *video;
extern Properties *properties;

// Global variables required by other modules
unsigned char *msxScreen = NULL;
int msxScreenPitch = 0;
int height = -1;

uint32_t screenWidth = 0;
uint32_t screenHeight = 0;

static SDL_Window* wnd = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;

int piInitVideo()
{
    printf("piInitVideo: Initializing Windows rendering pipeline...\n");

    // Speed up initialization by forcing directsound
    SDL_setenv("SDL_AUDIODRIVER", "directsound", 1);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init(VIDEO) failed: %s\n", SDL_GetError());
        return 0;
    }

    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    screenWidth = dm.w;
    screenHeight = dm.h;

    // Create a borderless window covering the full screen
    wnd = SDL_CreateWindow("blueMSX", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        screenWidth, screenHeight, 
        SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN_DESKTOP);

    if (!wnd) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 0;
    }

    // Create accelerated renderer
    renderer = SDL_CreateRenderer(wnd, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("Accelerated renderer failed, using software fallback.\n");
        renderer = SDL_CreateRenderer(wnd, -1, SDL_RENDERER_SOFTWARE);
    }

    // KEY: Set logical size to 4:3 (e.g., 640x480).
    // SDL will scale and center the output automatically.
    SDL_RenderSetLogicalSize(renderer, 640, 480);
    
    // Set scaling hint for better quality when stretching
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    // Create streaming texture for the MSX framebuffer
    // Format is RGB565 as identified in MsxTypes.h
    texture = SDL_CreateTexture(renderer, 
        SDL_PIXELFORMAT_RGB565, 
        SDL_TEXTUREACCESS_STREAMING, 
        FB_MAX_LINE_WIDTH, FB_MAX_LINES);

    if (!texture) {
        printf("SDL_CreateTexture failed: %s\n", SDL_GetError());
        return 0;
    }

    // Background audio initialization
    SDL_InitSubSystem(SDL_INIT_AUDIO);

    SDL_ShowCursor(SDL_DISABLE);
    printf("piInitVideo: Complete. Desktop: %dx%d, Logical: 640x480\n", screenWidth, screenHeight);

    return 1;
}

void piDestroyVideo()
{
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (wnd) SDL_DestroyWindow(wnd);
    SDL_Quit();
}

static void draw() {
    FrameBuffer* frameBuffer = frameBufferFlipViewFrame(properties->emulation.syncMethod == P_EMU_SYNCTOVBLANKASYNC);
    if (frameBuffer == NULL) {
        frameBuffer = frameBufferGetWhiteNoiseFrame();
    }
    
    // Update global height for core usage
    height = frameBuffer->lines;
    // msxScreenPitch is expected in bytes by some core modules
    msxScreenPitch = FB_MAX_LINE_WIDTH * 2;

    // The entire buffer width is FB_MAX_LINE_WIDTH
    SDL_Rect srcRect = { 0, 0, FB_MAX_LINE_WIDTH, height };
    
    // Update the texture with the latest framebuffer data
    SDL_UpdateTexture(texture, &srcRect, frameBuffer->fb, FB_MAX_LINE_WIDTH * 2);

    // Clear background (black bars)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // MSX resolution mapping logic:
    // MSX active area is usually centered in FB_MAX_LINE_WIDTH.
    // For 256x192, we want to map it to 4:3 aspect ratio.
    // Since SDL_RenderSetLogicalSize is 640x480 (4:3), 
    // we should map the actual MSX content to fill this logical space.
    
    // If we want to stretch the active MSX content to fill the 4:3 area:
    // Typical MSX active area starts at an offset within FB_MAX_LINE_WIDTH.
    // However, the simplest robust way is to map the full line width if it contains borders.
    
    SDL_RenderCopy(renderer, texture, &srcRect, NULL);

    // Simple scanline overlay (optional)
    if (video && video->scanLinesEnable) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 60);
        for (int i = 0; i < 480; i += 2) {
            SDL_Rect line = { 0, i, 640, 1 };
            SDL_RenderFillRect(renderer, &line);
        }
    }
}

void piUpdateEmuDisplay()
{
    draw();
    SDL_RenderPresent(renderer);
}
