#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <stdio.h>
#include <math.h>

#include "callbacks.h"
#include "graphics.h"

PSP_MODULE_INFO("grid", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define printf pspDebugScreenPrintf

#define RAYWHITE 0xFFF5F5F5
#define GREEN    0xFF00E430
#define BLACK    0xFF000000
#define MAX(X, Y) (X > Y) ? X : Y
#define MIN(X, Y) (X < Y) ? X : Y

int main() {

    ScePspFVector2 p = {0};
    setupCallbacks();
    pspDebugScreenInit();
    initGraphics();
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    while(1){
        SceCtrlData pad;
        sceCtrlReadBufferPositive(&pad, 1);

        float analogX = (pad.Lx - 128) / 128.0f;
        float analogY = (pad.Ly - 128) / 128.0f;

        if (fabsf(analogX) > 0.2) p.x += analogX;
        if (fabsf(analogY) > 0.2) p.y += analogY;
        
        p.x = MIN(110, p.x);
        p.x = MAX(0, p.x);
        p.y = MIN(110, p.y);
        p.y = MAX(0, p.y);

        if(pad.Buttons & PSP_CTRL_LEFT)
            p.x = MAX(0, p.x - 1);
        if(pad.Buttons & PSP_CTRL_RIGHT)
            p.x = MIN(110, p.x + 1);
        if(pad.Buttons & PSP_CTRL_UP)
            p.y = MAX(0, p.y - 1);
        if(pad.Buttons & PSP_CTRL_DOWN)
            p.y = MIN(110, p.y + 1);

        clearScreen(RAYWHITE);
        for (int i = 0; i <= 120; i += 10) {
            drawLineScreen(0, i, 120, i, GREEN);
        }
        for (int i = 0; i <= 120; i += 10) {
            drawLineScreen(i, 0, i, 120, GREEN);
        }
        fillScreenRect(BLACK, (int)p.x, (int)p.y, 10, 10);
        flipScreen();
    }
    sceKernelExitGame();
    return 0;
}
