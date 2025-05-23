/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Sdl/SdlInput.c,v $
**
** $Revision: 1.11 $
**
** $Date: 2008-03-31 19:42:23 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
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

#include "ArchInput.h"
#include "Language.h"
#include "Properties.h"
#include "InputEvent.h"
#include "JoystickPort.h"
#include <stdio.h>
#include <SDL.h>

static int kbdTable[3][2048];

static int inputTypeScanStart = 0;
static int inputTypeScanEnd = 1;

extern Properties *properties;

static void initKbdTable()
{
	memset (kbdTable, 0, sizeof(kbdTable));

	kbdTable[0][SDL_SCANCODE_0] = EC_0;
	kbdTable[0][SDL_SCANCODE_1] = EC_1;
	kbdTable[0][SDL_SCANCODE_2] = EC_2;
	kbdTable[0][SDL_SCANCODE_3] = EC_3;
	kbdTable[0][SDL_SCANCODE_4] = EC_4;
	kbdTable[0][SDL_SCANCODE_5] = EC_5;
	kbdTable[0][SDL_SCANCODE_6] = EC_6;
	kbdTable[0][SDL_SCANCODE_7] = EC_7;
	kbdTable[0][SDL_SCANCODE_8] = EC_8;
	kbdTable[0][SDL_SCANCODE_9] = EC_9;

	kbdTable[0][SDL_SCANCODE_MINUS       ] = EC_NEG;
	kbdTable[0][SDL_SCANCODE_EQUALS      ] = EC_CIRCFLX;
	kbdTable[0][SDL_SCANCODE_GRAVE   	 ] = EC_AT;
	kbdTable[0][SDL_SCANCODE_LEFTBRACKET ] = EC_LBRACK; // EC_AT;
	kbdTable[0][SDL_SCANCODE_RIGHTBRACKET] = EC_RBRACK;
	kbdTable[0][SDL_SCANCODE_SEMICOLON   ] = EC_SEMICOL;
	kbdTable[0][SDL_SCANCODE_APOSTROPHE  ] = EC_COLON; // EC_COLON; 
	kbdTable[0][SDL_SCANCODE_BACKSLASH   ] = EC_BKSLASH;
	kbdTable[0][SDL_SCANCODE_COMMA       ] = EC_COMMA;
	kbdTable[0][SDL_SCANCODE_PERIOD      ] = EC_PERIOD;
	kbdTable[0][SDL_SCANCODE_SLASH       ] = EC_DIV;
	kbdTable[0][SDL_SCANCODE_ESCAPE   ] = EC_ESC;
	kbdTable[0][SDL_SCANCODE_TAB      ] = EC_TAB;
	kbdTable[0][SDL_SCANCODE_BACKSPACE] = EC_BKSPACE;
	kbdTable[0][SDL_SCANCODE_RETURN   ] = EC_RETURN;
	kbdTable[0][SDL_SCANCODE_SPACE    ] = EC_SPACE;

	kbdTable[0][SDL_SCANCODE_A] = EC_A;
	kbdTable[0][SDL_SCANCODE_B] = EC_B;
	kbdTable[0][SDL_SCANCODE_C] = EC_C;
	kbdTable[0][SDL_SCANCODE_D] = EC_D;
	kbdTable[0][SDL_SCANCODE_E] = EC_E;
	kbdTable[0][SDL_SCANCODE_F] = EC_F;
	kbdTable[0][SDL_SCANCODE_G] = EC_G;
	kbdTable[0][SDL_SCANCODE_H] = EC_H;
	kbdTable[0][SDL_SCANCODE_I] = EC_I;
	kbdTable[0][SDL_SCANCODE_J] = EC_J;
	kbdTable[0][SDL_SCANCODE_K] = EC_K;
	kbdTable[0][SDL_SCANCODE_L] = EC_L;
	kbdTable[0][SDL_SCANCODE_M] = EC_M;
	kbdTable[0][SDL_SCANCODE_N] = EC_N;
	kbdTable[0][SDL_SCANCODE_O] = EC_O;
	kbdTable[0][SDL_SCANCODE_P] = EC_P;
	kbdTable[0][SDL_SCANCODE_Q] = EC_Q;
	kbdTable[0][SDL_SCANCODE_R] = EC_R;
	kbdTable[0][SDL_SCANCODE_S] = EC_S;
	kbdTable[0][SDL_SCANCODE_T] = EC_T;
	kbdTable[0][SDL_SCANCODE_U] = EC_U;
	kbdTable[0][SDL_SCANCODE_V] = EC_V;
	kbdTable[0][SDL_SCANCODE_W] = EC_W;
	kbdTable[0][SDL_SCANCODE_X] = EC_X;
	kbdTable[0][SDL_SCANCODE_Y] = EC_Y;
	kbdTable[0][SDL_SCANCODE_Z] = EC_Z;

	kbdTable[0][SDL_SCANCODE_F1       ] = EC_F1;
	kbdTable[0][SDL_SCANCODE_F2       ] = EC_F2;
	kbdTable[0][SDL_SCANCODE_F3       ] = EC_F3;
	kbdTable[0][SDL_SCANCODE_F4       ] = EC_F4;
	kbdTable[0][SDL_SCANCODE_F5       ] = EC_F5;
	kbdTable[0][SDL_SCANCODE_PAGEUP   ] = EC_STOP;
	kbdTable[0][SDL_SCANCODE_END      ] = EC_SELECT;
	kbdTable[0][SDL_SCANCODE_HOME     ] = EC_CLS;
	kbdTable[0][SDL_SCANCODE_INSERT   ] = EC_INS;
	kbdTable[0][SDL_SCANCODE_DELETE   ] = EC_DEL;
	kbdTable[0][SDL_SCANCODE_LEFT     ] = EC_LEFT;
	kbdTable[0][SDL_SCANCODE_UP       ] = EC_UP;
	kbdTable[0][SDL_SCANCODE_RIGHT    ] = EC_RIGHT;
	kbdTable[0][SDL_SCANCODE_DOWN     ] = EC_DOWN;

	kbdTable[0][SDL_SCANCODE_RCTRL       ] = EC_UNDSCRE;
	kbdTable[0][SDL_SCANCODE_KP_MULTIPLY] = EC_NUMMUL;
	kbdTable[0][SDL_SCANCODE_KP_PLUS    ] = EC_NUMADD;
	kbdTable[0][SDL_SCANCODE_KP_DIVIDE  ] = EC_NUMDIV;
	kbdTable[0][SDL_SCANCODE_KP_MINUS   ] = EC_NUMSUB;
	kbdTable[0][SDL_SCANCODE_KP_PERIOD  ] = EC_NUMPER;
	kbdTable[0][SDL_SCANCODE_PAGEDOWN   ] = EC_NUMCOM;
	kbdTable[0][SDL_SCANCODE_KP_0] = EC_NUM0;
	kbdTable[0][SDL_SCANCODE_KP_1] = EC_NUM1;
	kbdTable[0][SDL_SCANCODE_KP_2] = EC_NUM2;
	kbdTable[0][SDL_SCANCODE_KP_3] = EC_NUM3;
	kbdTable[0][SDL_SCANCODE_KP_4] = EC_NUM4;
	kbdTable[0][SDL_SCANCODE_KP_5] = EC_NUM5;
	kbdTable[0][SDL_SCANCODE_KP_6] = EC_NUM6;
	kbdTable[0][SDL_SCANCODE_KP_7] = EC_NUM7;
	kbdTable[0][SDL_SCANCODE_KP_8] = EC_NUM8;
	kbdTable[0][SDL_SCANCODE_KP_9] = EC_NUM9;

	kbdTable[0][SDL_SCANCODE_LGUI  ] = EC_TORIKE;
	kbdTable[0][SDL_SCANCODE_RGUI  ] = EC_JIKKOU;
	kbdTable[0][SDL_SCANCODE_LSHIFT  ] = EC_LSHIFT;
	kbdTable[0][SDL_SCANCODE_RSHIFT  ] = EC_RSHIFT;
	kbdTable[0][SDL_SCANCODE_LCTRL   ] = EC_CTRL;
	kbdTable[0][SDL_SCANCODE_LALT    ] = EC_GRAPH;
	kbdTable[0][SDL_SCANCODE_RALT    ] = EC_CODE;
	kbdTable[0][SDL_SCANCODE_CAPSLOCK] = EC_CAPS;
	kbdTable[0][SDL_SCANCODE_KP_ENTER] = EC_PAUSE;
	kbdTable[0][SDL_SCANCODE_SYSREQ  ] = EC_PRINT;

	kbdTable[1][SDL_SCANCODE_SPACE       ] = EC_JOY1_BUTTON1;
	kbdTable[1][SDL_SCANCODE_LCTRL       ] = EC_JOY1_BUTTON2;
	kbdTable[1][SDL_SCANCODE_LEFT        ] = EC_JOY1_LEFT;
	kbdTable[1][SDL_SCANCODE_UP          ] = EC_JOY1_UP;
	kbdTable[1][SDL_SCANCODE_RIGHT       ] = EC_JOY1_RIGHT;
	kbdTable[1][SDL_SCANCODE_DOWN        ] = EC_JOY1_DOWN;
	kbdTable[1][SDL_SCANCODE_0           ] = EC_COLECO1_0;
	kbdTable[1][SDL_SCANCODE_1           ] = EC_COLECO1_1;
	kbdTable[1][SDL_SCANCODE_2           ] = EC_COLECO1_2;
	kbdTable[1][SDL_SCANCODE_3           ] = EC_COLECO1_3;
	kbdTable[1][SDL_SCANCODE_4           ] = EC_COLECO1_4;
	kbdTable[1][SDL_SCANCODE_5           ] = EC_COLECO1_5;
	kbdTable[1][SDL_SCANCODE_6           ] = EC_COLECO1_6;
	kbdTable[1][SDL_SCANCODE_7           ] = EC_COLECO1_7;
	kbdTable[1][SDL_SCANCODE_8           ] = EC_COLECO1_8;
	kbdTable[1][SDL_SCANCODE_9           ] = EC_COLECO1_9;
	kbdTable[1][SDL_SCANCODE_MINUS       ] = EC_COLECO1_STAR;
	kbdTable[1][SDL_SCANCODE_EQUALS      ] = EC_COLECO1_HASH;

	kbdTable[2][SDL_SCANCODE_KP_0     ] = EC_COLECO2_0;
	kbdTable[2][SDL_SCANCODE_KP_1     ] = EC_COLECO2_1;
	kbdTable[2][SDL_SCANCODE_KP_2     ] = EC_COLECO2_2;
	kbdTable[2][SDL_SCANCODE_KP_3     ] = EC_COLECO2_3;
	kbdTable[2][SDL_SCANCODE_KP_4     ] = EC_COLECO2_4;
	kbdTable[2][SDL_SCANCODE_KP_5     ] = EC_COLECO2_5;
	kbdTable[2][SDL_SCANCODE_KP_6     ] = EC_COLECO2_6;
	kbdTable[2][SDL_SCANCODE_KP_7     ] = EC_COLECO2_7;
	kbdTable[2][SDL_SCANCODE_KP_8     ] = EC_COLECO2_8;
	kbdTable[2][SDL_SCANCODE_KP_9     ] = EC_COLECO2_9;
	kbdTable[2][SDL_SCANCODE_KP_MULTIPLY    ] = EC_COLECO2_STAR;
	kbdTable[2][SDL_SCANCODE_KP_DIVIDE      ] = EC_COLECO2_HASH;
}

