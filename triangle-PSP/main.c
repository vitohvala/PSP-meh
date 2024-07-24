#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspdebug.h>

#define SCREEN_WIDTH    480
#define SCREEN_HEIGHT 	272
#define BUFF_WIDTH      512

PSP_MODULE_INFO("Triangle PSP", PSP_MODULE_USER, 1, 0);

static unsigned int list[64*1024];

typedef struct {
    unsigned int color;
    float x, y, z;
} Vertex;

Vertex vertices[] = {
    {0xFF0000FF, 0,  1, -1},
    {0xFF00FF00, 1, -1, -1},
    {0xFFFF0000,-1, -1, -1},
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

void init(void) {
	void *video_ram = 0;
	void *disp_ram  = video_ram;
	void *draw_ram  = disp_ram + BUFF_WIDTH * SCREEN_HEIGHT * 4;

	sceGuInit();
	sceGuStart(GU_DIRECT, list);
	{
        sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, disp_ram, BUFF_WIDTH);
		sceGuDrawBuffer(GU_PSM_8888, draw_ram, BUFF_WIDTH);
        sceGuOffset(0, 0);
        sceGuViewport(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 
                      SCREEN_WIDTH, SCREEN_HEIGHT);
		sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		sceGuEnable(GU_SCISSOR_TEST);
        sceGuShadeModel(GU_SMOOTH);
        sceGuDisplay(GU_TRUE);
	} 
	sceGuFinish();
	sceGuSync(0, 0);
	sceDisplayWaitVblank();
	sceGuSwapBuffers();
}

void display(){
	sceGuStart(GU_DIRECT, list);

	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(90, 16.0/9.0, 1, 100);

	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();

    sceGuClearColor(0xFF2B353B);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);

    sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D, 3, 0, vertices);

	sceGuFinish();
	sceGuSync(0, 0);
	sceDisplayWaitVblank();
	sceGuSwapBuffers();

}

int main(){
    SetupCallbacks();
    int running = 1;
    init();
    
    while(running) {
        display();
    }
    sceKernelExitGame();
    return 0;
}
