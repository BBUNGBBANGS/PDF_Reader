
#include "PDFScanningDecoder.h"
#include "PDFBoundingBox.h"
#include "PDFDetectionResultColumn.h"
#include "PDFDetectionResult.h"
#include "PDFCodewordDecoder.h"
#include "DecodeStatus.h"
#include "library.h"
#include <stdlib.h>

static const int CODEWORD_SKEW_SIZE = 2;
static const int MAX_ERRORS = 3;
static const int MAX_EC_CODEWORDS = 512;

result_bits[8];

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
	CodeWord._startX = startX;
	CodeWord._endX = endX;
	CodeWord._bucket = bucket;
	CodeWord._value = value;	

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

static DetectionResultColumn_t * GetRowIndicatorColumn(const BitMatrix_t * image, const BoundingBox_t * boundingBox, const ResultPoint_t * startPoint, uint8_t leftToRight, int minCodewordWidth, int maxCodewordWidth)
{
	printf("GetRowIndicatorColumn \n");
	
	DetectionResultColumn(boundingBox, leftToRight ? Left : Right);
	for (int i = 0; i < 2; i++) 
	{
		int increment = i == 0 ? 1 : -1;
		int startColumn = (int)startPoint->m_value.x;
		for (int imageRow = (int)startPoint->m_value.y; imageRow <= boundingBox->_maxY && imageRow >= boundingBox->_minY; imageRow += increment) 
		{
			Codeword_t * codeword = DetectCodeword(image, 0, image->_width, leftToRight, startColumn, imageRow, minCodewordWidth, maxCodewordWidth);
			if (codeword != NULL) 
			{
				rowIndicatorColumn._codewords[imageRow] = codeword;
				if (leftToRight) 
				{
					startColumn = codeword->_startX;
				}
				else 
				{
					startColumn = codeword->_endX;
				}
			}
		}
	}
	return rowIndicatorColumn;
}

#if 0
static bool GetBarcodeMetadata(Nullable<DetectionResultColumn>& leftRowIndicatorColumn, Nullable<DetectionResultColumn>& rightRowIndicatorColumn, BarcodeMetadata& result)
{
#ifdef HK_DEBUG
	printf("GetBarcodeMetadata \n");
#endif
	
	BarcodeMetadata leftBarcodeMetadata;
	if (leftRowIndicatorColumn == nullptr || !leftRowIndicatorColumn.value().getBarcodeMetadata(leftBarcodeMetadata)) {
		return rightRowIndicatorColumn != nullptr && rightRowIndicatorColumn.value().getBarcodeMetadata(result);
	}

	BarcodeMetadata rightBarcodeMetadata;
	if (rightRowIndicatorColumn == nullptr || !rightRowIndicatorColumn.value().getBarcodeMetadata(rightBarcodeMetadata)) {
		result = leftBarcodeMetadata;
		return true;
	}

	if (leftBarcodeMetadata.columnCount() != rightBarcodeMetadata.columnCount() &&
		leftBarcodeMetadata.errorCorrectionLevel() != rightBarcodeMetadata.errorCorrectionLevel() &&
		leftBarcodeMetadata.rowCount() != rightBarcodeMetadata.rowCount()) {
		return false;
	}
	result = leftBarcodeMetadata;
	return true;
}

#endif
#if 0
template <typename Iter>
static auto GetMax(Iter start, Iter end) -> typename std::remove_reference<decltype(*start)>::type
{
	auto it = std::max_element(start, end);
	return it != end ? *it : -1;
}

