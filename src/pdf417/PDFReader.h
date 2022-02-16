#ifndef _PDFREADER_H
#define _PDFREADER_H

#include "DecodeHints.h"
#include "GenericLuminanceSource.h"
#include "Result.h"

extern result_t decode(GenericLuminanceSource_t * image,DecodeHints_t * hints);

#endif