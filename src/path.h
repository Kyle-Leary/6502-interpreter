#pragma once

// heavily abusing the macro concat in C here.

#define ASSET_PATH "assets/"

// concat the macro, better than making functions that
// sprintf and malloc the path at runtime. faster and easier.
#define TEXTURE_PATH(base_path) ASSET_PATH "textures/" base_path
#define SOUND_PATH(base_path) ASSET_PATH "sounds/" base_path
#define SHADER_PATH(base_path) ASSET_PATH "shaders/" base_path

// general purpose resources, like random area file-formats that we'll parse and
// use on the fly.
#define RESOURCE_PATH ASSET_PATH "resources/"
#define AREA_PATH(base_path) RESOURCE_PATH "areas/" base_path
