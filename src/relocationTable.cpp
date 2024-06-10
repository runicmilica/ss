#include "../inc/relocationTable.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
using namespace std;


RelocationTable::RelocationTable(string secName) {
  sectionName = secName;
}
void RelocationTable::add(string symbolName, long offset, long symbolTableRef, int type) {
  Entry e;
  e.offset = offset; e.symbolTableRef = symbolTableRef; e.type = type;
  table.push_back(e);
}

void RelocationTable::printRelocationTableForSectionIntoFile(string filename) {
  // long offset;
  // long symbolTableRef;
  // int type;
  
  ofstream file; 
  file.open ("p_" + filename, ios::app);
  file << endl;
  if(table.empty()) {
    file << "No relocations for section: " + sectionName << endl;
    file.close();
    return;
  }
  file << "- - - RELOCATION TABLE FOR SECTION: " + sectionName + " - - -" << endl;
  file << "- - - - - - - - - - - - - - - - - - - - - - - - - - - -" << endl;
  file << std::setw(10) << left << " offset |"
       << std::setw(15) << " symbolTableRef |"
       << std::setw(10) << " type " << endl;
  file << "- - - - - - - - - - - - - - - - - - - - - - - - - - - -" << endl;
  for(const auto& row: table) {
    file << std::setw(10) << left << "  " + to_string(row.offset)
        << std::setw(15) << "  " + to_string(row.symbolTableRef)  // symbol name
        << std::setw(10) << "  " + to_string(row.type) << endl;
  }
  file << "- - - - - - - - - - - - - - - - - - - - - - - - - - - -" << endl << endl;
  file.close();
}