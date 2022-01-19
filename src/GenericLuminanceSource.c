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
#include <stdio.h>

static inline copy_n(uint8_t * _First, unsigned int _Count_raw, uint8_t* _Dest) ;

static uint8_t RGBToGray(unsigned int r, unsigned int g, unsigned int b)
{
	// This optimization is not necessary as the computation below is cheap enough.
	//if (r == g && g == b) {
	//	// Image is already greyscale, so pick any channel.
	//	return static_cast<uint8_t>(r);
	//}

	// .299R + 0.587G + 0.114B (YUV/YIQ for PAL and NTSC),
	// (306*R) >> 10 is approximately equal to R*0.299, and so on.
	// 0x200 >> 10 is 0.5, it implements rounding.
	return (uint8_t)((306 * r + 601 * g + 117 * b + 0x200) >> 10);
}


static unsigned char * MakeCopy(unsigned char * pixels, int rowBytes, int left, int top, int width, int height)
{
	unsigned char * result;
	if ((top == 0) && (left == 0) && (width * height == sizeof(pixels))) 
	{
		return pixels;
	}
	else
	{
		result = pixels;
		const uint8_t* srcRow = (uint8_t)(pixels) + top * rowBytes + left;
		uint8_t* destRow = result;
		for (int y = 0; y < height; ++y, srcRow += rowBytes, destRow += width) 
		{
			copy_n(srcRow, width, destRow);
		}
		return result;
	}
}

unsigned char * GenericLuminanceSource(int left, int top, int width, int height, unsigned char * bytes, int rowBytes, int pixelBytes, int redIndex, int greenIndex, int blueIndex)
{
	unsigned char *ret = NULL;
	if ((left < 0) || (top < 0) || (width < 0) || (height < 0)) 
	{
		printf("Requested offset is outside the image");
	}

	if (pixelBytes == 1)
	{
		ret = MakeCopy(bytes, rowBytes, left, top, width, height);
	}
	else 
	{
		unsigned char pixels[width * height];
		const uint8_t *rgbSource = (bytes) + top * rowBytes;
		uint8_t *destRow = pixels;//pixels->data; // pixel의 data 값을 가져와야함 포인터로.
		for (int y = 0; y < height; ++y, rgbSource += rowBytes, destRow += width) 
		{
			const uint8_t *src = rgbSource + (left * pixelBytes);
			for (int x = 0; x < width; ++x, src += pixelBytes)
			{
				destRow[x] = RGBToGray(src[redIndex], src[greenIndex], src[blueIndex]);
			}
		}
		ret = pixels;
	}
	return ret;
}

static inline copy_n(uint8_t * _First, unsigned int _Count_raw, uint8_t* _Dest) 
{
    if (0 < _Count_raw) 
	{
        for (;;) 
		{
            *_Dest = *_First;
            ++_Dest;
            --_Count_raw;
            if (_Count_raw == 0) 
			{ // note that we avoid an extra ++_First here to allow istream_iterator to work,
                               // see LWG-2471
                break;
            }

            ++_First;
        }
    }

    return _Dest;
}