
#include "BitMatrix.h"

unsigned int getPatternRow(BitMatrix_t * m, int r, uint16_t * p_row)
{

	uint8_t * b_row = m->bits + r * m->_width;

	//uint16_t * row = (uint16_t*)malloc(sizeof(uint16_t) * array_length);

	for(int i = 0;i<m->_width + 2;i++)
	{
		p_row[i] = 0;
	}
	#if 0
	std::fill(p_row.begin(), p_row.end(), 0);

	auto* bitPos = b_row.begin();
	auto* intPos = p_row.data();

	intPos += BitMatrix::isSet(*bitPos); // first value is number of white pixels, here 0

	while (++bitPos < b_row.end()) {
		++(*intPos);
		intPos += bitPos[0] != bitPos[-1];
	}
	++(*intPos);

	if (BitMatrix::isSet(bitPos[-1]))
		intPos++;

	p_row.resize(intPos - p_row.data() + 1);

	#endif
	return 0;
}
