#ifndef LINKER_H_
#define LINKER_H_

#include <stdio.h>
#include <string>
#include <iostream>
#include <map>
#include <vector>

using namespace std;

class Linker {

private:
  vector<string> inputFiles;
  string outputFile;
  map<string, string> sectionAddr;  // for -place=section_name@address
  map<string, long> sectionSize;    // calculates total size of sections
  vector<string> sectionOrder;   
  vector<string> sectionOrderForPrint;   
  

  struct SymbolTableEntry {
    string symbolName;
    int id;
    int sec;
    long value;
    int size;

    SymbolTableEntry() : id(0), sec(0), value(0), size(0) {}

    SymbolTableEntry(const string& symName, int id, int sec, long value, int size)
        : symbolName(symName), id(id), sec(sec), value(value), size(size) {}
  };

  struct RelocationEntry {
      long offset;
      int symTabRef;
      long addend;
      int type;

      RelocationEntry(long offset, int symTabRef, long addend, int type)
          : offset(offset), symTabRef(symTabRef), addend(addend), type(type) {}
  };

  struct LiteralPoolEntry {
      long hexValue;
      string symName;
      long address;
      long size;
      int isSym;

      LiteralPoolEntry(long hexValue, const string& symName, long address, long size, int isSym)
          : hexValue(hexValue), symName(symName), address(address), size(size), isSym(isSym) {}
  };

  struct SectionTableEntry {
    string secName;
    int id;
    long size;
    long address;   // address where section is going to be placed
    bool itsPlaced = false;
    vector<string> code;
    vector<RelocationEntry> relocs;
    vector<LiteralPoolEntry> pool;
  };

  struct FileInfo {
    string filename;
    map<string, SymbolTableEntry> symTab;
    map<string, SectionTableEntry> secs;
  };

  vector<FileInfo> fileInfos;
  map<string, long> symbolTableLinker;
  map<string, long> sectionTableLinker; 

public:
  Linker(vector<string> inputFiles, string outputFile, map<string, string> sa);

  int parseInputFiles();
  int mapSections();
  int fillSymbolTableLinker();
  int fillSectionTableLinker();
  void fixRelocations();
  void printCode();

  long hexadecimalToDecimal(string hexad);
  string decimalToHexadecimal(long number, int width = 0);
};

#endif 