#endif
#if 0
static bool AdjustBoundingBox(Nullable<DetectionResultColumn>& rowIndicatorColumn, Nullable<BoundingBox>& result)
{
#ifdef HK_DEBUG
	printf("AdjustBoundingBox \n");
#endif
	
	if (rowIndicatorColumn == nullptr)
	{
		result = nullptr;
		return true;
	}

	std::vector<int> rowHeights;
	if (!rowIndicatorColumn.value().getRowHeights(rowHeights)) 
	{
		result = nullptr;
		return true;
	}

	int maxRowHeight = GetMax(rowHeights.begin(), rowHeights.end());
	int missingStartRows = 0;
	for (int rowHeight : rowHeights) {
		missingStartRows += maxRowHeight - rowHeight;
		if (rowHeight > 0) 
		{
			break;
		}
	}
	auto& codewords = rowIndicatorColumn.value().allCodewords();
	for (int row = 0; missingStartRows > 0 && codewords[row] == nullptr; row++) 
	{
		missingStartRows--;
	}

	int missingEndRows = 0;
	for (int row = Size(rowHeights) - 1; row >= 0; row--) 
	{
		missingEndRows += maxRowHeight - rowHeights[row];
		if (rowHeights[row] > 0) {
			break;
		}
	}
	for (int row = Size(codewords) - 1; missingEndRows > 0 && codewords[row] == nullptr; row--) {
		missingEndRows--;
	}
	BoundingBox box;
	if (BoundingBox::AddMissingRows(rowIndicatorColumn.value().boundingBox(), missingStartRows, missingEndRows, rowIndicatorColumn.value().isLeftRowIndicator(), box)) {
		result = box;
		return true;
	}
	return false;
}

#endif
#if 0
static bool Merge(Nullable<DetectionResultColumn>& leftRowIndicatorColumn, Nullable<DetectionResultColumn>& rightRowIndicatorColumn, DetectionResult& result)
{
	if (leftRowIndicatorColumn != nullptr || rightRowIndicatorColumn != nullptr) 
	{
		BarcodeMetadata barcodeMetadata;
		if (GetBarcodeMetadata(leftRowIndicatorColumn, rightRowIndicatorColumn, barcodeMetadata)) 
		{
			Nullable<BoundingBox> leftBox, rightBox, mergedBox;
			if (AdjustBoundingBox(leftRowIndicatorColumn, leftBox) && AdjustBoundingBox(rightRowIndicatorColumn, rightBox) && BoundingBox::Merge(leftBox, rightBox, mergedBox)) 
			{
				result.init(barcodeMetadata, mergedBox);
				return true;
			}
		}
	}
	return false;
}

#endif
#if 0
static bool IsValidBarcodeColumn(const DetectionResult& detectionResult, int barcodeColumn)
{
	return barcodeColumn >= 0 && barcodeColumn <= detectionResult.barcodeColumnCount() + 1;
}

#endif
#if 0
static int GetStartColumn(const DetectionResult& detectionResult, int barcodeColumn, int imageRow, bool leftToRight)
{
#ifdef HK_DEBUG
	printf("GetStartColumn barcodeColumn=%d, imageRow=%d, leftToRight=%d \n", barcodeColumn, imageRow, leftToRight);
#endif

	int offset = leftToRight ? 1 : -1;
	Nullable<Codeword> codeword;
	if (IsValidBarcodeColumn(detectionResult, barcodeColumn - offset)) 
	{
		codeword = detectionResult.column(barcodeColumn - offset).value().codeword(imageRow);
	}

	if (codeword != nullptr) 
	{
		return leftToRight ? codeword.value().endX() : codeword.value().startX();
	}

	codeword = detectionResult.column(barcodeColumn).value().codewordNearby(imageRow);
	if (codeword != nullptr) 
	{
		return leftToRight ? codeword.value().startX() : codeword.value().endX();
	}

	if (IsValidBarcodeColumn(detectionResult, barcodeColumn - offset)) {
		codeword = detectionResult.column(barcodeColumn - offset).value().codewordNearby(imageRow);
	}

	if (codeword != nullptr)
	{
		return leftToRight ? codeword.value().endX() : codeword.value().startX();
	}

	int skippedColumns = 0;

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

	return leftToRight ? detectionResult.getBoundingBox().value().minX() : detectionResult.getBoundingBox().value().maxX();
}

#endif
#if 0
static std::vector<std::vector<BarcodeValue>> CreateBarcodeMatrix(DetectionResult& detectionResult)
{
#ifdef HK_DEBUG
	printf("CreateBarcodeMatrix\n");
#endif
	
	std::vector<std::vector<BarcodeValue>> barcodeMatrix(detectionResult.barcodeRowCount());
	for (auto& row : barcodeMatrix) {
		row.resize(detectionResult.barcodeColumnCount() + 2);
	}

	int column = 0;
	for (auto& resultColumn : detectionResult.allColumns()) {
		if (resultColumn != nullptr) {
			for (auto& codeword : resultColumn.value().allCodewords()) {
				if (codeword != nullptr) {
					int rowNumber = codeword.value().rowNumber();
					if (rowNumber >= 0) {
						if (rowNumber >= Size(barcodeMatrix)) {
							// We have more rows than the barcode metadata allows for, ignore them.
							continue;
						}
						barcodeMatrix[rowNumber][column].setValue(codeword.value().value());
					}
				}
			}
		}
		column++;
	}
	return barcodeMatrix;
}

