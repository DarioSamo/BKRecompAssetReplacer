#include "modding.h"
#include "assets.h"
#include "functions.h"
#include "variables.h"
#include "recomputils.h"

typedef struct {
    u8 pad0[0xC];
    f32 unkC[3];
}Struct_core2_72060_0;

extern struct4Cs *D_80369280;
extern s32 D_80369284;
extern BKModelBin *D_80369288;
extern Gfx D_80369290[];
extern Gfx D_803692B0[];
extern f32 D_8038104C;
extern f32 D_80381050[3];
extern f32 D_80381060[3];
extern f32 D_8038108C;
extern Gfx *D_80381090;
extern Struct_core2_72060_0 *D_80381094;

void func_802F9134(s32 gfx);
bool func_802F989C(Gfx **gfx, Mtx **mtx, f32 arg2[3]);
void func_80349AD0(void);
f32 vtxList_getGlobalNorm(BKVertexList *);

// @mod Patched the snow display list code to support extended addresses.
RECOMP_PATCH void func_802F962C(Gfx **gfx, Mtx **mtx, Vtx **vtx) {
    u32 temp_s0_3;
    u32 temp_s0_4;
    void *temp_s0;
    void *temp_s0_2;
    BKVertexList *temp_s3;
    void *temp_s3_2;
    void *temp_s4;
    void *temp_s4_2;
    BKGfxList *gfx_list;
    struct4Ds *phi_s0;
    void *phi_s0_2;

    if ((D_80369280 != NULL) && (D_80369284 != 0)) {
        viewport_getPosition_vec3f(D_80381050);
        viewport_getRotation_vec3f(D_80381060);
        D_80381090 = (Gfx*)((s32)D_80369288 + D_80369288->gfx_list_offset_C + sizeof(BKGfxList));
        temp_s3 = (BKVertexList *)((s32)D_80369288 + D_80369288->vtx_list_offset_10);
        D_8038108C = vtxList_getGlobalNorm(temp_s3);
        func_80349AD0();
        gSPSegment((*gfx)++, 1, /*osVirtualToPhysical*/(temp_s3 + 1)); // @mod Removed osVirtualToPhysical.
        gSPSegment((*gfx)++, 0x02, /*osVirtualToPhysical*/((void*)((s32)D_80369288 + D_80369288->texture_list_offset_8 + sizeof(BKTextureList) + sizeof(BKTextureHeader)))); // @mod Removed osVirtualToPhysical.
        gSPSetGeometryMode((*gfx)++, G_ZBUFFER);
        gSPDisplayList((*gfx)++, D_80369290);
        gSPSegment((*gfx)++, 0x03, /*osVirtualToPhysical*/(&D_803692B0)); // @mod Removed osVirtualToPhysical.

        D_80381094 = (Struct_core2_72060_0 *)((s32)D_80369288 + D_80369288->geo_list_offset_4);
        
        for(phi_s0 = D_80369280->unk1C; phi_s0 < D_80369280->unk1C + D_80369284; phi_s0++) {
            if ((func_802F989C(gfx, mtx, phi_s0) == 0) && (phi_s0->unk0[1] < D_8038104C)) {
                func_802F9134(phi_s0 - D_80369280->unk1C);
                phi_s0--;
            }
        }
    }
}