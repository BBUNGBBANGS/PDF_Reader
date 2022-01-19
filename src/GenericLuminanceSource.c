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

static uint8_t RGBToGray(unsigned r, unsigned g, unsigned b)
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


static unsigned int * MakeCopy(const unsigned int * pixels, int rowBytes, int left, int top, int width, int height)
{
	#if 0
	if (top == 0 && left == 0 && width * height == Size(pixels)) {
		return std::make_shared<ByteArray>(pixels);
	}
	return MakeCopy(pixels.data(), rowBytes, left, top, width, height);
	#endif
	return 0;
}

void GenericLuminanceSource(int left, int top, int width, int height, const void* bytes, int rowBytes, int pixelBytes, int redIndex, int greenIndex, int blueIndex)
{
	if ((left < 0) || (top < 0) || (width < 0) || (height < 0)) 
	{
		printf("Requested offset is outside the image");
	}
	#if 0
	if (pixelBytes == 1)
		_pixels = MakeCopy(bytes, rowBytes, left, top, width, height);
	else {
		auto pixels = std::make_shared<ByteArray>(width * height);
		const uint8_t *rgbSource = static_cast<const uint8_t*>(bytes) + top * rowBytes;
		uint8_t *destRow = pixels->data();
		for (int y = 0; y < height; ++y, rgbSource += rowBytes, destRow += width) {
			const uint8_t *src = rgbSource + left * pixelBytes;
			for (int x = 0; x < width; ++x, src += pixelBytes) {
				destRow[x] = RGBToGray(src[redIndex], src[greenIndex], src[blueIndex]);
			}
		}
		_pixels = std::move(pixels);
	}
	#endif
}
