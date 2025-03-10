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

#include <SDL.h>


#include "PiMouse.h"
#include "MsxTypes.h"
#include "ArchInput.h"


static char* arrow[] = {
    "X                               ",
    "XX                              ",
    "X.X                             ",
    "X..X                            ",
    "X...X                           ",
    "X....X                          ",
    "X.....X                         ",
    "X......X                        ",
    "X.......X                       ",
    "X........X                      ",
    "X.........X                     ",
    "X..... XXXXX                    ",
    "X..X...X                        ",
    "X.X X...X                       ",
    "XX  X...X                       ",
    "X    X...X                      ",
    "     X...X                      ",
    "      X...X                     ",
    "      X...X                     ",
    "       XXX                      ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                "
};

static char* cross[] = {
    "             XXXXX              ",
    "            X.....X             ",
    "            X.....X             ",
    "            X.....X             ",
    "            X.....X             ",
    "             X...X              ",
    "             X...X              ",
    "             X...X              ",
    "              X.X               ",
    "              X.X               ",
    "              X.X               ",
    "               X                ",
    " XXXX          X          XXXX  ",
    "X....XXX               XXX....X ",
    "X.......XXX    X    XXX.......X ",
    "X..........XX X X XX..........X ",
    "X.......XXX    X    XXX.......X ",
    "X....XXX               XXX....X ",
    " XXXX          X          XXXX  ",
    "               X                ",
    "              X.X               ",
    "              X.X               ",
    "              X.X               ",
    "             X...X              ",
    "             X...X              ",
    "             X...X              ",
    "            X.....X             ",
    "            X.....X             ",
    "            X.....X             ",
    "            X.....X             ",
    "             XXXXX              ",
    "                                "
};

typedef struct SdlMouse {
    int hasFocus;
    SDL_Cursor* cursorArrow;
    SDL_Cursor* cursorCross;
    AmEnableMode mode;
    struct {
        int top;
        int left;
        int bottom;
        int right;
    } captRgn;
    int buttonState;
    int defX;
    int defY;
    int lockDx;
    int lockDy;
    int dx;
    int dy;
} SDLMouse;

SDLMouse mouse;

static SDL_Cursor* sdlCreateCursor(const char* image[], int hx, int hy)
{
    Uint8 data[4 * 32];
    Uint8 mask[4 * 32];
    int x;
    int y;
    int i = 0;

    for (y = 0; y < 32; y++) {
        for (x = 0; x < 32; x++) {
            mask[i] = (mask[i] << 1) | (image[y][x] != ' ' ? 1 : 0);
            data[i] = (data[i] << 1) | (image[y][x] == 'X' ? 1 : 0);
            if (x % 8 == 7) {
                i++;
            }
        }
    }
    
    return SDL_CreateCursor(data, mask, 32, 32, hx, hy);
}

static int piMouseInRect(int x, int y)
{
    return x > mouse.captRgn.left && x < mouse.captRgn.right &&
           y > mouse.captRgn.top  && y < mouse.captRgn.bottom;
}

void piMouseSetCaptureRect(int x, int y, int width, int height)
{
    if (mouse.cursorArrow == NULL) {
        mouse.cursorArrow = sdlCreateCursor(arrow, 0, 0);
    }

    if (mouse.cursorCross == NULL) {
        mouse.cursorCross = sdlCreateCursor(cross, 15, 15);
    }

    mouse.captRgn.top    = y;
    mouse.captRgn.left   = x;
    mouse.captRgn.bottom = y + height - 1;
    mouse.captRgn.right  = x + width - 1;
    mouse.defX           = x + 100;
    mouse.defY           = y + 100;

    SDL_SetCursor(mouse.cursorArrow);
}

void piMouseInit()
{
    SDL_ShowCursor(SDL_DISABLE);
    mouse.hasFocus = 1;
    SDL_WarpMouseGlobal(mouse.defX, mouse.defY);
}

void piMouseButton(int button, int pressed)
{
    if (mouse.mode == AM_DISABLE) {
        return;
    }
    if (pressed) {
        if (button == SDL_BUTTON_LEFT)   mouse.buttonState |= 1;
        if (button == SDL_BUTTON_MIDDLE) mouse.buttonState |= 2;
        if (button == SDL_BUTTON_RIGHT)  mouse.buttonState |= 2;
    }
    else {
        if (button == SDL_BUTTON_LEFT)   mouse.buttonState &= ~1;
        if (button == SDL_BUTTON_MIDDLE) mouse.buttonState &= ~2;
        if (button == SDL_BUTTON_RIGHT)  mouse.buttonState &= ~2;
    }
}

void piMouseMove(int x, int y)
{
    if (mouse.mode == AM_ENABLE_LASER) {
        mouse.hasFocus = piMouseInRect(x, y);
        mouse.dx = 0x10000 * (x - mouse.captRgn.left) / (mouse.captRgn.right  - mouse.captRgn.left);
        mouse.dy = 0x10000 * (y - mouse.captRgn.top)  / (mouse.captRgn.bottom - mouse.captRgn.top);
        if (mouse.hasFocus) {
            SDL_SetCursor(mouse.cursorCross);
        }
        else {
            SDL_SetCursor(mouse.cursorArrow);
        }
    }
 
    if (mouse.mode == AM_ENABLE_MOUSE) {
        int dx = x - mouse.defX;
        int dy = y - mouse.defY;

        mouse.lockDx += dx;
        mouse.lockDy += dy;
        mouse.dx     += dx;
        mouse.dy     += dy;

        SDL_WarpMouseGlobal(mouse.defX, mouse.defY);
    }
}

void archMouseGetState(int* dx, int* dy) 
{ 
    *dx = 0;
    *dy = 0;

    if (mouse.hasFocus) {
        if (mouse.mode == AM_ENABLE_LASER) {
            *dx = mouse.dx;
            *dy = mouse.dy;
        }

        if (mouse.mode == AM_ENABLE_MOUSE) {
            *dx = -mouse.dx;
            *dy = -mouse.dy;
            mouse.dx = 0;
            mouse.dy = 0;
        }
    }
}

int archMouseGetButtonState(int checkAlways) 
{ 
    if (mouse.hasFocus) {
        return mouse.buttonState; 
    }
    else {
        return 0;
    }
}

void archMouseEmuEnable(AmEnableMode mode) 
{ 
   mouse.mode = mode;
}

