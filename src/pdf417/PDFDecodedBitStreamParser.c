/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
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

#include "PDFDecodedBitStreamParser.h"
#include "PDFDecoderResultExtra.h"
#include "CharacterSet.h"
#include "TextDecoder.h"
#include "Library.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef enum 
{
	ALPHA,
	LOWER,
	MIXED,
	PUNCT,
	ALPHA_SHIFT,
	PUNCT_SHIFT
}Mode_t;

typedef enum
{
	TEXT_COMPACTION_MODE_LATCH = 900,
	BYTE_COMPACTION_MODE_LATCH = 901,
	NUMERIC_COMPACTION_MODE_LATCH = 902,
	// 903-912 reserved
	MODE_SHIFT_TO_BYTE_COMPACTION_MODE = 913,
	// 914-917 reserved
	LINKAGE_OTHER = 918,
	// 919 reserved
	LINKAGE_EANUCC = 920, // GS1 Composite
	READER_INIT = 921, // Reader Initialisation/Programming
	MACRO_PDF417_TERMINATOR = 922,
	BEGIN_MACRO_PDF417_OPTIONAL_FIELD = 923,
	BYTE_COMPACTION_MODE_LATCH_6 = 924,
	ECI_USER_DEFINED = 925, // 810900-811799 (1 codeword)
	ECI_GENERAL_PURPOSE = 926, // 900-810899 (2 codewords)
	ECI_CHARSET = 927, // 0-899 (1 codeword)
	BEGIN_MACRO_PDF417_CONTROL_BLOCK = 928
}code_t;


static const int MAX_NUMERIC_CODEWORDS = 15;
typedef enum
{
	MACRO_PDF417_OPTIONAL_FIELD_FILE_NAME = 0,
	MACRO_PDF417_OPTIONAL_FIELD_SEGMENT_COUNT = 1,
	MACRO_PDF417_OPTIONAL_FIELD_TIME_STAMP = 2,
	MACRO_PDF417_OPTIONAL_FIELD_SENDER = 3,
	MACRO_PDF417_OPTIONAL_FIELD_ADDRESSEE = 4,
	MACRO_PDF417_OPTIONAL_FIELD_FILE_SIZE = 5,
	MACRO_PDF417_OPTIONAL_FIELD_CHECKSUM = 6
}codeIndex_t;


static const char* PUNCT_CHARS = ";<>@[\\]_`~!\r\t,:\n-.$/\"|*()?{}'";
static const char* MIXED_CHARS = "0123456789&\r\t,:#-.$/+%*=^";

static const int NUMBER_OF_SEQUENCE_CODEWORDS = 2;

/**
* Whether a codeword terminates a Compaction mode.
*
* See ISO/IEC 15438:2015 5.4.2.5 (Text), 5.4.3.4 (Byte), 5.4.4.3 (Numeric)
*/

static uint8_t TerminatesCompaction(int code)
{
	switch (code) 
	{
		case TEXT_COMPACTION_MODE_LATCH:
		case BYTE_COMPACTION_MODE_LATCH:
		case NUMERIC_COMPACTION_MODE_LATCH:
		case BYTE_COMPACTION_MODE_LATCH_6:
		case BEGIN_MACRO_PDF417_CONTROL_BLOCK:
		case BEGIN_MACRO_PDF417_OPTIONAL_FIELD:
		case MACRO_PDF417_TERMINATOR:
			return 1;
		break;
	}

	return 0;
}

/**
* Helper to process ECIs.
**/
static int ProcessECI(const int * codewords, int codeIndex, const int length, const int code,
					  unsigned char * resultEncoded, char * result, CharacterSet_t * encoding)
{
	printf("ProcessECI\n");

	if (codeIndex < length && code >= ECI_USER_DEFINED && code <= ECI_CHARSET) 
	{
		if (code == ECI_CHARSET) 
		{
			//encoding = OnChangeAppendReset(codewords[codeIndex++], resultEncoded, result, encoding);
		}
		else 
		{
			// Don't currently handle non-character set ECIs so just ignore
			codeIndex += code == ECI_GENERAL_PURPOSE ? 2 : 1;
		}
	}

	return codeIndex;
}

