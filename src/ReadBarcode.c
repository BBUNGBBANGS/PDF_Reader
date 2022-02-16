/*
* Copyright 2019 Axel Waggershauser
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

#include "ReadBarcode.h"
#include "MultiFormatReader.h"

ImageView_t ImageView;
DecodeHints_t DecodeHints;
GenericLuminanceSource_t GenericLuminanceSource_;

static void DecodeHints_Init(void);
static int PixStride(unsigned int format);  
static int RedIndex(unsigned int format);   
static int GreenIndex(unsigned int format); 
static int BlueIndex(unsigned int format);  

static result_t ReadBarcode_Internal(GenericLuminanceSource_t *source,const DecodeHints_t *hints)
{
	source->pixels = ImageView.data;
	source->left = 0;
	source->top = 0;
	source->width = ImageView.width;
	source->height = ImageView.height;
	source->rowBytes = ImageView.width;
	printf("static Result ReadBarcode : source= %x",source);
	printf(" hints= %x \n",hints);

	return MultiFormatReader(source,hints);
}

result_t ReadBarcode(unsigned char *iv, int width,int height, unsigned int ImageFormat)
{
	result_t result;
	DecodeHints_t *hints;
	unsigned int rowStride,pixStride;
	uint32_t idxRed,idxGreen,idxBlue;

	DecodeHints_Init();
	hints = &DecodeHints;

    printf("Result ReadBarcode : iv= %x", &iv);
	printf(" hints= %x\n",hints);

	pixStride = PixStride(ImageFormat);
	rowStride = width * pixStride;
	idxRed = RedIndex(ImageFormat);
	idxGreen = GreenIndex(ImageFormat);
	idxBlue = BlueIndex(ImageFormat);

    printf("address : %x , Width : %d , height : %d , format :  %x \n",iv,width,height,ImageFormat);
 
	ImageView.data = GenericLuminanceSource(0, 0, width, height, iv, rowStride, pixStride, idxRed, idxGreen, idxBlue);
	ImageView.width = width; 
	ImageView.height = height;
	ImageView.format = ImageFormat; 
	ImageView.rowStride = rowStride; 
	ImageView.pixStride = pixStride;
	
    return ReadBarcode_Internal(&GenericLuminanceSource_,hints);
}


static void DecodeHints_Init(void)
{
	DecodeHints.tryHarder = 1;
	DecodeHints.tryRotate = 1;
	DecodeHints.isPure = 0;
	DecodeHints.returnCodabarStartEnd = 0;	
	DecodeHints.Binarizer = LocalAverage;
	DecodeHints.formats = NONE;
}

static int PixStride(unsigned int format)  
{ 
	return ((uint32_t)(format) >> 3*8) & 0xFF; 
}
static int RedIndex(unsigned int format)   
{ 
	return ((uint32_t)(format) >> 2*8) & 0xFF; 
}
static int GreenIndex(unsigned int format) 
{ 
	return ((uint32_t)(format) >> 1*8) & 0xFF; 
}
static int BlueIndex(unsigned int format)  
{ 
	return ((uint32_t)(format) >> 0*8) & 0xFF; 
}
