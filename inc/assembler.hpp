#ifndef ASSEMBLER_H_
#define ASSEMBLER_H_

#include <stdio.h>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include "symbolTable.hpp"
#include "relocationTable.hpp"

using namespace std;

struct LiteralTableEntry {
  int value;
  string hexValue;
  bool isSymbol;
  string symbolName;
  int address;
  int size;

  LiteralTableEntry(int value, string hexValue, bool isSymbol, string symbolName, 
      int address, int size) {
    this->value = value; 
    this->hexValue = hexValue; 
    this->isSymbol = isSymbol;
    this->symbolName = symbolName;
    this->address = address;
    this->size = size;
  }
};

struct LiteralUse {
  int address;
  int section;
  int value;
  LiteralUse(int a, int s, int v) {
    address = a; section = s; value = v;
  }
};

struct SectionTableEntry{
  string sectionName;
  long startAddress;
  long size;
  vector<string> code;
  RelocationTable* relocTable;
  vector<LiteralTableEntry> literalTable;
  long number;
  SectionTableEntry(){
    sectionName = ""; startAddress = 0; size = 0; number = 0;
  }
};


class Assembler {
  private:
  static int id;
  string inputFile;
  string outputFile;
  // static map<string, SymbolTableEntry> symbolTable;
  int currentSectionNumber;
  long locationCounter;
  SymbolTable symbolTable;
  map<string, SectionTableEntry> sectionTable;
  vector<LiteralUse> literalUse;
  // relocation table for each section
  // map<string, RelocationTable*> relocationTables;

  public:
  Assembler(string, string);
  static bool checkStart(int argc, char *argv[]);
  int assemble();

  bool sectionCheck(string& line);
  bool endCheck(string line);
  bool wordCheck(string& line);
  bool globalCheck(string& line);
  bool externCheck(string& line);
  bool skipCheck(string& line);
  bool emptyLineCheck(string line);
  // bool labelAtBeginningCheck(string label);
  // bool commentAtTheBeginningCheck(string line);
  void removeComment(string& comm);
  int labelCheck(string& label);
  int instructionCheck(string instruction);

  void fillRelocationTables();
  void setSectionSize(int num, long size);
  SectionTableEntry* getCurrentSectionTableEntry(int num);
  int processLabel(string labelName);
  int global(string symbols);
  int processExtern(string symbols);
  void printSymbolTableIntoFile();
  void printSectionTableIntoFile();
  // void printSymbolUseInfoTableIntoFile();
  int section(string sectionName);
  int word(string params);
  int skip(string literal);
  string decimalToHexadecimal(int num);
  string decimalToHexadecimalNoFill(long num);
  bool convertStringToNumber(string stringNum, int* number); // returns false if its symbol
  void addSymbolToLiteralTableIfNotAlreadyIn(string symbolName);
  string getRegNum(string reg);
};

#endif