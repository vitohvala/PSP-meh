#include "pspgu.h"
#include "psptypes.h"
#include "strings.h"
#include <pspdisplay.h>
#include <psprtc.h>
#include <malloc.h>
#include <stdlib.h>
#include <time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <string.h>

#include "graphics.h"


//MESH functions

static Mesh *rect;

SimpleVertex create_simple_vert(float x, float y) {
    SimpleVertex v = {
        .x = x,
        .y = y,
        .z = 0.0f,
    };
    return v;
}

//primitive
void draw_rectangle(float x, float y, float width, float height, unsigned int color) {
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();

    ScePspFVector3 v1 = {x + (width / 2),y + (height / 2), 0.0f};
    sceGumTranslate(&v1);

    //sceGumRotateZ(90.0f);

    ScePspFVector3 v = {width, height, 0.0f};
    sceGumScale(&v); 
   
    sceGuColor(color);

    sceGumDrawArray(GU_TRIANGLES, GU_INDEX_16BIT | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
            rect->index_count, rect->indices, rect->data);
}

void draw_rectangle_rec(ScePspFRect r, unsigned int color) {
    draw_rectangle(r.x, r.y, r.w, r.h, color);
}

Mesh *create_mesh(u32 vcount, u32 index_count, size_t size_vertex) {
    Mesh *mesh = malloc(sizeof(Mesh));
    if(mesh == NULL) return NULL;
    
    mesh->data = memalign(16, size_vertex * vcount);
    if(mesh->data == NULL) {
        free(mesh);
        return NULL;
    }
    mesh->indices = memalign(16, sizeof(u16) * index_count);
    if(mesh->indices == NULL){
        free(mesh->data);
        free(mesh);
        return NULL;
    }
    mesh->index_count = index_count;

    return mesh;
}

void draw_mesh(Mesh *mesh){
    sceGumDrawArray(GU_TRIANGLES, GU_INDEX_16BIT | GU_TEXTURE_32BITF |
                   GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                   mesh->index_count, mesh->indices, mesh->data);
}

void destroy_mesh(Mesh *mesh){
    free(mesh->data);
    free(mesh->indices);
    free(mesh);
}



//Vertex
//
Vertex create_vert(float u, float v, unsigned int color, float x, float y, float z) {
    Vertex vert = {
        .u = u,
        .v = v,
        .color = color,
        .x = x,
        .y = y,
        .z = z,
    };
    return vert;
}


//Sprite
Sprite *create_sprite(float x, float y, float sx, float sy, Texture *tex){
    Sprite *sprite = malloc(sizeof(Sprite));
    if(sprite == NULL) return NULL;
    
    sprite->mesh = create_mesh(4, 6, sizeof(Vertex));
    if(sprite->mesh == NULL) {
        free(sprite);
        return NULL;
    }

    sprite->x = x;
    sprite->y = y;
    sprite->rot = 0;
    sprite->layer = 0;
    sprite->sx =sx;
    sprite->sy = sy;
    sprite->tex = tex;
    sprite->mesh->index_count = 6;
    ((Vertex*)sprite->mesh->data)[0] = create_vert(0, 0, 0XFFFFFFFF, -0.25f, -0.25f, 0.0f);
    ((Vertex*)sprite->mesh->data)[1] = create_vert(0, 1, 0XFFFFFFFF, -0.25f,  0.25f, 0.0f);
    ((Vertex*)sprite->mesh->data)[2] = create_vert(1, 1, 0XFFFFFFFF,  0.25f,  0.25f, 0.0f);
    ((Vertex*)sprite->mesh->data)[3] = create_vert(1, 0, 0XFFFFFFFF,  0.25f, -0.25f, 0.0f);
    
    sprite->mesh->indices[0] = 0;
    sprite->mesh->indices[1] = 1;
    sprite->mesh->indices[2] = 2;
    sprite->mesh->indices[3] = 2;
    sprite->mesh->indices[4] = 3;
    sprite->mesh->indices[5] = 0;
    
    sceKernelDcacheWritebackInvalidateAll();
    return sprite;
}

void destroy_sprite(Sprite *sprite){
    destroy_mesh(sprite->mesh);
    free(sprite);
}

void draw_sprite(Sprite *sprite) {
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    
    ScePspFVector3 v = { sprite->x, sprite->y, sprite->layer};
    sceGumTranslate(&v);
    sceGumRotateZ(sprite->rot);

    ScePspFVector3 s = { sprite->sx, sprite->sy, 1.0f};
    sceGumScale(&s);
    bind_texture(sprite->tex);
    draw_mesh(sprite->mesh);
}

