/*
* Copyright 2016 Nu-book Inc.
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

#include <stdio.h>
#include <time.h>

#include "thirdparty/stb_image.h"
#include "src/ReadBarcode.h"

char filepath[200];
#define FILE_TEST 		(1)

int main()
{
	double startClock;
    int width, height, channels;
	unsigned char *buffer = NULL;
    const unsigned int *result;
    // need to Test 7 images
    #if   (FILE_TEST == 1)
    sprintf(filepath,"./images/01.png");
    #elif (FILE_TEST == 2)
    sprintf(filepath,"./images/07.png");
    #elif (FILE_TEST == 3)
    sprintf(filepath,"./images/10.png");
    #elif (FILE_TEST == 4)
    sprintf(filepath,"./images/14.png");
    #elif (FILE_TEST == 5)
    sprintf(filepath,"./images/20.bmp");
    #elif (FILE_TEST == 6)
    sprintf(filepath,"./images/21.jpg"); // Sample of No Decode
    #elif (FILE_TEST == 7)
    sprintf(filepath,"./images/22.jpg"); // Ecc Zero - Error with Trans
    #endif

    buffer = stbi_load(filepath,&width, &height, &channels, 4);

    printf("%d,%d,%d",width,height,channels);

	if (buffer == NULL)
	{
		printf("Failed to read image: %s\n", filepath);
		return -1;
	}

	printf("======TIME to decode=====\n");
	startClock = clock();

    result = ReadBarcode(buffer,width,height,RGBX);
    printf("address : %x \n",result);

	printf("Total Time = %3.2f msec.\r\n", clock() - startClock);
	printf("=========================\n\n");
    //while(1)
    //{
    printf("%s",filepath);
        //system("pause");
    //}
    return 0;
}

