# makefile
# build the SIN compiler

cc=g++

# Build the compiler
#
# todo:
# Consider the following:
#	symbol_table.o requires:
#		symbol.o
#		datatype.o
#	symbol.o requires:
#		datatype.o
#	Does symbol_table.o need to list datatype.o as a dependency?

default: sinx86

sinx86: compiler.o
	$(cc) -o sinx86 main.cpp

compiler.o: compile/compile.cpp compile/compile.h allocate.o parser.o exceptions.o datatype.o
	$(cc) -o compiler.o -c compile/compiler.cpp

allocate.o: compile/allocation.cpp compile/compile.h statement.o symbol.o datatype.o
	$(cc) -o allocate.o -c compile/allocation.cpp

symbol.o: compile/symbol.cpp compile/symbol.h datatype.o
	$(cc) -o symbol.o -c compile/symbol.cpp

# Build the parser

parser.o: parser/Parser.cpp parser/Parser.h parserutil.o parsestatement.o parseexpression.o exceptions.o datatype.o
	$(cc) -o parser.o -c parser/Parser.cpp

parserutil.o: parser/ParserUtil.cpp parser/Parser.h datatype.o statement.o expression.o
	$(cc) -o parserutil.o -c parser/ParserUtil.cpp

parsestatement.o: parser/ParseStatement.cpp parser/Parser.h statement.o datatype.o lexer.o
	$(cc) -o parsestatement.o -c parser/ParseStatement.cpp

parseexpression.o: parser/ParseExpression.cpp parser/Parser.h expression.o lexer.o
	$(cc) -o parseexpression.o -c parser/ParseExpression.cpp

statement.o: parser/Statement.cpp parser/Expression.h parser/Statement.h datatype.o
	$(cc) -o statement.o -c parser/Statement.cpp

expression.o: parser/Expression.cpp parser/Expression.h datatype.o
	$(cc) -o expression.o -c parser/Expression.cpp

lexer.o: parser/Lexer.cpp parser/Lexer.h
	$(cc) -o lexer.o -c parser/Lexer.cpp

# phase out 'TypeData', as the 'DataType' class is far better
# typedata.o: parser/TypeData.cpp parser/TypeData.h util/EnumeratedTypes.h
# 	$(cc) -o typedata.o -c parser/TypeData.cpp

# Build our utilities
binaryio.o: util/BinaryIO/BinaryIO.cpp util/BinaryIO/BinaryIO.h
	$(cc) -o binaryio.o -c util/BinaryIO/BinaryIO.cpp

datatype.o: util/DataType.cpp util/DataType.h EnumeratedTypes.h
	$(cc) -o datatype.o -c util/DataType.cpp

exceptions.o: util/Exceptions.cpp util/Exceptions.h
	$(cc) -o exceptions.o -c util/Exceptions.cpp

clean:
	rm *.o
