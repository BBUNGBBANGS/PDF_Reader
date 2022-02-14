#ifndef _PDFBARCODEVALUE_H
#define _PDFBARCODEVALUE_H

typedef struct 
{
	int _values[1000];
}BarcodeValue_t;

extern void BarcodeSetValue(BarcodeValue_t * BarcodeValue , int value);
extern int BarcodeValue(BarcodeValue_t * BarcodeValue,int length) ;
#endif
