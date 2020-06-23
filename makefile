SRC_DIR=.
PARSER_DIR=./parser
OBJ_DIR=./bin
SRC_FILES=$(wildcard $(SRC_DIR)/parser/*.cpp $(SRC_DIR)/util/*.cpp $(SRC_DIR)/compile/*.cpp $(SRC_DIR)/compile/compile_util/*.cpp)
OBJ_FILES=$(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $(SRC_FILES)))
cc=g++
cppversion=c++17
flags=-std=$(cppversion) -g
target=sinx86

default: $(target)

$(target): $(OBJ_FILES)
	@echo Finishing build...
	$(cc) $(flags) -o $@ main.cpp $^
	@echo Done.

$(OBJ_DIR)/%.o: $(SRC_DIR)/parser/%.cpp
	$(cc) $(flags) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/util/%.cpp
	$(cc) $(flags) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/compile/%.cpp
	$(cc) $(flags) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/compile/compile_util/%.cpp
	$(cc) $(flags) -c -o $@ $<

clean:
	rm bin/*.o

.PHONY: $(target) clean