#endif
#if 0
static int GetNumberOfECCodeWords(int barcodeECLevel)
{
	return 2 << barcodeECLevel;
}

#endif
#if 0
static bool AdjustCodewordCount(const DetectionResult& detectionResult, std::vector<std::vector<BarcodeValue>>& barcodeMatrix)
{
#ifdef HK_DEBUG
	printf("AdjustCodewordCount\n");
#endif
	
	auto numberOfCodewords = barcodeMatrix[0][1].value();
	int calculatedNumberOfCodewords = detectionResult.barcodeColumnCount() * detectionResult.barcodeRowCount() - GetNumberOfECCodeWords(detectionResult.barcodeECLevel());
	if (numberOfCodewords.empty()) {
		if (calculatedNumberOfCodewords < 1 || calculatedNumberOfCodewords > CodewordDecoder::MAX_CODEWORDS_IN_BARCODE) {
			return false;
		}
		barcodeMatrix[0][1].setValue(calculatedNumberOfCodewords);
	}
	else if (numberOfCodewords[0] != calculatedNumberOfCodewords) {
		// The calculated one is more reliable as it is derived from the row indicator columns
		barcodeMatrix[0][1].setValue(calculatedNumberOfCodewords);
	}
	return true;
}
// +++++++++++++++++++++++++++++++++++ Error Correction

#endif
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

/**
* @param received received codewords
* @param numECCodewords number of those codewords used for EC
* @param erasures location of erasures
* @return number of errors
* @throws ChecksumException if errors cannot be corrected, maybe because of too many errors
*/
ZXING_EXPORT_TEST_ONLY
bool DecodeErrorCorrection(std::vector<int>& received, int numECCodewords, const std::vector<int>& erasures, int& nbErrors)
{
#ifdef HK_DEBUG
	printf("DecodeErrorCorrection  numECCodewords=%d, nbErrors=%d\n", numECCodewords, nbErrors);
#endif
	
	const ModulusGF& field = GetModulusGF();
	ModulusPoly poly(field, received);
	std::vector<int> S(numECCodewords);
	bool error = false;
	for (int i = numECCodewords; i > 0; i--) 
	{
		int eval = poly.evaluateAt(field.exp(i));
		S[numECCodewords - i] = eval;
		if (eval != 0) {
			error = true;
		}
	}

	if (!error) 
	{
		nbErrors = 0;
		return true;
	}

	ModulusPoly knownErrors = field.one();
	for (int erasure : erasures) {
		int b = field.exp(Size(received) - 1 - erasure);
		// Add (1 - bx) term:
		ModulusPoly term(field, { field.subtract(0, b), 1 });
		knownErrors = knownErrors.multiply(term);
	}

	ModulusPoly syndrome(field, S);
	//syndrome = syndrome.multiply(knownErrors);

	ModulusPoly sigma, omega;
	if (!RunEuclideanAlgorithm(field.buildMonomial(numECCodewords, 1), syndrome, numECCodewords, sigma, omega)) {
		return false;
	}

	//sigma = sigma.multiply(knownErrors);

	std::vector<int> errorLocations;
	if (!FindErrorLocations(sigma, errorLocations)) {
		return false;
	}

	std::vector<int> errorMagnitudes = FindErrorMagnitudes(omega, sigma, errorLocations);

	int receivedSize = Size(received);
	for (size_t i = 0; i < errorLocations.size(); i++) {
		int position = receivedSize - 1 - field.log(errorLocations[i]);
		if (position < 0) {
			return false;
		}
		received[position] = field.subtract(received[position], errorMagnitudes[i]);
	}
	nbErrors = Size(errorLocations);
	return true;
}

#endif
#if 0
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
static bool CorrectErrors(std::vector<int>& codewords, const std::vector<int>& erasures, int numECCodewords, int& errorCount)
{
#ifdef HK_DEBUG
	printf("CorrectErrors  numECCodewords=%d, errorCount=%d\n", numECCodewords, errorCount);
#endif
	
	if (Size(erasures) > numECCodewords / 2 + MAX_ERRORS ||
		numECCodewords < 0 ||
		numECCodewords > MAX_EC_CODEWORDS) {
		// Too many errors or EC Codewords is corrupted
		return false;
	}
	return DecodeErrorCorrection(codewords, numECCodewords, erasures, errorCount);
}

