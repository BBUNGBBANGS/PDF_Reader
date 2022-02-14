#ifndef _TESTDECODER_H
#define _TESTDECODER_H
#include "CharacterSet.h"
#include <stdint.h>

extern void Append(unsigned char * str, const uint8_t* bytes, size_t length, CharacterSet_t charset);
#endif