/**
* The Text Compaction mode includes all the printable ASCII characters
* (i.e. values from 32 to 126) and three ASCII control characters: HT or tab
* (ASCII value 9), LF or line feed (ASCII value 10), and CR or carriage
* return (ASCII value 13). The Text Compaction mode also includes various latch
* and shift characters which are used exclusively within the mode. The Text
* Compaction mode encodes up to 2 characters per codeword. The compaction rules
* for converting data into PDF417 codewords are defined in 5.4.2.2. The sub-mode
* switches are defined in 5.4.2.3.
*
* @param textCompactionData The text compaction data.
* @param length             The size of the text compaction data.
* @param resultEncoded      The Unicode-encoded data.
* @param result             The data in the character set encoding.
* @param encoding           Currently active character encoding.
*/

static void DecodeTextCompaction(const int * textCompactionData, int length, unsigned char * resultEncoded,
								 char * result, CharacterSet_t * encoding)
{

	printf("DecodeTextCompaction\n");

	// Beginning from an initial state of the Alpha sub-mode
	// The default compaction mode for PDF417 in effect at the start of each symbol shall always be Text
	// Compaction mode Alpha sub-mode (uppercase alphabetic). A latch codeword from another mode to the Text
	// Compaction mode shall always switch to the Text Compaction Alpha sub-mode.
	Mode_t subMode = ALPHA;
	Mode_t priorToShiftMode = ALPHA;

	int i = 0;
	int character_idx = strlen(result);

	while (i < length) 
	{
		int subModeCh = textCompactionData[i];

		// Note only have ECI and MODE_SHIFT_TO_BYTE_COMPACTION_MODE function codewords in text compaction array
		if (subModeCh >= ECI_USER_DEFINED && subModeCh <= ECI_CHARSET) 
		{
			i = ProcessECI(textCompactionData, i + 1, length, subModeCh, resultEncoded, result, encoding);
			continue;
		}

		if (subModeCh == MODE_SHIFT_TO_BYTE_COMPACTION_MODE) 
		{
			i++;
			while (i < length && textCompactionData[i] >= ECI_USER_DEFINED && textCompactionData[i] <= ECI_CHARSET) 
			{
				i = ProcessECI(textCompactionData, i + 1, length, textCompactionData[i], resultEncoded, result,
							   encoding);
			}
			if (i < length) 
			{
				result=textCompactionData[i];
				i++;
			}
			continue;
		}

		char ch = 0;
		switch (subMode) 
		{
			case ALPHA:
			case LOWER:
				// Alpha (uppercase alphabetic) or Lower (lowercase alphabetic)
				if (subModeCh < 26) 
				{
					// Upper/lowercase character
					ch = (char)((subMode == ALPHA ? 'A' : 'a') + subModeCh);
				}
				else if (subModeCh == 26) 
				{ // Space
					ch = ' ';
				}
				else if (subModeCh == 27 && subMode == ALPHA) 
				{ // LL
					subMode = LOWER;
				}
				else if (subModeCh == 27 && subMode == LOWER) 
				{ // AS
					// Shift to alpha
					priorToShiftMode = subMode;
					subMode = ALPHA_SHIFT;
				}
				else if (subModeCh == 28) 
				{ // ML
					subMode = MIXED;
				}
				// 29 PS - ignore if last or followed by Shift to Byte, 5.4.2.4 (b) (1)
				else if (i + 1 < length && textCompactionData[i + 1] != MODE_SHIFT_TO_BYTE_COMPACTION_MODE) 
				{
					// Shift to punctuation
					priorToShiftMode = subMode;
					subMode = PUNCT_SHIFT;
				}
			break;
			case MIXED:
				// Mixed (numeric and some punctuation)
				if (subModeCh < 25) 
				{
					ch = MIXED_CHARS[subModeCh];
				}
				else if (subModeCh == 25) 
				{ // PL
					subMode = PUNCT;
				}
				else if (subModeCh == 26) 
				{ // Space
					ch = ' ';
				}
				else
				if (subModeCh == 27) 
				{ // LL
					subMode = LOWER;
				}
				else
				if (subModeCh == 28) 
				{ // AL
					subMode = ALPHA;
				}
				// 29 PS - ignore if last or followed by Shift to Byte, 5.4.2.4 (b) (1)
				else 
				if (i + 1 < length && textCompactionData[i + 1] != MODE_SHIFT_TO_BYTE_COMPACTION_MODE) 
				{
					// Shift to punctuation
					priorToShiftMode = subMode;
					subMode = PUNCT_SHIFT;
				}
			break;
			case PUNCT:
				// Punctuation
				if (subModeCh < 29) {
					ch = PUNCT_CHARS[subModeCh];
				}
				else { // 29 AL - note not ignored if followed by Shift to Byte, 5.4.2.4 (b) (2)
					subMode = ALPHA;
				}
			break;
				case ALPHA_SHIFT:
				// Restore sub-mode
				subMode = priorToShiftMode;
				if (subModeCh < 26) 
				{
					ch = (char)('A' + subModeCh);
				}
				else if (subModeCh == 26) 
				{ // Space
					ch = ' ';
				}
				// 27 LL, 28 ML, 29 PS used as padding
			break;
			case PUNCT_SHIFT:
				// Restore sub-mode
				subMode = priorToShiftMode;
				if (subModeCh < 29) 
				{
					ch = PUNCT_CHARS[subModeCh];
				}
				else 
				{ // 29 AL
					subMode = ALPHA;
				}
			break;
		}
		if (ch != 0) 
		{
			// Append decoded character to result
			result[character_idx] = ch;
			character_idx++;
		}
		i++;
	}
}


