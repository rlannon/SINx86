# makefile
# build the SIN compile

cc=g++
cpp_version=c++17
flags=-std=$(cpp_version) -g
target=sinx86

parser_dependencies=lexer.o parseexpression.o parsestatement.o parserutil.o statement.o expression.o
util_dependencies=datatype.o exceptions.o binaryio.o

build_depend=parser.o $(parser_dependencies) $(util_dependencies)

default: $(target)

# Build the whole program

sinx86: $(build_depend) main.cpp
	$(cc) $(flags) -o $(target) -g main.cpp $(build_depend)

# Build the compiler
# todo: build compiler

# Build the parser

parser.o: $(parser_dependencies) $(util_dependencies)
	$(cc) $(flags) -o parser.o -c parser/Parser.cpp

parseexpression.o: expression.o datatype.o exceptions.o
	$(cc) $(flags) -o parseexpression.o -c parser/ParseExpression.cpp

parserutil.o: statement.o expression.o datatype.o exceptions.o
	$(cc) $(flags) -o parserutil.o -c parser/ParserUtil.cpp

parsestatement.o: statement.o expression.o datatype.o exceptions.o
	$(cc) $(flags) -o parsestatement.o -c parser/ParseStatement.cpp

statement.o: expression.o datatype.o
	$(cc) $(flags) -o statement.o -c parser/Statement.cpp

expression.o: datatype.o
	$(cc) $(flags) -o expression.o -c parser/Expression.cpp

lexer.o: parser/Lexer.h
	$(cc) $(flags) -o lexer.o -c parser/Lexer.cpp

# Utilities

datatype.o: util/DataType.h util/EnumeratedTypes.h util/data_widths.h
	$(cc) $(flags) -o datatype.o -c util/DataType.cpp

exceptions.o: util/Exceptions.h util/CompilerErrorCodes.h
	$(cc) $(flags) -o exceptions.o -c util/Exceptions.cpp

binaryio.o:
	$(cc) $(flags) -o binaryio.o -c util/BinaryIO/BinaryIO.cpp

# cleanup
clean:
	rm *.o
