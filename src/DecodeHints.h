
#include <stdint.h>
#include <stdio.h>
#include "BarcodeFormat.h"
typedef enum 
{
	LocalAverage,    ///< T = average of neighboring pixels for 2D and GlobalHistogram for 1D (HybridBinarizer)
	GlobalHistogram ///< T = valley between the 2 largest peaks in the histogram (per line in 1D case)
}Binarizer_t;  // needs to unsigned for the bitfield below to work, uint8_t fails as well

typedef struct DecodeHints
{
	uint8_t tryHarder;
	uint8_t tryRotate;
	uint8_t isPure;
	uint8_t returnCodabarStartEnd;
	Binarizer_t Binarizer;
	BarcodeFormats_t formats;
	uint64_t * characterSet;
	uint64_t * allowedLengths;
}DecodeHints_t;