/*
* Helper to put ECI codewords into Text Compaction array.
*/
static int ProcessTextECI(int * textCompactionData, int index,
						  const int * codewords, int codeIndex, const int code) 
{

	printf("ProcessTextECI\n");

	textCompactionData[index++] = code;

	if (codeIndex < codewords[0]) 
	{
		textCompactionData[index++] = codewords[codeIndex++];
		if (codeIndex < codewords[0] && code == ECI_GENERAL_PURPOSE) 
		{
			textCompactionData[index++] = codewords[codeIndex++];
		}
	}

	return codeIndex;
}


/**
* Text Compaction mode (see 5.4.1.5) permits all printable ASCII characters to be
* encoded, i.e. values 32 - 126 inclusive in accordance with ISO/IEC 646 (IRV), as
* well as selected control characters.
*
* @param codewords     The array of codewords (data + error)
* @param codeIndex     The current index into the codeword array.
* @param resultEncoded The Unicode-encoded data.
* @param result        The data in the character set encoding.
* @param encoding      Currently active character encoding.
* @return The next index into the codeword array.
*/
static int TextCompaction(DecodeStatus_t status, const int * codewords, int codeIndex,
						  unsigned char * resultEncoded, char * result, CharacterSet_t * encoding)
{

	printf("TextCompaction\n");

	// 2 characters per codeword
	int size = (codewords[0] - codeIndex) * 2;
	int textCompactionData[size];
	fill(size,textCompactionData,0);

	int index = 0;
	uint8_t end = 0;
	while ((codeIndex < codewords[0]) && !end) 
	{
		int code = codewords[codeIndex++];
		if (code < TEXT_COMPACTION_MODE_LATCH) 
		{
			textCompactionData[index] = code / 30;
			textCompactionData[index + 1] = code % 30;
			index += 2;
		}
		else 
		{
			switch (code) 
			{
				case MODE_SHIFT_TO_BYTE_COMPACTION_MODE:
					// The Mode Shift codeword 913 shall cause a temporary
					// switch from Text Compaction mode to Byte Compaction mode.
					// This switch shall be in effect for only the next codeword,
					// after which the mode shall revert to the prevailing sub-mode
					// of the Text Compaction mode. Codeword 913 is only available
					// in Text Compaction mode; its use is described in 5.4.2.4.
					textCompactionData[index++] = MODE_SHIFT_TO_BYTE_COMPACTION_MODE;
					// 5.5.3.1 allows ECIs anywhere in Text Compaction, including after a Shift to Byte
					while (codeIndex < codewords[0] && codewords[codeIndex] >= ECI_USER_DEFINED
							&& codewords[codeIndex] <= ECI_CHARSET) 
					{
						codeIndex = ProcessTextECI(textCompactionData, index, codewords, codeIndex + 1,
												codewords[codeIndex]);
					}

					if (codeIndex < codewords[0]) 
					{
						textCompactionData[index++] = codewords[codeIndex++]; // Byte to shift
					}
				break;
				case ECI_CHARSET:
				case ECI_GENERAL_PURPOSE:
				case ECI_USER_DEFINED:
					codeIndex = ProcessTextECI(textCompactionData, index, codewords, codeIndex, code);
				break;
				default:
					if (!TerminatesCompaction(code)) 
					{
						status = FormatError;
						return codeIndex;
					}
					codeIndex--;
					end = 1;
				break;
			}
		}
	}

	DecodeTextCompaction(textCompactionData, index, resultEncoded, result, encoding);
	return codeIndex;
}


