
#include "PDFBarcodeValue.h"

/**
* Add an occurrence of a value
*/
void BarcodeSetValue(BarcodeValue_t * BarcodeValue , int value)
{
	BarcodeValue->_values[value] += 1;
}

/**
* Determines the maximum occurrence of a set value and returns all values which were set with this occurrence.
* @return an array of int, containing the values with the highest occurrence, or null, if no value was set
*/
static int max_element(int length,BarcodeValue_t * BarcodeValue,int * idx)
{
	int ret_max = 0;
	for(int i=0;i<length;i++)
	{
		if(BarcodeValue->_values[i]>ret_max)
		{
			ret_max = BarcodeValue->_values[i];
			*idx = i;
		}
		
	}
	return ret_max; 
}
int BarcodeValue(BarcodeValue_t * BarcodeValue,int length) 
{
	int result = 0;

	//if (!result->_values.empty()) 
	//{
		int maxConfidence = max_element(length,BarcodeValue->_values,&result);
	//}

	return result;
}

int confidence(int value) 
{
	#if 0
	auto it = _values.find(value);
	return it != _values.end() ? it->second : 0;
	#endif
}

