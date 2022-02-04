#ifndef _MULTIFORMATREADER_H
#define _MULTIFORMATREADER_H

#include "DecodeHints.h"
#include "GenericLuminanceSource.h"

extern unsigned char * MultiFormatReader(GenericLuminanceSource_t * image,DecodeHints_t * hints);
#endif