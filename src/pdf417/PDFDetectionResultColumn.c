/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "PDFBarcodeMetadata.h"
#include "PDFBarcodeValue.h"
#include "PDFScanningDecoder.h"
#include <stdlib.h>
#include <stdio.h>
#include "Library.h"

static const int MAX_NEARBY_DISTANCE = 5;
static const int MIN_ROWS_IN_BARCODE = 3;
static const int MAX_ROWS_IN_BARCODE = 90;

int codewords[1000];

void DetectionResultColumn(const BoundingBox_t * boundingBox, RowIndicator_t rowInd, DetectionResultColumn_t * RowIndicatorColumn)
{
	RowIndicatorColumn->m_value._boundingBox.m_hasValue = 1;
	RowIndicatorColumn->m_value._boundingBox.m_value._imgWidth = boundingBox->m_value._imgWidth;
	RowIndicatorColumn->m_value._boundingBox.m_value._imgHeight = boundingBox->m_value._imgHeight;
	RowIndicatorColumn->m_value._boundingBox.m_value._topLeft = boundingBox->m_value._topLeft;
	RowIndicatorColumn->m_value._boundingBox.m_value._bottomLeft = boundingBox->m_value._bottomLeft;
	RowIndicatorColumn->m_value._boundingBox.m_value._topRight = boundingBox->m_value._topRight;
	RowIndicatorColumn->m_value._boundingBox.m_value._bottomRight = boundingBox->m_value._bottomRight;
	RowIndicatorColumn->m_value._boundingBox.m_value._minX = boundingBox->m_value._minX;
	RowIndicatorColumn->m_value._boundingBox.m_value._maxX = boundingBox->m_value._maxX;
	RowIndicatorColumn->m_value._boundingBox.m_value._minY = boundingBox->m_value._minY;
	RowIndicatorColumn->m_value._boundingBox.m_value._maxY = boundingBox->m_value._maxY;
	RowIndicatorColumn->m_value._rowIndicator = rowInd;
	RowIndicatorColumn->m_hasValue = 1;
	
	if (boundingBox->m_value._maxY < boundingBox->m_value._minY) 
	{
		printf("Invalid bounding box");
	}
}

Codeword_t * allCodewords(DetectionResultColumn_t * RowIndicatorColumn) 
{
	return RowIndicatorColumn->m_value._codewords;
}
uint8_t isRowIndicator(DetectionResultColumn_t * RowIndicatorColumn)
{
	return RowIndicatorColumn->m_value._rowIndicator != None;
}
uint8_t isLeftRowIndicator(DetectionResultColumn_t * RowIndicatorColumn) 
{
	return RowIndicatorColumn->m_value._rowIndicator == Left;
}

Codeword_t codewordNearby(DetectionResultColumn_t * RowIndicatorColumn,int imageRow,int size) 
{
	int index = imageRow - RowIndicatorColumn->m_value._boundingBox.m_value._minY;
	if (RowIndicatorColumn->m_value._codewords[index].m_hasValue != 0) 
	{
		return RowIndicatorColumn->m_value._codewords[index];
	}

	for (int i = 1; i < MAX_NEARBY_DISTANCE; i++) 
	{
		int nearImageRow = imageRow - RowIndicatorColumn->m_value._boundingBox.m_value._minY - i;
		if (nearImageRow >= 0) 
		{
			if (RowIndicatorColumn->m_value._codewords[nearImageRow].m_hasValue != 0) 
			{
				return RowIndicatorColumn->m_value._codewords[nearImageRow];
			}
		}
		nearImageRow = imageRow - RowIndicatorColumn->m_value._boundingBox.m_value._minY + i;
		if (nearImageRow < size) 
		{
			if (RowIndicatorColumn->m_value._codewords[nearImageRow].m_hasValue != 0) 
			{
				return RowIndicatorColumn->m_value._codewords[nearImageRow];
			}
		}
	}

	return RowIndicatorColumn->m_value._codewords[index];
}

void setRowNumbers(Codeword_t * codewords,int size)
{
	for (int i = 0;i<size;i++) 
	{
		if (codewords->m_hasValue != 0) 
		{
			setRowNumberAsRowIndicatorColumn(codewords+i);
		}
	}
}

