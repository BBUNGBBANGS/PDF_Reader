#ifndef _MULTIFORMATREADER_H
#define _MULTIFORMATREADER_H

#include "DecodeHints.h"
#include "GenericLuminanceSource.h"
#include "Result.h"

extern result_t MultiFormatReader(GenericLuminanceSource_t * image,DecodeHints_t * hints);

#endif