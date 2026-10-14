#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <string>
#include <map>
#include <iosfwd>

struct Symbol {
    std::string name;
    int value;
    bool rflag;
    bool mflag;
};

class SymbolTable {
private:
    std::map<std::string, Symbol> table;

public:
    bool load(const std::string& filename);
    bool exists(const std::string& name) const;
    Symbol get(const std::string& name) const;
    void display(std::ostream& output) const;
};

#endif
