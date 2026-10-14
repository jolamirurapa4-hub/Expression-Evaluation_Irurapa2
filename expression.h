#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <string>

class SymbolTable;

struct ParsedExpr {
    std::string op1;
    std::string op2;
    char op1op = '\0';
};

struct ExprResult {
    int nBit = 1;
    int iBit = 1;
    int xBit = 0;
    int value = 0;
    int rflag = 0;
    bool error = false;
    std::string errorMessage;
    std::string objectCode;
    int objectLength = 0;
};

struct ValueResult {
    int value = 0;
    int rflag = 0;
    bool error = false;
    std::string errorMessage;
    std::string objectCode;
    int objectLength = 0;
};

ExprResult evaluateExpression(const std::string& expr);
ExprResult evaluateExpression(const std::string& expr, SymbolTable& symtab);
ParsedExpr parseOperands(const std::string& expr);
ValueResult getValue(const std::string& token, SymbolTable& symtab);
int computeRflag(int r1, int r2, char op, bool& error);

#endif