//Texture
Texture *load_texture(const char *filename, const int vram, int flip) {
    int width, height, nchannels;
    stbi_set_flip_vertically_on_load(flip);

    unsigned char *data = stbi_load(filename, &width, &height, &nchannels, STBI_rgb_alpha);

    if(!data) return NULL;


    Texture *tex = (Texture *)malloc(sizeof(Texture));
    if(!tex) return NULL;

    tex->width = width;
    tex->height = height;
    tex->pw = pow2(width);
    tex->ph = pow2(height);

    size_t size = tex->ph * tex->pw * 4;
    unsigned int *data_buffer = (unsigned int*)memalign(16, size);
    if(!data_buffer) {
        free(tex);
        return NULL;
    }
    copy_tex_data(data_buffer, data, tex->pw, tex->width, tex->height);
    stbi_image_free(data);

    unsigned int *swizzled_pixels = NULL;
    if(vram) 
        swizzled_pixels = (unsigned int*)getStaticVramTexture(tex->pw, tex->ph, GU_PSM_8888);
    else swizzled_pixels = (unsigned int*)memalign(16, size);

    if(!swizzled_pixels) {
        free(tex);
        free(data_buffer);
        return NULL;
    }
    swizzle_fast((unsigned char*)swizzled_pixels, (const unsigned char*)data_buffer, tex->pw * 4, tex->ph);

    tex->data = swizzled_pixels;

    free(data_buffer);
    sceKernelDcacheWritebackInvalidateAll();
    return tex;
}

void copy_tex_data(void *dest, const void *src, const int pw, const int width, const int height){
    for(unsigned int y = 0; y < height; ++y){
        for(unsigned int x = 0; x < width; ++x){
            ((unsigned int*) dest)[x + y  * pw] = ((unsigned int*)src)[x + y * width];
        }
    }
}

void bind_texture(Texture *tex) {
    if(!tex) return;

    sceGuTexMode(GU_PSM_8888, 0, 0, GU_TRUE);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA); //outputcolr = vertexcolor * texturecolor
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    sceGuTexWrap(GU_REPEAT, GU_REPEAT);
    sceGuTexImage(0, tex->pw, tex->ph, tex->pw, tex->data);
}

//Camera 
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

//Tilemaps
Tilemap *create_tilemap(TextureAtlas atlas, Texture *tex, int sizex, int sizey, float scalex, float scaley){
    Tilemap *t = (Tilemap*)malloc(sizeof(Tilemap));
    if(!t) return NULL;

    t->tiles = (Tile*)malloc(sizeof(Tile) * sizex * sizey);
    if(!t->tiles){
        free(t);
        return NULL;
    }

    t->mesh = create_mesh(sizex * sizey * 4, sizex * sizey * 6, sizeof(Vertex));
    if(!t->mesh){
        free(t->tiles);
        free(t);
        return NULL;
    }

    memset(t->mesh->data, 0, sizeof(Vertex) * sizex * sizey);
    memset(t->mesh->indices, 0, sizeof(u16) * sizex * sizey);
    memset(t->tiles, 0, sizeof(Tile) * sizex * sizey);

    t->atlas = atlas;
    t->tex = tex;
    t->x = 0;
    t->y = 0;
    t->w = sizex;
    t->h = sizey;
    t->mesh->index_count = sizex * sizey * 6;
    t->scale_x = scalex;
    t->scale_y = scaley;

    return t;
}

void get_uv_index(TextureAtlas *atlas, float *buf, int index) {
    int row = index / (int)atlas->w;
    int column = index % (int)atlas->h;

    float sizex = 1.0f / (float)atlas->w;
    float sizey = 1.0f / (float)atlas->h;

    float y = (float)row * sizey;
    float x = (float)column * sizex;

    float h = y + sizey;
    float w = x + sizex;

    buf[0] = x;
    buf[1] = h;
    buf[2] = x;
    buf[3] = y;
    buf[4] = w;
    buf[5] = y;
    buf[6] = w;
    buf[7] = h;
}

void destroy_tilemap(Tilemap *t) {
    destroy_mesh(t->mesh);
    free(t->tiles);
    free(t);
}

void draw_tilemap(Tilemap *t) {
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    
    ScePspFVector3 v = {.x = t->x, .y = t->y, .z =0.0f};
    sceGumTranslate(&v);

    ScePspFVector3 v1 = {.x = t->scale_x, .y = t->scale_y, .z = 0.0f};
    sceGumScale(&v1);

    bind_texture(t->tex);
    draw_mesh(t->mesh);
}

void build_tilemap(Tilemap *t){
    for(int i = 0; i < t->w * t->h; i++){
        float buf[8];
        get_uv_index(&t->atlas, buf, t->tiles[i].tex_index);
        
        float tx = (float)t->tiles[i].x;
        float ty = (float)t->tiles[i].y;
        float tw = tx + 1.0f;
        float th = ty + 1.0f;

        ((Vertex*)t->mesh->data)[i * 4 + 0] = create_vert(buf[0], buf[1], 0xFFFFFFFF, tx, th, 0.0f); 
        ((Vertex*)t->mesh->data)[i * 4 + 1] = create_vert(buf[2], buf[3], 0xFFFFFFFF, tx, ty, 0.0f); 
        ((Vertex*)t->mesh->data)[i * 4 + 2] = create_vert(buf[4], buf[5], 0xFFFFFFFF, tw, ty, 0.0f); 
        ((Vertex*)t->mesh->data)[i * 4 + 3] = create_vert(buf[6], buf[7], 0xFFFFFFFF, tw, th, 0.0f); 
        
        t->mesh->indices[i * 6 + 0] = (i * 4) + 0;
        t->mesh->indices[i * 6 + 1] = (i * 4) + 1;
        t->mesh->indices[i * 6 + 2] = (i * 4) + 2;
        t->mesh->indices[i * 6 + 3] = (i * 4) + 2;
        t->mesh->indices[i * 6 + 4] = (i * 4) + 3;
        t->mesh->indices[i * 6 + 5] = (i * 4) + 0;
    }
    sceKernelDcacheWritebackInvalidateAll();
}

