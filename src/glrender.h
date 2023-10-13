#ifndef RENDER_H
#define RENDER_H

#include <gl/gl.h>

typedef enum
glr_pixel_format
{
    GLR_PIXEL_FORMAT_RGBA
} glr_pixel_format;

typedef struct
glr_texture
{
    int used;
    int to_destroy;
    GLuint gl_texture;
} glr_texture;

typedef struct
glr_vertex
{
    float x, y, z;
    float r, g, b, a;
    float u, v;
    glr_texture *texture;
} glr_vertex;

#define GLR_PRIMITIVE_NUM_VERTEXES 3

#ifndef GLR_MAX_PRIMITIVES
#define GLR_MAX_PRIMITIVES 1024
#endif

#ifndef GLR_MAX_VERTEXES
#define GLR_MAX_VERTEXES GLR_MAX_PRIMITIVES*GLR_PRIMITIVE_NUM_VERTEXES
#endif

#ifndef GLR_MAX_TEXTURES
#define GLR_MAX_TEXTURES 20
#endif

typedef struct
glr_renderer
{
    unsigned int flags;

    glr_vertex vertexes[GLR_MAX_VERTEXES];
    unsigned int vertexes_count;

    glr_texture textures[GLR_MAX_TEXTURES];
    unsigned int textures_count;
} glr_renderer;

enum
{
    GLR_ALPHA_BLEND = 0x00000001,
    GLR_DEPTH_TEST  = 0x00000002
};

void glr_init(glr_renderer *renderer, unsigned int flags);

void glr_begin(glr_renderer *renderer);
void glr_end(glr_renderer *renderer);

glr_texture *glr_texture_create_from_raw_data(glr_renderer *renderer, void *data, int w, int h, glr_pixel_format format);
void glr_texture_destroy(glr_renderer *renderer, glr_texture *texture);

void glr_clear(glr_renderer *renderer, float r, float g, float b, float a);

void glr_triangle_push(glr_renderer *renderer, glr_vertex vertexes[3]);
void glr_quad_push(glr_renderer *renderer, glr_vertex vertexes[4]);

#endif
