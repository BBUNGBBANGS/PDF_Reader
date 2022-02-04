#ifndef _PDFREADER_H
#define _PDFREADER_H

#include "DecodeHints.h"
#include "GenericLuminanceSource.h"

extern unsigned char * decode(GenericLuminanceSource_t * image,DecodeHints_t * hints);

#endif