void piInputResetJoysticks()
{
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	SDL_JoystickEventState(SDL_ENABLE);
	SDL_JoystickOpen(0);
	SDL_JoystickOpen(1);
}

void piInputResetMSXDevices(int realMice, int realJoysticks)
{
	int port = 0;
	// Any connected joysticks take priority
	while (realJoysticks > 0 && port < 2) {
		if (port == 0) {
			properties->joy1.typeId = JOYSTICK_PORT_JOYSTICK;
			joystickPortSetType(port, JOYSTICK_PORT_JOYSTICK);
		} else if (port == 1) {
			properties->joy2.typeId = JOYSTICK_PORT_JOYSTICK;
			joystickPortSetType(port, JOYSTICK_PORT_JOYSTICK);
		}
		
		realJoysticks--;
		port++;

		fprintf(stderr, "Connecting a joystick to port %d\n", port);
	}

	// If there are still open ports and a mouse,
	// connect it to the remaining port 
	if (realMice > 0) {
		if (port == 0) {
			properties->joy1.typeId = (int)JOYSTICK_PORT_MOUSE;
			joystickPortSetType(port++, (JoystickPortType)properties->joy1.typeId);
			fprintf(stderr, "Connecting a mouse to port 1\n");
		} else if (port == 1) {
			properties->joy2.typeId = (int)JOYSTICK_PORT_MOUSE;
			joystickPortSetType(port++, (JoystickPortType)properties->joy2.typeId);
			fprintf(stderr, "Connecting a mouse to port 2\n");
		}
		
		realMice--;
	}
}

