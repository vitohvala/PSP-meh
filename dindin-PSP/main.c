#include <pspctrl.h>
#include <pspgum.h>
#include <pspgu.h>
#include <pspdisplay.h>
#include <psprtc.h>
#include <math.h>

#include "pgeFont.h"
#include "graphics.h"
#include "callbacks.h"
#include "psptypes.h"

PSP_MODULE_INFO("dindin", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);


#define RAYWHITE 0xFFF5F5F5
#define GREEN    0xFF00E430
#define BLACK    0xFF000000

typedef struct{
    ScePspFVector2 p_vel;
    float o_vel;
    int o_index;
    int min;
} Helpers;

typedef ScePspFRect Rectangle;

SceKernelUtilsMt19937Context ctx;
static unsigned int __attribute__((aligned(16))) list[262144];

void init_random() {
    u32 tick = sceKernelGetSystemTimeLow();
    sceKernelUtilsMt19937Init(&ctx, tick);
}

int get_random(int min, int max) {
    u32 rand = sceKernelUtilsMt19937UInt(&ctx);
    return min + (rand % (max - min + 1));
}

float get_y() {
    float chance = get_random(0, 10); 
    return (chance > 6) ? 222 : 242; 
}

void init(Rectangle *p, Rectangle *o, Helpers *h) {
    h->p_vel.x = h->p_vel.y = 0;
    p->x = 0;
    p->y = 245;
    p->w = 27;
    p->h = 27;
    h->o_vel = 200;
    h->o_index = 2;
    h->min = 200;
    o[0].x = SCREEN_WIDTH ;
    o[0].y = get_y();
    o[0].w = get_random(17, 24);
    o[0].h = 30;
    for (int i = 1; i < 3; i++) {
        float tmp = o[i - 1].x;
        o[i].x = get_random(tmp + h->min, tmp + h->min + 50);
        o[i].y = get_y();
        o[i].w = get_random(17, 25);
        o[i].h = 30;
    }
}

u8 CheckCollisionRecs(Rectangle rec1, Rectangle rec2) {
    return ((rec1.x < (rec2.x + rec2.w) && (rec1.x + rec1.w) > rec2.x) &&
            (rec1.y < (rec2.y + rec2.h) && (rec1.y + rec1.h) > rec2.y));
}

int get_numlen(int n) {
    int l = 1;
    while((n/10) > 9) l++;
    return l;
}

int main() {
    Rectangle player = { 0, 245, 27, 27};
    Rectangle obstacles[3] = {0};
    Helpers h = {0};

    init(&player, obstacles, &h);
    init_random();
    setupCallbacks();
    init_graphics(list, PRIM_RECT);
    float dt = get_frame_time();
    sceGumMatrixMode(GU_PROJECTION); 
    sceGumLoadIdentity();
    sceGumOrtho(0.f, 480.f, 272.f, 0.f, -10.0f, 10.0f);

    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    int score;
    u8 death = 0;
    u8 f = 0;

    pgeFontInit();
    pgeFont *ry = pgeFontLoad("ps10.ttf", 12, PGE_FONT_SIZE_PIXELS, 128);

    while(1){
        SceCtrlData pad;
        sceCtrlReadBufferPositive(&pad, 1);
        dt = get_frame_time();
        
        if(death) {
            f++;
            start_frame(list);
            sceGuDisable(GU_DEPTH_TEST);
            sceGuDisable(GU_TEXTURE_2D);
            clear(RAYWHITE);

            draw_rectangle(0, 90, 480, 50, BLACK);
            draw_rectangle_rec(player, GREEN);

            for(int i = 0; i < 3; i++){
                draw_rectangle_rec(obstacles[i], BLACK);
            }
            pgeFontActivate(ry);
            pgeFontPrintf(ry, 192, 121, 0xFF0000FF, "GAME OVER");
            end_frame();
            if(f > 120){
                death = 0;
                init(&player, obstacles, &h);
                score = 0;
                f = 0;
            }
            continue;
        }

        h.p_vel.x = 0;
        if(player.x < (float)SCREEN_WIDTH / 4)
            h.p_vel.x = 250;
   
        h.p_vel.y += 1000 * dt;

        if(pad.Buttons & PSP_CTRL_CROSS && player.y + 1 > SCREEN_HEIGHT - player.h) {
            h.p_vel.y = -300;
        }else if(pad.Buttons & PSP_CTRL_DOWN && player.y + 1 > SCREEN_HEIGHT - player.h) {
            player.h = 18;
            player.y = 254;
        } else {
            player.h = 27;
        }

        player.x += h.p_vel.x  * dt;
        player.y += h.p_vel.y * dt;
        
        if(player.y + player.h > SCREEN_HEIGHT) {
            player.y = SCREEN_HEIGHT - player.h;
        }
        
        for(int i = 0; i < 3; i++){
            obstacles[i].x -= h.o_vel * dt;
            if(obstacles[i].x + obstacles[i].w < 0) {
                score += 10;
                float tmp = get_random(200, 300);
                obstacles[i].x = obstacles[h.o_index].x + tmp;
                obstacles[i].y = get_y();
                h.o_index++;
                if(h.o_index > 2){
                    h.o_index = 0;
                }
                if(!(score%100) && score > 0) {
                    h.o_vel += 10;
                    h.min += 3;
                }
            }
        }

        start_frame(list);
        sceGuDisable(GU_DEPTH_TEST);
        sceGuDisable(GU_TEXTURE_2D);
 
        clear(RAYWHITE);
        draw_rectangle_rec(player, GREEN);

        for(int i = 0; i < 3; i++){
            draw_rectangle_rec(obstacles[i], BLACK);
        }
        for(int i = 0; i < 3; i++){
            if(CheckCollisionRecs(player, obstacles[i])){
                death = 1;
                //init(&player, obstacles, &h);
                //score = 0;
            }
        }
        pgeFontActivate(ry);
        pgeFontPrintf(ry, 5, 10, BLACK, "SCORE %d", score);
        end_frame();
    }
    psp_close();
    return 0;
}
