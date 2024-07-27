#include <pspctrl.h>
#include <pspgum.h>
#include <pspgu.h>
#include <pspdisplay.h>
#include <psprtc.h>
#include <math.h>

#include "callbacks.h"

PSP_MODULE_INFO("dindin", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);


#define RAYWHITE 0xFFF5F5F5
#define GREEN    0xFF00E430
#define BLACK    0xFF000000
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272
#define BUF_WIDTH     512

typedef struct {
    float x, y;
    float width, height;
} Rectangle;

typedef struct {
    float x, y;
}Vector2;

typedef struct{
    Vector2 p_vel;
    float o_vel;
    int o_index;
    int min;
} Helpers;

typedef struct {
    float u, v;
    unsigned int color;
    float x, y, z;
}Vertex;

SceKernelUtilsMt19937Context ctx;
static unsigned int __attribute__((aligned(16))) list[262144];

void init_graphics(){
    sceGuInit();
    sceGuStart(GU_DIRECT, list);

    sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUF_WIDTH);
    sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, (void*)0x88000, BUF_WIDTH);
    sceGuDepthBuffer((void*)0x110000, BUF_WIDTH);
    sceGuOffset(2048 - (SCREEN_WIDTH/2), 2048 - (SCREEN_HEIGHT/2));
    sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceGuDepthRange(0xc350, 0x2710);
    sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuFrontFace(GU_CW);
    sceGuShadeModel(GU_SMOOTH);
    sceGuEnable(GU_CULL_FACE);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuEnable(GU_CLIP_PLANES);

    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

void draw_rectangle(Rectangle *p, unsigned int color) {
    Vertex* v = (Vertex*)sceGuGetMemory(2 * sizeof(Vertex));
    v[0].color = color;  
    v[0].x = p->x;
    v[0].y = p->y;
    v[0].z = v[0].u = v[0].v = 0;
    v[1].color = color; 
    v[1].x = p->x + p->width;
    v[1].y = p->y + p->height;
    v[1].z = v[1].u = v[1].v = 1;

    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, v);
}

float get_frame_time(){
    u64 tick;
    static u64 last_tick;
    sceRtcGetCurrentTick(&tick);
    float dt = (tick - last_tick) / (float)sceRtcGetTickResolution();
    last_tick = tick;
    return dt;
}

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
    p->width = 27;
    p->height = 27;
    h->o_vel = 250;
    h->o_index = 2;
    h->min = 200;
    o[0].x = SCREEN_WIDTH ;
    o[0].y = get_y();
    o[0].width = get_random(17, 24);
    o[0].height = 30;
    for (int i = 1; i < 3; i++) {
        float tmp = o[i - 1].x;
        o[i].x = get_random(tmp + h->min, tmp + h->min + 50);
        o[i].y = get_y();
        o[i].width = get_random(17, 25);
        o[i].height = 30;
    }
}

u8 CheckCollisionRecs(Rectangle rec1, Rectangle rec2) {
    return ((rec1.x < (rec2.x + rec2.width) && (rec1.x + rec1.width) > rec2.x) &&
            (rec1.y < (rec2.y + rec2.height) && (rec1.y + rec1.height) > rec2.y));
}



int main() {
    Rectangle player = { 0, 245, 27, 27};
    Rectangle obstacles[3] = {0};
    Helpers h = {0};

    init(&player, obstacles, &h);
    init_random();
    setupCallbacks();
    init_graphics();
    float dt = get_frame_time();

    while(1){
        SceCtrlData pad;
        sceCtrlReadBufferPositive(&pad, 1);
        dt = get_frame_time();

        h.p_vel.x = 0;
        if(player.x < (float)SCREEN_WIDTH / 4)
            h.p_vel.x = 250;
   
        h.p_vel.y += 1000 * dt;

        if(pad.Buttons & PSP_CTRL_CROSS && player.y + 1 > SCREEN_HEIGHT - player.height) {
            h.p_vel.y = -300;
        }else if(pad.Buttons & PSP_CTRL_DOWN && player.y + 1 > SCREEN_HEIGHT - player.height) {
            player.height = 18;
            player.y = 254;
        } else {
            player.height = 27;
        }

        player.x += h.p_vel.x  * dt;
        player.y += h.p_vel.y * dt;
        
        if(player.y + player.height > SCREEN_HEIGHT) {
            player.y = SCREEN_HEIGHT - player.height;
        }
        
        for(int i = 0; i < 3; i++){
            obstacles[i].x -= h.o_vel * dt;
            if(obstacles[i].x + obstacles[i].width < 0) {
                float tmp = get_random(200, 300);
                obstacles[i].x = obstacles[h.o_index].x + tmp;
                obstacles[i].y = get_y();
                h.o_index++;
                if(h.o_index > 2){
                    h.o_index = 0;
                }
            }
        }

        sceGuStart(GU_DIRECT, list);
        
         
        sceGuClearColor(RAYWHITE);
        sceGuClear(GU_COLOR_BUFFER_BIT);

        draw_rectangle(&player, GREEN);

        for(int i = 0; i < 3; i++){
            draw_rectangle(&obstacles[i], BLACK);
        }
       
        for(int i = 0; i < 3; i++){
            if(CheckCollisionRecs(player, obstacles[i])){
                init(&player, obstacles, &h);
            }
        }
        sceGuFinish();
        sceGuSync(0, 0);
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    sceGuTerm();

    sceKernelExitGame();
    return 0;
}