void clear_Codewords(Codeword_t * codewords)
{
	codewords->m_hasValue = 0;
	codewords->m_value._startX = 0;
	codewords->m_value._endX = 0;
	codewords->m_value._bucket = 0;
	codewords->m_value._value = 0;
	codewords->m_value._rowNumber = -1;
}
static void RemoveIncorrectCodewords(uint8_t isLeft, Codeword_t * codewords, const BarcodeMetadata_t * barcodeMetadata,int length)
{

	printf("RemoveIncorrectCodewords: \n");

	// Remove codewords which do not match the metadata
	// TODO Maybe we should keep the incorrect codewords for the start and end positions?
	for (int i=0;i<length;i++) 
	{
		if (codewords[i].m_hasValue == 0) 
		{
			continue;
		}

		int rowIndicatorValue = codewords[i].m_value._value % 30;
		int codewordRowNumber = codewords[i].m_value._rowNumber;
		if (codewordRowNumber > rowCount(barcodeMetadata)) 
		{
			clear_Codewords(codewords+i);
			continue;
		}

		if (!isLeft) 
		{
			codewordRowNumber += 2;
		}

		switch (codewordRowNumber % 3) 
		{
			case 0:
			if (rowIndicatorValue * 3 + 1 != barcodeMetadata->_rowCountUpperPart) 
			{
				clear_Codewords(codewords+i);
			}
			break;
			case 1:
			if (rowIndicatorValue / 3 != barcodeMetadata->_errorCorrectionLevel ||
				rowIndicatorValue % 3 != barcodeMetadata->_rowCountLowerPart) 
			{
				clear_Codewords(codewords+i);
			}
			break;
			case 2:
			if (rowIndicatorValue + 1 != barcodeMetadata->_columnCount) 
			{
				clear_Codewords(codewords+i);
			}
			break;
		}
	}

}

// TODO implement properly
// TODO maybe we should add missing codewords to store the correct row number to make
// finding row numbers for other columns easier
// use row height count to make detection of invalid row numbers more reliable
void adjustCompleteIndicatorColumnRowNumbers(DetectionResultColumn_t * detectionResultColumn,const BarcodeMetadata_t * barcodeMetadata,int size)
{
	printf("DetectionResultColumn::adjustCompleteIndicatorColumnRowNumbers: \n");
	
	if (!isRowIndicator(detectionResultColumn))
	{
		return;
	}

	Codeword_t * codewords = allCodewords(detectionResultColumn);
	setRowNumbers(codewords,size);
	RemoveIncorrectCodewords(isLeftRowIndicator(detectionResultColumn), codewords, barcodeMetadata,size);
	const BoundingBox_t bb = detectionResultColumn->m_value._boundingBox;
	ResultPoint_t top = isLeftRowIndicator(detectionResultColumn) ? topLeft(&bb) : topRight(&bb);
	ResultPoint_t bottom = isLeftRowIndicator(detectionResultColumn) ? bottomLeft(&bb) : bottomRight(&bb);
	int firstRow = (int)top.m_value.y - detectionResultColumn->m_value._boundingBox.m_value._minY;
	int lastRow = (int)bottom.m_value.y - detectionResultColumn->m_value._boundingBox.m_value._minY;
	// We need to be careful using the average row height. Barcode could be skewed so that we have smaller and
	// taller rows
	int barcodeRow = -1;
	int maxRowHeight = 1;
	int currentRowHeight = 0;
	int increment = 1;
	for (int codewordsRow = firstRow; codewordsRow < lastRow; codewordsRow++) 
	{
		if (codewords[codewordsRow].m_hasValue == 0) 
		{
			continue;
		}

		Codeword_t codeword = codewords[codewordsRow];
		if (barcodeRow == -1 && codeword.m_value._rowNumber == rowCount(barcodeMetadata) - 1) 
		{
			increment = -1;
			barcodeRow = rowCount(barcodeMetadata);
		}
		//      float expectedRowNumber = (codewordsRow - firstRow) / averageRowHeight;
		//      if (Math.abs(codeword.getRowNumber() - expectedRowNumber) > 2) {
		//        SimpleLog.log(LEVEL.WARNING,
		//            "Removing codeword, rowNumberSkew too high, codeword[" + codewordsRow + "]: Expected Row: " +
		//                expectedRowNumber + ", RealRow: " + codeword.getRowNumber() + ", value: " + codeword.getValue());
		//        codewords[codewordsRow] = null;
		//      }

		int rowDifference = codeword.m_value._rowNumber - barcodeRow;

		if (rowDifference == 0) 
		{
			currentRowHeight++;
		}
		else if (rowDifference == increment) 
		{
			maxRowHeight = __max(maxRowHeight, currentRowHeight);
			currentRowHeight = 1;
			barcodeRow = codeword.m_value._rowNumber;
		}
		else if (rowDifference < 0 || codeword.m_value._rowNumber >= rowCount(barcodeMetadata) ||
			rowDifference > codewordsRow) 
		{
			codewords[codewordsRow].m_hasValue = 0;
		}
		else 
		{
			int checkedRows;
			if (maxRowHeight > 2) 
			{
				checkedRows = (maxRowHeight - 2) * rowDifference;
			}
			else 
			{
				checkedRows = rowDifference;
			}

			uint8_t closePreviousCodewordFound;
			if(checkedRows >= codewordsRow)
			{
				closePreviousCodewordFound = 1;
			}
			else
			{
				closePreviousCodewordFound = 0;
			}
			for (int i = 1; i <= checkedRows && !closePreviousCodewordFound; i++) 
			{
				// there must be (height * rowDifference) number of codewords missing. For now we assume height = 1.
				// This should hopefully get rid of most problems already.
				if(codewords[codewordsRow - i].m_hasValue != 0)
				{
					closePreviousCodewordFound = 1;
				}
				else
				{
					closePreviousCodewordFound = 0;
				}
			}

			if (closePreviousCodewordFound) 
			{
				codewords[codewordsRow].m_hasValue = 0;
			}
			else 
			{
				barcodeRow = codeword.m_value._rowNumber;
				currentRowHeight = 1;
			}
		}
	}

}

