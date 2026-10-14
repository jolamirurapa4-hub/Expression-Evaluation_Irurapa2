#include "literal.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <utility>

bool isLiteral(const std::string& s) {
    return !s.empty() && s[0] == '=';
}

ParsedLiteral parseLiteral(const std::string& literal) {
    if (literal.size() < 4 || literal[0] != '=' ||
        literal[2] != '\'' || literal.back() != '\'') {
        throw std::invalid_argument("Malformed literal: " + literal);
    }

    const std::string contents = literal.substr(3, literal.size() - 4);
    const char type = static_cast<char>(
        std::toupper(static_cast<unsigned char>(literal[1])));
    ParsedLiteral parsed;

    if (type == 'C') {
        if (contents.empty()) {
            throw std::invalid_argument("Character literal is empty: " + literal);
        }
        unsigned long long value = 0;
        bool valueFits = true;
        static const char hexDigits[] = "0123456789ABCDEF";
        for (unsigned char ch : contents) {
            if (value > (static_cast<unsigned long long>(
                             std::numeric_limits<int>::max()) - ch) / 256) {
                valueFits = false;
            } else if (valueFits) {
                value = (value << 8) | ch;
            }
            parsed.objectCode += hexDigits[ch >> 4];
            parsed.objectCode += hexDigits[ch & 0x0F];
        }
        if (valueFits) {
            parsed.value = static_cast<int>(value);
        } else {
            parsed.valueFitsInt = false;
        }
        parsed.byteLength = static_cast<int>(contents.size());
        return parsed;
    }

    if (type == 'X') {
        if (contents.empty()) {
            throw std::invalid_argument("Hex literal is empty: " + literal);
        }
        for (unsigned char ch : contents) {
            if (!std::isxdigit(ch)) {
                throw std::invalid_argument("Invalid hex literal: " + literal);
            }
        }

        std::string objectCode = contents;
        for (char& ch : objectCode) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
        if (objectCode.size() % 2 != 0) {
            objectCode.insert(objectCode.begin(), '0');
        }
        unsigned long long value = 0;
        bool valueFits = true;
        for (char ch : objectCode) {
            const unsigned int nibble = std::isdigit(static_cast<unsigned char>(ch))
                ? static_cast<unsigned int>(ch - '0')
                : static_cast<unsigned int>(
                    std::toupper(static_cast<unsigned char>(ch)) - 'A' + 10);
            if (value > (static_cast<unsigned long long>(
                             std::numeric_limits<int>::max()) - nibble) / 16) {
                valueFits = false;
            } else if (valueFits) {
                value = (value << 4) | nibble;
            }
        }
        if (valueFits) {
            parsed.value = static_cast<int>(value);
        } else {
            parsed.valueFitsInt = false;
        }
        parsed.objectCode = std::move(objectCode);
        parsed.byteLength = static_cast<int>(parsed.objectCode.size() / 2);
        return parsed;
    }

    throw std::invalid_argument("Unsupported literal type: " + literal);
}

LiteralTable::LiteralTable() : head(nullptr), tail(nullptr) {}

LiteralTable::~LiteralTable() {
    while (head != nullptr) {
        LiteralNode* node = head;
        head = head->next;
        delete node;
    }
}

bool LiteralTable::add(const std::string& literal) {
    ParsedLiteral parsed = parseLiteral(literal);
    for (LiteralNode* node = head; node != nullptr; node = node->next) {
        if (node->name == literal) {
            return false;
        }
    }

    LiteralNode* node = new LiteralNode{
        literal, parsed.objectCode, parsed.byteLength, -1, nullptr
    };
    if (tail == nullptr) {
        head = node;
    } else {
        tail->next = node;
    }
    tail = node;
    return true;
}

void LiteralTable::assignAddresses(int startAddress) {
    if (startAddress < 0) {
        throw std::invalid_argument("Literal table start address cannot be negative");
    }

    long long address = startAddress;
    for (LiteralNode* node = head; node != nullptr; node = node->next) {
        if (address > std::numeric_limits<int>::max()) {
            throw std::overflow_error("Literal table address exceeds supported range");
        }
        node->address = static_cast<int>(address);
        ++address;
    }
}

void LiteralTable::print(std::ostream& output) const {
    std::size_t nameWidth = std::string("NAME").size();
    std::size_t valueWidth = std::string("VALUE").size();
    for (const LiteralNode* node = head; node != nullptr; node = node->next) {
        nameWidth = std::max(nameWidth, node->name.size());
        valueWidth = std::max(valueWidth, node->objectCode.size());
    }
    nameWidth += 2;
    valueWidth += 2;

    output << "LITERAL TABLE\n"
           << std::left << std::setw(static_cast<int>(nameWidth)) << "NAME"
           << std::setw(static_cast<int>(valueWidth)) << "VALUE"
           << std::setw(8) << "LENGTH"
           << "ADDRESS\n";
    for (const LiteralNode* node = head; node != nullptr; node = node->next) {
        output << std::left << std::setw(static_cast<int>(nameWidth)) << node->name
               << std::setw(static_cast<int>(valueWidth)) << node->objectCode
               << std::setw(8) << node->byteLength;
        if (node->address < 0) {
            output << "UNASSIGNED\n";
        } else {
            output << node->address << '\n';
        }
    }
}
