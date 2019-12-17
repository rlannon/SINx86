# makefile
# build the SIN compile

cc=g++
cpp_version=c++17
flags=-std=$(cpp_version) -g
target=sinx86
bin=bin/

# object file dependencies
parser_objs=lexer.o parseexpression.o parsestatement.o parserutil.o statement.o expression.o
util_objs=datatype.o exceptions.o binaryio.o
build_objs=parser.o $(parser_objs) $(util_objs)

# source file dependencies
parser_dependencies=parser/*
util_dependencies=util/*
all_dependencies=$(parser_dependencies) $(util_dependencies)

# todo: simplify builds, dependencies, etc.

build: $(target)

default: $(target)

# Build the whole program

$(target): $(build_objs)
	@echo "Building target..."
	$(cc) $(flags) -o $@ main.cpp $^
	@echo "Build successful!"

# Build the compiler
# todo: build compiler

# Build the parser

parser.o: $(parser_objs) $(util_objs) $(parser_dependencies)
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
	rm sinx86
	rm *.o

.PHONY: build clean
