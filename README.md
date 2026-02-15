# BK Recomp Asset Expansion Pak

Experimental mod that rewrites the asset management system of the game to allow direct asset in-memory replacements by other mods.

It also extends the game's asset system to use a much bigger allocation heap, freeing up a significant amount of the game's own memory heap in the process.

This mod is essential to resolve crashes for other mods that unlock the game's limits such as turning off draw distance or similar enhancements.

### Exports

The following exports are available to other mods to register in-memory replacements of assets. Any time the game tries to retrieve an asset using the asset cache, the data passed to the function will be returned instead. The lifetime of this memory is entirely up to the caller of the API, as this data will not be copied to the asset cache's heap.

If another mod tries to register the same replacement, no conflict resolution is provided: it'll be up entirely to the order the mods call the API to determine who gets to replace the asset.

The size version of the function is provided for mods that need to replace assets retrieved with the function that copies the asset into a destination pointer. On the regular version of the game, this is only used by two types of assets:
- The furnace fun questions (`ASSET_*_FF_QUIZ_QUESTION`).
- `SPRITE_BOLD_FONT_NUMBERS_ALPHAMASK` and `SPRITE_BOLD_FONT_LETTERS_ALPHAMASK`.

The size is necessary to know how many bytes to copy from the asset into the destination pointer safely.

```
RECOMP_EXPORT void bk_recomp_aep_register_replacement(enum asset_e asset_id, void *asset_data);
RECOMP_EXPORT void bk_recomp_aep_register_replacement_with_size(enum asset_e asset_id, void *asset_data, u32 asset_size);
RECOMP_EXPORT void bk_recomp_aep_unregister_replacement(enum asset_e asset_id);
```

### Libraries
o1heap: https://github.com/pavel-kirienko/o1heap - Copyright (c) 2020 Pavel Kirienko