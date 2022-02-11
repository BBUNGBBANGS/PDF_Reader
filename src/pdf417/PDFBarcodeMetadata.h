#ifndef _PDFBARCODEMETADATA_H
#define _PDFBARCODEMETADATA_H

/**
* @author Guenther Grau
*/
typedef struct 
{
	int _columnCount;
	int _errorCorrectionLevel;
	int _rowCountUpperPart;
	int _rowCountLowerPart;
}BarcodeMetadata_t;

#endif
