
#include "PDFScanningDecoder.h"
#include "PDFBoundingBox.h"
#include "PDFDetectionResultColumn.h"
#include "PDFDetectionResult.h"
#include "PDFCodewordDecoder.h"
#include "PDFBarcodeValue.h"
#include "PDFDecodedBitStreamParser.h"
#include "DecodeStatus.h"
#include "DecoderResult.h"
#include "library.h"
#include <stdlib.h>

static const int CODEWORD_SKEW_SIZE = 2;
static const int MAX_ERRORS = 3;
static const int MAX_EC_CODEWORDS = 512;
const int BARCODE_ROW_UNKNOWN = -1;

int result_bits[8];

Codeword_t CodeWord;
DetectionResultColumn_t rowIndicatorColumn;

static int AdjustCodewordStartColumn(const BitMatrix_t * image, int minColumn, int maxColumn, uint8_t leftToRight, int codewordStartColumn, int imageRow)
{

	printf("GetModuleBitCount minColumn=%d, maxColumn=%d, leftToRight=%d, codewordStartColumn=%d, imageRow=%d\n",
		minColumn, maxColumn, leftToRight, codewordStartColumn, imageRow);
	
	int correctedStartColumn = codewordStartColumn;
	int increment = leftToRight ? -1 : 1;
	// there should be no black pixels before the start column. If there are, then we need to start earlier.
	for (int i = 0; i < 2; i++) 
	{
		while ((leftToRight ? correctedStartColumn >= minColumn : correctedStartColumn < maxColumn) &&
			leftToRight == image->bits[imageRow * image->_width + correctedStartColumn]) 
			{
			if (abs(codewordStartColumn - correctedStartColumn) > CODEWORD_SKEW_SIZE) 
			{
				return codewordStartColumn;
			}
			correctedStartColumn += increment;
		}
		increment = -increment;
		leftToRight = !leftToRight;
	}
	return correctedStartColumn;
}

static uint8_t GetModuleBitCount(const BitMatrix_t * image, int minColumn, int maxColumn, uint8_t leftToRight, int startColumn, int imageRow, int * moduleBitCount, uint8_t length)
{

	printf("GetModuleBitCount minColumn=%d, maxColumn=%d, leftToRight=%d, startColumn=%d, imageRow=%d\n", 
		minColumn, maxColumn, leftToRight, startColumn, imageRow );
	
	int imageColumn = startColumn;
	unsigned int moduleNumber = 0;
	int increment = leftToRight ? 1 : -1;
	uint8_t previousPixelValue = leftToRight;
	fill(length,moduleBitCount,0);
	
	while ((leftToRight ? (imageColumn < maxColumn) : (imageColumn >= minColumn)) && moduleNumber < length) 
	{
		if (image->bits[imageRow * image->_width + imageColumn] == previousPixelValue) 
		{
			moduleBitCount[moduleNumber] += 1;
			imageColumn += increment;
		}
		else {
			moduleNumber += 1;
			previousPixelValue = !previousPixelValue;
		}
	}
	return moduleNumber == length || (imageColumn == (leftToRight ? maxColumn : minColumn) && moduleNumber == length - 1);
}

static uint8_t CheckCodewordSkew(int codewordSize, int minCodewordWidth, int maxCodewordWidth)
{
	return minCodewordWidth - CODEWORD_SKEW_SIZE <= codewordSize &&
		codewordSize <= maxCodewordWidth + CODEWORD_SKEW_SIZE;
}

static int * GetBitCountForCodeword(int codeword)
{

	printf("GetBitCountForCodeword codeword=%d\n", codeword);

    fill(BARS_IN_MODULE,result_bits,0);
	int previousValue = 0;
	int i = sizeof(result_bits)/sizeof(result_bits[0]) - 1;
	while (1) 
	{
		if ((codeword & 0x1) != previousValue) 
		{
			previousValue = codeword & 0x1;
			i--;
			if (i < 0) 
			{
				break;
			}
		}
		result_bits[i]++;
		codeword >>= 1;
	}
	return result_bits;
}

static int GetCodewordBucketNumberRet(const int * moduleBitCount)
{
	return (moduleBitCount[0] - moduleBitCount[2] + moduleBitCount[4] - moduleBitCount[6] + 9) % 9;
}


static int GetCodewordBucketNumber(int codeword)
{
	return GetCodewordBucketNumberRet(GetBitCountForCodeword(codeword));
}

static Codeword_t * Codeword(int startX, int endX, int bucket, int value)
{
	CodeWord.m_hasValue = 1;
	CodeWord.m_value._startX = startX;
	CodeWord.m_value._endX = endX;
	CodeWord.m_value._bucket = bucket;
	CodeWord.m_value._value = value;
	CodeWord.m_value._rowNumber = BARCODE_ROW_UNKNOWN;	

	return &CodeWord;
}

