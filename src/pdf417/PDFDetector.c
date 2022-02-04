

#include "PDFDetector.h"
#include "DecodeStatus.h"
#include "BinaryBitmap.h"
#include "HybridBinarizer.h"
#include <stdio.h>

static const int ROW_STEP = 8; // used to be 5, but 8 is enough for conforming symbols

uint8_t HasStartPattern(const BitMatrix_t * m)
{
	unsigned int array_length = 0;;
	printf("HasStartPattern \n");

	int START_PATTERN[8] = { 8, 1, 1, 1, 1, 1, 1, 3 };
	int minSymbolWidth = 3*8+1; // compact symbol

	uint16_t * row = (uint16_t*)malloc(sizeof(uint16_t) * array_length);

	for (int r = ROW_STEP; r < m->_height; r += ROW_STEP) 
	{
		array_length = getPatternRow(m, r, row);

		//if (FindLeftGuard(row, minSymbolWidth, START_PATTERN, 2).isValid())
			//return true;
		//std::reverse(row.begin(), row.end());
		//if (FindLeftGuard(row, minSymbolWidth, START_PATTERN, 2).isValid())
			//return true;
	}

	return 0;
}

DecodeStatus_t Detect(GenericLuminanceSource_t * image, unsigned char multiple, unsigned char * result)
{
	printf("Detector::Detect \n");
	BitMatrix_t * binImg;
	unsigned char * barcodeCoordinates;
	unsigned char * newBits;
	
	binImg = getBlackMatrix(image);
	if (binImg == NULL) 
	{
		return NotFound;
	}

	if (!HasStartPattern(binImg))
	{
		return NotFound;
	}

	#if 0
	barcodeCoordinates = DetectBarcode(*binImg, multiple);
	if (barcodeCoordinates.empty()) 
	{
		newBits = binImg->copy();
		newBits->rotate180();
		binImg = newBits;
		barcodeCoordinates = DetectBarcode(*binImg, multiple);
	}
	if (barcodeCoordinates.empty()) {
		return NotFound;
	}
	result.points = barcodeCoordinates;
	result.bits = binImg;
	#endif
	return NoError;
}

