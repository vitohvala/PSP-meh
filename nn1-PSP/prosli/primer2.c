#include "psptypes.h"
#include "sys/_intsup.h"
#include "sys/types.h"
#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspgu.h>
#include <pspgum.h>

#define SCREEN_WIDTH    480
#define SCREEN_HEIGHT 	272
#define BUFF_WIDTH      512
#define uint unsigned int

typedef struct {
    unsigned int color;
    float x, y, z;
} Vertex;

PSP_MODULE_INFO("nn2-PSP", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static uint __attribute__((aligned(16))) list[262144];

Vertex __attribute__((aligned(16))) vertices[3] = {
    {0xFF00FF00, 0.35f, 0.0f, -1.0f},
    {0xFF0000FF, -0.35f, 0.0f, -1.0f},
    {0xFFFF0000, 0.0f, 0.5f, -1.0f},
};

Vertex __attribute__((aligned(16))) square[4] = {
    {0xFF00FF00, -0.35f, -0.35f, -1.0f},
    {0xFF0000FF, -0.35f, 0.35f, -1.0f},
    {0xFFFF0000, 0.35f, 0.35f, -1.0f},
    //{0xFFFF0000, 0.35f, 0.35f, -1.0f},
    {0xFFFFFFFF, 0.35f, -0.35f, -1.0f},
    //{0xFF00FF00, -0.35f, -0.35f, -1.0f},
};

unsigned short __attribute__((aligned(16))) indices[6] = { 
    0, 1, 2, 2, 3, 0
};

int exit_callback(int arg1, int arg2, void* common){
    sceKernelExitGame();
    return 0;
}

int CallbackThread(SceSize args, void* argp){
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int SetupCallbacks(void){
    int thid = 0;
    thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
    if(thid > -1){
        sceKernelStartThread(thid, 0, 0);
    }
    return thid;
}

static uint get_memory_size(uint width, uint height, uint psm){
    uint size = width * height;
    switch (psm) {
        case GU_PSM_T4:  return size / 2;
        case GU_PSM_T8:  return size;
        case GU_PSM_5650:
        case GU_PSM_5551:
        case GU_PSM_4444:
        case GU_PSM_T16: return size * 2;
        case GU_PSM_8888:  
        case GU_PSM_T32: return size * 4;
        default: return 0;
    }
}

void *get_static_vram_buffer(uint width, uint height, uint psm) {
    static uint static_offset = 0;
    uint mem_size = get_memory_size(width, height, psm);
    void *result = (void*)static_offset;
    static_offset += mem_size;
    return result;
}
void *get_static_vram_texture(uint width, uint height, uint psm) {
    void *result = get_static_vram_buffer(width, height, psm);
    return (void*)( ((uint) result) + ((uint)sceGeEdramGetAddr()));
}

void init_graphics(void) {
    void *fbp0 = get_static_vram_buffer(BUFF_WIDTH, SCREEN_HEIGHT, GU_PSM_8888);
    void *fbp1 = get_static_vram_buffer(BUFF_WIDTH, SCREEN_HEIGHT, GU_PSM_8888);
    void *zbp =  get_static_vram_buffer(BUFF_WIDTH, SCREEN_HEIGHT, GU_PSM_4444);

    sceGuInit();
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, fbp0, BUFF_WIDTH);
    sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, fbp1,BUFF_WIDTH);
    sceGuDepthBuffer(zbp, BUFF_WIDTH);
    sceGuOffset(2048 - (SCREEN_WIDTH / 2), 2048 - (SCREEN_HEIGHT / 2));
    sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    sceGuDepthRange(65535, 0);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDepthFunc(GU_EQUAL);
    sceGuFrontFace(GU_CW);
    sceGuEnable(GU_CULL_FACE); //
    sceGuShadeModel(GU_SMOOTH);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuEnable(GU_CLIP_PLANES);

    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblank();
    sceGuDisplay(GU_TRUE);
}

void start_frame(){
    sceGuStart(GU_DIRECT, list);
}

void end_frame(){
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblank();
    sceGuSwapBuffers();
}

void reset_translate(float x, float y, float z) {
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();

    ScePspFVector3 v = {x, y, z};
    sceGumTranslate(&v);
}

void display(){
    start_frame();
    
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_TEXTURE_2D);

    sceGuClearColor(0xFF000000);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT | GU_STENCIL_BUFFER_BIT);

    reset_translate(-0.5f, 0.0f, 0.0f);
    sceGumDrawArray(GU_TRIANGLES, 
            GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
            3, NULL, vertices);

    reset_translate(1.0f, 0.25f, 0.0f);
    sceGumDrawArray(GU_TRIANGLES, 
            GU_INDEX_16BIT | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
            6, indices, square);

    end_frame();
}

int main(){
    SetupCallbacks();
    init_graphics();

    sceGumMatrixMode(GU_PROJECTION); //2d
    sceGumLoadIdentity();
    sceGumOrtho(-16.0f / 9.0f, 16.0f / 9.0f, -1.0f, 1.0f, -10.0f, 10.0f);

    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();

    while(1) {
        display();
    }

    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