static Codeword_t * DetectCodeword(const BitMatrix_t * image, int minColumn, int maxColumn, uint8_t leftToRight, int startColumn, int imageRow, int minCodewordWidth, int maxCodewordWidth)
{
	printf("DetectCodeword minColumn=%d, maxColumn=%d, leftToRight=%d, startColumn=%d, imageRow=%d, minCodewordWidth=%d, maxCodewordWidth=%d\n", 
		minColumn, maxColumn, leftToRight, startColumn, imageRow, minCodewordWidth, maxCodewordWidth);

	startColumn = AdjustCodewordStartColumn(image, minColumn, maxColumn, leftToRight, startColumn, imageRow);
	// we usually know fairly exact now how long a codeword is. We should provide minimum and maximum expected length
	// and try to adjust the read pixels, e.g. remove single pixel errors or try to cut off exceeding pixels.
	// min and maxCodewordWidth should not be used as they are calculated for the whole barcode an can be inaccurate
	// for the current position
	int moduleBitCount[8];
	int length = sizeof(moduleBitCount) / sizeof(moduleBitCount[0]);
	if (!GetModuleBitCount(image, minColumn, maxColumn, leftToRight, startColumn, imageRow, moduleBitCount , length)) 
	{
		return NULL;
	}
	int endColumn;
	int codewordBitCount = Reduce(length,moduleBitCount);
	if (leftToRight) 
	{
		endColumn = startColumn + codewordBitCount;
	}
	else 
	{
		reverse(length,moduleBitCount);
		endColumn = startColumn;
		startColumn = endColumn - codewordBitCount;
	}

	// We could also use the width of surrounding codewords for more accurate results, but this seems
	// sufficient for now
	if (!CheckCodewordSkew(codewordBitCount, minCodewordWidth, maxCodewordWidth)) {
		// We could try to use the startX and endX position of the codeword in the same column in the previous row,
		// create the bit count from it and normalize it to 8. This would help with single pixel errors.
		return NULL;
	}

	int decodedValue = GetDecodedValue(moduleBitCount);
	if (decodedValue != -1) 
	{
		int codeword = GetCodeword(decodedValue);
		if (codeword != -1) 
		{
			return Codeword(startColumn, endColumn, GetCodewordBucketNumber(decodedValue), codeword);
		}
	}

	return NULL;
}
static void setCodeword(int imageRow, Codeword_t * codeword, DetectionResultColumn_t * RowIndicatorColumn,uint8_t empty)
{
	int idx = imageRow - RowIndicatorColumn->m_value._boundingBox.m_value._minY;
	if(empty == 0)
	{
		RowIndicatorColumn->m_value._codewords[idx].m_hasValue = codeword->m_hasValue;
		RowIndicatorColumn->m_value._codewords[idx].m_value._bucket = codeword->m_value._bucket;
		RowIndicatorColumn->m_value._codewords[idx].m_value._endX = codeword->m_value._endX;
		RowIndicatorColumn->m_value._codewords[idx].m_value._rowNumber = codeword->m_value._rowNumber;
		RowIndicatorColumn->m_value._codewords[idx].m_value._startX = codeword->m_value._startX;
		RowIndicatorColumn->m_value._codewords[idx].m_value._value = codeword->m_value._value;
	}
	else
	{
		RowIndicatorColumn->m_value._codewords[idx].m_hasValue = 0;
		RowIndicatorColumn->m_value._codewords[idx].m_value._bucket = 0;
		RowIndicatorColumn->m_value._codewords[idx].m_value._endX = 0;
		RowIndicatorColumn->m_value._codewords[idx].m_value._rowNumber = 0;
		RowIndicatorColumn->m_value._codewords[idx].m_value._startX = 0;
		RowIndicatorColumn->m_value._codewords[idx].m_value._value = 0;
	}
}

