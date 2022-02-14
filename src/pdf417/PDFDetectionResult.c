
#include "PDFDetectionResult.h"
#include "PDFCodewordDecoder.h"
#include "PDFScanningDecoder.h"

static const int ADJUST_ROW_NUMBER_SKIP = 2;

void Detection_Result_init(const BarcodeMetadata_t * barcodeMetadata, const BoundingBox_t * boundingBox, DetectionResult_t * result)
{
	result->_barcodeMetadata = *barcodeMetadata;
	result->_boundingBox = *boundingBox;
	result->_detectionResultColumns[barcodeMetadata->_columnCount + 2];
	//std::fill(_detectionResultColumns.begin(), _detectionResultColumns.end(), nullptr);
}

static void AdjustIndicatorColumnRowNumbers(DetectionResultColumn_t * detectionResultColumn, const BarcodeMetadata_t * barcodeMetadata,int size)
{
	if (detectionResultColumn->m_hasValue != 0) 
	{
		adjustCompleteIndicatorColumnRowNumbers(detectionResultColumn,barcodeMetadata,size);
	}
}

static void AdjustRowNumbersFromBothRI(DetectionResultColumn_t * detectionResultColumns,int column,int size)
{
	if ((detectionResultColumns[0].m_hasValue == 0) || detectionResultColumns[column-1].m_hasValue == 0) 
	{
		return;
	}

	Codeword_t * LRIcodewords = allCodewords(&detectionResultColumns[0]);
	Codeword_t * RRIcodewords = allCodewords(&detectionResultColumns[column-1]);

	for (size_t codewordsRow = 0; codewordsRow < size; codewordsRow++) 
	{

		if ((LRIcodewords[codewordsRow].m_hasValue != 0) && (RRIcodewords[codewordsRow].m_hasValue != 0) &&
			(LRIcodewords[codewordsRow].m_value._rowNumber == RRIcodewords[codewordsRow].m_value._rowNumber)) 
		{

			DetectionResultColumn_t * lastColumn = &detectionResultColumns[column-1];
			for (DetectionResultColumn_t * columnIter = (&detectionResultColumns[0] + 1); columnIter != lastColumn; ++columnIter) 
			{
				if (!(columnIter->m_hasValue)) 
				{
					continue;
				}
				Codeword_t codeword = columnIter->m_value._codewords[codewordsRow];
				if (codeword.m_hasValue != 0) 
				{
					codeword.m_value._rowNumber = LRIcodewords[codewordsRow].m_value._rowNumber;
					if (!(codeword.m_value._rowNumber != BARCODE_ROW_UNKNOWN && codeword.m_value._bucket == (codeword.m_value._rowNumber % 3) * 3)) 
					{
						clear_Codewords(&codeword);
					}
					columnIter->m_value._codewords[codewordsRow] = codeword;
				}
			}

		}

	}
}

static int AdjustRowNumberIfValid(int rowIndicatorRowNumber, int invalidRowCounts, Codeword_t * codeword) 
{
	if (!(codeword->m_value._rowNumber != BARCODE_ROW_UNKNOWN && codeword->m_value._bucket == (codeword->m_value._rowNumber % 3) * 3)) 
	{
		if (rowIndicatorRowNumber != BARCODE_ROW_UNKNOWN && codeword->m_value._bucket == (rowIndicatorRowNumber % 3) * 3) 
		{
			codeword->m_value._rowNumber = rowIndicatorRowNumber;
			invalidRowCounts = 0;
		}
		else 
		{
			++invalidRowCounts;
		}
	}
	return invalidRowCounts;
}