// TODO maybe we should add missing codewords to store the correct row number to make
// finding row numbers for other columns easier
// use row height count to make detection of invalid row numbers more reliable
void adjustIncompleteIndicatorColumnRowNumbers(DetectionResultColumn_t * rowIndicatorColumn , const BarcodeMetadata_t * barcodeMetadata)
{

	printf("DetectionResultColumn::adjustIncompleteIndicatorColumnRowNumbers: \n");
	
	if (!isRowIndicator(rowIndicatorColumn))
	{
		return;
	}

	const BoundingBox_t bb = rowIndicatorColumn->m_value._boundingBox;
	ResultPoint_t top = isLeftRowIndicator(rowIndicatorColumn) ? topLeft(&bb) : topRight(&bb);
	ResultPoint_t bottom = isLeftRowIndicator(rowIndicatorColumn) ? bottomLeft(&bb) : bottomRight(&bb);
	int firstRow = (int)top.m_value.y - bb.m_value._minY;
	int lastRow = (int)bottom.m_value.y - bb.m_value._minY;
	Codeword_t * codewords = rowIndicatorColumn->m_value._codewords;
	int barcodeRow = -1;
	int maxRowHeight = 1;
	int currentRowHeight = 0;
	for (int codewordsRow = firstRow; codewordsRow < lastRow; codewordsRow++) 
	{
		if (codewords[codewordsRow].m_hasValue == 0) 
		{
			continue;
		}

		setRowNumberAsRowIndicatorColumn(codewords+codewordsRow);

		int rowDifference = codewords[codewordsRow].m_value._rowNumber - barcodeRow;

		// TODO improve handling with case where first row indicator doesn't start with 0

		if (rowDifference == 0) 
		{
			currentRowHeight++;
		}
		else if (rowDifference == 1) 
		{
			maxRowHeight = __max(maxRowHeight, currentRowHeight);
			currentRowHeight = 1;
			barcodeRow = codewords[codewordsRow].m_value._rowNumber;
		}
		else if (codewords[codewordsRow].m_value._rowNumber >= rowCount(barcodeMetadata)) 
		{
			codewords[codewordsRow].m_hasValue = 0;
		}
		else 
		{
			barcodeRow = codewords[codewordsRow].m_value._rowNumber;
			currentRowHeight = 1;
		}
	}
}

