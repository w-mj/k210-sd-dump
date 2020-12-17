#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#define SUPER_FLASH_SPEED 115200

#if KENDRYTE_SDK_TYPE == KENDRYTE_SDK_TYPE_STANDALONE

#define SUPER_FLASH_ENABLED 1
// void super_flash_main();

#elif KENDRYTE_SDK_TYPE == KENDRYTE_SDK_TYPE_FREERTOS

// #define SUPER_FLASH_ENABLED 2
// void super_flash_main();

#pragma message \
    "fast flash freertos is not support now, please wait our work complete."

#else

#pragma message "fast flash require FREERTOS or STANDALONE sdk."

#endif




#ifndef SUPER_FLASH_ENABLED
static void super_flash_main() {}
#endif

#ifdef __cplusplus
}
#endif
