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

#include "PDFBoundingBox.h"
#include <stdint.h>
#include <stdlib.h>

static void BoundingBox_Init(BoundingBox_t * result) 
{
	result->m_value._imgWidth = 0;
	result->m_value._imgHeight = 0;
	result->m_value._minX = 0;
	result->m_value._maxX = 0;
	result->m_value._minY = 0;
	result->m_value._maxY = 0;
}

ResultPoint_t topLeft(BoundingBox_t * box)
{
	return box->m_value._topLeft;
} 

ResultPoint_t topRight(BoundingBox_t * box)
{
	return box->m_value._topRight;
} 

ResultPoint_t bottomLeft(BoundingBox_t * box)
{
	return box->m_value._bottomLeft;
} 

ResultPoint_t bottomRight(BoundingBox_t * box)
{
	return box->m_value._bottomRight;
} 

uint8_t Create(int imgWidth, int imgHeight, const ResultPoint_t topLeft, const ResultPoint_t bottomLeft, const ResultPoint_t topRight, const ResultPoint_t bottomRight, BoundingBox_t * result)
{
	BoundingBox_Init(result);

	if ((topLeft.m_hasValue == 0 && topRight.m_hasValue == 0) ||
		(bottomLeft.m_hasValue == 0 && bottomRight.m_hasValue == 0) ||
		(topLeft.m_hasValue != 0 && bottomLeft.m_hasValue == 0) ||
		(topRight.m_hasValue != 0 && bottomRight.m_hasValue == 0)) 
	{
		return 0;
	}

	result->m_value._imgWidth = imgWidth;
	result->m_value._imgHeight = imgHeight;
	result->m_value._topLeft = topLeft;
	result->m_value._bottomLeft = bottomLeft;
	result->m_value._topRight = topRight;
	result->m_value._bottomRight = bottomRight;
	result->m_hasValue = 1;
	calculateMinMaxValues(result);

	return 1;//true

}

void copy(BoundingBox_t * box,BoundingBox_t * result)
{
	result->m_hasValue = box->m_hasValue;
	result->m_value._imgWidth = box->m_value._imgWidth;
	result->m_value._imgHeight = box->m_value._imgHeight;
	result->m_value._topLeft = box->m_value._topLeft;
	result->m_value._bottomLeft = box->m_value._bottomLeft;
	result->m_value._topRight = box->m_value._topRight;
	result->m_value._bottomRight = box->m_value._bottomRight;
	result->m_value._minX = box->m_value._minX;
	result->m_value._maxX = box->m_value._maxX;
	result->m_value._minY = box->m_value._minY;
	result->m_value._maxY = box->m_value._maxY;
}

void calculateMinMaxValues(BoundingBox_t * result)
{
	if (result->m_value._topLeft.m_hasValue == 0) 
	{
		result->m_value._topLeft.m_value.x = 0.f;
		result->m_value._topLeft.m_value.x = result->m_value._topRight.m_value.y;
		result->m_value._bottomLeft.m_value.x = 0.f;
		result->m_value._bottomLeft.m_value.x = result->m_value._bottomRight.m_value.y;
	}
	if (result->m_value._topRight.m_hasValue == 0) 
	{
		result->m_value._topRight.m_value.x = (float)(result->m_value._imgWidth - 1);
		result->m_value._topRight.m_value.y = result->m_value._topLeft.m_value.y;
		result->m_value._bottomRight.m_value.x = (float)(result->m_value._imgHeight - 1); 
		result->m_value._bottomRight.m_value.y = result->m_value._bottomLeft.m_value.y;
	}

	result->m_value._minX = (int)(__min(result->m_value._topLeft.m_value.x, result->m_value._bottomLeft.m_value.x));
	result->m_value._maxX = (int)(__max(result->m_value._topRight.m_value.x, result->m_value._bottomRight.m_value.x));
	result->m_value._minY = (int)(__min(result->m_value._topLeft.m_value.y, result->m_value._topRight.m_value.y));
	result->m_value._maxY = (int)(__max(result->m_value._bottomLeft.m_value.y, result->m_value._bottomRight.m_value.y));
}

uint8_t Bounding_Merge(const BoundingBox_t * leftBox, const BoundingBox_t * rightBox, BoundingBox_t * result)
{

	if (leftBox->m_hasValue == 0) 
	{
		result = rightBox;
		return 1;
	}

	if (rightBox->m_hasValue == 0) 
	{
		result = leftBox;
		return 1;
	}

	BoundingBox_t box;
	if (Create(leftBox->m_value._imgWidth, leftBox->m_value._imgHeight, leftBox->m_value._topLeft, leftBox->m_value._bottomLeft, rightBox->m_value._topRight, rightBox->m_value._bottomRight, &box)) 
	{
		copy(&box,result);
		return 1;
	}

	return 0;

}

uint8_t AddMissingRows(const BoundingBox_t * box, int missingStartRows, int missingEndRows, uint8_t isLeft, BoundingBox_t * result)
{

	ResultPoint_t newTopLeft = box->m_value._topLeft;
	ResultPoint_t newBottomLeft = box->m_value._bottomLeft;
	ResultPoint_t newTopRight = box->m_value._topRight;
	ResultPoint_t newBottomRight = box->m_value._bottomRight;

	if (missingStartRows > 0) 
	{
		ResultPoint_t top = isLeft ? box->m_value._topLeft : box->m_value._topRight;
		int newMinY = (int)(top.m_value.y) - missingStartRows;
		if (newMinY < 0) 
		{
			newMinY = 0;
		}

		ResultPoint_t newTop;
		newTop.m_hasValue = 1;
		newTop.m_value.x = top.m_value.x;
		newTop.m_value.y = (float)newMinY;

		if (isLeft) 
		{
			newTopLeft = newTop;
		}
		else 
		{
			newTopRight = newTop;
		}
	}

	if (missingEndRows > 0) 
	{
		ResultPoint_t bottom = isLeft ? box->m_value._bottomLeft : box->m_value._bottomRight;
		int newMaxY = (int)bottom.m_value.y + missingEndRows;
		if (newMaxY >= box->m_value._imgHeight) 
		{
			newMaxY = box->m_value._imgHeight - 1;
		}

		ResultPoint_t newBottom;
		newBottom.m_hasValue = 1;
		newBottom.m_value.x = bottom.m_value.x;
		newBottom.m_value.y = (float)newMaxY;
		
		if (isLeft) 
		{
			newBottomLeft = newBottom;
		}
		else 
		{
			newBottomRight = newBottom;
		}
	}

	return Create(box->m_value._imgWidth, box->m_value._imgHeight, newTopLeft, newBottomLeft, newTopRight, newBottomRight, result);

}

