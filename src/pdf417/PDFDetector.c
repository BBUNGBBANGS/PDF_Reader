

#include "PDFDetector.h"
#include "DecodeStatus.h"
#include "BinaryBitmap.h"
#include "HybridBinarizer.h"
#include "ResultPoint.h"
#include <stdio.h>
#include <stdlib.h>

static const int INDEXES_START_PATTERN[] = { 0, 4, 1, 5 };
static const int INDEXES_STOP_PATTERN[] = { 6, 2, 7, 3 };
static const float MAX_AVG_VARIANCE = 0.42f;
static const float MAX_INDIVIDUAL_VARIANCE = 0.8f;

// B S B S B S B S Bar/Space pattern
// 11111111 0 1 0 1 0 1 000
static const int START_PATTERN[8] = { 8, 1, 1, 1, 1, 1, 1, 3 };
// 1111111 0 1 000 1 0 1 00 1
static const int STOP_PATTERN[9] = { 7, 1, 1, 3, 1, 1, 1, 2, 1 };
static const int MAX_PIXEL_DRIFT = 3;
static const int MAX_PATTERN_DRIFT = 5;
// if we set the value too low, then we don't detect the correct height of the bar if the start patterns are damaged.
// if we set the value too high, then we might detect the start pattern from a neighbor barcode.
static const int SKIPPED_ROW_COUNT_MAX = 25;
// A PDF471 barcode should have at least 3 rows, with each row being >= 3 times the module width. Therefore it should be at least
// 9 pixels tall. To be conservative, we use about half the size to ensure we don't miss it.
static const int ROW_STEP = 8; // used to be 5, but 8 is enough for conforming symbols
static const int BARCODE_MIN_HEIGHT = 10;

static ResultPoint_t Vertice_Result[8];
#define FLT_MAX          3.402823466e+38F        // max value
#if 0
PatternView FindLeftGuard(const PatternView& view, int minSize, const * pattern, float minQuiteZone)
{
	return FindLeftGuard<LEN>(view, std::max(minSize, LEN),(const PatternView& window, int spaceInPixel) 
	{
								  return IsPattern(window, pattern, spaceInPixel, minQuiteZone);
							  });
}
#endif
static float PatternMatchVariance(const int * counters, const int * pattern,int length, float maxIndividualVariance)
{
	printf("PatternMatchVariance  maxIndividualVariance = %f\n", maxIndividualVariance);

	int total = 0;
	int patternLength = 0;
	for (size_t i = 0; i < length; i++) 
	{
		total += counters[i];
		patternLength += pattern[i];
	}

	if (total < patternLength) 
	{
		// If we don't even have one pixel per unit of bar width, assume this
		// is too small to reliably match, so fail:
		return FLT_MAX;
	}
	// We're going to fake floating-point math in integers. We just need to use more bits.
	// Scale up patternLength so that intermediate values below like scaledCounter will have
	// more "significant digits".
	float unitBarWidth = (float)total / patternLength;
	maxIndividualVariance *= unitBarWidth;

	float totalVariance = 0.0f;
	for (size_t x = 0; x < length; x++) 
	{
		int counter = counters[x];
		float scaledPattern = pattern[x] * unitBarWidth;
		float variance = counter > scaledPattern ? counter - scaledPattern : scaledPattern - counter;
		if (variance > maxIndividualVariance) {
			return FLT_MAX;
		}
		totalVariance += variance;
	}

	return totalVariance / total;
}

static uint8_t FindGuardPattern(const BitMatrix_t * matrix, int column, int row, int width, uint8_t whiteFirst,const int * pattern, int patternLength, int * counters, int * startPos, int * endPos)
{

	printf("FindGuardPattern col=%d, row=%d, widht=%d, WhiteFirst=%d\n", column, row, width, whiteFirst);

	for(uint8_t i=0;i<8;i++)
	{
		counters[i] = 0;
	}

	uint8_t isWhite = whiteFirst;
	int patternStart = column;
	int pixelDrift = 0;

	// if there are black pixels left of the current pixel shift to the left, but only for MAX_PIXEL_DRIFT pixels 
	while ((matrix->bits[(row * matrix->_width + patternStart)]) && patternStart > 0 && pixelDrift++ < MAX_PIXEL_DRIFT) {
		patternStart--;
	}
	int x = patternStart;
	int counterPosition = 0;
	for (; x < width; x++) {
		uint8_t pixel = matrix->bits[(row * matrix->_width + x)];
		if (pixel != isWhite) {
			counters[counterPosition]++;
		}
		else {
			if (counterPosition == patternLength - 1) {
				if (PatternMatchVariance(counters, pattern,patternLength, MAX_INDIVIDUAL_VARIANCE) < MAX_AVG_VARIANCE) {
					*startPos = patternStart;
					*endPos = x;
					return 1;
				}
				patternStart += counters[0] + counters[1];
				for(uint8_t i=0;i<(patternLength-2);i++)
				{
					counters[i] = counters[2+i];
				}
				counters[patternLength - 2] = 0;
				counters[patternLength - 1] = 0;
				counterPosition--;
			}
			else {
				counterPosition++;
			}
			counters[counterPosition] = 1;
			isWhite = !isWhite;
		}
	}
	if (counterPosition == patternLength - 1) {
		if (PatternMatchVariance(counters, pattern,patternLength, MAX_INDIVIDUAL_VARIANCE) < MAX_AVG_VARIANCE) {
			*startPos = patternStart;
			*endPos = x - 1;
			return 1;
		}
	}
	return 0;
}

