#include "modding.h"
#include "assets.h"
#include "functions.h"
#include "variables.h"
#include "recomputils.h"
#include "o1heap.h"

// Forward declarations.

extern u8 D_80370A1C;
extern s32 D_80383CB0;
extern s32 D_80383CCC;
extern s32 assetCacheCurrentSize;
extern u8 assetCacheLength;
extern u8 assetCacheCurrentIndex;
extern s16 *assetCacheAssetIdList;
extern u8* assetCacheDependencyCount;
extern void** assetCachePtrList;
extern AssetFileMeta *assetSectionRomMetaList;
extern BKSpriteDisplayData **D_80383CD4;
extern u32 heap_occupiedBytes;

bool func_8025498C(s32 size);
void func_80254C98(void);
void func_8033B020(void *ptr);
void func_8033BAB0(enum asset_e asset_id, s32 offset, s32 size, void *dst_ptr);
void func_803449DC(BKSpriteDisplayData *arg0);
int rarezip_inflate(u8 *src, u8 *dst);
s32 rarezip_get_uncompressed_size(u8 *arg0);

// Replacement data arrays.

#define MAX_ASSETS 0x4000
#define ASSET_REPLACEMENT_HEAP_SIZE 0x800000

void *asset_replacements_data[MAX_ASSETS];
u16 asset_replacements_indices[MAX_ASSETS];
u16 asset_replacements_active = 0;
u8 asset_replacement_heap_memory[ASSET_REPLACEMENT_HEAP_SIZE] __attribute__((aligned(0x10)));
O1HeapInstance *asset_replacement_heap_instance;

RECOMP_HOOK("assetCache_init") void assetCache_init_hook(void) {
    for (u16 i = 0; i < MAX_ASSETS; i++) {
        asset_replacements_indices[i] = MAX_ASSETS;
    }

    asset_replacement_heap_instance = o1heapInit(&asset_replacement_heap_memory[0], ASSET_REPLACEMENT_HEAP_SIZE);
}

u16 insert_asset_replacement_data(void *asset_data) {
    if (asset_replacements_active >= MAX_ASSETS) {
        return MAX_ASSETS;
    }

    u16 left = 0;
    u16 right = asset_replacements_active;
    while (left < right) {
        u16 mid = left + ((right - left) / 2);
        if (asset_replacements_data[mid] < asset_data) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }

    for (u16 i = asset_replacements_active; i > left; i--) {
        asset_replacements_data[i] = asset_replacements_data[i - 1];
    }

    asset_replacements_data[left] = asset_data;
    asset_replacements_active++;

    return left;
}

u16 get_asset_replacement_data_index(void *asset_data) {
    u16 left = 0;
    u16 right = asset_replacements_active;
    while (left < right) {
        u16 mid = left + ((right - left) / 2);
        if (asset_replacements_data[mid] < asset_data) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }

    return asset_replacements_data[left] == asset_data ? left : MAX_ASSETS;
}

RECOMP_EXPORT void bk_recomp_aep_register_replacement(enum asset_e asset_id, void *asset_data) {
    asset_replacements_indices[asset_id] = insert_asset_replacement_data(asset_data);
}

RECOMP_EXPORT void bk_recomp_aep_unregister_replacement(enum asset_e asset_id) {
    u16 replacement_index = asset_replacements_indices[asset_id];
    if (replacement_index < MAX_ASSETS) {
        asset_replacements_active--;

        for (u16 i = replacement_index; i < asset_replacements_active; i++) {
            asset_replacements_data[i] = asset_replacements_data[i + 1];
        }
    }

    asset_replacements_indices[asset_id] = MAX_ASSETS;
}

void *bk_recomp_aep_malloc(s32 size) {
    return o1heapAllocate(asset_replacement_heap_instance, size);
}

void bk_recomp_aep_free(void *ptr) {
    o1heapFree(asset_replacement_heap_instance, ptr);
}

void *bk_recomp_aep_realloc(void *ptr, s32 size) {
    void *new_ptr = bk_recomp_aep_malloc(size);
    memcpy(new_ptr, ptr, size);
    bk_recomp_aep_free(ptr);
    return new_ptr;
}