static int GetRowIndicatorColumn(const BitMatrix_t * image, const BoundingBox_t * boundingBox, const ResultPoint_t * startPoint, uint8_t leftToRight, int minCodewordWidth, int maxCodewordWidth, DetectionResultColumn_t * RowIndicatorColumn)
{
	int length = 0;
	printf("GetRowIndicatorColumn \n");

	DetectionResultColumn(boundingBox,leftToRight ? Left : Right,RowIndicatorColumn);

	for (int i = 0; i < 2; i++) 
	{
		int increment = i == 0 ? 1 : -1;
		int startColumn = (int)startPoint->m_value.x;
		for (int imageRow = (int)startPoint->m_value.y; imageRow <= boundingBox->m_value._maxY && imageRow >= boundingBox->m_value._minY; imageRow += increment) 
		{
			Codeword_t * codeword = DetectCodeword(image, 0, image->_width, leftToRight, startColumn, imageRow, minCodewordWidth, maxCodewordWidth);

			if (codeword != NULL) 
			{
				setCodeword(imageRow, codeword, RowIndicatorColumn,0);
				if (leftToRight) 
				{
					startColumn = codeword->m_value._startX;
				}
				else 
				{
					startColumn = codeword->m_value._endX;
				}
			}
			else
			{
				setCodeword(imageRow, codeword, RowIndicatorColumn,1);
			}
			length++;
		}
	}
	return length-1;
}
static void Metadata_Init(BarcodeMetadata_t * Metadata)
{
	Metadata->_columnCount = 0;
	Metadata->_errorCorrectionLevel = 0;
	Metadata->_rowCountUpperPart = 0;
	Metadata->_rowCountLowerPart = 0;
}
int columnCount(BarcodeMetadata_t * Metadata)
{
	return (Metadata->_columnCount);
}
int errorCorrectionLevel(BarcodeMetadata_t * Metadata)
{
	return (Metadata->_errorCorrectionLevel);
}
int rowCount(BarcodeMetadata_t * Metadata)
{
	return (Metadata->_rowCountUpperPart + Metadata->_rowCountLowerPart);
}
static uint8_t GetBarcodeMetadata(DetectionResultColumn_t * leftRowIndicatorColumn, DetectionResultColumn_t * rightRowIndicatorColumn, BarcodeMetadata_t * result, int length)
{
	printf("GetBarcodeMetadata \n");

	BarcodeMetadata_t leftBarcodeMetadata;
	Metadata_Init(&leftBarcodeMetadata);
	if ((leftRowIndicatorColumn == NULL) || (!getBarcodeMetadata(leftRowIndicatorColumn,&leftBarcodeMetadata,length))) 
	{
		return (rightRowIndicatorColumn != NULL) && (getBarcodeMetadata(rightRowIndicatorColumn,result,length));
	}

	BarcodeMetadata_t rightBarcodeMetadata;
	Metadata_Init(&rightBarcodeMetadata);
	if ((rightRowIndicatorColumn == NULL) || (!getBarcodeMetadata(rightRowIndicatorColumn,&rightBarcodeMetadata,length))) 
	{
		result->_columnCount = leftBarcodeMetadata._columnCount;
		result->_errorCorrectionLevel = leftBarcodeMetadata._errorCorrectionLevel;
		result->_rowCountUpperPart = leftBarcodeMetadata._rowCountUpperPart;
		result->_rowCountLowerPart = leftBarcodeMetadata._rowCountLowerPart;
		return 1;
	}

	if (columnCount(&leftBarcodeMetadata) != columnCount(&rightBarcodeMetadata) &&
		errorCorrectionLevel(&leftBarcodeMetadata) != errorCorrectionLevel(&rightBarcodeMetadata) &&
		rowCount(&leftBarcodeMetadata) != rowCount(&rightBarcodeMetadata)) 
	{
		return 0;
	}
	result->_columnCount = leftBarcodeMetadata._columnCount;
	result->_errorCorrectionLevel = leftBarcodeMetadata._errorCorrectionLevel;
	result->_rowCountUpperPart = leftBarcodeMetadata._rowCountUpperPart;
	result->_rowCountLowerPart = leftBarcodeMetadata._rowCountLowerPart;
	return 1;
}

#if 0
template <typename Iter>
static auto GetMax(Iter start, Iter end) -> typename std::remove_reference<decltype(*start)>::type
{
	auto it = std::max_element(start, end);
	return it != end ? *it : -1;
}

#endif
static int GetMax(int * buffer, int length)
{
	int ret_max = 0;

	for(int i=0;i<length;i++)
	{
		if(buffer[i]>ret_max)
		{
			ret_max = buffer[i];
		}
	}

	return ret_max;
}
static uint8_t AdjustBoundingBox(DetectionResultColumn_t * rowIndicatorColumn, BoundingBox_t * result,int length)
{
	printf("AdjustBoundingBox \n");
	
	if (rowIndicatorColumn->m_hasValue == 0)
	{
		result->m_hasValue = 0;
		return 1;
	}

	int rowHeights[100];
	int size = 0;
	if (!getRowHeights(rowIndicatorColumn,&rowHeights,length,&size)) 
	{
		result = NULL;
		return 1;
	}

	int maxRowHeight = GetMax(rowHeights,size);
	int missingStartRows = 0;
	for (int i=0;i<size;i++) 
	{
		missingStartRows += maxRowHeight - rowHeights[i];
		if (rowHeights[i] > 0) 
		{
			break;
		}
	}

	Codeword_t * codewords = rowIndicatorColumn->m_value._codewords;
	for (int row = 0; (missingStartRows > 0) && ((codewords+row)->m_hasValue == 0); row++) 
	{
		missingStartRows--;
	}

	int missingEndRows = 0;
	for (int row = size - 1; row >= 0; row--) 
	{
		missingEndRows += maxRowHeight - rowHeights[row];
		if (rowHeights[row] > 0) 
		{
			break;
		}
	}
	for (int row = length - 1; missingEndRows > 0 && ((codewords+row)->m_hasValue == 0); row--) 
	{
		missingEndRows--;
	}
	BoundingBox_t box;
	BoundingBox_t boundingBox = rowIndicatorColumn->m_value._boundingBox;
	if (AddMissingRows(&boundingBox, missingStartRows, missingEndRows, isLeftRowIndicator(rowIndicatorColumn), &box)) 
	{
		copy(&box,result);
		return 1;
	}

	return 0;
}


