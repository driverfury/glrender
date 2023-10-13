#include "glrender.h"

#include <gl/gl.h>
#include <stdint.h>

void
glr_init(glr_renderer *renderer, unsigned int flags)
{
    renderer->vertexes_count = 0;
    renderer->textures_count = 0;

    for(unsigned int texture_index = 0;
        texture_index < GLR_MAX_TEXTURES;
        ++texture_index)
    {
        renderer->textures[texture_index].used = 0;
    }

    renderer->flags = 0;

    if(flags & GLR_ALPHA_BLEND)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        renderer->flags |= GLR_ALPHA_BLEND;
    }
    else
    {
        glDisable(GL_BLEND);
    }

    if(flags & GLR_DEPTH_TEST)
    {
        glEnable(GL_DEPTH_TEST);
        renderer->flags |= GLR_DEPTH_TEST;
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void
glr_begin(glr_renderer *renderer)
{
    for(unsigned int texture_index = 0;
        texture_index < GLR_MAX_TEXTURES;
        ++texture_index)
    {
        if( renderer->textures[texture_index].used &&
            renderer->textures[texture_index].to_destroy)
        {
            glDeleteTextures(1, &renderer->textures[texture_index].gl_texture);
            renderer->textures[texture_index].used = 0;
            renderer->textures_count -= 1;
        }
    }

    renderer->vertexes_count = 0;
}

void
glr_end(glr_renderer *renderer)
{
    if(renderer->flags & GLR_DEPTH_TEST)
    {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    for(unsigned int prim_index = 0;
        prim_index < renderer->vertexes_count / GLR_PRIMITIVE_NUM_VERTEXES;
        ++prim_index)
    {
        glr_texture *texture = 0;
        if(renderer->vertexes[prim_index*GLR_PRIMITIVE_NUM_VERTEXES].texture)
        {
            texture = renderer->vertexes[prim_index*GLR_PRIMITIVE_NUM_VERTEXES].texture;
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, texture->gl_texture);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glBegin(GL_TRIANGLES);
        for(unsigned int index = 0;
            index < GLR_PRIMITIVE_NUM_VERTEXES;
            ++index)
        {
            glr_vertex *vertex = &renderer->vertexes[prim_index*GLR_PRIMITIVE_NUM_VERTEXES + index];

            glColor4f(vertex->r, vertex->g, vertex->b, vertex->a);
            if(texture)
            {
                glTexCoord2f(vertex->u, 1.0f - vertex->v);
            }

            glVertex3f(vertex->x, vertex->y, vertex->z);
        }
        glEnd();
    }

    glFlush();
}

static glr_texture *
glr_texture_find_free_slot(glr_renderer *renderer)
{
    glr_texture *texture = 0;

    if(renderer->textures_count < GLR_MAX_TEXTURES)
    {
        for(unsigned int texture_index = 0;
            texture_index < GLR_MAX_TEXTURES;
            ++texture_index)
        {
            if(!renderer->textures[texture_index].used)
            {
                texture = &renderer->textures[texture_index];
                break;
            }
        }
    }

    return(texture);
}

glr_texture *
glr_texture_create_from_raw_data(
    glr_renderer *renderer,
    void *data, int w, int h,
    glr_pixel_format format)
{
    GLenum gl_format = GL_RGBA;
    switch(format)
    {
        case GLR_PIXEL_FORMAT_RGBA: gl_format = GL_RGBA; break;

        default:
        {
            // TODO: Invalid pixel format error
            return(0);
        } break;
    }

    glr_texture *texture = glr_texture_find_free_slot(renderer);
    if(!texture)
    {
        return(0);
    }

#if 0
    glr_raw_data_flip_on_y_axis(data, w, h);
#endif

    GLuint gl_texture;
    glGenTextures(1, &gl_texture);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gl_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)w, (GLsizei)h, 0, gl_format, GL_UNSIGNED_BYTE, (const void *)data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    texture->used = 1;
    texture->to_destroy = 0;
    texture->gl_texture = gl_texture;
    renderer->textures_count += 1;

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

#if 0
    glr_raw_data_flip_on_y_axis(data, w, h);
#endif

    return(texture);
}

void
glr_texture_destroy(glr_renderer *renderer, glr_texture *texture)
{
    if(!texture)
    {
        return;
    }

    texture->to_destroy = 1;
}

#if 0
static void
glr_raw_data_flip_on_y_axis(void *data, int width, int height)
{
    uint32_t *pixels = (u32 *)data;
    for(int Y = 0; Y < height / 2; ++Y)
    {
        for(int X = 0; X < width; ++X)
        {
            uint32_t tmp = pixels[Y*width + X];
            pixels[Y*width + X] = pixels[(height - 1 - Y)*width + X];
            pixels[(height - 1 - Y)*width + X] = tmp;
        }
    }
}
#endif

void
glr_clear(glr_renderer *renderer, float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
    renderer->vertexes_count = 0;
}

void
glr_triangle_push(glr_renderer *renderer, glr_vertex vertexes[3])
{
    if(renderer->vertexes_count + 3 > GLR_MAX_VERTEXES)
    {
        return;
    }

    for(unsigned int index = 0;
        index < 3;
        ++index)
    {
        renderer->vertexes[renderer->vertexes_count + index] = vertexes[index];
    }

    renderer->vertexes_count += 3;
}

void
glr_quad_push(glr_renderer *renderer, glr_vertex vertexes[4])
{
    if(renderer->vertexes_count + 6 > GLR_MAX_VERTEXES)
    {
        return;
    }

    glr_vertex vertexes_to_push[3];

    vertexes_to_push[0] = vertexes[0];
    vertexes_to_push[1] = vertexes[1];
    vertexes_to_push[2] = vertexes[2];
    glr_triangle_push(renderer, vertexes_to_push);

    vertexes_to_push[0] = vertexes[2];
    vertexes_to_push[1] = vertexes[3];
    vertexes_to_push[2] = vertexes[0];
    glr_triangle_push(renderer, vertexes_to_push);
}
