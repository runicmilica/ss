#ifndef _SYMBOLTABLE_HPP_
#define _SYMBOLTABLE_HPP_

#include <string>
#include <iostream>
#include <map>
#include <vector>

using namespace std;

struct Info {
  long address;
  string sectionName;
  int type;   // 0 - pc relative, 1 - absolute
  Info() {
    address = 0; sectionName = ""; type = -1;
  }
};

struct SymbolTableEntry {
    string symbolName;
    int sectionNumber;
    int value;
    bool isGlobal;
    bool isExtern;
    int id;
    long size;
    vector<Info> usedHere;
    // use list
    SymbolTableEntry() {
      this->symbolName = "";
      this->sectionNumber = 0;
      this->value = 0;
      this->isGlobal = false;
      this->isExtern = false;
      this->id = 0;
      this->size = -1;
    }
  };

class SymbolTable {
  private:
  static int id;

  
  map<string, SymbolTableEntry> table;
  public:
  
  void add(string symName);
  string getPrintRow(string symName);
  SymbolTable();
  bool exists(string symName);
  void printSymbolTable(string filename, bool flag);
  void printSymbolTableForLinker(string filename);
  string decimalToHexadecimal(long num, int width);
  void addInfoToSymbol(string symbolName, Info info);
  map<string, SymbolTableEntry> getAllSymbols();
  // vector<Info> getInfoListForSymbol(string symName);
  // setters
  void setSectionNumber(string symName, int secNum);
  void setValue(string symName, int value);
  void setIsGlobal(string symName, bool isGlobal);
  void setSize(string symName, long size);
  void setSize(int id, long size);
  void setIsExtern(string symName, bool isExtern);
  // getters
  int getSectionNumber(string symName);
  int getValue(string symName);
  bool getIsGlobal(string symName);
  long getSize(string symName);
  long getSize(int id);
  bool getIsExtern(string symName);
  int getId(string symName);
  int getMaxId() {
    return id;
  }
};

#endif