static uint8_t Merge(DetectionResultColumn_t * leftRowIndicatorColumn, DetectionResultColumn_t * rightRowIndicatorColumn, DetectionResult_t * result,int length)
{
	if (leftRowIndicatorColumn != NULL || rightRowIndicatorColumn != NULL) 
	{
		BarcodeMetadata_t barcodeMetadata;
		Metadata_Init(&barcodeMetadata);
		if (GetBarcodeMetadata(leftRowIndicatorColumn, rightRowIndicatorColumn, &barcodeMetadata,length)) 
		{
			BoundingBox_t leftBox, rightBox, mergedBox;
			if (AdjustBoundingBox(leftRowIndicatorColumn, &leftBox,length) && AdjustBoundingBox(rightRowIndicatorColumn, &rightBox,length) && Bounding_Merge(&leftBox, &rightBox, &mergedBox)) 
			{
				Detection_Result_init(&barcodeMetadata, &mergedBox,result);
				return 1;
			}
		}
	}
	return 0;
}



static uint8_t IsValidBarcodeColumn(const DetectionResult_t * detectionResult, int barcodeColumn)
{
	return barcodeColumn >= 0 && barcodeColumn <= detectionResult->_barcodeMetadata._columnCount + 1;
}


static int GetStartColumn(const DetectionResult_t * detectionResult, int barcodeColumn, int imageRow, uint8_t leftToRight,int size)
{
	printf("GetStartColumn barcodeColumn=%d, imageRow=%d, leftToRight=%d \n", barcodeColumn, imageRow, leftToRight);

	int offset = leftToRight ? 1 : -1;
	Codeword_t codeword;
	if (IsValidBarcodeColumn(detectionResult, barcodeColumn - offset)) 
	{
		codeword = detectionResult->_detectionResultColumns[barcodeColumn - offset].m_value._codewords[imageRow-detectionResult->_boundingBox.m_value._minY];
	}

	if (codeword.m_hasValue != 0) 
	{
		return leftToRight ? codeword.m_value._endX : codeword.m_value._startX;
	}

	codeword = codewordNearby(&(detectionResult->_detectionResultColumns[barcodeColumn]),imageRow,size);
	if (codeword.m_hasValue != 0) 
	{
		return leftToRight ? codeword.m_value._startX : codeword.m_value._endX;
	}

	if (IsValidBarcodeColumn(detectionResult, barcodeColumn - offset)) 
	{
		codeword = codewordNearby(&(detectionResult->_detectionResultColumns[barcodeColumn - offset]),imageRow,size);
	}

	if (codeword.m_hasValue != 0)
	{
		return leftToRight ? codeword.m_value._endX : codeword.m_value._startX;
	}

	int skippedColumns = 0;
	#if 0
	while (IsValidBarcodeColumn(detectionResult, barcodeColumn - offset)) 
	{
		barcodeColumn -= offset;
		for (auto& previousRowCodeword : detectionResult.column(barcodeColumn).value().allCodewords()) 
		{
			if (previousRowCodeword != nullptr) 
			{
				return (leftToRight ? previousRowCodeword.value().endX() : previousRowCodeword.value().startX()) +
					offset *
					skippedColumns *
					(previousRowCodeword.value().endX() - previousRowCodeword.value().startX());
			}
		}
		skippedColumns++;
	}
	#endif
	return leftToRight ? detectionResult->_boundingBox.m_value._minX : detectionResult->_boundingBox.m_value._maxX;
}


static void CreateBarcodeMatrix(BarcodeValue_t * barcodeMatrix,DetectionResult_t * detectionResult,int size,int row,int column)
{
	printf("CreateBarcodeMatrix\n");

	DetectionResultColumn_t * resultColumn = allColumns(detectionResult,column,size);
	
	for (int columnNum = 0;columnNum<column;columnNum++) 
	{
		if ((resultColumn+columnNum)->m_hasValue != 0) 
		{
			for (int j=0;j<size;j++) 
			{
				Codeword_t codeword = (resultColumn+columnNum)->m_value._codewords[j];
				if (codeword.m_hasValue != 0) 
				{
					int rowNumber = codeword.m_value._rowNumber;
					if (rowNumber >= 0) 
					{
						if (rowNumber >= column * row) 
						{
							// We have more rows than the barcode metadata allows for, ignore them.
							continue;
						}
						BarcodeSetValue(barcodeMatrix + rowNumber*column + columnNum,codeword.m_value._value);
					}
				}
			}
		}
	}
}

static int GetNumberOfECCodeWords(int barcodeECLevel)
{
	return 2 << barcodeECLevel;
}


