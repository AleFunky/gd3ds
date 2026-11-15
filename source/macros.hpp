#pragma once

// Match cocos2d source
#define CC_PI 3.14159265358979323846
#define CC_PI_2 CC_PI / 2.0
#define CC_PI_X_2 (float)CC_PI * 2.0f

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define UNUSED __attribute__((unused)) 
#define IN_BOUNDS(index, arr) ((size_t)index < ARRAY_LEN(arr))