#include <pspdisplay.h>
#include <psprtc.h>
#include <pspkernel.h>
#include <pspgu.h>
#include <pspctrl.h>
#include <pspgum.h>
#include <math.h>
#include <malloc.h>
#include <stdlib.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SCREEN_WIDTH    480
#define SCREEN_HEIGHT 	272
#define BUFF_WIDTH      512
#define uint unsigned int
#define uchar unsigned char
#define MAX(a, b) (a > b) ? a : b
#define MIN(a, b) (a < b) ? a : b

typedef struct {
    float u, v;
    unsigned int color;
    float x, y, z;
} Vertex;

typedef struct {
    uint width, height;
    uint pw, ph;
    void *data;
}Texture;

typedef struct {
    float x, y;
    float rot;
} Camera2D;

typedef struct {
    float x, y, z;
    float yaw, pitch;
} Camera3D;

PSP_MODULE_INFO("Camera-PSP", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static uint __attribute__((aligned(16))) list[262144];

Vertex __attribute__((aligned(16))) square[4] = {
    {0.0f, 0.0f, 0xFF00FF00, -0.25f, -0.25f, -1.0f},
    {1.0f, 0.0f, 0xFF0000FF, -0.25f, 0.25f, -1.0f},
    {1.0f, 1.0f, 0xFFFF0000, 0.25f, 0.25f, -1.0f},
    {0.0f, 1.0f, 0xFFFFFFFF, 0.25f, -0.25f, -1.0f},
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
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

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

void reset_transform(float x, float y, float z) {
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();

    ScePspFVector3 v = {x, y, z};
    sceGumTranslate(&v);
}


uint pow2(const uint value) {
    uint powtwo = 1;
    while(powtwo < value) {
        powtwo <<= 2;
    }
    return powtwo;
}

void copy_tex_data(void *dest, const void *src, const int pw, const int width, const int height){
    for(uint y = 0; y < height; ++y){
        for(uint x = 0; x < width; ++x){
            ((uint*) dest)[x + y  * pw] = ((uint*)src)[x + y * width];
        }
    }
}


void swizzle_fast(uchar *out, const uchar *in, uint width, uint height) {
    uint blockx, blocky, j;

    uint width_blocks = (width / 16);
    uint height_blocks = (height / 8);

    uint src_pitch = (width - 16) / 4;
    uint src_row = width * 8;

    const uchar *ysrc = in;
    uint *dst = (uint*)out;
    for(blocky = 0; blocky < height_blocks; ++blocky){
        const uchar *xsrc = ysrc;
        for(blockx = 0; blockx < width_blocks; ++blockx){
            const uint *src = (uint*) xsrc;
            for(j = 0; j < 8; ++j){
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                src += src_pitch;
            }
            xsrc += 16;
        }
        ysrc += src_row;
    }
}

Texture *load_texture(const char *filename, const int vram) {
    int width, height, nchannels;
    stbi_set_flip_vertically_on_load(GU_TRUE);

    uchar *data = stbi_load(filename, &width, &height, &nchannels, STBI_rgb_alpha);

    if(!data) return NULL;


    Texture *tex = (Texture *)malloc(sizeof(Texture));
    if(!tex) return NULL;

    tex->width = width;
    tex->height = height;
    tex->pw = pow2(width);
    tex->ph = pow2(height);

    size_t size = tex->ph * tex->pw * 4;
    uint *data_buffer = (uint*)memalign(16, size);
    if(!data_buffer) {
        free(tex);
        return NULL;
    }
    copy_tex_data(data_buffer, data, tex->pw, tex->width, tex->height);
    stbi_image_free(data);

    uint *swizzled_pixels = NULL;
    if(vram) 
        swizzled_pixels = (uint*)get_static_vram_texture(tex->pw, tex->ph, GU_PSM_8888);
    else swizzled_pixels = (uint*)memalign(16, size);

    if(!swizzled_pixels) {
        free(tex);
        free(data_buffer);
        return NULL;
    }
    swizzle_fast((uchar*)swizzled_pixels, (const uchar*)data_buffer, tex->pw * 4, tex->ph);

    tex->data = swizzled_pixels;

    free(data_buffer);
    sceKernelDcacheWritebackInvalidateAll();
    return tex;
}

void bind_texture(Texture *tex) {
    if(!tex) return;

    sceGuTexMode(GU_PSM_8888, 0, 0, GU_TRUE);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA); //outputcolr = vertexcolor * texturecolor
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    sceGuTexWrap(GU_REPEAT, GU_REPEAT);
    sceGuTexImage(0, tex->pw, tex->ph, tex->pw, tex->data);
}
void clear(uint Color) {
    sceGuClearColor(Color);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT | GU_STENCIL_BUFFER_BIT);
}