static uint8_t AdjustCodewordCount(const DetectionResult_t * detectionResult, BarcodeValue_t * barcodeMatrix,int size)
{
	printf("AdjustCodewordCount\n");
	#if 0
	int numberOfCodewords = BarcodeValue(barcodeMatrix[0][1],size);
	int calculatedNumberOfCodewords = detectionResult->_barcodeMetadata._columnCount * rowCount(&detectionResult->_barcodeMetadata) - GetNumberOfECCodeWords(detectionResult->_barcodeMetadata._errorCorrectionLevel);
	if (numberOfCodewords==0) 
	{
		if (calculatedNumberOfCodewords < 1 || calculatedNumberOfCodewords > MAX_CODEWORDS_IN_BARCODE) 
		{
			return 0;
		}
		(barcodeMatrix[0][1],calculatedNumberOfCodewords);
	}
	else if (numberOfCodewords[0] != calculatedNumberOfCodewords) 
	{
		// The calculated one is more reliable as it is derived from the row indicator columns
		BarcodeSetValue(barcodeMatrix[0][1],calculatedNumberOfCodewords);
	}
	#endif
	return 1;
}
// +++++++++++++++++++++++++++++++++++ Error Correction

#if 0
static const ModulusGF& GetModulusGF()
{
	static const ModulusGF field(CodewordDecoder::NUMBER_OF_CODEWORDS, 3);
	return field;
}

#endif
#if 0
static bool RunEuclideanAlgorithm(ModulusPoly a, ModulusPoly b, int R, ModulusPoly& sigma, ModulusPoly& omega)
{
#ifdef HK_DEBUG
	printf("RunEuclideanAlgorithm\n");
#endif
	
	const ModulusGF& field = GetModulusGF();

	// Assume a's degree is >= b's
	if (a.degree() < b.degree()) 
	{
		swap(a, b);
	}

	ModulusPoly rLast = a;
	ModulusPoly r = b;
	ModulusPoly tLast = field.zero();
	ModulusPoly t = field.one();

	// Run Euclidean algorithm until r's degree is less than R/2
	while (r.degree() >= R / 2) 
	{
		ModulusPoly rLastLast = rLast;
		ModulusPoly tLastLast = tLast;
		rLast = r;
		tLast = t;

		// Divide rLastLast by rLast, with quotient in q and remainder in r
		if (rLast.isZero()) {
			// Oops, Euclidean algorithm already terminated?
			return false;
		}
		r = rLastLast;
		ModulusPoly q = field.zero();
		int denominatorLeadingTerm = rLast.coefficient(rLast.degree());
		int dltInverse = field.inverse(denominatorLeadingTerm);
		while (r.degree() >= rLast.degree() && !r.isZero()) {
			int degreeDiff = r.degree() - rLast.degree();
			int scale = field.multiply(r.coefficient(r.degree()), dltInverse);
			q = q.add(field.buildMonomial(degreeDiff, scale));
			r = r.subtract(rLast.multiplyByMonomial(degreeDiff, scale));
		}

		t = q.multiply(tLast).subtract(tLastLast).negative();
	}

	int sigmaTildeAtZero = t.coefficient(0);
	if (sigmaTildeAtZero == 0) {
		return false;
	}

	int inverse = field.inverse(sigmaTildeAtZero);
	sigma = t.multiply(inverse);
	omega = r.multiply(inverse);
	return true;
}

#endif
#if 0
static bool FindErrorLocations(const ModulusPoly& errorLocator, std::vector<int>& result)
{
#ifdef HK_DEBUG
	printf("FindErrorLocations \n");
#endif
	
	const ModulusGF& field = GetModulusGF();
	// This is a direct application of Chien's search
	int numErrors = errorLocator.degree();
	result.resize(numErrors);
	int e = 0;
	for (int i = 1; i < field.size() && e < numErrors; i++) 
	{
		if (errorLocator.evaluateAt(i) == 0) {
			result[e] = field.inverse(i);
			e++;
		}
	}
	return e == numErrors;
}

#endif
#if 0
static std::vector<int> FindErrorMagnitudes(const ModulusPoly& errorEvaluator, const ModulusPoly& errorLocator, const std::vector<int>& errorLocations)
{
#ifdef HK_DEBUG
	printf("FindErrorMagnitudes \n");
#endif
	const ModulusGF& field = GetModulusGF();
	int errorLocatorDegree = errorLocator.degree();
	std::vector<int> formalDerivativeCoefficients(errorLocatorDegree);
	for (int i = 1; i <= errorLocatorDegree; i++) {
		formalDerivativeCoefficients[errorLocatorDegree - i] = field.multiply(i, errorLocator.coefficient(i));
	}

	ModulusPoly formalDerivative(field, formalDerivativeCoefficients);
	// This is directly applying Forney's Formula
	std::vector<int> result(errorLocations.size());
	for (size_t i = 0; i < result.size(); i++) {
		int xiInverse = field.inverse(errorLocations[i]);
		int numerator = field.subtract(0, errorEvaluator.evaluateAt(xiInverse));
		int denominator = field.inverse(formalDerivative.evaluateAt(xiInverse));
		result[i] = field.multiply(numerator, denominator);
	}
	return result;
}
#endif
/**
* @param received received codewords
* @param numECCodewords number of those codewords used for EC
* @param erasures location of erasures
* @return number of errors
* @throws ChecksumException if errors cannot be corrected, maybe because of too many errors
*/

