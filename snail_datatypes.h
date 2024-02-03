#ifndef SNAILDT_H
#define SNAILDT_H

#include <stdexcept>
#include <string>

// snaildb.h
#include <vector>

enum ColumnType
{
    STR_TYPE,
    INT_TYPE
};

struct ColumnInfo
{
    std::string name;
    size_t max_length;
    ColumnType type;
};

class SnailDataType
{
public:
    SnailDataType(size_t max_size);
    virtual ~SnailDataType();

    virtual void setValue(const std::string &new_value);

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

    void setValue(const std::string &new_string) override;
};

class IntCol : public SnailDataType
{
public:
    IntCol(size_t max_length);

    using SnailDataType::setValue; // Bring the base class's setValue into the scope

    void setValue(int value);

private:
    int intValue;
};

#endif // SNAILDT_H