#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <stdio.h>
#include <psppower.h>

#include "callbacks.h"
#include "graphics.h"

PSP_MODULE_INFO("BC", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define printf pspDebugScreenPrintf

#define RAYWHITE 0xFFF5F5F5
#define GREEN    0xFF00E430
#define BLACK    0xFF000000
#define RED      0xFF0000FF
#define BLUE     0xFFFF0000

#define MAX(X, Y) (X > Y) ? X : Y
#define MIN(X, Y) (X < Y) ? X : Y

int main() {
    scePowerSetClockFrequency(333, 333, 166);
    setupCallbacks();
    pspDebugScreenInit();
    initGraphics();
    int i;
    Color back = RAYWHITE;
    int selComponent = 0;
    char filler[10];

    Color highlightColor = 0xFFC8C8C8;
    Color dimmedColor =    0xFF646464;
    Color shadowColorH =   0xFF373737;
    Color shadowColorD = 0xFF373737;

    while(1){
        SceCtrlData pad;
        sceCtrlReadBufferPositive(&pad, 1);
        if(pad.Buttons & PSP_CTRL_UP) {
            if(selComponent > 0) {
                selComponent--;
            }
            for(i=0; i<10; i++) {
                sceDisplayWaitVblankStart();
            }
        } else if(pad.Buttons & PSP_CTRL_DOWN) {
            if(selComponent < 2) {
                selComponent++;
            }
            for(i=0; i<10; i++) {
                sceDisplayWaitVblankStart();
            }
        }
        if(pad.Buttons & PSP_CTRL_RIGHT) {
            switch(selComponent) {
                case 0:
                    if((back & 0xFF) < 255)
                        back++;
                    break;
                case 1:
                    if(((back >> 8) & 0xFF) < 255)
                    back += 0x100;
                    break;
                case 2:
                    if(((back >> 16) & 0xFF) < 255)
                    back += 0x10000;
                    break;
                default:
                    break;
            }
        } else if(pad.Buttons & PSP_CTRL_LEFT) {
            switch(selComponent) {
                case 0:
                    if((back & 0xFF) > 0)
                        back--;
                    break;
                case 1:
                    if(((back >> 8) & 0xFF) > 0)
                        back -= 0x100;
                    break;
                case 2:
                    if(((back >> 16) & 0xFF) > 0)
                        back -= 0x10000;
                    break;
                default:
                    break;
            }
        }
        clearScreen(back);
        sprintf(filler, " RED: %i", (back & 0xFF));
        if(selComponent == 0) {
            printTextScreen(11, 10, filler, shadowColorH);
            printTextScreen(10, 10, filler, highlightColor);
        } else {
            printTextScreen(11, 10, filler, shadowColorD);
            printTextScreen(10, 10, filler, dimmedColor);
        }
        sprintf(filler, "GREEN: %i", (back >> 8) & 0xFF);
        if(selComponent == 1) {
            printTextScreen(11, 20, filler, shadowColorH);
            printTextScreen(10, 20, filler, highlightColor);
        } else {
            printTextScreen(11, 20, filler, shadowColorD);
            printTextScreen(10, 20, filler, dimmedColor);
        }

        sprintf(filler, " BLUE: %i", (back >> 16) & 0xFF);
        if(selComponent == 2) {
            printTextScreen(11, 30, filler, shadowColorH);
            printTextScreen(10, 30, filler, highlightColor);
        } else {
            printTextScreen(11, 30, filler, shadowColorD);
            printTextScreen(10, 30, filler, dimmedColor);
        }
        flipScreen();
        sceDisplayWaitVblankStart();
    }
    sceKernelExitGame();
    return 0;
}