static int AdjustRowNumbersFromLRI(DetectionResultColumn_t * detectionResultColumns,int column,int size) 
{
	if (detectionResultColumns[0].m_hasValue == 0) 
	{
		return 0;
	}
	int unadjustedCount = 0;
	Codeword_t * codewords = detectionResultColumns[0].m_value._codewords;
	for (size_t codewordsRow = 0; codewordsRow < size; codewordsRow++) 
	{
		if (codewords[codewordsRow].m_hasValue == 0) 
		{
			continue;
		}
		int rowIndicatorRowNumber = codewords[codewordsRow].m_value._rowNumber;
		int invalidRowCounts = 0;
		DetectionResultColumn_t * lastColumn = &detectionResultColumns[column-1];
		for (DetectionResultColumn_t * columnIter = &detectionResultColumns[0] + 1; columnIter != lastColumn && invalidRowCounts < ADJUST_ROW_NUMBER_SKIP; ++columnIter) 
		{
			if (!(columnIter->m_hasValue)) 
			{
				continue;
			}
			Codeword_t codeword = columnIter->m_value._codewords[codewordsRow];
			if (codeword.m_hasValue != 0) 
			{
				invalidRowCounts = AdjustRowNumberIfValid(rowIndicatorRowNumber, invalidRowCounts, &codeword);
				if (!(codeword.m_value._rowNumber != BARCODE_ROW_UNKNOWN && codeword.m_value._bucket == (codeword.m_value._rowNumber % 3) * 3)) 
				{
					unadjustedCount++;
				}
			}
			columnIter->m_value._codewords[codewordsRow] = codeword;
		}
	}
	return unadjustedCount;
}

static int AdjustRowNumbersFromRRI(DetectionResultColumn_t *detectionResultColumns,int column,int size) 
{
	if (detectionResultColumns[column-1].m_hasValue == 0) 
	{
		return 0;
	}
	int unadjustedCount = 0;
	Codeword_t * codewords = detectionResultColumns[column-1].m_value._codewords;
	for (size_t codewordsRow = 0; codewordsRow < size; codewordsRow++) 
	{
		if (codewords[codewordsRow].m_hasValue == 0) 
		{
			continue;
		}
		int rowIndicatorRowNumber = codewords[codewordsRow].m_value._rowNumber;
		int invalidRowCounts = 0;
		DetectionResultColumn_t * lastColumn = &detectionResultColumns[column-1];
		for (DetectionResultColumn_t * columnIter = &detectionResultColumns[0] + 1; columnIter != lastColumn && invalidRowCounts < ADJUST_ROW_NUMBER_SKIP; ++columnIter) 
		{
			if (!(columnIter->m_hasValue)) 
			{
				continue;
			}
			Codeword_t codeword = columnIter->m_value._codewords[codewordsRow];
			if (codeword.m_hasValue != 0) 
			{
				invalidRowCounts = AdjustRowNumberIfValid(rowIndicatorRowNumber, invalidRowCounts, &codeword);
				if (!(codeword.m_value._rowNumber != BARCODE_ROW_UNKNOWN && codeword.m_value._bucket == (codeword.m_value._rowNumber % 3) * 3)) 
				{
					unadjustedCount++;
				}
			}
			columnIter->m_value._codewords[codewordsRow] = codeword;
		}
	}

	return unadjustedCount;
}

static int AdjustRowNumbersByRow(DetectionResultColumn_t * detectionResultColumns,int column,int size) 
{

	AdjustRowNumbersFromBothRI(detectionResultColumns,column,size);
	// TODO we should only do full row adjustments if row numbers of left and right row indicator column match.
	// Maybe it's even better to calculated the height (in codeword rows) and divide it by the number of barcode
	// rows. This, together with the LRI and RRI row numbers should allow us to get a good estimate where a row
	// number starts and ends.
	int unadjustedCount = AdjustRowNumbersFromLRI(detectionResultColumns,column,size);
	return unadjustedCount + AdjustRowNumbersFromRRI(detectionResultColumns,column,size);

}

/**
* @return true, if row number was adjusted, false otherwise
*/
static uint8_t AdjustRowNumber(Codeword_t * codeword, const Codeword_t * otherCodeword) 
{
	if ((codeword->m_hasValue != 0) && (otherCodeword->m_hasValue != 0) && 
	   ((otherCodeword->m_value._rowNumber != BARCODE_ROW_UNKNOWN) && (otherCodeword->m_value._bucket == (otherCodeword->m_value._rowNumber % 3) * 3)) && 
	   (otherCodeword->m_value._bucket == codeword->m_value._bucket)) 
	{
		codeword->m_value._rowNumber = (otherCodeword->m_value._rowNumber);
		return 1;
	}
	return 0;
}


