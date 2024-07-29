#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include <pspkernel.h>
#include <pspgu.h>
#include <pspgum.h>

#define SCREEN_WIDTH    480
#define SCREEN_HEIGHT 	272
#define BUFF_WIDTH      512
#define MAX(a, b) (a > b) ? a : b
#define MIN(a, b) (a < b) ? a : b

typedef struct {
    float u, v;
    unsigned int color;
    float x, y, z;
} Vertex;

typedef struct {
    unsigned int width, height;
    unsigned int pw, ph;
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

typedef struct {
    void *data;
    u16 *indices;
    u32 index_count;
} Mesh;

typedef struct {
    float x, y;
    float rot;
    float sx, sy;
    int layer;
    Mesh *mesh;
    Texture *tex;
} Sprite;

typedef struct {
    float w, h;
} TextureAtlas;

typedef struct {
    int x, y;
    int tex_index;
} Tile;

typedef struct {
    float x, y;
    float scale_x, scale_y;
    int w, h;
    TextureAtlas atlas;
    Texture *tex;
    Tile *tiles;
    Mesh *mesh;
} Tilemap;



Mesh *create_mesh(u32 vcount, u32 index_count);
Vertex create_vert(float u, float v, unsigned int color, float x, float y, float z);
Sprite *create_sprite(float x, float y, float sx, float sy, Texture *tex);
void draw_mesh(Mesh *mesh);
void destroy_mesh(Mesh *mesh);
void destroy_sprite(Sprite *sprite);
void draw_sprite(Sprite *sprite);
unsigned int get_memory_size(unsigned int width, unsigned int height, unsigned int psm);
void *getStaticVramBuffer(unsigned int width, unsigned int height, unsigned int psm);
void *getStaticVramTexture(unsigned int width, unsigned int height, unsigned int psm);
void init_graphics(void *list);
void start_frame(void *list);
void end_frame(void);
void reset_transform(float x, float y, float z);
unsigned int pow2(const unsigned int value);
void copy_tex_data(void *dest, const void *src, const int pw, const int width, const int height);
void swizzle_fast(unsigned char *out, const unsigned char *in, unsigned int width, unsigned int height);
Texture *load_texture(const char *filename, const int vram, int flip);
void bind_texture(Texture *tex);
void clear(unsigned int Color);
void apply_camera(const Camera2D *cam);
float get_frame_time();
void get_uv_index(TextureAtlas *atlas, float *buf, int index);
Tilemap *create_tilemap(TextureAtlas atlas, Texture *tex, int sizex, int sizey);
void destroy_tilemap(Tilemap *t);
void draw_tilemap(Tilemap *t);
void build_tilemap(Tilemap *t);
void draw_text(Tilemap *t, const char *src);

#endif
