#ifndef SNAILDTYPES_H
#define SNAILDTYPES_H

#include <string>

class BaseSDataType
{
public:
    BaseSDataType(size_t max_size) : max_size(max_size) {}
    virtual ~BaseSDataType() {}
    virtual std::string getValue() const { return padStr(); }

protected:
    size_t max_size;
    std::string strValue;
    std::string padStr() const
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
};

class StrCol : public BaseSDataType
{
public:
    StrCol(size_t max_length) : BaseSDataType(max_length) {}

    void setValue(const std::string &new_string)
    {
        if (new_string.length() <= max_size)
        {
            strValue = new_string;
        }
        else
        {
            std::cerr << "Error: Input string exceeds the maximum length."
                      << std::endl;
        }
    }
};

class IntCol : public BaseSDataType
{
public:
    IntCol(size_t max_length) : BaseSDataType(max_length) { setValue(0); }
    void setValue(int value)
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
        strValue = sValue;
    }

private:
    int intValue;
};

#endif // SNAILDTYPES