/*
* Helper for Byte Compaction to look ahead and count 5-codeword batches and trailing bytes, with some checking of
* format errors.
*/
static int CountByteBatches(DecodeStatus_t status, int mode, const int * codewords, int codeIndex,
							int * trailingCount)
{
	int count = 0;

	while (codeIndex < codewords[0]) 
	{
		int code = codewords[codeIndex++];
		if (code >= TEXT_COMPACTION_MODE_LATCH) 
		{
			if (mode == BYTE_COMPACTION_MODE_LATCH_6 && count && count % 5) 
			{
				status = FormatError;
				return 0;
			}
			if (code >= ECI_USER_DEFINED && code <= ECI_CHARSET) 
			{
				codeIndex += code == ECI_GENERAL_PURPOSE ? 2 : 1;
				continue;
			}
			if (!TerminatesCompaction(code)) 
			{
				status = FormatError;
				return 0;
			}
			break;
		}
		count++;
	}
	if (codeIndex > codewords[0]) 
	{
		status = FormatError;
		return 0;
	}
	if (count == 0) {
		return 0;
	}

	if (mode == BYTE_COMPACTION_MODE_LATCH) 
	{
		*trailingCount = count % 5;
		if (*trailingCount == 0) 
		{
			*trailingCount = 5;
			count -= 5;
		}
	}
	else 
	{ // BYTE_COMPACTION_MODE_LATCH_6
		if (count % 5 != 0) 
		{
			status = FormatError;
			return 0;
		}
	}

	return count / 5;
}


/*
* Helper to handle Byte Compaction ECIs.
*/
static int ProcessByteECIs(const int * codewords, int codeIndex,
						   unsigned char * resultEncoded, char * result, CharacterSet_t * encoding)
{
	while (codeIndex < codewords[0] && codewords[codeIndex] >= TEXT_COMPACTION_MODE_LATCH
			&& !TerminatesCompaction(codewords[codeIndex])) 
		{
		int code = codewords[codeIndex++];
		if (code >= ECI_USER_DEFINED && code <= ECI_CHARSET) 
		{
			codeIndex = ProcessECI(codewords, codeIndex, codewords[0], code, resultEncoded, result, encoding);
		}
	}

	return codeIndex;
}

/**
* Byte Compaction mode (see 5.4.3) permits all 256 possible 8-bit byte values to be encoded.
* This includes all ASCII characters value 0 to 127 inclusive and provides for international
* character set support.
*
* @param status        Set on format error.
* @param mode          The byte compaction mode i.e. 901 or 924
* @param codewords     The array of codewords (data + error)
* @param codeIndex     The current index into the codeword array.
* @param resultEncoded The Unicode-encoded data.
* @param result        The data in the character set encoding.
* @param encoding      Currently active character encoding.
* @return The next index into the codeword array.
*/

static int ByteCompaction(DecodeStatus_t status, int mode, const int * codewords, int codeIndex,
						  unsigned char * resultEncoded, char * result, CharacterSet_t * encoding)
{
	// Count number of 5-codeword batches and trailing bytes
	int trailingCount = 0;;
	int batches = CountByteBatches(status, mode, codewords, codeIndex, &trailingCount);

	if (StatusIsError(status)) 
	{
		return codeIndex;
	}

	// Deal with initial ECIs
	codeIndex = ProcessByteECIs(codewords, codeIndex, resultEncoded, result, encoding);

	for (int batch = 0; batch < batches; batch++) 
	{
		int64_t value = 0;
		//int32_t value = 0;
		for (int count = 0; count < 5; count++) 
		{
			value = 900 * value + codewords[codeIndex++];
		}

		for (int j = 0; j < 6; ++j) 
		{
			result=(uint8_t)(value >> (8 * (5 - j)));
		}

		// Deal with inter-batch ECIs
		codeIndex = ProcessByteECIs(codewords, codeIndex, resultEncoded, result, encoding);
	}

	for (int i = 0; i < trailingCount; i++) 
	{
		result = (uint8_t)codewords[codeIndex++];
		// Deal with inter-byte ECIs
		codeIndex = ProcessByteECIs(codewords, codeIndex, resultEncoded, result, encoding);
	}

	return codeIndex;
}