void keyboardInit(Properties *properties)
{
	if (strncmp(properties->emulation.machineName, "COL", 3) == 0) {
		inputTypeScanStart = 1;
		inputTypeScanEnd = 2;
		fprintf(stderr, "Initializing ColecoVision input\n");
	}

	initKbdTable();
	inputEventReset();
}

void keyboardUpdate(SDL_KeyboardEvent *event)
{
	int i;
	for (i = inputTypeScanStart; i <= inputTypeScanEnd; i++) {
		if (event->keysym.scancode == 0x7A) {
			if (event->type == SDL_KEYUP) 
				inputEventUnset(kbdTable[i][SDL_SCANCODE_RALT]);
				// inputEventUnset(kbdTable[i][0]);
			else if (event->type == SDL_KEYDOWN)
				inputEventSet(kbdTable[i][SDL_SCANCODE_RALT]);
				// inputEventUnset(kbdTable[i][0]);
		} else if (event->type == SDL_KEYUP) {
			if (event->keysym.sym == 0 && event->keysym.scancode == 58)
				inputEventUnset(kbdTable[i][SDL_SCANCODE_CAPSLOCK]);
				// inputEventUnset(kbdTable[i][1]);
			else
				inputEventUnset(kbdTable[i][event->keysym.scancode]);
		} else if (event->type == SDL_KEYDOWN) {
			if (event->keysym.sym == 0 && event->keysym.scancode == 58)
				inputEventSet(kbdTable[i][SDL_SCANCODE_CAPSLOCK]);
				// inputEventSet(kbdTable[i][1]);
			else
				inputEventSet(kbdTable[i][event->keysym.scancode]);
		}
	}
}

