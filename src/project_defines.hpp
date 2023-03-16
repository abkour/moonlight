#pragma once

// [Moonlight] Concatenates two compile time tokens
#define ML_CONCAT(x, y) x ## y
// [Moonlight] Appends the "L" character to the beginning of a string literal
// effectively widening the bit depth to 16.
#define ML_L(x) ML_CONCAT(L, x)    

#define ROOT_DIRECTORY_WIDE ML_L(ROOT_DIRECTORY_ASCII)

#define ML_MISC_FLAGS_NONE                      0x00
#define ML_MISC_FLAG_CENTROIDS_INCLUDED         0x01
#define ML_MISC_FLAG_ATTR_VERTEX                0x02
#define ML_MISC_FLAG_ATTR_VERTEX_NORMAL         0x04
#define ML_MISC_FLAG_ATTR_VERTEX_NORMAL_UV      0x08
#define ML_MISC_FLAG_INDICES                    0x10
#define ML_MISC_FLAG_PRINT_TRIANGLES_TO_CONSOLE 0x20
#define ML_MISC_FLAG_MATERIALS_APPENDED         0x40