# makefile
# build the SIN compile

cc=g++
cpp_version=c++17
flags=-std=$(cpp_version) -g
target=sinx86
bin=bin/

# object file dependencies
compiler_objs=allocation.o assign.o evaluate_expression.o function_symbol.o symbol.o
compiler_util_objs=register_usage.o utilities.o
parser_objs=lexer.o parseexpression.o parsestatement.o parserutil.o statement.o expression.o
util_objs=datatype.o exceptions.o binaryio.o
build_objs=parser.o $(parser_objs) $(util_objs)

# source file dependencies
compiler_dependencies=compile/*
parser_dependencies=parser/*
util_dependencies=util/*
all_dependencies=$(parser_dependencies) $(util_dependencies)

# todo: simplify builds, dependencies, etc.

default: $(target)

# Build the whole program

$(target): $(build_objs)
	@echo "Building target..."
	$(cc) $(flags) -o $@ main.cpp $^
	@echo "Build successful!"

# Build the compiler
compiler.o: $(compiler_objs) $(parser_objs) $(util_objs) $(parser_dependencies) $(compiler_dependencies)
	$(cc) $(flags) -o compiler.o -c compile/compile.cpp

allocation.o: compile/allocation.cpp compile/compile.h
	$(cc) $(flags) -o allocation.o -c compile/allocation.cpp

assign.o: compile/assign.cpp compile/compile.h
	$(cc) $(flags) -o assign.o -c compile/assign.cpp

evaluate_expression.o: compile/evaluate_expression.cpp compile/compile.h
	$(cc) $(flags) -o evaluate_expression.o -c compile/evaluate_expression.cpp

function_symbol.o: compile/symbol.cpp compile/function_symbol.cpp compile/symbol.h
	$(cc) $(flags) -o function_symbol.o -c compile/function_symbol.cpp

symbol.o: compile/symbol.cpp compile/symbol.h
	$(cc) $(flags) -o symbol.o -c compile/symbol.cpp

# compiler utilities
register_usage.o: compile/compile_util/register_usage.cpp compile/compile_util/register_usage.h
	$(cc) $(flags) -o register_usage.o -c compile/compile_util/register_usage.cpp

utilities.o: compile/compile_util/*
	$(cc) $(flags) -o utilities.o -c compile/compile_util/utilities.cpp

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
	rm *.o

.PHONY: $(target) clean
