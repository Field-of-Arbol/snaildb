#include "snail_datatypes.h"

SnailDataType::SnailDataType(size_t max_size) : max_size(max_size) {}

SnailDataType::~SnailDataType() {}

void SnailDataType::setValue(const std::string &new_value)
{
    if (new_value.length() <= max_size)
    {
        strValue = new_value;
    }
    else
    {
        throw std::runtime_error("Error: Input string exceeds the maximum length.");
    }
}

std::string SnailDataType::padStr(const std::string &value,
                                  size_t maxlength) const
{

    if (value.length() >= max_size)
    {
        return value.substr(0, max_size);
    }
    else
    {
        return std::string(max_size - value.length(), ' ') + value;
    }
}

std::string SnailDataType::getValue() const
{
    return padStr(strValue, max_size);
}

StrCol::StrCol(size_t max_length) : SnailDataType(max_length) {}

void StrCol::setValue(const std::string &new_string)
{
    SnailDataType::setValue(new_string);
}

IntCol::IntCol(size_t max_length) : SnailDataType(max_length) { setValue(0); }

void IntCol::setValue(int value)
{
    strValue = std::to_string(value);
    intValue = value;
}