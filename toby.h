#include <stdlib.h>

#ifdef _WIN32
#define TOBY_EXTERN __declspec(dllexport)
#else
#define TOBY_EXTERN /* nothing */
#endif


#ifdef __cplusplus
namespace toby {
extern "C" {
#endif

typedef void  (*TobyOnloadCB)(void* isolate);
typedef void  (*TobyOnunloadCB)(void* isolate, int exitCode);
typedef char* (*TobyHostcallCB)(const char* name, const char* value);
typedef void  (*TobyHostonCB)(int argc, char** argv);

TOBY_EXTERN void tobyInit(const char* processName,
                         const char* userScript,
                         TobyOnloadCB,
                         TobyOnunloadCB,
                         TobyHostcallCB);
TOBY_EXTERN int  tobyJSCompile(const char* source, char* dest, size_t n);
TOBY_EXTERN int  tobyJSCall(const char* name, const char* value, char* dest, size_t n);
TOBY_EXTERN int  tobyJSEmit(const char* name, const char* value);
TOBY_EXTERN int  tobyHostOn(const char* name, TobyHostonCB);

#ifdef __cplusplus
}   // extern
}   // namespace
#endif