/**
* Convert a list of Numeric Compacted codewords from Base 900 to Base 10.
*
* @param codewords The array of codewords
* @param count     The number of codewords
* @return The decoded string representing the Numeric data.
*/
/*
EXAMPLE
Encode the fifteen digit numeric string 000213298174000
Prefix the numeric string with a 1 and set the initial value of
t = 1 000 213 298 174 000
Calculate codeword 0
d0 = 1 000 213 298 174 000 mod 900 = 200

t = 1 000 213 298 174 000 div 900 = 1 111 348 109 082
Calculate codeword 1
d1 = 1 111 348 109 082 mod 900 = 282

t = 1 111 348 109 082 div 900 = 1 234 831 232
Calculate codeword 2
d2 = 1 234 831 232 mod 900 = 632

t = 1 234 831 232 div 900 = 1 372 034
Calculate codeword 3
d3 = 1 372 034 mod 900 = 434

t = 1 372 034 div 900 = 1 524
Calculate codeword 4
d4 = 1 524 mod 900 = 624

t = 1 524 div 900 = 1
Calculate codeword 5
d5 = 1 mod 900 = 1
t = 1 div 900 = 0
Codeword sequence is: 1, 624, 434, 632, 282, 200

Decode the above codewords involves
1 x 900 power of 5 + 624 x 900 power of 4 + 434 x 900 power of 3 +
632 x 900 power of 2 + 282 x 900 power of 1 + 200 x 900 power of 0 = 1000213298174000

Remove leading 1 =>  Result is 000213298174000
*/

static DecodeStatus_t DecodeBase900toBase10(const int * codewords, int count, char * resultString)
{
	// Table containing values for the exponent of 900.
	#if 0
	static const auto EXP900 = []() 
	{
		std::array<BigInteger, 16> table = {1, 900};
		for (size_t i = 2; i < table.size(); ++i) {
			table[i] = table[i - 1] * 900;
		}
		return table;
	}();

	assert(count <= 16);

	BigInteger result;
	for (int i = 0; i < count; i++) 
	{
		result += EXP900[count - i - 1] * codewords[i];
	}

	resultString = result.toString();

	if (!resultString.empty() && resultString.front() == '1') 
	{
		resultString = resultString.substr(1);
		return NoError;
	}
	#endif
	return FormatError;
}


/**
* Numeric Compaction mode (see 5.4.4) permits efficient encoding of numeric data strings.
*
* @param status    Set on format error.
* @param codewords The array of codewords (data + error)
* @param codeIndex The current index into the codeword array.
* @param result    The decoded data is appended to the result.
* @param encoding  Currently active character encoding.
* @return The next index into the codeword array.
*/
static int NumericCompaction(DecodeStatus_t status, const int * codewords, int codeIndex,
							 unsigned char * resultEncoded, char * result, CharacterSet_t * encoding)
{
	int count = 0;
	uint8_t end = 0;

	int numericCodewords[MAX_NUMERIC_CODEWORDS];

	while (codeIndex < codewords[0] && !end) 
	{
		int code = codewords[codeIndex++];
		if (code >= TEXT_COMPACTION_MODE_LATCH) 
		{
			if (code >= ECI_USER_DEFINED && code <= ECI_CHARSET) 
			{
				// As operating in Basic Channel Mode (i.e. not embedding backslashed ECIs and doubling backslashes)
				// allow ECIs anywhere in Numeric Compaction (i.e. ISO/IEC 15438:2015 5.5.3.4 doesn't apply).
				if (count > 0) 
				{
					char tmp[100] = {0,};
					status = DecodeBase900toBase10(numericCodewords, count, tmp);
					if (StatusIsError(status)) {
						return codeIndex;
					}
					result += *tmp;
					count = 0;
				}
				codeIndex = ProcessECI(codewords, codeIndex, codewords[0], code, resultEncoded, result, encoding);
				continue;
			}
			if (!TerminatesCompaction(code)) 
			{
				status = FormatError;
				return codeIndex;
			}
			codeIndex--;
			end = 1;
		}
		if (codeIndex == codewords[0]) {
			end = 1;
		}
		if (code < TEXT_COMPACTION_MODE_LATCH) 
		{
			numericCodewords[count] = code;
			count++;
		}
		if (count % MAX_NUMERIC_CODEWORDS == 0 || code == NUMERIC_COMPACTION_MODE_LATCH || end) 
		{
			// Re-invoking Numeric Compaction mode (by using codeword 902
			// while in Numeric Compaction mode) serves  to terminate the
			// current Numeric Compaction mode grouping as described in 5.4.4.2,
			// and then to start a new one grouping.
			if (count > 0) 
			{
				char tmp[100] = {0,};
				status = DecodeBase900toBase10(numericCodewords, count, tmp);
				if (StatusIsError(status)) 
				{
					return codeIndex;
				}
				result += *tmp;
				count = 0;
			}
		}
	}
	return codeIndex;
}