uint8_t DecodeErrorCorrection(int * received, int numECCodewords, const int * erasures, int * nbErrors)
{
	#if 0
	printf("DecodeErrorCorrection  numECCodewords=%d, nbErrors=%d\n", numECCodewords, nbErrors);
	
	const ModulusGF& field = GetModulusGF();
	ModulusPoly poly(field, received);
	std::vector<int> S(numECCodewords);
	uint8_t error = 0;
	for (int i = numECCodewords; i > 0; i--) 
	{
		int eval = poly.evaluateAt(field.exp(i));
		S[numECCodewords - i] = eval;
		if (eval != 0) 
		{
			error = 1;
		}
	}

	if (!error) 
	{
		nbErrors = 0;
		return 1;
	}

	ModulusPoly knownErrors = field.one();
	for (int erasure : erasures) 
	{
		int b = field.exp(Size(received) - 1 - erasure);
		// Add (1 - bx) term:
		ModulusPoly term(field, { field.subtract(0, b), 1 });
		knownErrors = knownErrors.multiply(term);
	}

	ModulusPoly syndrome(field, S);
	//syndrome = syndrome.multiply(knownErrors);

	ModulusPoly sigma, omega;
	if (!RunEuclideanAlgorithm(field.buildMonomial(numECCodewords, 1), syndrome, numECCodewords, sigma, omega)) 
	{
		return 0;
	}

	//sigma = sigma.multiply(knownErrors);

	std::vector<int> errorLocations;
	if (!FindErrorLocations(sigma, errorLocations)) 
	{
		return 0;
	}

	std::vector<int> errorMagnitudes = FindErrorMagnitudes(omega, sigma, errorLocations);

	int receivedSize = Size(received);
	for (size_t i = 0; i < errorLocations.size(); i++) 
	{
		int position = receivedSize - 1 - field.log(errorLocations[i]);
		if (position < 0) 
		{
			return 0;
		}
		received[position] = field.subtract(received[position], errorMagnitudes[i]);
	}
	nbErrors = Size(errorLocations);
	#endif
	return 1;
}



// --------------------------------------- Error Correction

/**
* <p>Given data and error-correction codewords received, possibly corrupted by errors, attempts to
* correct the errors in-place.</p>
*
* @param codewords   data and error correction codewords
* @param erasures positions of any known erasures
* @param numECCodewords number of error correction codewords that are available in codewords
* @throws ChecksumException if error correction fails
*/
static uint8_t CorrectErrors(int * codewords, const int * erasures, int numECCodewords, int * errorCount,int erasure_size)
{

	printf("CorrectErrors  numECCodewords=%d, errorCount=%d\n", numECCodewords, errorCount);

	if (erasure_size > numECCodewords / 2 + MAX_ERRORS ||
		numECCodewords < 0 ||
		numECCodewords > MAX_EC_CODEWORDS) 
	{
		// Too many errors or EC Codewords is corrupted
		return 0;
	}
	return DecodeErrorCorrection(codewords, numECCodewords, erasures, errorCount);
}


/**
* Verify that all is OK with the codeword array.
*/
static uint8_t VerifyCodewordCount(int * codewords, int numECCodewords,int codewordsNum)
{
	if (codewordsNum < 4) 
	{
		// Codeword array size should be at least 4 allowing for
		// Count CW, At least one Data CW, Error Correction CW, Error Correction CW
		return 0;
	}
	// The first codeword, the Symbol Length Descriptor, shall always encode the total number of data
	// codewords in the symbol, including the Symbol Length Descriptor itself, data codewords and pad
	// codewords, but excluding the number of error correction codewords.
	int numberOfCodewords = codewords[0];

	if (numberOfCodewords > codewordsNum) 
	{
		return 0;
	}

	if (numberOfCodewords == 0) 
	{
		// Reset to the length of the array - 8 (Allow for at least level 3 Error Correction (8 Error Codewords)
		if (numECCodewords < codewordsNum) 
		{
			codewords[0] = codewordsNum - numECCodewords;
		}
		else 
		{
			return 0;
		}
	}
	return 1;
}
static void result_init(DecoderResult_t * result)
{
	result->status = NoError;
	result->_rawBytes = NULL;
	result->_numBits = 0;
	result->_text = NULL; //wstring
	result->_ecLevel = NULL; //wstring
	result->_errorsCorrected = 0;
	result->_erasures = 0;
	//result->_structuredAppend = 0;
	result->_readerInit = 0; // bool
	result->_extra = NULL; //custom data
}

