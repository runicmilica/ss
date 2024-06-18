#include "../inc/relocationTable.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
using namespace std;


RelocationTable::RelocationTable(string secName) {
  sectionName = secName;
}
void RelocationTable::add(string symbolName, long offset, long symbolTableRef, int type, int addend) {
  Entry e;
  e.offset = offset; e.symbolTableRef = symbolTableRef; e.type = type; e.addend = addend;
  table.push_back(e);
}

string RelocationTable::decimalToHexadecimal(long num, int width) {
  stringstream ss;
  ss << setw(width) << hex << num;
  return ss.str();
}

void RelocationTable::printForLinker(string filename) {
  ofstream file; 
  file.open(filename, ios::app);
  file << ":RELOC:" << endl;
  if(table.size() < 1)
      file << "NOTHING" << endl;
  else {
    file << " offset :symTabRef:addend:type:" << endl;
    for(const auto& rel:table) {
      file  << setw(8) << decimalToHexadecimal(rel.offset, 8) << ":"
            << setw(9) << decimalToHexadecimal(rel.symbolTableRef,0) << ":"
            << setw(6) << decimalToHexadecimal(rel.addend, 0) << ":"
            << setw(4) << rel.type << ":"
            << endl;
    }
  }
  file.close();
}

bool RelocationTable::relocExistsForPoolRelocations(long offs, long symTabRef, int add) {
  for(const auto& r:table) {
    if(r.offset == offs && r.symbolTableRef == symTabRef && r.addend == add)
      return true;
  }
  return false;
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
       << std::setw(15) << " addend |"
       << std::setw(10) << " type " << endl;
  file << "- - - - - - - - - - - - - - - - - - - - - - - - - - - -" << endl;
  for(const auto& row: table) {
    file << std::setw(10) << left << "  " + to_string(row.offset)
        << std::setw(15) << "  " + to_string(row.symbolTableRef)  // symbol name
        << std::setw(15) << "  " + to_string(row.addend)
        << std::setw(10) << "  " + to_string(row.type) << endl;
  }
  file << "- - - - - - - - - - - - - - - - - - - - - - - - - - - -" << endl << endl;
  file.close();
}