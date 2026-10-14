#ifndef LITERAL_H
#define LITERAL_H

#include <iosfwd>
#include <string>

struct ParsedLiteral {
    int value = 0;
    bool valueFitsInt = true;
    std::string objectCode;
    int byteLength = 0;
};

struct LiteralNode {
    std::string name;
    std::string objectCode;
    int byteLength;
    int address;
    LiteralNode* next;
};

class LiteralTable {
public:
    LiteralTable();
    ~LiteralTable();

    LiteralTable(const LiteralTable&) = delete;
    LiteralTable& operator=(const LiteralTable&) = delete;

    bool add(const std::string& literal);
    void assignAddresses(int startAddress = 0);
    void print(std::ostream& output) const;

private:
    LiteralNode* head;
    LiteralNode* tail;
};

bool isLiteral(const std::string& s);
ParsedLiteral parseLiteral(const std::string& literal);

#endif
