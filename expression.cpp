#include "expression.h"
#include "SymbolTable.h"
#include "literal.h"

#include <cctype>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::string trim(const std::string& value) {
    const std::size_t start = value.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }

    const std::size_t end = value.find_last_not_of(" \t\n\r");
    return value.substr(start, end - start + 1);
}

bool isNumber(const std::string& value) {
    if (value.empty()) {
        return false;
    }
    for (unsigned char ch : value) {
        if (!std::isdigit(ch)) {
            return false;
        }
    }
    return true;
}

bool stripIndexedSuffix(std::string& expr) {
    bool inLiteral = false;
    for (std::size_t i = 0; i < expr.size(); ++i) {
        if (expr[i] == '\'') {
            inLiteral = !inLiteral;
        } else if (!inLiteral && expr[i] == ',') {
            const std::string suffix = trim(expr.substr(i));
            if (suffix != ",X" && suffix != ",x") {
                throw std::invalid_argument("Malformed indexed addressing suffix");
            }
            expr = trim(expr.substr(0, i));
            return true;
        }
    }
    return false;
}

}

int computeRflag(int r1, int r2, char op, bool& error) {
    error = false;

    if (op == '+') {
        if (r1 == 1 && r2 == 1) {
            error = true;
            return 0;
        }
        return (r1 || r2);
    }

    if (op == '-') {
        if (r1 == 0 && r2 == 1) {
            error = true;
            return 0;
        }
        if (r1 == 1 && r2 == 1) {
            return 0;
        }
        return r1;
    }

    return 0;
}

ValueResult getValue(const std::string& token, SymbolTable& symtab) {
    ValueResult result;
    try {
        if (isLiteral(token)) {
            const ParsedLiteral literal = parseLiteral(token);
            if (!literal.valueFitsInt) {
                throw std::out_of_range(
                    "Literal value exceeds the supported expression range: " + token);
            }
            result.value = literal.value;
            result.objectCode = literal.objectCode;
            result.objectLength = literal.byteLength;
        } else if (isNumber(token)) {
            const long long value = std::stoll(token);
            if (value > std::numeric_limits<int>::max()) {
                throw std::out_of_range("integer operand exceeds supported range");
            }
            result.value = static_cast<int>(value);
        } else if (symtab.exists(token)) {
            const Symbol symbol = symtab.get(token);
            if (symbol.mflag) {
                result.error = true;
                result.errorMessage = "Multiply defined symbol: " + token;
                return result;
            }
            result.value = symbol.value;
            result.rflag = symbol.rflag ? 1 : 0;
        } else {
            result.error = true;
            result.errorMessage = "Unknown operand: " + token;
        }
    } catch (const std::exception& error) {
        result.error = true;
        result.errorMessage = error.what();
    }
    return result;
}

ParsedExpr parseOperands(const std::string& expr) {
    std::vector<std::string> operands;
    std::vector<char> operators;
    std::string input = trim(expr);
    if (!input.empty() && (input[0] == '@' || input[0] == '#')) {
        input.erase(0, 1);
    }

    stripIndexedSuffix(input);

    std::string operand;
    bool inLiteral = false;

    for (char ch : input) {
        if (ch == '\'') {
            inLiteral = !inLiteral;
        }

        if (!inLiteral && (ch == '+' || ch == '-')) {
            std::string parsedOperand = trim(operand);
            if (parsedOperand.empty()) {
                throw std::invalid_argument("Expression contains an empty operand");
            }
            operands.push_back(parsedOperand);
            operators.push_back(ch);
            operand.clear();
        } else {
            operand += ch;
        }
    }

    if (inLiteral) {
        throw std::invalid_argument("Expression contains an unterminated literal");
    }

    operand = trim(operand);
    if (operand.empty()) {
        throw std::invalid_argument("Expression contains an empty operand");
    }
    operands.push_back(operand);

    if (operands.size() > 2) {
        throw std::invalid_argument("Expression has more than two operands");
    }

    ParsedExpr parsed{};
    parsed.op1 = operands[0];
    if (operands.size() > 1) {
        parsed.op2 = operands[1];
        parsed.op1op = operators[0];
    }
    return parsed;
}

ExprResult evaluateExpression(const std::string& expr, SymbolTable& symtab) {
    ExprResult r;

    const std::string s = trim(expr);

    if (!s.empty() && s[0] == '@') {
        r.nBit = 1;
        r.iBit = 0;
    }
    else if (!s.empty() && s[0] == '#') {
        r.nBit = 0;
        r.iBit = 1;
    }

    ParsedExpr parsed;
    bool indexed = false;
    try {
        std::string indexCheck = s;
        indexed = stripIndexedSuffix(indexCheck);
        parsed = parseOperands(s);
    } catch (const std::exception& error) {
        r.error = true;
        r.errorMessage = error.what();
        return r;
    }
    r.xBit = indexed ? 1 : 0;
    if ((s.empty() || (s[0] != '@' && s[0] != '#')) &&
        isNumber(parsed.op1)) {
        r.nBit = 0;
        r.iBit = 1;
    }

    if (parsed.op2.empty() && isLiteral(parsed.op1)) {
        try {
            const ParsedLiteral literal = parseLiteral(parsed.op1);
            if (literal.valueFitsInt) {
                r.value = literal.value;
            }
            r.rflag = 0;
            r.objectCode = literal.objectCode;
            r.objectLength = literal.byteLength;
            return r;
        } catch (const std::exception& error) {
            r.error = true;
            r.errorMessage = error.what();
            return r;
        }
    }

    ValueResult v1 = getValue(parsed.op1, symtab);
    if (v1.error) {
        r.error = true;
        r.errorMessage = v1.errorMessage;
        return r;
    }

    r.value = v1.value;
    r.rflag = v1.rflag;
    ValueResult v2;
    if (!parsed.op2.empty()) {
        v2 = getValue(parsed.op2, symtab);
        if (v2.error) {
            r.error = true;
            r.errorMessage = v2.errorMessage;
            return r;
        }

    }

    bool error = false;

    // If OP2 exists, evaluate OP1 op OP2
    if (!parsed.op2.empty()) {
        const long long combined = parsed.op1op == '+'
            ? static_cast<long long>(r.value) + v2.value
            : static_cast<long long>(r.value) - v2.value;
        if (combined < std::numeric_limits<int>::min() ||
            combined > std::numeric_limits<int>::max()) {
            r.error = true;
            r.errorMessage = "Integer overflow while evaluating expression";
            return r;
        }
        r.value = static_cast<int>(combined);

        bool rErr = false;
        r.rflag = computeRflag(r.rflag, v2.rflag, parsed.op1op, rErr);
        if (rErr) error = true;
    }

    if (error) {
        r.error = true;
        r.errorMessage = "Invalid relocation arithmetic";
    }

    return r;
}

ExprResult evaluateExpression(const std::string& expr) {
    static SymbolTable symtab;
    static bool loaded = false;
    if (!loaded) {
        loaded = symtab.load("SYMS.DAT");
    }
    if (!loaded) {
        ExprResult result;
        result.error = true;
        result.errorMessage = "Could not load SYMS.DAT";
        return result;
    }
    return evaluateExpression(expr, symtab);
}
