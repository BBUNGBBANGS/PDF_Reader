#ifndef _DECODESTATUS_H
#define _DECODESTATUS_H
typedef enum 
{
	NoError = 0,
	NotFound,
	FormatError,
	ChecksumError,
}DecodeStatus_t;

#define StatusIsOK(status) (status == NoError)
#define StatusIsError(status) (status != NoError)
#if 0
inline const char* ToString(DecodeStatus_t status)
{
	constexpr const char* names[] = {"NoError", "NotFound", "FormatError", "ChecksumError"};
	return names[static_cast<int>(status)];
}

#endif
#endif