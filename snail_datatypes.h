#ifndef SNAILDT_H
#define SNAILDT_H

#include <string>
#include <stdexcept>
class BaseSDataType
{
public:
    BaseSDataType(size_t max_size);
    virtual ~BaseSDataType();

    std::string getValue() const;
    std::string prefix(int length) const;
    std::string padStr(const std::string &value, size_t maxlength) const;

protected:
    size_t max_size;
    std::string strValue;
};

class StrCol : public BaseSDataType
{
public:
    StrCol(size_t max_length);

    void setValue(const std::string &new_string);
};

class IntCol : public BaseSDataType
{
public:
    IntCol(size_t max_length);

    void setValue(int value);

private:
    int intValue;
};

#endif // SNAILDT_H