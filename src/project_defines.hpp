#pragma once

#define CONCAT(x, y) x ## y
#define L(x) CONCAT(L, x)

#define ROOT_DIRECTORY_WIDE L(ROOT_DIRECTORY_ASCII)