#ifndef _RESULTPOINT_H
#define _RESULTPOINT_H
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
#include <stdint.h>
typedef struct 
{
	float x;
	float y;
}PointT_t;
typedef struct 
{
	int x;
	int y;
}PointI_t;
typedef struct 
{
	double x;
	double y;
}PointF_t;
typedef struct 
{
	uint8_t m_hasValue;
	PointT_t m_value;
}ResultPoint_t;


extern float Distance(int aX, int aY, int bX, int bY);

#endif