/*
* Helper to deal with optional text fields in Macros.
*/
static int DecodeMacroOptionalTextField(DecodeStatus_t status, const int * codewords, int codeIndex,
										char * field)
{
	unsigned char resultEncoded[100] = {0,};
	char result[100];
	// Each optional field begins with an implied reset to ECI 2 (Annex H.2.3). ECI 2 is ASCII for 0-127, and Cp437
	// for non-ASCII (128-255). Text optional fields can contain ECIs.
	CharacterSet_t encoding = Cp437;

	codeIndex = TextCompaction(status, codewords, codeIndex, resultEncoded, &result, &encoding);
	Append(resultEncoded, (const uint8_t*)(result), strlen(result), encoding); 

	return codeIndex;
}


/*
* Helper to deal with optional numeric fields in Macros.
*/
static int DecodeMacroOptionalNumericField(DecodeStatus_t status, const int * codewords, int codeIndex,
										   uint64_t field)
{

	printf("DecodeMacroOptionalNumericField\n");

	
	unsigned char resultEncoded[100];
	char result[100];
	// Each optional field begins with an implied reset to ECI 2 (Annex H.2.3).
	// Numeric optional fields should not contain ECIs, but processed anyway.
	CharacterSet_t encoding = Cp437;

	codeIndex = NumericCompaction(status, codewords, codeIndex, resultEncoded, result, &encoding);
	Append(resultEncoded, (const uint8_t*)(result), strlen(result), encoding); 

	field = strtol(resultEncoded,strlen(resultEncoded),10);

	return codeIndex;
}


