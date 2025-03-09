#ifndef zmx_call_h
#define zmx_call_h

#if defined(_MSC_VER) || defined(_MINGW32) || defined(__MINGW32__) || defined(WIN32) || defined(WIN64)
#define RTLD_GLOBAL 0x100 /* do not hide entries in this module */
#define RTLD_LOCAL  0x000 /* hide entries in this module */
#define RTLD_LAZY   0x000 /* accept unresolved externs */
#define RTLD_NOW    0x001 /* abort if module has unresolved externs */
    #include <windows.h>
    #define ZEMMIX_BUS "zmxbus.zxw"
    #define ZEMMIX_DRIVE "zmxdrive.zxw"        
#elif defined(__GNUC__)
    #include <dlfcn.h>
    #define ZEMMIX_BUS "./zmxbus.zxl"    
    #define ZEMMIX_DRIVE "./zmxdrive.zxl"
#else
    #error define your compiler
#endif

extern "C" {
    void *OpenZemmix(char *pcDllname, int iMode);
    void *GetZemmixFunc(void *Lib, char *Fnname);
    int CloseZemmix(void *hDLL);
    char *GetZemmixError();    
}
#endif