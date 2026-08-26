#include "object_renderer.h"

#include <stdlib.h>

#include "graphics.h"
#include "object_shbin.h"
#include "utils/c2d_internal.h"

typedef struct {
    float local[2];
    float position[2];
    float rotation[2];
    float texcoord[2];
    u32 color;
} ObjectVertex;

_Static_assert(sizeof(ObjectVertex) == 36, "ObjectVertex layout must match object.v.pica");

static DVLB_s *object_shader_dvlb;
static shaderProgram_s object_shader;
static C3D_AttrInfo object_attr_info;
static C3D_BufInfo object_buf_info;
static ObjectVertex *object_vertices;
static u16 *object_indices;
static int object_mdlv_uniform;
static int object_proj_uniform;
static int object_colors_uniform;
static int object_p1_uniform;
static int built_sprite_count;
static int current_blending = -1;
static C3D_Tex *current_texture;

static void set_vertex(
    ObjectVertex *vertex,
    const SpriteObject *sprite,
    float local_x,
    float local_y,
    const float uv[2]
) {
    vertex->local[0] = local_x;
    vertex->local[1] = local_y;
    vertex->position[0] = sprite->x;
    vertex->position[1] = sprite->y;
    vertex->rotation[0] = sprite->rotation_sin;
    vertex->rotation[1] = sprite->rotation_cos;
    vertex->texcoord[0] = uv[0];
    vertex->texcoord[1] = uv[1];
    vertex->color = sprite->color_meta;
}

bool object_renderer_init(void) {
    object_vertices = linearAlloc(sizeof(ObjectVertex) * MAX_SPRITES * 4);
    object_indices = linearAlloc(sizeof(u16) * MAX_SPRITES * 6);
    if (!object_vertices || !object_indices) {
        object_renderer_fini();
        return false;
    }

    for (int i = 0; i < MAX_SPRITES; i++) {
        u16 base = i * 4;
        u16 *indices = &object_indices[i * 6];
        indices[0] = base;
        indices[1] = base + 2;
        indices[2] = base + 1;
        indices[3] = base + 1;
        indices[4] = base + 2;
        indices[5] = base + 3;
    }

    object_shader_dvlb = DVLB_ParseFile((u32 *)object_shbin, object_shbin_size);
    if (!object_shader_dvlb) {
        object_renderer_fini();
        return false;
    }

    shaderProgramInit(&object_shader);
    shaderProgramSetVsh(&object_shader, &object_shader_dvlb->DVLE[0]);
    object_mdlv_uniform = shaderInstanceGetUniformLocation(object_shader.vertexShader, "mdlvMtx");
    object_proj_uniform = shaderInstanceGetUniformLocation(object_shader.vertexShader, "projMtx");
    object_colors_uniform = shaderInstanceGetUniformLocation(object_shader.vertexShader, "channelColors");
    object_p1_uniform = shaderInstanceGetUniformLocation(object_shader.vertexShader, "p1Color");
    if (object_mdlv_uniform < 0 || object_proj_uniform < 0
        || object_colors_uniform < 0 || object_p1_uniform < 0) {
        object_renderer_fini();
        return false;
    }

    AttrInfo_Init(&object_attr_info);
    AttrInfo_AddLoader(&object_attr_info, 0, GPU_FLOAT, 2);
    AttrInfo_AddLoader(&object_attr_info, 1, GPU_FLOAT, 2);
    AttrInfo_AddLoader(&object_attr_info, 2, GPU_FLOAT, 2);
    AttrInfo_AddLoader(&object_attr_info, 3, GPU_FLOAT, 2);
    AttrInfo_AddLoader(&object_attr_info, 4, GPU_UNSIGNED_BYTE, 4);

    BufInfo_Init(&object_buf_info);
    BufInfo_Add(&object_buf_info, object_vertices, sizeof(ObjectVertex), 5, 0x43210);
    return true;
}

void object_renderer_fini(void) {
    if (object_shader_dvlb) {
        shaderProgramFree(&object_shader);
        DVLB_Free(object_shader_dvlb);
        object_shader_dvlb = NULL;
    }
    if (object_indices) {
        linearFree(object_indices);
        object_indices = NULL;
    }
    if (object_vertices) {
        linearFree(object_vertices);
        object_vertices = NULL;
    }
}

