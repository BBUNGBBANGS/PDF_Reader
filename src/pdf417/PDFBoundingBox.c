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
	result->_imgWidth = 0;
	result->_imgHeight = 0;
	result->_minX = 0;
	result->_maxX = 0;
	result->_minY = 0;
	result->_maxY = 0;
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

	result->_imgWidth = imgWidth;
	result->_imgHeight = imgHeight;
	result->_topLeft = topLeft;
	result->_bottomLeft = bottomLeft;
	result->_topRight = topRight;
	result->_bottomRight = bottomRight;
	calculateMinMaxValues(result);

	return 1;//true

}

void calculateMinMaxValues(BoundingBox_t * result)
{
	if (result->_topLeft.m_hasValue == 0) 
	{
		result->_topLeft.m_value.x = 0.f;
		result->_topLeft.m_value.x = result->_topRight.m_value.y;
		result->_bottomLeft.m_value.x = 0.f;
		result->_bottomLeft.m_value.x = result->_bottomRight.m_value.y;
	}
	if (result->_topRight.m_hasValue == 0) 
	{
		result->_topRight.m_value.x = (float)(result->_imgWidth - 1);
		result->_topRight.m_value.y = result->_topLeft.m_value.y;
		result->_bottomRight.m_value.x = (float)(result->_imgHeight - 1); 
		result->_bottomRight.m_value.y = result->_bottomLeft.m_value.y;
	}

	result->_minX = (int)(__min(result->_topLeft.m_value.x, result->_bottomLeft.m_value.x));
	result->_maxX = (int)(__max(result->_topRight.m_value.x, result->_bottomRight.m_value.x));
	result->_minY = (int)(__min(result->_topLeft.m_value.y, result->_topRight.m_value.y));
	result->_maxY = (int)(__max(result->_bottomLeft.m_value.y, result->_bottomRight.m_value.y));
}

uint8_t Merge(const BoundingBox_t leftBox, const BoundingBox_t rightBox, BoundingBox_t result)
{
	#if 0
	if (leftBox.m_hasValue == 0) 
	{
		result = rightBox;
		return true;
	}

	if (rightBox.m_hasValue == 0) 
	{
		result = leftBox;
		return true;
	}

	BoundingBox box;
	if (Create(leftBox.value()._imgWidth, leftBox.value()._imgHeight, leftBox.value()._topLeft, leftBox.value()._bottomLeft, rightBox.value()._topRight, rightBox.value()._bottomRight, box)) {
		result = box;
		return true;
	}

	return false;
	#endif
}

uint8_t AddMissingRows(const BoundingBox_t box, int missingStartRows, int missingEndRows, uint8_t isLeft, BoundingBox_t result)
{
	#if 0
	auto newTopLeft = box._topLeft;
	auto newBottomLeft = box._bottomLeft;
	auto newTopRight = box._topRight;
	auto newBottomRight = box._bottomRight;

	if (missingStartRows > 0) 
	{
		auto top = isLeft ? box._topLeft : box._topRight;
		int newMinY = static_cast<int>(top.value().y()) - missingStartRows;
		if (newMinY < 0) 
		{
			newMinY = 0;
		}

		ResultPoint newTop(top.value().x(), static_cast<float>(newMinY));

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
		auto bottom = isLeft ? box._bottomLeft : box._bottomRight;
		int newMaxY = (int)bottom.value().y() + missingEndRows;
		if (newMaxY >= box._imgHeight) 
		{
			newMaxY = box._imgHeight - 1;
		}

		ResultPoint_t newBottom(bottom.value().x(), static_cast<float>(newMaxY));
		
		if (isLeft) 
		{
			newBottomLeft = newBottom;
		}
		else 
		{
			newBottomRight = newBottom;
		}
	}

	return Create(box._imgWidth, box._imgHeight, newTopLeft, newBottomLeft, newTopRight, newBottomRight, result);
	#endif
}

