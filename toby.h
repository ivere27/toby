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

typedef void  (*TobyOnloadCB)(void* isolate, void* data);
typedef void  (*TobyOnunloadCB)(void* isolate, int exitCode, void* data);
typedef char* (*TobyHostcallCB)(const char* name, const char* value, void* data);
typedef void  (*TobyHostonCB)(int argc, char** argv, void* data);


TOBY_EXTERN void tobyInit(const char* processName,
                         const char* userScript,
                         TobyOnloadCB,
                         TobyOnunloadCB,
                         TobyHostcallCB,
                         void* data);
TOBY_EXTERN int  tobyJSCompile(const char* source, char* dest, size_t n);
TOBY_EXTERN int  tobyJSCall(const char* name, const char* value, char* dest, size_t n);
TOBY_EXTERN int  tobyJSEmit(const char* name, const char* value);
TOBY_EXTERN int  tobyHostOn(const char* name, TobyHostonCB);

#ifdef __cplusplus
}   // extern
}   // namespace
#endif