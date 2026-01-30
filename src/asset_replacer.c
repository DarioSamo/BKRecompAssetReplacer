#include "modding.h"
#include "assets.h"
#include "functions.h"
#include "variables.h"
#include "recomputils.h"

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
extern s32 rarezip_get_uncompressed_size(u8 *arg0);

bool func_8025498C(s32 size);
void func_80254C98(void);
void func_8033B020(void *ptr);
void func_8033BAB0(enum asset_e asset_id, s32 offset, s32 size, void *dst_ptr);
int rarezip_inflate(u8 *src, u8 *dst);

// Replacement data arrays.

#define MAX_ASSETS 0x4000

void *asset_replacements_data[MAX_ASSETS];
u16 asset_replacements_indices[MAX_ASSETS];
u16 asset_replacements_active = 0;

RECOMP_HOOK("assetCache_init") void assetCache_init_hook(void) {
    for (u16 i = 0; i < MAX_ASSETS; i++) {
        asset_replacements_indices[i] = MAX_ASSETS;
    }
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

RECOMP_EXPORT void bkrecomp_asset_replacer_register(enum asset_e asset_id, void *asset_data) {
    asset_replacements_indices[asset_id] = insert_asset_replacement_data(asset_data);
}

RECOMP_EXPORT void bkrecomp_asset_replacer_unregister(enum asset_e asset_id) {
    u16 replacement_index = asset_replacements_indices[asset_id];
    if (replacement_index < MAX_ASSETS) {
        asset_replacements_active--;

        for (u16 i = replacement_index; i < asset_replacements_active; i++) {
            asset_replacements_data[i] = asset_replacements_data[i + 1];
        }
    }

    asset_replacements_indices[asset_id] = MAX_ASSETS;
}

// @mod Modded to skip asset extraction and return the in-memory replacement if it exists.
RECOMP_PATCH void *assetcache_get(enum asset_e assetId) {
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

    if(assetSectionRomMetaList[assetId].compFlag & 0x0001){//compressed
        func_8033BAB0(assetId, 0, 0x10, &D_80383CB0);
        assetCacheCurrentSize = rarezip_get_uncompressed_size(&D_80383CB0);
        uncomp_size = assetCacheCurrentSize;
        if(uncomp_size & 0xF){
            uncomp_size -= uncomp_size & 0xF;
            uncomp_size += 0x10;
        }
        
        if (func_8025498C((u32)comp_size + uncomp_size) && !sp28) {
            sp33 = 1;
            uncompressed_file = malloc((u32)comp_size + uncomp_size);
            compressed_file = (void *)((s32) uncompressed_file + uncomp_size);
        } else {
            sp33 = 2;
            if (sp28 != 0) {
                func_80254C98();
            }
            uncompressed_file = malloc(uncomp_size);
            compressed_file = malloc(comp_size);
        }
    } else { //uncompressed
        uncompressed_file = malloc(comp_size);
        compressed_file = uncompressed_file;
    }
    piMgr_read(compressed_file, assetSectionRomMetaList[assetId].offset + D_80383CCC, sp3C);
    if(assetSectionRomMetaList[assetId].compFlag & 0x0001){//decompress
        rarezip_inflate(compressed_file, uncompressed_file);
        realloc(uncompressed_file, assetCacheCurrentSize);
        osWritebackDCache(uncompressed_file, assetCacheCurrentSize);
        if (sp33 == 2) {
            free(compressed_file);
        }
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