void display_tex(Texture *tex){
    bind_texture(tex);
    sceGumDrawArray(GU_TRIANGLES, GU_INDEX_16BIT | GU_TEXTURE_32BITF |
                    GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                    6, indices, square);
}

void apply_camera(const Camera2D *cam){
    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    
    // SCALE -> ROTATION -> TRANSLATION
    // do this in reverse order
    // TODO: sceGumScale 
    ScePspFVector3 v = {cam->x, cam->y, 0.0f};
    sceGumTranslate(&v);
    sceGumRotateZ(cam->rot / 180.0f / 3.14159f);
    
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
}
float get_frame_time(){
    u64 tick;
    static u64 last_tick;
    sceRtcGetCurrentTick(&tick);
    float dt = (tick - last_tick) / (float)sceRtcGetTickResolution();
    last_tick = tick;
    return dt;
}

int main(){
    SetupCallbacks();
    init_graphics();

    sceGumMatrixMode(GU_PROJECTION); 
    sceGumLoadIdentity();
    sceGumOrtho(-16.0f / 9.0f, 16.0f / 9.0f, -1.0f, 1.0f, -10.0f, 10.0f);

    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    Texture *tex = load_texture("./container.jpg", GU_TRUE);
    Texture *tex2 = load_texture("./circle.png", GU_TRUE);
    
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    Camera2D camera = {0.5f, 0.0f, 45.0f};
    get_frame_time();

    while(1) {
        SceCtrlData buttons;
        sceCtrlReadBufferPositive(&buttons, 1);
        start_frame();
        sceGuDisable(GU_DEPTH_TEST);

        clear(0xFF000000);
        apply_camera(&camera);

        reset_transform(-0.5f, 0.0f, 0.0f);
        display_tex(tex);

        reset_transform(0.5f, 0.0f, 0.0f);
        display_tex(tex2);
        //camera.rot += 00.0f * get_frame_time();
        camera.rot += 200.0f * get_frame_time();

        float analog_x = (buttons.Lx - 128) / 128.0;
        float analog_y = (buttons.Ly - 128) / 128.0;

        if(fabsf(analog_x) < 0.02f) analog_x = 0;
        if(fabsf(analog_y) < 0.02f) analog_x = 0;
        
        camera.x += analog_x * 0.01;
        camera.y -= analog_y * 0.01;

        camera.x = MIN(1.8, camera.x);
        camera.x = MAX(-1.8f, camera.x);
        camera.y = MIN(1.0, camera.y);
        camera.y = MAX(-1.0f, camera.y);
        
        if(buttons.Buttons & PSP_CTRL_DOWN)
            camera.y = MAX(-1, camera.y - 0.01f);
        if(buttons.Buttons & PSP_CTRL_UP) 
            camera.y = MIN(1, camera.y + 0.01f);
        if(buttons.Buttons & PSP_CTRL_LEFT) 
            camera.x = MAX(-1.8, camera.x - 0.01f);
        if(buttons.Buttons & PSP_CTRL_RIGHT) 
            camera.x = MIN(1.8, camera.x + 0.01f);



        end_frame();
    }

    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
