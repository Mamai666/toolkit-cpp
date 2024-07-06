// fast_memcpy.h

#ifndef FASTMEMCPY_H
#define FASTMEMCPY_H

void X_aligned_memcpy_sse2(char* dest, const char* src, const unsigned long size);

#endif
