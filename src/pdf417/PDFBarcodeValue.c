
#include "PDFBarcodeValue.h"

/**
* Add an occurrence of a value
*/
void setValue(int value)
{
	//_values[value] += 1;
}
#if 0
/**
* Determines the maximum occurrence of a set value and returns all values which were set with this occurrence.
* @return an array of int, containing the values with the highest occurrence, or null, if no value was set
*/
std::vector<int>
BarcodeValue::value() const
{
	std::vector<int> result;
	if (!_values.empty()) 
	{
		int maxConfidence = std::max_element(_values.begin(), _values.end(), [](auto& l, auto& r) { return l.second < r.second; })->second;
		for (auto [value, count] : _values)
			if (count == maxConfidence)
				result.push_back(value);
	}
	return result;
}

int
BarcodeValue::confidence(int value) const
{
	auto it = _values.find(value);
	return it != _values.end() ? it->second : 0;
}

#endif
