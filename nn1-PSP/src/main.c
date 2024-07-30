#include <pspctrl.h>
#include <stdlib.h>
#include <string.h>
#include <pspgu.h>
#include <pspgum.h>
#include <malloc.h>

#include "graphics.h"
#include "callbacks.h"

PSP_MODULE_INFO("Tilemaps-PSP", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static uint __attribute__((aligned(16))) list[262144];


int main(){
    SetupCallbacks();
    init_graphics(list, PRIM_RECT);

    sceGumMatrixMode(GU_PROJECTION); 
    sceGumLoadIdentity();
    sceGumOrtho(0.f, 480.f, 272.f, 0.f, -10.0f, 10.0f);

    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    
    
    while(1) {
        start_frame(list);
        sceGuDisable(GU_DEPTH_TEST);
        sceGuDisable(GU_TEXTURE_2D);
        clear(0xFFFFFFFF);
        draw_rectangle(439, 231, 40, 40, 0xFF00FF00);

        end_frame();
    }
    psp_close();
    return 0;
}
