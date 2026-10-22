all: expr

expr: main.cpp expression.cpp literal.cpp SymbolTable.cpp
	g++ main.cpp expression.cpp literal.cpp SymbolTable.cpp -o expr

clean:
	rm -f expr