void draw_text(Tilemap *t, const char *str) {
    int len = strlen(str);

    for(int i = 0; i < len; i++){
        char c = str[i];
        Tile tile = {
            .x = i % t->w, 
            .y = i / t->w,
            .tex_index = c,
        };
        t->tiles[i] = tile;
    }
}
// G
void init_graphics(void *list, InitOptions opts) {
    void *fbp0 = getStaticVramBuffer(BUFF_WIDTH, SCREEN_HEIGHT, GU_PSM_8888);
    void *fbp1 = getStaticVramBuffer(BUFF_WIDTH, SCREEN_HEIGHT, GU_PSM_8888);
    void *zbp =  getStaticVramBuffer(BUFF_WIDTH, SCREEN_HEIGHT, GU_PSM_4444);

    sceGuInit();

    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, fbp0, BUFF_WIDTH);
    sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, fbp1,BUFF_WIDTH);
    sceGuDepthBuffer(zbp, BUFF_WIDTH);
    sceGuOffset(2048 - (SCREEN_WIDTH / 2), 2048 - (SCREEN_HEIGHT / 2));
    sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDepthFunc(GU_EQUAL);
    sceGuFrontFace(GU_CW);
    sceGuShadeModel(GU_SMOOTH);
    sceGuEnable(GU_CULL_FACE); //
    sceGuEnable(GU_TEXTURE_2D);
    sceGuEnable(GU_CLIP_PLANES);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblank();
    sceGuDisplay(GU_TRUE);

    if(opts == PRIM_RECT) {
        rect = create_mesh(4, 6, sizeof(SimpleVertex));

        ((SimpleVertex*)rect->data)[0] = create_simple_vert(-0.5f, -0.5f);
        ((SimpleVertex*)rect->data)[1] = create_simple_vert( 0.5f, -0.5f);
        ((SimpleVertex*)rect->data)[2] = create_simple_vert( 0.5f,  0.5f);
        ((SimpleVertex*)rect->data)[3] = create_simple_vert(-0.5f,  0.5f);

        rect->indices[0] = 0;
        rect->indices[1] = 1;
        rect->indices[2] = 2;
        rect->indices[3] = 2;
        rect->indices[4] = 3;
        rect->indices[5] = 0;

    }
}

unsigned int get_memory_size(unsigned int width, unsigned int height, unsigned int psm){
    unsigned int size = width * height;
    switch (psm) {
        case GU_PSM_T4:  return (size) >> 1;
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

void *getStaticVramBuffer(unsigned int width, unsigned int height, unsigned int psm) {
    static unsigned int static_offset = 0;
    unsigned int mem_size = get_memory_size(width, height, psm);
    void *result = (void*)static_offset;
    static_offset += mem_size;
    return result;
}
void *getStaticVramTexture(unsigned int width, unsigned int height, unsigned int psm) {
    void *result = getStaticVramBuffer(width, height, psm);
    return (void*)(((unsigned int) result) + ((unsigned int)sceGeEdramGetAddr()));
}

void start_frame(void *list){
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


unsigned int pow2(const unsigned int value) {
    unsigned int powtwo = 1;
    while(powtwo < value) {
        powtwo <<= 1;
    }
    return powtwo;
}

void swizzle_fast(u8 *out, const u8 *in, unsigned int width, unsigned int height) {
    unsigned int blockx, blocky, j;

    unsigned int width_blocks = (width / 16);
    unsigned int height_blocks = (height / 8);

    unsigned int src_pitch = (width - 16) / 4;
    unsigned int src_row = width * 8;

    const unsigned char *ysrc = in;
    u32 *dst = (u32*)out;

    for(blocky = 0; blocky < height_blocks; ++blocky){
        const u8 *xsrc = ysrc;
        for(blockx = 0; blockx < width_blocks; ++blockx){
            const u32 *src = (u32*) xsrc;
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
void clear(unsigned int Color) {
    sceGuClearColor(Color);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT | GU_STENCIL_BUFFER_BIT);
}

float get_frame_time(){
    u64 tick;
    static u64 last_tick;
    sceRtcGetCurrentTick(&tick);
    float dt = (tick - last_tick) / (float)sceRtcGetTickResolution();
    last_tick = tick;
    return dt;
}

void psp_close() {
    if(rect != NULL)
        destroy_mesh(rect);
    sceGuTerm();
    sceKernelExitGame();
}