#endif
#if 0
/**
* Verify that all is OK with the codeword array.
*/
static bool VerifyCodewordCount(std::vector<int>& codewords, int numECCodewords)
{
	if (codewords.size() < 4) 
	{
		// Codeword array size should be at least 4 allowing for
		// Count CW, At least one Data CW, Error Correction CW, Error Correction CW
		return false;
	}
	// The first codeword, the Symbol Length Descriptor, shall always encode the total number of data
	// codewords in the symbol, including the Symbol Length Descriptor itself, data codewords and pad
	// codewords, but excluding the number of error correction codewords.
	int numberOfCodewords = codewords[0];

	if (numberOfCodewords > Size(codewords)) 
	{
		return false;
	}

	if (numberOfCodewords == 0) 
	{
		// Reset to the length of the array - 8 (Allow for at least level 3 Error Correction (8 Error Codewords)
		if (numECCodewords < Size(codewords)) 
		{
			codewords[0] = Size(codewords) - numECCodewords;
		}
		else 
		{
			return false;
		}
	}
	return true;
}

#endif
#if 0
DecoderResult DecodeCodewords(std::vector<int>& codewords, int ecLevel, const std::vector<int>& erasures,
							  const std::string& characterSet)
{
	if (codewords.empty()) 
	{
		return DecodeStatus::FormatError;
	}

	int numECCodewords = 1 << (ecLevel + 1);
	int correctedErrorsCount = 0;
	if (!CorrectErrors(codewords, erasures, numECCodewords, correctedErrorsCount))
		return DecodeStatus::ChecksumError;

	if (!VerifyCodewordCount(codewords, numECCodewords))
		return DecodeStatus::FormatError;

	// Decode the codewords
	auto result = DecodedBitStreamParser::Decode(codewords, ecLevel, characterSet);
	if (result.isValid()) 
	{
		result.setErrorsCorrected(correctedErrorsCount);
		result.setErasures(Size(erasures));
	}

	return result;
}


#endif
#if 0
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
static DecoderResult CreateDecoderResultFromAmbiguousValues(int ecLevel, std::vector<int>& codewords,
	const std::vector<int>& erasureArray, const std::vector<int>& ambiguousIndexes,
	const std::vector<std::vector<int>>& ambiguousIndexValues, const std::string& characterSet)
{
	std::vector<int> ambiguousIndexCount(ambiguousIndexes.size(), 0);

	int tries = 100;
	while (tries-- > 0) {
		for (size_t i = 0; i < ambiguousIndexCount.size(); i++) {
			codewords[ambiguousIndexes[i]] = ambiguousIndexValues[i][ambiguousIndexCount[i]];
		}
		auto result = DecodeCodewords(codewords, ecLevel, erasureArray, characterSet);
		if (result.errorCode() != DecodeStatus::ChecksumError) {
			return result;
		}

		if (ambiguousIndexCount.empty()) {
			return DecodeStatus::ChecksumError;
		}
		for (size_t i = 0; i < ambiguousIndexCount.size(); i++) {
			if (ambiguousIndexCount[i] < Size(ambiguousIndexValues[i]) - 1) {
				ambiguousIndexCount[i]++;
				break;
			}
			else {
				ambiguousIndexCount[i] = 0;
				if (i == ambiguousIndexCount.size() - 1) {
					return DecodeStatus::ChecksumError;
				}
			}
		}
	}
	return DecodeStatus::ChecksumError;
}


