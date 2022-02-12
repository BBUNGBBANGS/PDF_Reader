#ifndef _PDFCODEWORD_H
#define _PDFCODEWORD_H

typedef struct 
{	
	int _startX;
	int _endX;
	int _bucket;
	int _value;
	int _rowNumber;
}Codeword_val_t;
typedef struct
{
	uint8_t m_hasValue;
	Codeword_val_t m_value;
}Codeword_t;

#endif