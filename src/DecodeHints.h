
#include <stdint.h>
#include <stdio.h>

typedef enum 
{
	LocalAverage,    ///< T = average of neighboring pixels for 2D and GlobalHistogram for 1D (HybridBinarizer)
	GlobalHistogram ///< T = valley between the 2 largest peaks in the histogram (per line in 1D case)
}Binarizer_t;  // needs to unsigned for the bitfield below to work, uint8_t fails as well

typedef struct DecodeHints
{
	uint8_t tryHarder : 1;
	uint8_t tryRotate : 1;
	uint8_t isPure : 1;
	uint8_t returnCodabarStartEnd : 1;
	Binarizer_t LocalAverage;
}DecodeHints_t;