static uint8_t * FindRowsWithPattern(const BitMatrix_t * matrix, int height, int width, int startRow, int startColumn,const int * pattern, int patternlength , ResultPoint_t * result)
{

	printf("FindRowsWithPattern startRow= %d \n",startRow);

	uint8_t found = 0;
	int startPos, endPos;

	int counters[patternlength];
	for (; startRow < height; startRow += ROW_STEP) {
		if (FindGuardPattern(matrix, startColumn, startRow, width, 0, pattern,patternlength, counters, &startPos, &endPos)) {
			while (startRow > 0) {
				if (!FindGuardPattern(matrix, startColumn, --startRow, width, 0, pattern,patternlength, counters, &startPos, &endPos)) {
					startRow++;
					break;
				}
			}
			result[0].m_hasValue = 1;
			result[0].m_value.x = startPos;
			result[0].m_value.y = startRow;
			result[1].m_hasValue = 1;
			result[1].m_value.x = endPos;
			result[1].m_value.y = startRow;
			found = 1;
			break;
		}
	}
	int stopRow = startRow + 1;
	// Last row of the current symbol that contains pattern
	if (found) {
		int skippedRowCount = 0;
		int previousRowStart = (int)(result[0].m_value.x);
		int previousRowEnd = (int)(result[1].m_value.x);
		for (; stopRow < height; stopRow++) {
			int startPos, endPos;
			found = FindGuardPattern(matrix, previousRowStart, stopRow, width, 0, pattern,patternlength, counters, &startPos, &endPos);
			// a found pattern is only considered to belong to the same barcode if the start and end positions
			// don't differ too much. Pattern drift should be not bigger than two for consecutive rows. With
			// a higher number of skipped rows drift could be larger. To keep it simple for now, we allow a slightly
			// larger drift and don't check for skipped rows.
			if (found && abs(previousRowStart - startPos) < MAX_PATTERN_DRIFT && abs(previousRowEnd - endPos) < MAX_PATTERN_DRIFT) {
				previousRowStart = startPos;
				previousRowEnd = endPos;
				skippedRowCount = 0;
			}
			else if (skippedRowCount > SKIPPED_ROW_COUNT_MAX) {
				break;
			}
			else {
				skippedRowCount++;
			}
		}
		stopRow -= skippedRowCount + 1;
		result[2].m_hasValue = 1;
		result[2].m_value.x = previousRowStart;
		result[2].m_value.y = stopRow;
		result[3].m_hasValue = 1;
		result[3].m_value.x = previousRowEnd;
		result[3].m_value.y = stopRow;
	}
	if (stopRow - startRow < BARCODE_MIN_HEIGHT) 
	{
		for(uint8_t i = 0;i<4;i++)
		{
			result[i].m_hasValue = 0;
			result[i].m_value.x = 0;
			result[i].m_value.y = 0;
		}
		
	}

	return result;
}

static void CopyToResult(ResultPoint_t * result, const ResultPoint_t * tmpResult, const int destinationIndexes[4])
{
	for (int i = 0; i < 4; i++) 
	{
		result[destinationIndexes[i]].m_hasValue = tmpResult[i].m_hasValue;
		result[destinationIndexes[i]].m_value.x = tmpResult[i].m_value.x;
		result[destinationIndexes[i]].m_value.y = tmpResult[i].m_value.y;
	}
}