static void AdjustRowNumbers_internal(const DetectionResultColumn_t * detectionResultColumns, int barcodeColumn, int codewordsRow, Codeword_t * codewords,int size) 
{
	Codeword_t codeword = codewords[codewordsRow];
	Codeword_t * previousColumnCodewords = allCodewords(&detectionResultColumns[barcodeColumn - 1]);
	Codeword_t * nextColumnCodewords = detectionResultColumns[barcodeColumn + 1].m_hasValue != 0 ? allCodewords(&detectionResultColumns[barcodeColumn + 1]) : previousColumnCodewords;

	Codeword_t otherCodewords[14];
	int size1 = sizeof(otherCodewords)/sizeof(otherCodewords[0]);

	otherCodewords[2] = previousColumnCodewords[codewordsRow];
	otherCodewords[3] = nextColumnCodewords[codewordsRow];

	if (codewordsRow > 0) 
	{
		otherCodewords[0] = codewords[codewordsRow - 1];
		otherCodewords[4] = previousColumnCodewords[codewordsRow - 1];
		otherCodewords[5] = nextColumnCodewords[codewordsRow - 1];
	}
	if (codewordsRow > 1) 
	{
		otherCodewords[8] = codewords[codewordsRow - 2];
		otherCodewords[10] = previousColumnCodewords[codewordsRow - 2];
		otherCodewords[11] = nextColumnCodewords[codewordsRow - 2];
	}

	if (codewordsRow < size - 1) 
	{
		otherCodewords[1] = codewords[codewordsRow + 1];
		otherCodewords[6] = previousColumnCodewords[codewordsRow + 1];
		otherCodewords[7] = nextColumnCodewords[codewordsRow + 1];
	}
	if (codewordsRow < size - 2) 
	{
		otherCodewords[9] = codewords[codewordsRow + 2];
		otherCodewords[12] = previousColumnCodewords[codewordsRow + 2];
		otherCodewords[13] = nextColumnCodewords[codewordsRow + 2];
	}

	for (int i=0;i<size1;i++) 
	{
		if (AdjustRowNumber(&codeword, otherCodewords)) 
		{
			return;
		}
	}
}


// TODO ensure that no detected codewords with unknown row number are left
// we should be able to estimate the row height and use it as a hint for the row number
// we should also fill the rows top to bottom and bottom to top
/**
* @return number of codewords which don't have a valid row number. Note that the count is not accurate as codewords
* will be counted several times. It just serves as an indicator to see when we can stop adjusting row numbers
*/
static int AdjustRowNumbers(DetectionResultColumn_t * detectionResultColumns,int column,int size) 
{
	int unadjustedCount = AdjustRowNumbersByRow(detectionResultColumns,column,size);
	if (unadjustedCount == 0) 
	{
		return 0;
	}
	
	for (int barcodeColumn = 1; barcodeColumn <column; barcodeColumn++) 
	{
		if (detectionResultColumns[barcodeColumn].m_hasValue == 0) 
		{
			continue;
		}
		Codeword_t * codewords = detectionResultColumns[barcodeColumn].m_value._codewords;
		for (int codewordsRow = 0; codewordsRow < size; codewordsRow++) 
		{
			if (codewords[codewordsRow].m_hasValue == 0) 
			{
				continue;
			}
			if (!codewords[codewordsRow].m_value._rowNumber != BARCODE_ROW_UNKNOWN && codewords[codewordsRow].m_value._bucket == (codewords[codewordsRow].m_value._rowNumber % 3) * 3) 
			{
				AdjustRowNumbers_internal(detectionResultColumns, barcodeColumn, codewordsRow, codewords,size);
			}
		}
	}
	return unadjustedCount;
}

const DetectionResultColumn_t * allColumns(DetectionResult_t * detectionResult,int column,int size)
{
	AdjustIndicatorColumnRowNumbers(&(detectionResult->_detectionResultColumns[0]), &(detectionResult->_barcodeMetadata),size);
	AdjustIndicatorColumnRowNumbers(&(detectionResult->_detectionResultColumns[column-1]), &(detectionResult->_barcodeMetadata),size);//back
	int unadjustedCodewordCount = MAX_CODEWORDS_IN_BARCODE;
	int previousUnadjustedCount;
	do 
	{
		previousUnadjustedCount = unadjustedCodewordCount;
		unadjustedCodewordCount = AdjustRowNumbers(detectionResult->_detectionResultColumns,column,size);
	} while (unadjustedCodewordCount > 0 && unadjustedCodewordCount < previousUnadjustedCount);
	return (detectionResult->_detectionResultColumns);
}

