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
    // sign??
    Entry() {
      offset = 0; symbolTableRef = 0; type = -1;
    }
  };
  string sectionName;
  vector<Entry> table;

  public:

  RelocationTable(string secName);
  void add(string symbolName, long offset, long symbolTableRef, int type);
  void printRelocationTableForSectionIntoFile(string filename);
};


#endif