DecoderResult_t DecodeCodewords(int * codewords, int ecLevel, const int * erasures,
							  const unsigned char * characterSet,int erasure_size,int codewordsNum)
{
	DecoderResult_t result;
	result_init(&result);
	#if 0
	if (codewords.empty()) 
	{
		return FormatError;
	}
	#endif
	
	int numECCodewords = 1 << (ecLevel + 1);
	int correctedErrorsCount = 0;

	if (!CorrectErrors(codewords, erasures, numECCodewords, correctedErrorsCount,erasure_size))
	{
		result.status = ChecksumError;
		return result;
	}


	if (!VerifyCodewordCount(codewords, numECCodewords,codewordsNum))
	{
		result.status = FormatError;
		return result;
	}

	// Decode the codewords
	result = BitStreamDecode(codewords, ecLevel, characterSet);

	if (result.status == NoError) 
	{
		result._errorsCorrected = correctedErrorsCount;
		result._erasures = erasure_size;
	}


	return result;
}


/**
* This method deals with the fact, that the decoding process doesn't always yield a single most likely value. The
* current error correction implementation doesn't deal with erasures very well, so it's better to provide a value
* for these ambiguous codewords instead of treating it as an erasure. The problem is that we don't know which of
* the ambiguous values to choose. We try decode using the first value, and if that fails, we use another of the
* ambiguous values and try to decode again. This usually only happens on very hard to read and decode barcodes,
* so decoding the normal barcodes is not affected by this.
*
* @param erasureArray contains the indexes of erasures
* @param ambiguousIndexes array with the indexes that have more than one most likely value
* @param ambiguousIndexValues two dimensional array that contains the ambiguous values. The first dimension must
* be the same length as the ambiguousIndexes array
*/
static DecoderResult_t * CreateDecoderResultFromAmbiguousValues(int ecLevel, int * codewords,
	const int * erasureArray, const int * ambiguousIndexes,
	const int * ambiguousIndexValues, const char * characterSet,int erasure_size,int codewordsNum)
{	

	//int ambiguousIndexCount[ambiguousIndexes.size()] = 0; size 계산 필요

	int tries = 100;
	while (tries-- > 0) 
	{
		#if 0
		for (size_t i = 0; i < ambiguousIndexCount.size(); i++) 
		{
			codewords[ambiguousIndexes[i]] = ambiguousIndexValues[i][ambiguousIndexCount[i]];
		}
		#endif

		DecoderResult_t result = DecodeCodewords(codewords, ecLevel, erasureArray, characterSet,erasure_size,codewordsNum);
		if (result.status != ChecksumError) 
		{
			return &result;
		}

		#if 0
		if (ambiguousIndexCount.empty()) 
		{
			return ChecksumError;
		}
		#endif
		#if 0
		for (size_t i = 0; i < ambiguousIndexCount.size(); i++) 
		{
			if (ambiguousIndexCount[i] < Size(ambiguousIndexValues[i]) - 1) 
			{
				ambiguousIndexCount[i]++;
				break;
			}
			else 
			{
				ambiguousIndexCount[i] = 0;
				if (i == ambiguousIndexCount.size() - 1) 
				{
					return ChecksumError;
				}
			}
		}
		#endif
	}

	return ChecksumError;
}
static int find_idx(BarcodeValue_t * buffer)
{
	int idx = 0;
	for(int i=0;i<1000;i++)
	{
		if(buffer->_values[i]!=0)
		{
			idx = i;
			break;
		}
	}

	return idx;
}
static DecoderResult_t * CreateDecoderResult(DetectionResult_t * detectionResult, const unsigned char * characterSet,int size)
{
	int rowNum = detectionResult->_barcodeMetadata._rowCountUpperPart + detectionResult->_barcodeMetadata._rowCountLowerPart;
	int columnNum = detectionResult->_barcodeMetadata._columnCount + 2;
	BarcodeValue_t barcodeMatrix[rowNum][columnNum];
	fill(rowNum*columnNum*1000,barcodeMatrix,0);
	CreateBarcodeMatrix(barcodeMatrix[0],detectionResult,size,rowNum,columnNum);
	if (!AdjustCodewordCount(detectionResult, barcodeMatrix,size)) 
	{
		return NotFound;
	}
	
	int erasures[10];
	int codewordsNum = rowCount(&detectionResult->_barcodeMetadata) * columnCount(&detectionResult->_barcodeMetadata);
	int codewords[codewordsNum];
	int ambiguousIndexValues;
	int ambiguousIndexesList;
	fill(10,erasures,0);
	fill(codewordsNum,codewords,0);

	int idx = 0;

	for (int row = 0; row < rowCount(&(detectionResult->_barcodeMetadata)); row++) 
	{
		for (int column = 0; column < columnCount(&(detectionResult->_barcodeMetadata)); column++) 
		{
			int values[1] = {0};
			values[0] = find_idx(&barcodeMatrix[row][column + 1]);
			int codewordIndex = row * columnCount(&(detectionResult->_barcodeMetadata)) + column;
			if (values[0] == 0) 
			{
				erasures[idx] = codewordIndex;
				idx++;
			}
			else if (values[0] != 0) 
			{
				codewords[codewordIndex] = values[0];
			}
			else 
			{
				//ambiguousIndexesList.push_back(codewordIndex);
				//ambiguousIndexValues.push_back(values);
			}
		}
	}

	return CreateDecoderResultFromAmbiguousValues(errorCorrectionLevel(&detectionResult->_barcodeMetadata), codewords, erasures,
												  ambiguousIndexesList, ambiguousIndexValues, characterSet,idx,codewordsNum);

}


