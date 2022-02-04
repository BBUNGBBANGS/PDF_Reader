
#ifndef _BYTEARRAY_H
#define _BYTEARRAY_H
#if 0
/**
	ByteArray is an extension of std::vector<unsigned char>.
*/
class ByteArray : public std::vector<uint8_t>
{
public:
	ByteArray() = default;
	ByteArray(std::initializer_list<uint8_t> list) : std::vector<uint8_t>(list) {}
	explicit ByteArray(int len) : std::vector<uint8_t>(len, 0) {}
}
#endif
#endif