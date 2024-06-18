#ifndef _RELOCTABLE_HPP_
#define _RELOCTABLE_HPP_

#include <string>
#include <iostream>
#include <map>
#include <vector>

using namespace std;

class RelocationTable {

  private:

  struct Entry {
    long offset;
    long symbolTableRef;
    int type;
    int addend;
    // sign??
    Entry() {
      offset = 0; symbolTableRef = 0; type = -1; addend = 0;
    }
  };
  string sectionName;
  vector<Entry> table;

  public:

  RelocationTable(string secName);
  void add(string symbolName, long offset, long symbolTableRef, int type, int addend);
  void printRelocationTableForSectionIntoFile(string filename);
  bool relocExistsForPoolRelocations(long offs, long symTabRef, int add);
  void printForLinker(string filename);
  string decimalToHexadecimal(long num, int width);
};


#endif