void joystickAxisUpdate(SDL_JoyAxisEvent *event)
{
	if (event->which == 0) {
		if (event->axis == 0) {
			// Left/right
			if (event->value < -3200) {
				inputEventSet(EC_JOY1_LEFT);
				inputEventUnset(EC_JOY1_RIGHT);
			} else if (event->value > 3200) {
				inputEventUnset(EC_JOY1_LEFT);
				inputEventSet(EC_JOY1_RIGHT);
			} else {
				inputEventUnset(EC_JOY1_RIGHT);
				inputEventUnset(EC_JOY1_LEFT);
			}
		} else if (event->axis == 1) {
			// Up/down
			if (event->value < -3200) {
				inputEventSet(EC_JOY1_UP);
				inputEventUnset(EC_JOY1_DOWN);
			} else if (event->value > 3200) {
				inputEventUnset(EC_JOY1_UP);
				inputEventSet(EC_JOY1_DOWN);
			} else {
				inputEventUnset(EC_JOY1_UP);
				inputEventUnset(EC_JOY1_DOWN);
			}
		}
	} else if (event->which == 1) {
		if (event->axis == 0) {
			// Left/right
			if (event->value < -3200) {
				inputEventSet(EC_JOY2_LEFT);
				inputEventUnset(EC_JOY2_RIGHT);
			} else if (event->value > 3200) {
				inputEventUnset(EC_JOY2_LEFT);
				inputEventSet(EC_JOY2_RIGHT);
			} else {
				inputEventUnset(EC_JOY2_RIGHT);
				inputEventUnset(EC_JOY2_LEFT);
			}
		} else if (event->axis == 1) {
			// Up/down
			if (event->value < -3200) {
				inputEventSet(EC_JOY2_UP);
				inputEventUnset(EC_JOY2_DOWN);
			} else if (event->value > 3200) {
				inputEventUnset(EC_JOY2_UP);
				inputEventSet(EC_JOY2_DOWN);
			} else {
				inputEventUnset(EC_JOY2_UP);
				inputEventUnset(EC_JOY2_DOWN);
			}
		}
	}
}

