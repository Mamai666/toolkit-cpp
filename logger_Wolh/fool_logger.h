#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>

    inline void printf_stub(const char* s, ...)
    {
        va_list args;
        va_start(args, s);
        printf(s, args);
        va_end(args);
    }

    #define pLogFatal(...) printf_stub(__VA_ARGS__)
    #define pLogWarning(...) printf_stub(__VA_ARGS__)
    #define pLogDebug(...) printf_stub(__VA_ARGS__)

#ifdef __cplusplus
}
#endif
