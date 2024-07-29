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
    init_graphics(list);

    sceGumMatrixMode(GU_PROJECTION); 
    sceGumLoadIdentity();
    sceGumOrtho(0, 480, 0.0f, 272.0f, -10.0f, 10.0f);

    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();

    Texture *tex = load_texture("terrain.png", GU_TRUE, GU_FALSE);
    Texture *tex2 = load_texture("default.png", GU_TRUE, GU_FALSE);

    if(tex == NULL) goto cleanup;
    if(tex2 == NULL) goto cleanup;

    TextureAtlas atlas = { .w = 16, .h = 16 };
    Tilemap *til = create_tilemap(atlas, tex, 8, 8); //can fail
    Tilemap *til2 = create_tilemap(atlas, tex2, 24, 24); //can fail
    if(til2 == NULL) goto cleanup;
                                                         //
    til2->x = 144;
    til2->y = 16;


    draw_text(til2, "Hello World");
    build_tilemap(til2);

    for(int y = 0; y < 8; y++){
        for(int x = 0; x < 8; x++){
            Tile tile = {.x = x, .y = y, .tex_index = x + y * 8};
            til->tiles[x + y * 8] = tile;
        }
    }

    til->x = 176;
    til->y = 136;
    build_tilemap(til);
    
    while(1) {
        start_frame(list);
        sceGuDisable(GU_DEPTH_TEST);
        sceGuClearColor(0xFF000000);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT | GU_STENCIL_BUFFER_BIT);

        draw_tilemap(til);
        draw_tilemap(til2);

        end_frame();
    }
    destroy_tilemap(til);
    destroy_tilemap(til2);
cleanup:
    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
