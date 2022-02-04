
#include "HybridBinarizer.h"
#include "BitMatrix.h"
#include "Matrix.h"
#include <stdlib.h>

Matrix_t blackPoints;
BitMatrix_t matrix;

static const int BLOCK_SIZE = 8;
static const int MINIMUM_DIMENSION = BLOCK_SIZE * 5;
static const int MIN_DYNAMIC_RANGE = 24;

unsigned int blackpoints_data[1000][1000]; //사이즈 조정 필요
uint8_t matrix_bits[115200];
/**
* Calculates the final BitMatrix once for all requests. This could be called once from the
* constructor instead, but there are some advantages to doing it lazily, such as making
* profiling easier, and not doing heavy lifting when callers don't expect it.
*/
static int * CalculateBlackPoints(const uint8_t* luminances, int subWidth, int subHeight, int width, int height, int stride)
{
	//#ifdef HK_DEBUG
	printf("CalculateBlackPoints : subWidth=%d subHeight=%d width=%d height=%d strides=%d \n",subHeight,subHeight,width,height,stride);
	//#endif
	
	blackPoints.width = subWidth;
	blackPoints.height = subHeight;

	for (int y = 0; y < subHeight; y++) 
	{
		int yoffset = __min(y * BLOCK_SIZE, height - BLOCK_SIZE);
		for (int x = 0; x < subWidth; x++) 
		{
			int xoffset = __min(x * BLOCK_SIZE, width - BLOCK_SIZE);
			int sum = 0;
			uint8_t min = 0xFF;
			uint8_t max = 0;
			for (int yy = 0, offset = yoffset * stride + xoffset; yy < BLOCK_SIZE; yy++, offset += stride) 
			{
				for (int xx = 0; xx < BLOCK_SIZE; xx++) 
				{
					uint8_t pixel = luminances[offset + xx];
					sum += pixel;
					min = min < pixel ? min : pixel;
					max = max > pixel ? max : pixel;
				}
				// short-circuit min/max tests once dynamic range is met
				if (max - min > MIN_DYNAMIC_RANGE) 
				{
					// finish the rest of the rows quickly
					for (yy++, offset += stride; yy < BLOCK_SIZE; yy++, offset += stride) 
					{
						for (int xx = 0; xx < BLOCK_SIZE; xx++) 
						{
							sum += luminances[offset + xx];
						}
					}
				}
			}

			// The default estimate is the average of the values in the block.
			int average = sum / (BLOCK_SIZE * BLOCK_SIZE);
			if (max - min <= MIN_DYNAMIC_RANGE) 
			{
				// If variation within the block is low, assume this is a block with only light or only
				// dark pixels. In that case we do not want to use the average, as it would divide this
				// low contrast area into black and white pixels, essentially creating data out of noise.
				//
				// The default assumption is that the block is light/background. Since no estimate for
				// the level of dark pixels exists locally, use half the min for the block.
				average = min / 2;

				if (y > 0 && x > 0) 
				{
					// Correct the "white background" assumption for blocks that have neighbors by comparing
					// the pixels in this block to the previously calculated black points. This is based on
					// the fact that dark barcode symbology is always surrounded by some amount of light
					// background for which reasonable black point estimates were made. The bp estimated at
					// the boundaries is used for the interior.

					// The (min < bp) is arbitrary but works better than other heuristics that were tried.
					int averageNeighborBlackPoint =
						(blackpoints_data[x][y - 1] + (2 * blackpoints_data[x - 1][y]) + blackpoints_data[x - 1][y - 1]) / 4;

					if (min < averageNeighborBlackPoint) 
					{
						average = averageNeighborBlackPoint;
					}
				}
			}

			blackpoints_data[x][y] = average;


			printf("X=%d, Y=%d, AVG=%d\n", x, y, average);

		}
	}

	return blackpoints_data;
}

static inline clamp(const int __val, const int __lo, const int __hi)
{
     // __glibcxx_assert(!(__hi < __lo));
      return (__val < __lo) ? __lo : (__hi < __val) ? __hi : __val;
}

/**
* Applies a single threshold to a block of pixels.
*/
static void ThresholdBlock(const uint8_t * luminances, int xoffset, int yoffset, int threshold, int stride, BitMatrix_t * matrix)
{

	printf("static void ThresholdBlock : ");

	for (int y = yoffset; y < yoffset + BLOCK_SIZE; ++y) 
	{
		uint8_t * src = luminances + y * stride + xoffset;
		uint8_t * const dstBegin = matrix->bits + y * matrix->_width + xoffset;
		for (uint8_t * dst = dstBegin; dst < dstBegin + BLOCK_SIZE; ++dst, ++src)
			*dst = *src <= threshold;
	}

	printf("threshold : %d \n",threshold);
}

/**
* For each block in the image, calculate the average black point using a 5x5 grid
* of the blocks around it. Also handles the corner cases (fractional blocks are computed based
* on the last pixels in the row/column which are also used in the previous block).
*/
static void CalculateThresholdForBlock(const uint8_t* luminances, int subWidth, int subHeight, int width, int height,
                                       int stride, BitMatrix_t * matrix)
{
	printf("static void CalculateThresholdForBlock : Calls ThresholdBlock \n");

	for (int y = 0; y < subHeight; y++) 
	{
		int yoffset = __min(y * BLOCK_SIZE, height - BLOCK_SIZE);
		for (int x = 0; x < subWidth; x++) 
		{
			int xoffset = __min(x * BLOCK_SIZE, width - BLOCK_SIZE);
			int left = clamp(x, 2, subWidth - 3);
			int top  = clamp(y, 2, subHeight - 3);
			int sum = 0;
			for (int dy = -2; dy <= 2; ++dy) 
			{
				for (int dx = -2; dx <= 2; ++dx) 
				{
					sum += blackpoints_data[left + dx][top + dy];
				}
			}

			int average = sum / 25;
			ThresholdBlock(luminances, xoffset, yoffset, average, stride, matrix);
		}
	}

#ifdef HK_DEBUG
	//std::cout << "width : " << width << "height : " << height << "\n";
#endif

}

static void InitBlackMatrix(GenericLuminanceSource_t * source, BitMatrix_t * outMatrix)
{
	BitMatrix_t * matrix = outMatrix;
	int width = source->width;
	int height = source->height;
	uint8_t * buffer = NULL;
	int stride = 0;

	uint8_t* luminances = getMatrix(source, buffer, &stride,0);
	int subWidth = (width + BLOCK_SIZE - 1) / BLOCK_SIZE; // ceil(width/BS)
	int subHeight = (height + BLOCK_SIZE - 1) / BLOCK_SIZE; // ceil(height/BS)
	blackPoints.data = CalculateBlackPoints(luminances, subWidth, subHeight, width, height, stride);

	//matrix = std::make_shared<BitMatrix>(width, height);
	CalculateThresholdForBlock(luminances, subWidth, subHeight, width, height, stride, matrix);
}

unsigned char * getBlackMatrix(GenericLuminanceSource_t * image)
{
	
	int width = image->width;
	int height = image->height;
	matrix._width = width;
	matrix._height = height;
	matrix._rowSize = width;
	matrix.bits = matrix_bits;
	if (width >= MINIMUM_DIMENSION && height >= MINIMUM_DIMENSION) 
	{
		InitBlackMatrix(image,&matrix);
		return &matrix;
	}
	else 
	{
		// If the image is too small, fall back to the global histogram approach.
		return getBlackMatrix(image);
	}

}
