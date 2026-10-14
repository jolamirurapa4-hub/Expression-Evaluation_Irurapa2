
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include "expression.h"
#include "literal.h"
#include "SymbolTable.h"

int main(int argc, char* argv[]) {
    std::string filename;

    if (argc == 1) {
        std::cout << "Enter expression file name: ";
        std::cin >> filename;
    } else {
        filename = argv[1];
    }

    std::ifstream fin(filename);
    if (!fin) {
        std::cout << "Error: Could not open file " << filename << std::endl;
        return 1;
    }

    SymbolTable symbols;
    if (!symbols.load("SYMS.DAT")) {
        return 1;
    }

    std::cout << std::left
              << std::setw(16) << "EXPRESSION"
              << std::setw(12) << "VALUE"
              << std::setw(14) << "RELOCATABLE"
              << std::setw(7) << "N-Bit"
              << std::setw(7) << "I-Bit"
              << "X-Bit\n";

    LiteralTable literals;
    std::string line;
    while (std::getline(fin, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        ParsedExpr parsed;
        try {
            parsed = parseOperands(line);
        } catch (const std::exception& error) {
            const ExprResult result = evaluateExpression(line, symbols);
            std::cout << std::left
                      << std::setw(16) << line
                      << std::setw(12) << "ERROR"
                      << std::setw(14) << "-"
                      << std::setw(7) << result.nBit
                      << std::setw(7) << result.iBit
                      << result.xBit << '\n';
            std::cerr << "ERROR evaluating \"" << line << "\": "
                      << error.what() << '\n';
            continue;
        }

        const ExprResult result = evaluateExpression(line, symbols);
        if (parsed.op2.empty() && isLiteral(parsed.op1)) {
            if (!result.error) {
                literals.add(parsed.op1);
            } else {
                std::cerr << "ERROR evaluating literal \"" << line << "\": "
                          << result.errorMessage << '\n';
            }
            continue;
        }
        if (result.error) {
            std::cout << std::left
                      << std::setw(16) << line
                      << std::setw(12) << "ERROR"
                      << std::setw(14) << "-"
                      << std::setw(7) << result.nBit
                      << std::setw(7) << result.iBit
                      << std::setw(7) << result.xBit
                      << std::setw(14) << "-"
                      << "-\n";
            std::cerr << "ERROR evaluating \"" << line << "\": "
                      << result.errorMessage << '\n';
            continue;
        }

        if (isLiteral(parsed.op1)) {
            literals.add(parsed.op1);
        }
        if (!parsed.op2.empty() && isLiteral(parsed.op2)) {
            literals.add(parsed.op2);
        }

        std::cout << std::left
                  << std::setw(16) << line
                  << std::setw(12) << result.value
                  << std::setw(14) << (result.rflag ? "RELATIVE" : "ABSOLUTE")
                  << std::setw(7) << result.nBit
                  << std::setw(7) << result.iBit
                  << result.xBit << '\n';
    }

    symbols.display(std::cout);
    literals.assignAddresses();
    literals.print(std::cout);
    return 0;
}
