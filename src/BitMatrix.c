
#include "BitMatrix.h"

uint16_t Row_array[1000];

static uint8_t isSet(uint8_t v) 
{ 
	return v != 0; // see SET_V above
} 

void getPatternRow(BitMatrix_t * m, int r, uint16_t * p_row)
{

	uint8_t * b_row = m->bits + r * m->_width;
	uint8_t * bitPos = NULL;
	uint16_t * intPos = NULL;

	for(int i = 0;i<m->_width + 2;i++)
	{
		Row_array[i] = 0;
	}

	bitPos = b_row;
	intPos = Row_array;

	intPos += isSet(*bitPos); // first value is number of white pixels, here 0

	while (++bitPos < (m->bits + (r + 1) * m->_width)) 
	{
		++(*intPos);
		intPos += bitPos[0] != bitPos[-1];
	}
	++(*intPos);

	if (isSet(bitPos[-1]))
	{
		intPos++;
	}

	p_row = Row_array;
	//p_row.resize(intPos - p_row.data() + 1);

	//return 0;
}