// This is example of bad design, a getter should not modify object's state
uint8_t getRowHeights(DetectionResultColumn_t * rowIndicatorColumn,int * result,int length,int * size)
{
	printf("DetectionResultColumn::getRowHeights : \n");

	BarcodeMetadata_t barcodeMetadata;

	if (!getBarcodeMetadata(rowIndicatorColumn,&barcodeMetadata,length)) 
	{
		return 0;
	}

	adjustIncompleteIndicatorColumnRowNumbers(rowIndicatorColumn,&barcodeMetadata);

	int sizeRow = rowCount(&barcodeMetadata);
	int ret[sizeRow];
	fill(sizeRow,ret,0);
	for (int i=0;i<length;i++) 
	{
		Codeword_t * item = (rowIndicatorColumn->m_value._codewords+i);
		if (item->m_hasValue != 0) 
		{
			uint64_t rowNumber = item->m_value._rowNumber;
			if (rowNumber >= sizeRow) 
			{
				// We have more rows than the barcode metadata allows for, ignore them.
				continue;
			}
			ret[rowNumber]++;
		} // else throw exception?
	}

	for(int i=0;i<sizeRow;i++)
	{
		result[i] = ret[i];
	}

	*size = sizeRow;

	return 1;
}

void setRowNumberAsRowIndicatorColumn(Codeword_t * Codewords) 
{
	Codewords->m_value._rowNumber = (Codewords->m_value._value / 30) * 3 + Codewords->m_value._bucket / 3;
}

// This is example of bad design, a getter should not modify object's state
uint8_t getBarcodeMetadata(DetectionResultColumn_t * RowIndicatorColumn, BarcodeMetadata_t * result,int length)
{
	printf("DetectionResultColumn::getBarcodeMetadata : \n");

	if (!isRowIndicator(RowIndicatorColumn))
	{
		return 0;
	}

	Codeword_t * codewords = allCodewords(RowIndicatorColumn);
	BarcodeValue_t barcodeColumnCount;
	BarcodeValue_t barcodeRowCountUpperPart;
	BarcodeValue_t barcodeRowCountLowerPart;
	BarcodeValue_t barcodeECLevel;
	for (uint16_t i=0;i<length;i++)
	{
		barcodeColumnCount._values[i] = 0;
		barcodeRowCountUpperPart._values[i] = 0;
		barcodeRowCountLowerPart._values[i] = 0;
		barcodeECLevel._values[i] = 0;
	}
	for (uint16_t i=0;i<length;i++) 
	{
		if (codewords[i].m_hasValue == 0) 
		{
			continue;
		}

		setRowNumberAsRowIndicatorColumn(codewords+i);
		int rowIndicatorValue = codewords[i].m_value._value % 30;
		int codewordRowNumber = codewords[i].m_value._rowNumber;
		if (!isLeftRowIndicator(RowIndicatorColumn)) 
		{
			codewordRowNumber += 2;
		}
		switch (codewordRowNumber % 3) 
		{
			case 0:
			BarcodeSetValue(&barcodeRowCountUpperPart,rowIndicatorValue * 3 + 1);
			break;
			case 1:
			BarcodeSetValue(&barcodeECLevel,rowIndicatorValue / 3);
			BarcodeSetValue(&barcodeRowCountLowerPart,rowIndicatorValue % 3);
			break;
			case 2:
			BarcodeSetValue(&barcodeColumnCount,rowIndicatorValue + 1);
			break;
		}
	}
	// Maybe we should check if we have ambiguous values?
	int cc = BarcodeValue(&barcodeColumnCount,length);
	int rcu = BarcodeValue(&barcodeRowCountUpperPart,length);
	int rcl = BarcodeValue(&barcodeRowCountLowerPart,length);
	int ec = BarcodeValue(&barcodeECLevel,length);

	if (cc < 1 || rcu + rcl < MIN_ROWS_IN_BARCODE || rcu + rcl > MAX_ROWS_IN_BARCODE) 
	{
		return 0;
	}

	result->_columnCount = cc;
	result->_errorCorrectionLevel = ec;
	result->_rowCountUpperPart = rcu;
	result->_rowCountLowerPart = rcl;
	
	RemoveIncorrectCodewords(isLeftRowIndicator(RowIndicatorColumn), codewords, result,length);

	return 1;
}