void joystickButtonUpdate(SDL_JoyButtonEvent *event)
{
	if (event->type == SDL_JOYBUTTONDOWN) {
		if (event->which == 0) {
			if (event->button == 0) {
				inputEventSet(EC_JOY1_BUTTON1);
			} else if (event->button == 1) {
				inputEventSet(EC_JOY1_BUTTON2);
			} else if (event->button == 2) {
				inputEventSet(EC_JOY1_BUTTON1);
			} else if (event->button == 3) {
				inputEventSet(EC_JOY1_BUTTON2);
				inputEventSet(EC_JOY1_BUTTON4);
			} else if (event->button == 8) {
                		inputEventSet(EC_JOY_BUTTONL);
            		} else if (event->button == 9) {
                		inputEventSet(EC_JOY_BUTTONR);
            		}
		} else if (event->which == 1) {
			if (event->button == 0) {
				inputEventSet(EC_JOY2_BUTTON1);
			} else if (event->button == 1) {
				inputEventSet(EC_JOY2_BUTTON2);
			} else if (event->button == 2) {
				inputEventSet(EC_JOY2_BUTTON1);
			} else if (event->button == 3) {
				inputEventSet(EC_JOY2_BUTTON2);
				inputEventSet(EC_JOY2_BUTTON4);
			} else if (event->button == 8) {
                		inputEventSet(EC_JOY_BUTTONL);
            		} else if (event->button == 9) {
                		inputEventSet(EC_JOY_BUTTONR);
			}
		}
	} else if (event->type == SDL_JOYBUTTONUP) {
		if (event->which == 0) {
			if (event->button == 0) {
				inputEventUnset(EC_JOY1_BUTTON1);
			} else if (event->button == 1) {
				inputEventUnset(EC_JOY1_BUTTON2);
			} else if (event->button == 2) {
				inputEventUnset(EC_JOY1_BUTTON1);
			} else if (event->button == 3) {
				inputEventUnset(EC_JOY1_BUTTON2);
				inputEventUnset(EC_JOY1_BUTTON4);
			} else if (event->button == 8) {
                		inputEventUnset(EC_JOY_BUTTONL);
            		} else if (event->button == 9) {
                		inputEventUnset(EC_JOY_BUTTONR);
			}
		} else if (event->which == 1) {
			if (event->button == 0) {
				inputEventUnset(EC_JOY2_BUTTON1);
			} else if (event->button == 1) {
				inputEventUnset(EC_JOY2_BUTTON2);
			} else if (event->button == 2) {
				inputEventUnset(EC_JOY2_BUTTON1);
			} else if (event->button == 3) {
				inputEventUnset(EC_JOY2_BUTTON2);
				inputEventUnset(EC_JOY2_BUTTON4);
			} else if (event->button == 8) {
                		inputEventUnset(EC_JOY_BUTTONL);
            		} else if (event->button == 9) {
                		inputEventUnset(EC_JOY_BUTTONR);
			}
		}
	}
}

void  archUpdateJoystick() {}
UInt8 archJoystickGetState(int joystickNo) { return 0; }
int   archJoystickGetCount() { return 0; }
char* archJoystickGetName(int index) { return ""; }
void  archMouseSetForceLock(int lock) { }
void  archPollInput() { }
void  archKeyboardSetSelectedKey(int keyCode) {}
char* archGetSelectedKey() { return ""; }
char* archGetMappedKey() { return ""; }
int   archKeyboardIsKeyConfigured(int msxKeyCode) { return 0; }
int   archKeyboardIsKeySelected(int msxKeyCode) { return 0; }
char* archKeyconfigSelectedKeyTitle() { return ""; }
char* archKeyconfigMappedToTitle() { return ""; }
char* archKeyconfigMappingSchemeTitle() { return ""; }
