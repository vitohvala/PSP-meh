#include "psptypes.h"
#include <pspctrl.h>
#include <stdlib.h>
#include <string.h>
#include <pspgu.h>
#include <pspgum.h>
#include <malloc.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "graphics.h"
#include "callbacks.h"

FT_Face face;
FT_Library  library;


typedef struct {
    unsigned int texture_id;
    ScePspVector2 size;
    ScePspVector2 bearing;
    unsigned int advance;
} Character;

PSP_MODULE_INFO("Tilemaps-PSP", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static uint __attribute__((aligned(16))) list[262144];

int main(){
    SetupCallbacks();
    init_graphics(list);

    sceGumMatrixMode(GU_PROJECTION); 
    sceGumLoadIdentity();
    sceGumOrtho(0.f, 480.f, 272.0f, 0.f, -10.0f, 10.0f);

    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();


    Texture *tex = load_texture("terrain.png", GU_TRUE, GU_FALSE);

    if(tex == NULL) goto cleanup;

    TextureAtlas atlas = { .w = 16, .h = 16 };
    Tilemap *til = create_tilemap(atlas, tex, 16, 16, 8, 8); //can fail
                                                         //

    for(int y = 0; y < 16; y++){
        for(int x = 0; x < 16; x++){
            Tile tile = {.x = x, .y = y, .tex_index = x + y * 16};
            til->tiles[x + y * 16] = tile;
        }
    }

    til->x = 0;
    til->y = 0;
    build_tilemap(til);

    if(FT_Init_FreeType(&library)){
        goto tclean;
    }
    //Note that you must not deallocate the font file buffer before calling FT_Done_Face. 
    if(FT_New_Face(library,"ps10.ttf", 0, &face)){
        goto tclean;
    }
    FT_Set_Pixel_Sizes(face, 0, 10);

    char characters[] = "Score FPS1234567890";

    while(1) {
        start_frame(list);
        sceGuDisable(GU_DEPTH_TEST);
        clear(0xFF000000);
        draw_tilemap(til);

        end_frame();
    }
tclean:
    destroy_tilemap(til);
cleanup:
    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
