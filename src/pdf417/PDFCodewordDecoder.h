#ifndef _PDFCODEWORDDECODER_H
#define _PDFCODEWORDDECODER_H
/**
* @author Guenther Grau
* @author creatale GmbH (christoph.schulz@creatale.de)
*/
#if 0
class CodewordDecoder
{
public:
	static const int NUMBER_OF_CODEWORDS = 929;
	// Maximum Codewords (Data + Error).
	static const int MAX_CODEWORDS_IN_BARCODE = NUMBER_OF_CODEWORDS - 1;
	// One left row indication column + max 30 data columns + one right row indicator column
	//public static final int MAX_CODEWORDS_IN_ROW = 32;
	static const int MODULES_IN_CODEWORD = 17;
	static const int BARS_IN_MODULE = 8;

	/**
	* @param symbol encoded symbol to translate to a codeword
	* @return the codeword corresponding to the symbol.
	*/
	static int GetCodeword(int symbol);

	static int GetDecodedValue(const std::array<int, BARS_IN_MODULE>& moduleBitCount);
};
#endif

extern const int MODULES_IN_CODEWORD;
extern const int BARS_IN_MODULE;
extern int GetDecodedValue(const int * moduleBitCount);
extern int GetCodeword(int symbol);
#endif