void object_renderer_build(SpriteObject *const *objects, int count) {
    built_sprite_count = 0;

    for (int i = 0; i < count; i++) {
        SpriteObject *sprite = objects[i];
        sprite->render_slot = -1;
        if (sprite->obj == -1 || !sprite->visible || built_sprite_count >= MAX_SPRITES) continue;

        float uv_top_left[2];
        float uv_top_right[2];
        float uv_bottom_left[2];
        float uv_bottom_right[2];
        Tex3DS_SubTextureTopLeft(sprite->image.subtex, uv_top_left, uv_top_left + 1);
        Tex3DS_SubTextureTopRight(sprite->image.subtex, uv_top_right, uv_top_right + 1);
        Tex3DS_SubTextureBottomLeft(sprite->image.subtex, uv_bottom_left, uv_bottom_left + 1);
        Tex3DS_SubTextureBottomRight(sprite->image.subtex, uv_bottom_right, uv_bottom_right + 1);

        if (sprite->flip_x) {
            C2Di_SwapUV(uv_top_left, uv_top_right);
            C2Di_SwapUV(uv_bottom_left, uv_bottom_right);
        }
        if (sprite->flip_y) {
            C2Di_SwapUV(uv_top_left, uv_bottom_left);
            C2Di_SwapUV(uv_top_right, uv_bottom_right);
        }

        ObjectVertex *vertices = &object_vertices[built_sprite_count * 4];
        set_vertex(&vertices[0], sprite, -sprite->half_width, -sprite->half_height, uv_top_left);
        set_vertex(&vertices[1], sprite,  sprite->half_width, -sprite->half_height, uv_top_right);
        set_vertex(&vertices[2], sprite, -sprite->half_width,  sprite->half_height, uv_bottom_left);
        set_vertex(&vertices[3], sprite,  sprite->half_width,  sprite->half_height, uv_bottom_right);

        sprite->render_slot = built_sprite_count++;
    }
}

void object_renderer_begin(void) {
    C2D_Flush();

    C3D_BindProgram(&object_shader);
    C3D_SetAttrInfo(&object_attr_info);
    C3D_SetBufInfo(&object_buf_info);

    C2Di_Context *c2d = C2Di_GetContext();
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, object_mdlv_uniform, &c2d->mdlvMtx);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, object_proj_uniform, &c2d->projMtx);

    const float color_scale = 1.f / 255.f;
    for (int i = 0; i < COL_CHANNEL_NUM; i++) {
        Color color = channels[i].color;
        C3D_FVUnifSet(GPU_VERTEX_SHADER, object_colors_uniform + i,
            color.r * color_scale, color.g * color_scale, color.b * color_scale, 1.f);
    }
    C3D_FVUnifSet(GPU_VERTEX_SHADER, object_colors_uniform + COL_CHANNEL_NUM, 1.f, 1.f, 1.f, 1.f);

    Color p1 = get_white_if_black(p1_color);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, object_p1_uniform,
        p1.r * color_scale, p1.g * color_scale, p1.b * color_scale, 1.f);

    C3D_TexEnv *env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
    C3D_TexEnvInit(C3D_GetTexEnv(1));
    C3D_TexEnvInit(C3D_GetTexEnv(2));
    C3D_TexEnvInit(C3D_GetTexEnv(3));

    // The sorted object list is painter-ordered and is interrupted to draw the
    // player. Disable depth rejection so later layers always win across those
    // raw/Citro2D transitions.
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
    C3D_CullFace(GPU_CULL_NONE);
    current_blending = -1;
    current_texture = NULL;
}

static void set_blending(bool blending) {
    if (current_blending == blending) return;

    C3D_TexEnv *env = C3D_GetTexEnv(4);
    C3D_TexEnvInit(env);
    if (blending) {
        C3D_AlphaBlend(
            GPU_BLEND_ADD, GPU_BLEND_ADD,
            GPU_SRC_ALPHA, GPU_ONE,
            GPU_ONE, GPU_ZERO);
        C3D_TexEnvSrc(env, C3D_Alpha, GPU_PREVIOUS, GPU_PREVIOUS, 0);
        C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
    } else {
        C3D_AlphaBlend(
            GPU_BLEND_ADD, GPU_BLEND_ADD,
            GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA,
            GPU_ONE, GPU_ZERO);
    }
    current_blending = blending;
}

void object_renderer_draw_batch(int first_slot, int sprite_count, C3D_Tex *texture, bool blending) {
    if (sprite_count <= 0 || first_slot < 0 || first_slot + sprite_count > built_sprite_count) return;

    set_blending(blending);
    if (texture != current_texture) {
        C3D_TexBind(0, texture);
        current_texture = texture;
    }

    C3D_DrawElements(
        GPU_TRIANGLES,
        sprite_count * 6,
        C3D_UNSIGNED_SHORT,
        &object_indices[first_slot * 6]);
}

void object_renderer_end(void) {
    // Restore the complete Citro2D pipeline before player/particle/UI drawing.
    C2D_Prepare();
    C3D_AlphaBlend(
        GPU_BLEND_ADD, GPU_BLEND_ADD,
        GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA,
        GPU_ONE, GPU_ZERO);
    C3D_TexEnvInit(C3D_GetTexEnv(4));
}
