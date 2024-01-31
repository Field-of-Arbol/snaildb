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

std::string StrCol::getValue() const
{
    return strValue;
}

IntCol::IntCol(size_t max_length) : BaseSDataType(max_length)
{
    setValue(0);
}

void IntCol::setValue(int value)
{
    std::string sValue = std::to_string(value);

    // Trim or pad the string to fit the specified max_length
    if (sValue.length() > max_size)
    {
        strValue = sValue.substr(0, max_size);
    }
    else
    {
        strValue.resize(max_size, '0');
    }
    intValue = value;
}

std::string IntCol::getValue() const
{
    return strValue;
}