DecodeStatus_t DecodeMacroBlock(const int * codewords, int codeIndex, DecoderResultExtra_t * resultMetadata,
							  int next)
{

	printf("DecodeMacroBlock  \n");
	
	if (codeIndex + NUMBER_OF_SEQUENCE_CODEWORDS > codewords[0])
	{
		// we must have at least two bytes left for the segment index
		return FormatError;
	}

	int segmentIndexArray[NUMBER_OF_SEQUENCE_CODEWORDS];
	for (int i = 0; i < NUMBER_OF_SEQUENCE_CODEWORDS; i++, codeIndex++) 
	{
		segmentIndexArray[i] = codewords[codeIndex];
	}

	char strBuf[100] = {0,};
	DecodeStatus_t status = DecodeBase900toBase10(segmentIndexArray, NUMBER_OF_SEQUENCE_CODEWORDS, strBuf);
	if (StatusIsError(status)) 
	{
		return status;
	}

	resultMetadata->_segmentIndex = strtol(strBuf,strlen(strBuf),10);

	// Decoding the fileId codewords as 0-899 numbers, each 0-filled to width 3. This follows the spec
	// (See ISO/IEC 15438:2015 Annex H.6) and preserves all info, but some generators (e.g. TEC-IT) write
	// the fileId using text compaction, so in those cases the fileId will appear mangled.
	#if 0
	std::ostringstream fileId;
	fileId.fill('0');
	for (int i = 0; codeIndex < codewords[0] && codewords[codeIndex] != MACRO_PDF417_TERMINATOR
			&& codewords[codeIndex] != BEGIN_MACRO_PDF417_OPTIONAL_FIELD; i++, codeIndex++) 
	{
		fileId << std::setw(3) << codewords[codeIndex];
	}

	resultMetadata.setFileId(fileId.str());
	#endif //os stream ?????? ?????????
	int optionalFieldsStart = -1;
	if (codeIndex < codewords[0] && codewords[codeIndex] == BEGIN_MACRO_PDF417_OPTIONAL_FIELD) 
	{
		optionalFieldsStart = codeIndex + 1;
	}

	while (codeIndex < codewords[0]) 
	{
		switch (codewords[codeIndex]) 
		{
			case BEGIN_MACRO_PDF417_OPTIONAL_FIELD: 
			{
				codeIndex++;
				if (codeIndex >= codewords[0]) 
				{
					break;
				}

				switch (codewords[codeIndex]) 
				{
					case MACRO_PDF417_OPTIONAL_FIELD_FILE_NAME: 
					{
						char fileName[100] = {0,};
						codeIndex = DecodeMacroOptionalTextField(status, codewords, codeIndex + 1, fileName);
						resultMetadata->_fileName = (fileName);
						break;
					}

					case MACRO_PDF417_OPTIONAL_FIELD_SENDER: 
					{
						char sender[100] = {0,};
						codeIndex = DecodeMacroOptionalTextField(status, codewords, codeIndex + 1, sender);
						resultMetadata->_sender = (sender);
						break;
					}

					case MACRO_PDF417_OPTIONAL_FIELD_ADDRESSEE: 
					{
						char addressee[100] = {0,};
						codeIndex = DecodeMacroOptionalTextField(status, codewords, codeIndex + 1, addressee);
						resultMetadata->_addressee = (addressee);
						break;
					}
					case MACRO_PDF417_OPTIONAL_FIELD_SEGMENT_COUNT: 
					{
						uint64_t segmentCount = 0;
						codeIndex = DecodeMacroOptionalNumericField(status, codewords, codeIndex + 1, segmentCount);
						resultMetadata->_segmentCount = (segmentCount);
						break;
					}

					case MACRO_PDF417_OPTIONAL_FIELD_TIME_STAMP: 
					{
						uint64_t timestamp = 0;;
						codeIndex = DecodeMacroOptionalNumericField(status, codewords, codeIndex + 1, timestamp);
						resultMetadata->_timestamp = (timestamp);
						break;
					}

					case MACRO_PDF417_OPTIONAL_FIELD_CHECKSUM: 
					{
						uint64_t checksum = 0;;
						codeIndex = DecodeMacroOptionalNumericField(status, codewords, codeIndex + 1, checksum);
						resultMetadata->_checksum = (checksum);
						break;
					}

					case MACRO_PDF417_OPTIONAL_FIELD_FILE_SIZE: 
					{
						uint64_t fileSize = 0;;
						codeIndex = DecodeMacroOptionalNumericField(status, codewords, codeIndex + 1, fileSize);
						resultMetadata->_fileSize = (fileSize);
						break;
					}

					default: 
					{
						status = FormatError;
						break;
					}
				}

				break;
			}

			case MACRO_PDF417_TERMINATOR: 
			{
				codeIndex++;
				resultMetadata->_lastSegment = 1;
				break;
			}

			default: 
			{
				status = FormatError;
				break;
			}
		}

		if (StatusIsError(status)) 
		{
			return status;
		}
	}

	// copy optional fields to additional options
	if (optionalFieldsStart != -1) {
		int optionalFieldsLength = codeIndex - optionalFieldsStart;
		if (resultMetadata->_lastSegment) 
		{
			// do not include terminator
			optionalFieldsLength--;
		}
		resultMetadata->_optionalData = codewords + optionalFieldsStart;
	}

	next = codeIndex;
	return NoError;
}

static void DecoderResultExtra_init(DecoderResultExtra_t * result)
{
	result->_segmentIndex = -1;
	result->_fileId = NULL;
	result->_optionalData = NULL;
	result->_lastSegment = 0;
	result->_segmentCount = -1;
	result->_sender = NULL;
	result->_addressee = NULL;
	result->_fileName = NULL;
	result->_fileSize = -1;
	result->_timestamp = -1;
	result->_checksum = -1;
}
static void StructuredAppendInfo_Init(StructuredAppendInfo_t * sai)
{
	sai->index = -1;
	sai->count = -1;
	sai->id = NULL;
}

