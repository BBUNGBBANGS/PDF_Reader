#ifndef _BARCODEFORMAT_H
#define _BARCODEFORMAT_H
typedef enum  
{
	// The values are an implementation detail. The c++ use-case (ZXing::Flags) could have been designed such that it
	// would not have been necessary to explicitly set the values to single bit constants. This has been done to ease
	// the interoperability with C-like interfaces, the python and the Qt wrapper.
	NONE            = 0,         ///< Used as a return value if no valid barcode has been detected
	Aztec           = (1 << 0),  ///< Aztec (2D)
	Codabar         = (1 << 1),  ///< Codabar (1D)
	Code39          = (1 << 2),  ///< Code39 (1D)
	Code93          = (1 << 3),  ///< Code93 (1D)
	Code128         = (1 << 4),  ///< Code128 (1D)
	DataBar         = (1 << 5),  ///< GS1 DataBar, formerly known as RSS 14
	DataBarExpanded = (1 << 6),  ///< GS1 DataBar Expanded, formerly known as RSS EXPANDED
	DataMatrix      = (1 << 7),  ///< DataMatrix (2D)
	EAN8            = (1 << 8),  ///< EAN-8 (1D)
	EAN13           = (1 << 9),  ///< EAN-13 (1D)
	ITF             = (1 << 10), ///< ITF (Interleaved Two of Five) (1D)
	MaxiCode        = (1 << 11), ///< MaxiCode (2D)
	PDF417          = (1 << 12), ///< PDF417 (1D) or (2D)
	QRCode          = (1 << 13), ///< QR Code (2D)
	UPCA            = (1 << 14), ///< UPC-A (1D)
	UPCE            = (1 << 15), ///< UPC-E (1D)

	OneDCodes = Codabar | Code39 | Code93 | Code128 | EAN8 | EAN13 | ITF | DataBar | DataBarExpanded | UPCA | UPCE,
	TwoDCodes = Aztec | DataMatrix | MaxiCode | PDF417 | QRCode,
	Any       = OneDCodes | TwoDCodes,
}BarcodeFormats_t;

#endif