// @mod Modded to skip asset extraction and return the in-memory replacement if it exists.
RECOMP_PATCH void *assetcache_get(enum asset_e assetId) {
    //recomp_printf("assetCacheLength %d\n", assetCacheLength);

    // @mod If a replacement was made for this asset by another mod, return it instead of extracting it from the game.
    if (asset_replacements_indices[assetId] < MAX_ASSETS) {
        u16 replacement_index = asset_replacements_indices[assetId];
        return asset_replacements_data[replacement_index];
    }

    s32 comp_size;//sp_44
    s32 i;
    volatile s32 sp3C; //sp3C
    s32 uncomp_size; //sp38
    void *uncompressed_file;//sp34
    u8 sp33; //sp33
    void *compressed_file;//sp2C
    s32 sp28;//sp28
    
    sp28 = (s32 )D_80370A1C;
    D_80370A1C = (u8)0U;
    for(i = 0; i < assetCacheLength && assetId != assetCacheAssetIdList[i]; i++);
    assetCacheCurrentIndex = i;
    if(i == 0x96)
        return NULL;
    
    if(i < assetCacheLength){ //asset exists in array;
        assetCacheDependencyCount[i]++;
        return assetCachePtrList[i];
    }
    comp_size = assetSectionRomMetaList[assetId+1].offset - assetSectionRomMetaList[assetId].offset;
    if(comp_size & 1) 
        comp_size++;
    sp3C = comp_size;

    // @mod This entire section was rewritten to use the mod's own heap allocation.
    if (assetSectionRomMetaList[assetId].compFlag & 0x0001) {
        // Asset is compressed.
        func_8033BAB0(assetId, 0, 0x10, &D_80383CB0);
        assetCacheCurrentSize = rarezip_get_uncompressed_size(&D_80383CB0);
        uncomp_size = assetCacheCurrentSize;
        if(uncomp_size & 0xF){
            uncomp_size -= uncomp_size & 0xF;
            uncomp_size += 0x10;
        }

        if (sp28 != 0) {
            func_80254C98();
        }

        compressed_file = malloc(comp_size);
        piMgr_read(compressed_file, assetSectionRomMetaList[assetId].offset + D_80383CCC, sp3C);

        uncompressed_file = bk_recomp_aep_malloc(uncomp_size);
        rarezip_inflate(compressed_file, uncompressed_file);
        free(compressed_file);
    }
    else {
        // Asset is not compressed. Allocate on the game's heap, read the asset, and then move it to the mod's heap.
        // piMgr_read will only read correctly to the game's own heap.
        compressed_file = malloc(comp_size);
        piMgr_read(compressed_file, assetSectionRomMetaList[assetId].offset + D_80383CCC, sp3C);
        uncompressed_file = bk_recomp_aep_malloc(comp_size);
        memcpy(uncompressed_file, compressed_file, comp_size);
        free(compressed_file);
    }

    assetCacheCurrentIndex = assetCacheLength;
    assetCacheDependencyCount[assetCacheLength] = 1;
    assetCachePtrList[assetCacheLength] = uncompressed_file;
    D_80383CD4[assetCacheLength] = 0;
    assetCacheAssetIdList[assetCacheLength] = assetId;
    assetCacheLength++;
    return uncompressed_file;
}

// @mod Modded to ignore frees on replaced assets.
RECOMP_PATCH void assetCache_free(void *arg0) {
    // @mod If the pointer belongs to an asset replacement, ignore the free operation.
    u16 replacement_index = get_asset_replacement_data_index(arg0);
    if (replacement_index < MAX_ASSETS) {
        return;
    }

    func_8033B020(arg0);
}

// @mod Patched to replace the malloc routine.
RECOMP_PATCH void func_8033B2A4(s32 arg0) {
    assetCachePtrList[assetCacheLength] = bk_recomp_aep_malloc(arg0); // @mod Replaced with the mod's malloc.
    D_80383CD4[assetCacheLength] = NULL;
    assetCacheDependencyCount[assetCacheLength] = 1;
    assetCacheAssetIdList[assetCacheLength] = -1;
    assetCacheLength += 1;
}

// @mod Patched to replace the realloc routine.
RECOMP_PATCH void assetCache_resizeAsset(void *assetPtr, s32 size){
    s32 tmp;
    s32 i;

    for(i = 0; i < assetCacheLength  && assetPtr != assetCachePtrList[i]; i++);
    assetCachePtrList[i] = bk_recomp_aep_realloc(assetPtr, size); // @mod Replaced with the mod's realloc.
}

// @mod Patched to replace the free routine.
RECOMP_PATCH s32 assetcache_release(void * arg0){
    s32 i;
    if(arg0){
        for(i = 0; i < assetCacheLength  && arg0 != assetCachePtrList[i]; i++);
        
        if(i == assetCacheLength)
            return 2;

        assetCacheCurrentIndex = i;
        if(assetCacheDependencyCount[i] == 1){
            if(D_80383CD4[i])
                func_803449DC(D_80383CD4[i]);
            bk_recomp_aep_free(arg0); // @mod Replaced with the mod's free.
            assetCacheLength--;
            assetCacheDependencyCount[i] = assetCacheDependencyCount[assetCacheLength];
            assetCachePtrList[i] = assetCachePtrList[assetCacheLength];
            D_80383CD4[i] = D_80383CD4[assetCacheLength];
            assetCacheAssetIdList[i] = assetCacheAssetIdList[assetCacheLength];
            return 0;
        }
        else{
            assetCacheDependencyCount[i]--;
            return 1;
        }
    } else{
        return 3;
    }
}

// @mod Patched to disable defragmentation for assets entirely.
RECOMP_PATCH void *defrag_asset(void *arg0){
    // @mod Running defragmentation for assets is no longer necessary as they don't live on the game's heap.
    return arg0;
#if 0
    void *sp1C;
    if(arg0 == NULL || arg0 == D_8027659C)
        return arg0;

    sp1C = defrag(arg0);
    assetcache_update_ptr(arg0, sp1C);
    return sp1C;
#endif
}