static ResultPoint_t * FindVertices(const BitMatrix_t * matrix, int startRow, int startColumn)
{
	int width = matrix->_width;
	int height = matrix->_height;

	ResultPoint_t tmp[4];

	for(uint8_t i=0;i<4;i++)
	{
		tmp[i].m_hasValue = 0;
		tmp[i].m_value.x = 0.0f;
		tmp[i].m_value.y = 0.0f;
	}

	for(uint8_t i=0;i<8;i++)
	{
		Vertice_Result[i].m_hasValue = 0;
		Vertice_Result[i].m_value.x = 0.0f;
		Vertice_Result[i].m_value.y = 0.0f;
	}

	CopyToResult(Vertice_Result, FindRowsWithPattern(matrix, height, width, startRow, startColumn, START_PATTERN,8, tmp), INDEXES_START_PATTERN);

	if (Vertice_Result[4].m_hasValue != 0) 
	{
		startColumn = (Vertice_Result[4].m_value.x);
		startRow    = (Vertice_Result[4].m_value.y);
		// 2x speed improvement for images with no PDF417 symbol by not looking for symbols without start guard (which are not conforming to spec anyway)
		CopyToResult(Vertice_Result, FindRowsWithPattern(matrix, height, width, startRow, startColumn, STOP_PATTERN,9, tmp), INDEXES_STOP_PATTERN);
	}

	return Vertice_Result;
}

static ResultPoint_t * DetectBarcode(const BitMatrix_t * bitMatrix, uint8_t multiple)
{
	printf("DetectBarcode %d \n",multiple);
	int row = 0;
	int column = 0;
	uint8_t foundBarcodeInRow = 0;
	ResultPoint_t * barcodeCoordinates = NULL;
	ResultPoint_t * vertices = NULL;

	while (row < bitMatrix->_height) 
	{
		 vertices = FindVertices(bitMatrix, row, column);

		if (vertices[0].m_hasValue == 0 && vertices[3].m_hasValue == 0) 
		{
			if (!foundBarcodeInRow) 
			{
				// we didn't find any barcode so that's the end of searching 
				break;
			}
			// we didn't find a barcode starting at the given column and row. Try again from the first column and slightly
			// below the lowest barcode we found so far.
			#if 0
			foundBarcodeInRow = false;
			column = 0;
			for (auto& barcodeCoordinate : barcodeCoordinates) 
			{
				if (barcodeCoordinate[1] != NULL) 
				{
					row = __max(row, (barcodeCoordinate[1].value().y()));
				}

				if (barcodeCoordinate[3] != NULL) 
				{
					row = __max(row, (barcodeCoordinate[3].value().y()));
				}
			}

			row += ROW_STEP;
			continue;
			#endif
		}

		foundBarcodeInRow = 1;
		barcodeCoordinates = vertices;
		if (!multiple) 
		{
			break;
		}

		// if we didn't find a right row indicator column, then continue the search for the next barcode after the 
		// start pattern of the barcode just found.
		if (vertices[2].m_hasValue != 0) 
		{
			column = (vertices[2].m_value.x);
			row    = (vertices[2].m_value.y);
		}
		else 
		{
			column = (vertices[4].m_value.x);
			row    = (vertices[4].m_value.y);
		}
	}

	return barcodeCoordinates;
}

uint8_t HasStartPattern(const BitMatrix_t * m)
{
	printf("HasStartPattern \n");

	int START_PATTERN[8] = { 8, 1, 1, 1, 1, 1, 1, 3 };
	int minSymbolWidth = 3*8+1; // compact symbol

	uint16_t * row = NULL;

	for (int r = ROW_STEP; r < m->_height; r += ROW_STEP) 
	{
		getPatternRow(m, r, row);

		//if (FindLeftGuard(row, minSymbolWidth, START_PATTERN, 2).isValid())
			//return true;
		//std::reverse(row.begin(), row.end());
		//if (FindLeftGuard(row, minSymbolWidth, START_PATTERN, 2).isValid())
			//return true;
	}

	return 1; // leftguard 구현 필요
}
static uint8_t Empty_Check(ResultPoint_t * barcodeCoordinates)
{
	uint8_t status=0;
	uint8_t ret=0;
	for(uint8_t i=0;i<8;i++)
	{
		if(barcodeCoordinates->m_hasValue == 0)
		{
			status |= 1<<i;
		} 
	}

	if(status == 0xff)
	{
		ret = 1; //empty
	}
	else
	{
		ret = 0; //no empty
	}
	
	return ret;
}
DecodeStatus_t Detect(GenericLuminanceSource_t * image, unsigned char multiple, Detector_Result_t * result)
{
	printf("Detector::Detect \n");
	BitMatrix_t * binImg;
	ResultPoint_t * barcodeCoordinates;
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

	barcodeCoordinates = DetectBarcode(binImg, multiple);

	if (Empty_Check(barcodeCoordinates))
	{
		//newBits = binImg->copy();
		//newBits->rotate180();
		binImg = newBits;
		barcodeCoordinates = DetectBarcode(binImg, multiple);
	}
	if (Empty_Check(barcodeCoordinates)) 
	{
		return NotFound;
	}
	result->points = barcodeCoordinates;
	result->bits = binImg;

	return NoError;
}