void BitStreamDecode(DecoderResult_t * DecoderResult, const int * codewords, int ecLevel, const unsigned char * characterSet)
{
	unsigned char resultEncoded[100] = {0,};
	unsigned char result[100] = {0,};
	CharacterSet_t encoding = ISO8859_1; //= CharacterSetECI::InitEncoding(characterSet);
	uint8_t readerInit = 0;
	DecoderResultExtra_t resultMetadata;
	DecoderResultExtra_init(&resultMetadata);
	int codeIndex = 1;
	DecodeStatus_t status = NoError;

	while (codeIndex < codewords[0] && status == NoError) 
	{
		int code = codewords[codeIndex++];
		switch (code) 
		{
			case TEXT_COMPACTION_MODE_LATCH:
				codeIndex = TextCompaction(status, codewords, codeIndex, resultEncoded, result, &encoding);
			break;
			case MODE_SHIFT_TO_BYTE_COMPACTION_MODE:
				// This should only be encountered once in this loop, when default Text Compaction mode applies
				// (see default case below)
				codeIndex = TextCompaction(status, codewords, codeIndex - 1, resultEncoded, result, &encoding);
			break;
			case BYTE_COMPACTION_MODE_LATCH:
			case BYTE_COMPACTION_MODE_LATCH_6:
				codeIndex = ByteCompaction(status, code, codewords, codeIndex, resultEncoded, result, &encoding);
			break;
			case NUMERIC_COMPACTION_MODE_LATCH:
				codeIndex = NumericCompaction(status, codewords, codeIndex, resultEncoded, result, &encoding);
			break;
			case ECI_CHARSET:
			case ECI_GENERAL_PURPOSE:
			case ECI_USER_DEFINED:
				codeIndex = ProcessECI(codewords, codeIndex, codewords[0], code, resultEncoded, result, &encoding);
			break;
			case BEGIN_MACRO_PDF417_CONTROL_BLOCK:
				status = DecodeMacroBlock(codewords, codeIndex, &resultMetadata, codeIndex);
			break;
			case BEGIN_MACRO_PDF417_OPTIONAL_FIELD:
			case MACRO_PDF417_TERMINATOR:
				// Should not see these outside a macro block
				status = FormatError;
			break;
			case READER_INIT:
				if (codeIndex != 2) 
				{ // Must be first codeword after symbol length (ISO/IEC 15438:2015 5.4.1.4)
					status = FormatError;
				}
				else 
				{
					readerInit = 1;
				}
			break;
			case LINKAGE_EANUCC:
				if (codeIndex != 2) 
				{ // Must be first codeword after symbol length (GS1 Composite ISO/IEC 24723:2010 4.3)
					status = FormatError;
				}
				else 
				{
					// TODO: handle
				}
			break;
			case LINKAGE_OTHER:
				// Allowed to treat as invalid by ISO/IEC 24723:2010 5.4.1.5 and 5.4.6.1 when in Basic Channel Mode
				status = FormatError; // TODO: add NotSupported error
			break;
			default:
				if (code >= TEXT_COMPACTION_MODE_LATCH) 
				{ // Reserved codewords (all others in switch)
					// Allowed to treat as invalid by ISO/IEC 24723:2010 5.4.6.1 when in Basic Channel Mode
					status = FormatError; // TODO: add NotSupported error
				}
				else 
				{
					// Default mode is Text Compaction mode Alpha sub-mode (ISO/IEC 15438:2015 5.4.2.1)
					codeIndex = TextCompaction(status, codewords, codeIndex - 1, resultEncoded, result, &encoding);
				}
			break;
		}
	}

	Append(resultEncoded, (const uint8_t*)(result), strlen(result), encoding);

	if ((strlen(resultEncoded) == 0) && (resultMetadata._segmentIndex == -1))
	{
		DecoderResult->status = FormatError;
	}


	if (StatusIsError(status))
	{
		DecoderResult->status = status;
	}


	StructuredAppendInfo_t sai;
	StructuredAppendInfo_Init(&sai);
	if (resultMetadata._segmentIndex > -1) 
	{
		sai.count = resultMetadata._segmentCount != -1
						? resultMetadata._segmentCount
						: (resultMetadata._lastSegment ? resultMetadata._segmentIndex + 1 : 0);
		sai.index = resultMetadata._segmentIndex;
		sai.id    = resultMetadata._fileId;
	}

	int ecLevelChar[2];
	if(ecLevel<10)
	{
		ecLevelChar[0] = ecLevel+48;
	}
	else if(ecLevel<100)
	{
		
		ecLevelChar[0] = (ecLevel%10)+48;
		ecLevelChar[1] = (ecLevel%100)/10+48;	
	}
	else{}

	//DecoderResult->_rawBytes = ;
	//DecoderResult->_numBits;
	DecoderResult->_text = result;
	DecoderResult->_ecLevel = ecLevelChar;
	DecoderResult->_structuredAppend = sai;
	DecoderResult->_readerInit = readerInit;
	DecoderResult->_extra = &resultMetadata;

}

