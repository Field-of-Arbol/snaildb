#ifndef SNAILDT_H
#define SNAILDT_H

#include <string>
#include <stdexcept> 

class BaseSDataType
{
public:
    BaseSDataType(size_t max_size);
    virtual ~BaseSDataType();
    virtual std::string getValue() const = 0;

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
    std::string getValue() const override;
};

class IntCol : public BaseSDataType
{
public:
    IntCol(size_t max_length);

    void setValue(int value);
    std::string getValue() const override;

private:
    int intValue;
};

#endif // SNAILDT_H