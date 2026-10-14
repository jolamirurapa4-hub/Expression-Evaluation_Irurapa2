#include "SymbolTable.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cctype>
#include <limits>
#include <stdexcept>

bool SymbolTable::load(const std::string& filename) {
    std::ifstream fin(filename);
    if (!fin) {
        std::cerr << "ERROR: Cannot open " << filename << '\n';
        return false;
    }

    table.clear();
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(fin, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line == ":") break;
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string name;
        std::string valueText;
        std::string rflagStr;
        std::string extra;

        if (!(ss >> name >> valueText >> rflagStr) || (ss >> extra)) {
            std::cerr << "WARNING: Skipping malformed symbol at "
                      << filename << ':' << lineNumber << '\n';
            continue;
        }

        if (!name.empty() && name.back() == ':') {
            name.pop_back();
        }
        bool validName = !name.empty() && name.size() <= 6 &&
                         std::isalpha(static_cast<unsigned char>(name[0]));
        for (std::size_t i = 1; validName && i < name.size(); ++i) {
            validName = std::isalnum(static_cast<unsigned char>(name[i]));
        }
        int value = 0;
        try {
            std::size_t parsed = 0;
            const long long parsedValue = std::stoll(valueText, &parsed);
            if (parsed != valueText.size() ||
                parsedValue < std::numeric_limits<int>::min() ||
                parsedValue > std::numeric_limits<int>::max()) {
                validName = false;
            } else {
                value = static_cast<int>(parsedValue);
            }
        } catch (const std::exception&) {
            validName = false;
        }
        if (!validName ||
            (rflagStr != "0" && rflagStr != "1" &&
             rflagStr != "true" && rflagStr != "false")) {
            std::cerr << "WARNING: Skipping malformed symbol at "
                      << filename << ':' << lineNumber << '\n';
            continue;
        }

        Symbol sym;
        sym.name = name;
        sym.value = value;
        sym.mflag = false;

        sym.rflag = (rflagStr == "1" || rflagStr == "true");

        if (table.count(sym.name)) {
            table[sym.name].mflag = true;
            std::cerr << "ERROR: Duplicate symbol '" << sym.name
                      << "' at " << filename << ':' << lineNumber << '\n';
        } else {
            table[sym.name] = sym;
        }
    }

    return true;
}

bool SymbolTable::exists(const std::string& name) const {
    return table.count(name) > 0;
}

Symbol SymbolTable::get(const std::string& name) const {
    return table.at(name);
}

void SymbolTable::display(std::ostream& output) const {
    output << "SYMBOL TABLE\n"
           << std::left << std::setw(10) << "NAME"
           << std::right << std::setw(10) << "VALUE"
           << std::setw(10) << "RFLAG"
           << std::setw(10) << "MFLAG" << '\n';
    for (const auto& entry : table) {
        const Symbol& symbol = entry.second;
        output << std::left << std::setw(10) << symbol.name
               << std::right << std::setw(10) << symbol.value
               << std::setw(10) << (symbol.rflag ? 1 : 0)
               << std::setw(10) << (symbol.mflag ? 1 : 0) << '\n';
    }
}
