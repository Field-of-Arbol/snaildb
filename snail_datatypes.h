#ifndef SNAILDT_H
#define SNAILDT_H

#include <stdexcept>
#include <string>
class SnailDataType
{
public:
    SnailDataType(size_t max_size);
    virtual ~SnailDataType();

    std::string getValue() const;
    std::string prefix(int length) const;
    std::string padStr(const std::string &value, size_t maxlength) const;

protected:
    size_t max_size;
    std::string strValue;
};

class StrCol : public SnailDataType
{
public:
    StrCol(size_t max_length);

    void setValue(const std::string &new_string);
};

class IntCol : public SnailDataType
{
public:
    IntCol(size_t max_length);

    void setValue(int value);

private:
    int intValue;
};

#endif // SNAILDT_H