#ifndef zmx_call_h
#define zmx_call_h

#if defined(_MSC_VER) || defined(_MINGW32) || defined(__MINGW32__)
#define RTLD_GLOBAL 0x100 /* do not hide entries in this module */
#define RTLD_LOCAL  0x000 /* hide entries in this module */
#define RTLD_LAZY   0x000 /* accept unresolved externs */
#define RTLD_NOW    0x001 /* abort if module has unresolved externs */
    #include <windows.h>
    #define ZEMMIX_EXT ".zxw"    
#elif defined(__GNUC__)
    #include <dlfcn.h>
    #define ZEMMIX_EXT ".zxl"    
#else
    #error define your compiler
#endif

void *OpenZemmix(char *pcDllname, int iMode);
void *GetZemmixFunc(void *Lib, char *Fnname);
int CloseZemmix(void *hDLL);
char *GetZemmixError();

#endif
