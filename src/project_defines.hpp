#pragma once

// [Moonlight] Concatenates two compile time tokens
#define ML_CONCAT(x, y) x ## y
// [Moonlight] Appends the "L" character to the beginning of a string literal
// effectively widening the bit depth to 16.
#define ML_L(x) ML_CONCAT(L, x)	

#define ROOT_DIRECTORY_WIDE ML_L(ROOT_DIRECTORY_ASCII)