// TODO don't pass in minCodewordWidth and maxCodewordWidth, pass in barcode columns for start and stop pattern
// columns. That way width can be deducted from the pattern column.
// This approach also allows to detect more details about the barcode, e.g. if a bar type (white or black) is wider 
// than it should be. This can happen if the scanner used a bad blackpoint.
DecoderResult_t * Decode(const BitMatrix_t * image, ResultPoint_t imageTopLeft, ResultPoint_t imageBottomLeft,
	ResultPoint_t imageTopRight, ResultPoint_t imageBottomRight,
	int minCodewordWidth, int maxCodewordWidth, const unsigned char * characterSet)
{
	BoundingBox_t boundingBox;
	if (!Create(image->_width, image->_height, imageTopLeft, imageBottomLeft, imageTopRight, imageBottomRight, &boundingBox)) 
	{
		return NotFound;
	}
	int length = 0;

	DetectionResultColumn_t leftRowIndicatorColumn;
	DetectionResultColumn_t rightRowIndicatorColumn;
	DetectionResult_t detectionResult;
	for (int i = 0; i < 2; i++) 
	{
		if (imageTopLeft.m_hasValue != 0) 
		{
			length = GetRowIndicatorColumn(image, &boundingBox, &imageTopLeft, 1, minCodewordWidth, maxCodewordWidth,&leftRowIndicatorColumn);
		}
		if (imageTopRight.m_hasValue != 0) 
		{
			length = GetRowIndicatorColumn(image, &boundingBox, &imageTopRight, 0, minCodewordWidth, maxCodewordWidth,&rightRowIndicatorColumn);
		}
		if (!Merge(&leftRowIndicatorColumn, &rightRowIndicatorColumn, &detectionResult,length)) 
		{
			return NotFound;
		}
		if (i == 0 && (detectionResult._boundingBox.m_hasValue != 0) && (detectionResult._boundingBox.m_value._minY < boundingBox.m_value._minY || detectionResult._boundingBox.m_value._maxY > boundingBox.m_value._maxY)) 
		{
			boundingBox = detectionResult._boundingBox;
		}
		else 
		{
			detectionResult._boundingBox = boundingBox;
			break;
		}
	}

	int maxBarcodeColumn = detectionResult._barcodeMetadata._columnCount + 1;
	detectionResult._detectionResultColumns[0] = leftRowIndicatorColumn;
	detectionResult._detectionResultColumns[maxBarcodeColumn] = rightRowIndicatorColumn;

	uint8_t leftToRight;

	if(leftRowIndicatorColumn.m_hasValue != 0)
	{
		leftToRight = 1;
	}
	else
	{
		leftToRight = 0;
	}

	for (int barcodeColumnCount = 1; barcodeColumnCount <= maxBarcodeColumn; barcodeColumnCount++) 
	{
		int barcodeColumn = leftToRight ? barcodeColumnCount : maxBarcodeColumn - barcodeColumnCount;
		if (detectionResult._detectionResultColumns[barcodeColumn].m_hasValue != 0) 
		{
			// This will be the case for the opposite row indicator column, which doesn't need to be decoded again.
			continue;
		}
		RowIndicator_t rowIndicator = barcodeColumn == 0 ? Left : (barcodeColumn == maxBarcodeColumn ? Right : None);
		DetectionResultColumn(&boundingBox, rowIndicator,&(detectionResult._detectionResultColumns[barcodeColumn]));
		int startColumn = -1;
		int previousStartColumn = startColumn;
		// TODO start at a row for which we know the start position, then detect upwards and downwards from there.
		for (int imageRow = boundingBox.m_value._minY; imageRow <= boundingBox.m_value._maxY; imageRow++) 
		{
			startColumn = GetStartColumn(&detectionResult, barcodeColumn, imageRow, leftToRight,length);
			if (startColumn < 0 || startColumn > boundingBox.m_value._maxX) 
			{
				if (previousStartColumn == -1) 
				{
					continue;
				}
				startColumn = previousStartColumn;
			}
			Codeword_t * codeword = DetectCodeword(image, boundingBox.m_value._minX, boundingBox.m_value._maxX, leftToRight, startColumn, imageRow, minCodewordWidth, maxCodewordWidth);
			if (codeword != NULL) 
			{
				detectionResult._detectionResultColumns[barcodeColumn].m_value._codewords[imageRow - detectionResult._detectionResultColumns[barcodeColumn].m_value._boundingBox.m_value._minY] = *codeword;
				previousStartColumn = startColumn;
				minCodewordWidth = __min(minCodewordWidth, (codeword->m_value._endX - codeword->m_value._startX));
				maxCodewordWidth = __max(maxCodewordWidth, (codeword->m_value._endX - codeword->m_value._startX));
			}
		}
	}
	return CreateDecoderResult(&detectionResult, characterSet,length);
}
