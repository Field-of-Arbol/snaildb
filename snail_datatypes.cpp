#include "snail_datatypes.h"

BaseSDataType::BaseSDataType(size_t max_size) : max_size(max_size) {}

BaseSDataType::~BaseSDataType() {}

std::string BaseSDataType::padStr(const std::string &value, size_t maxlength) const
{
    if (strValue.length() >= max_size)
    {
        return strValue.substr(0, max_size);
    }
    else
    {
        return std::string(max_size - strValue.length(), ' ') + strValue;
    }
}

std::string BaseSDataType::getValue() const
{
    return padStr(strValue, max_size);
}

StrCol::StrCol(size_t max_length) : BaseSDataType(max_length) {}

void StrCol::setValue(const std::string &new_string)
{
    if (new_string.length() <= max_size)
    {
        strValue = new_string;
    }
    else
    {
        throw std::runtime_error("Error: Input string exceeds the maximum length.");
    }
}

IntCol::IntCol(size_t max_length) : BaseSDataType(max_length)
{
    setValue(0);
}

void IntCol::setValue(int value)
{
    strValue = std::to_string(value);
    intValue = value;
}
