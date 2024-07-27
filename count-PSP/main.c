#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdebug.h>

PSP_MODULE_INFO("Hello PSP", PSP_MODULE_USER, 1, 0);

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

int main(){
    pspDebugScreenInit();
    sceDisplayWaitVblankStart();
    SetupCallbacks();
    int counter = 0;
    int i = 0;
    SceCtrlData pad;
    pspDebugScreenPrintf("Press x to start the timer");
    while(1) {
        sceCtrlReadBufferPositive(&pad, 1);
        if(pad.Buttons & PSP_CTRL_CROSS) break;
    }
    while(1) {
        pspDebugScreenClear();
        sceCtrlReadBufferPositive(&pad, 1);
        if(pad.Buttons & PSP_CTRL_CIRCLE) break;
        pspDebugScreenPrintf("Press O to start the timer\n");
        pspDebugScreenPrintf("Counter: %i", counter);

        counter++;

        for(i = 0; i < 5; i++)
            sceDisplayWaitVblankStart();
    }
    pspDebugScreenClear();
    pspDebugScreenPrintf("Final Count: %i", counter);
    sceKernelSleepThread();
    return 0;
}
