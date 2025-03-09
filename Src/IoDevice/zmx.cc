#define DLL_EXPORTS
#include "zmx.h"
#include <string>

extern "C" {
    void *OpenZemmix(char *pcDllname, int iMode = 2)
    {
        #if defined(_MSC_VER) || defined(_MINGW32) || defined(__MINGW32__) || defined(WIN32) || defined(WIN64)
            return (void*)LoadLibrary(std::string(pcDllname).c_str());
        #elif defined(__GNUC__)
            return dlopen(std::string(pcDllname).c_str(), iMode);
        #endif
    }
    void *GetZemmixFunc(void *Lib, char *Fnname)
    { 
        #if defined(_MSC_VER) || defined(_MINGW32) || defined(__MINGW32__) || defined(WIN32) || defined(WIN64)
            return (void *)GetProcAddress((HINSTANCE)Lib,Fnname);
        #elif defined(__GNUC__)
            return dlsym(Lib,Fnname);
        #endif
    }
    int CloseZemmix(void *hDLL)
    {
        #if defined(_MSC_VER) || defined(_MINGW32) || defined(__MINGW32__) || defined(WIN32) || defined(WIN64) 
            return FreeLibrary((HINSTANCE)hDLL);
        #elif defined(__GNUC__)
            return dlclose(hDLL);
        #endif
    }
    char *GetZemmixError()
    {
        #if defined(_MSC_VER) || defined(_MINGW32) || defined(__MINGW32__) || defined(WIN32) || defined(WIN64)
            static char errors[512];
            sprintf(errors, "DLL open error: %d", GetLastError());
            return errors;
        #elif defined(__GNUC__)
            return dlerror();
        #endif
    }    
}