#endif
#if 0
static DecoderResult CreateDecoderResult(DetectionResult& detectionResult, const std::string& characterSet)
{
	auto barcodeMatrix = CreateBarcodeMatrix(detectionResult);
	if (!AdjustCodewordCount(detectionResult, barcodeMatrix)) {
		return DecodeStatus::NotFound;
	}
	std::vector<int> erasures;
	std::vector<int> codewords(detectionResult.barcodeRowCount() * detectionResult.barcodeColumnCount(), 0);
	std::vector<std::vector<int>> ambiguousIndexValues;
	std::vector<int> ambiguousIndexesList;
	for (int row = 0; row < detectionResult.barcodeRowCount(); row++) {
		for (int column = 0; column < detectionResult.barcodeColumnCount(); column++) {
			auto values = barcodeMatrix[row][column + 1].value();
			int codewordIndex = row * detectionResult.barcodeColumnCount() + column;
			if (values.empty()) {
				erasures.push_back(codewordIndex);
			}
			else if (values.size() == 1) {
				codewords[codewordIndex] = values[0];
			}
			else {
				ambiguousIndexesList.push_back(codewordIndex);
				ambiguousIndexValues.push_back(values);
			}
		}
	}
	return CreateDecoderResultFromAmbiguousValues(detectionResult.barcodeECLevel(), codewords, erasures,
												  ambiguousIndexesList, ambiguousIndexValues, characterSet);
}
#endif

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

	DetectionResultColumn_t * leftRowIndicatorColumn;
	DetectionResultColumn_t * rightRowIndicatorColumn;
	DetectionResult_t detectionResult;
	for (int i = 0; i < 2; i++) 
	{
		if (imageTopLeft.m_hasValue != 0) 
		{
			leftRowIndicatorColumn = GetRowIndicatorColumn(image, &boundingBox, &imageTopLeft, 1, minCodewordWidth, maxCodewordWidth);
		}
		if (imageTopRight.m_hasValue != 0) 
		{
			rightRowIndicatorColumn = GetRowIndicatorColumn(image, &boundingBox, &imageTopRight, 0, minCodewordWidth, maxCodewordWidth);
		}
		//if (!Merge(leftRowIndicatorColumn, rightRowIndicatorColumn, detectionResult)) 
		//{
			//return NotFound;
		//}
		if (i == 0 && (detectionResult._boundingBox != NULL) && (detectionResult._boundingBox->_minY < boundingBox._minY || detectionResult._boundingBox->_maxY > boundingBox._maxY)) 
		{
			boundingBox = *detectionResult._boundingBox;
		}
		else 
		{
			detectionResult._boundingBox = &boundingBox;
			break;
		}
	}
	#if 0
	int maxBarcodeColumn = detectionResult.barcodeColumnCount() + 1;
	detectionResult.setColumn(0, leftRowIndicatorColumn);
	detectionResult.setColumn(maxBarcodeColumn, rightRowIndicatorColumn);

	bool leftToRight = leftRowIndicatorColumn != nullptr;
	for (int barcodeColumnCount = 1; barcodeColumnCount <= maxBarcodeColumn; barcodeColumnCount++) {
		int barcodeColumn = leftToRight ? barcodeColumnCount : maxBarcodeColumn - barcodeColumnCount;
		if (detectionResult.column(barcodeColumn) != nullptr) {
			// This will be the case for the opposite row indicator column, which doesn't need to be decoded again.
			continue;
		}
		DetectionResultColumn::RowIndicator rowIndicator = barcodeColumn == 0 ? DetectionResultColumn::RowIndicator::Left : (barcodeColumn == maxBarcodeColumn ? DetectionResultColumn::RowIndicator::Right : DetectionResultColumn::RowIndicator::None);
		detectionResult.setColumn(barcodeColumn, DetectionResultColumn(boundingBox, rowIndicator));
		int startColumn = -1;
		int previousStartColumn = startColumn;
		// TODO start at a row for which we know the start position, then detect upwards and downwards from there.
		for (int imageRow = boundingBox.minY(); imageRow <= boundingBox.maxY(); imageRow++) {
			startColumn = GetStartColumn(detectionResult, barcodeColumn, imageRow, leftToRight);
			if (startColumn < 0 || startColumn > boundingBox.maxX()) {
				if (previousStartColumn == -1) {
					continue;
				}
				startColumn = previousStartColumn;
			}
			Nullable<Codeword> codeword = DetectCodeword(image, boundingBox.minX(), boundingBox.maxX(), leftToRight, startColumn, imageRow, minCodewordWidth, maxCodewordWidth);
			if (codeword != nullptr) {
				detectionResult.column(barcodeColumn).value().setCodeword(imageRow, codeword);
				previousStartColumn = startColumn;
				minCodewordWidth = std::min(minCodewordWidth, codeword.value().width());
				maxCodewordWidth = std::max(maxCodewordWidth, codeword.value().width());
			}
		}
	}
	return CreateDecoderResult(detectionResult, characterSet);
	#endif
}
