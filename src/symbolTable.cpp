
#include "../inc/symbolTable.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
using namespace std;

int SymbolTable::id = 0;

SymbolTable::SymbolTable() {
  SymbolTableEntry ste;
  string und = "UND";
  ste.symbolName = und;
  ste.id = SymbolTable::id++;
  ste.size = 0;
  table.insert(pair<string, SymbolTableEntry>(und, ste));
}

void SymbolTable::addInfoToSymbol(string symbolName, Info info) {
  auto find = table.find(symbolName);
  if(find != table.end())
     find->second.usedHere.push_back(info);
}

void SymbolTable::add(string symName) {
  SymbolTableEntry ste;
  ste.symbolName = symName;
  ste.id = SymbolTable::id++;
  // cout << "SymbolTable::add: " + ste.symbolName << "sectionNumber: " << ste.sectionNumber << ste.size << endl;
  table.insert(pair<string, SymbolTableEntry>(symName, ste));
  // auto s = table.find(symName);
  // cout << s->second.symbolName << " " << s->second.id << endl;
}

string SymbolTable::getPrintRow(string symName) {
  auto s = table.find(symName);
  if(s != table.end())
    return 
    "symbolName: " + s->second.symbolName + ", sectionNumber: " + to_string(s->second.sectionNumber) + 
    ", value: " + to_string(s->second.value) +  ", isGlobal: " + to_string(s->second.isGlobal) + 
    ", id: " + to_string(s->second.id) +  ", size: " + to_string(s->second.size) +
    ", isExtern: " + to_string(s->second.isExtern);
  return "No symbol!";
}
string SymbolTable::decimalToHexadecimal(long num, int width) {
  stringstream s;
  s << setfill('0') << setw(width) << hex << num;
  return s.str();
}

void SymbolTable::printSymbolTableForLinker(string filename) {
  ofstream file; 
  file.open (filename);
  file << ":SYMBOL TABLE:" << endl;
  file << "symbolName:id:sec:  value :size:" << endl;
  for(const auto& s: table) {
    // for sections and global symbols
    if(s.second.isGlobal || s.second.id == s.second.sectionNumber) {
      string value;
      string size;
      if(s.second.id == s.second.sectionNumber) {
        value = decimalToHexadecimal(0, 8);
        size = decimalToHexadecimal(s.second.size, 0);
      }
      else {
        if(s.second.value == -1)
          value = "-1";
        else
          value = decimalToHexadecimal(s.second.value, 8);
        size = decimalToHexadecimal(0, 0);
      }
      
      file << setw(10) <<  s.second.symbolName << ":"
           << setw(2) << decimalToHexadecimal(s.second.id, 0) << ":"
           << setw(3) << decimalToHexadecimal(s.second.sectionNumber, 0) << ":"
           << setw(8) << value << ":"
           << setw(4) << size << ":"
           << endl;
    }
  }

  file.close();
}

void SymbolTable::printSymbolTable(string filename, bool flag) {
  if(!flag) { // print
    cout  << std::setw(15) << "symbolName: "
            << std::setw(15) << "sectionNumber: "
            << std::setw(10) << "value: "
            << std::setw(10) << "isGlobal: "
            << std::setw(5)  << "id: "
            << std::setw(10) << "size: "
            << std::setw(10) << "isExtern: " << endl;
    for (const auto& s: table)
    {
      cout << std::setw(15) << std::left << setfill(' ') << s.second.symbolName << std::setw(15) << to_string(s.second.sectionNumber) 
      << std::setw(10) << to_string(s.second.value) << std::setw(10) << to_string(s.second.isGlobal) 
      << std::setw(5) << to_string(s.second.id) << std::setw(10) << to_string(s.second.size) 
      << std::setw(10) << to_string(s.second.isExtern) << endl;
      cout << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -" << endl;
    }
  } else {  // print to file
    ofstream file; 
    file.open ("p_" + filename);
    file << "- - - - - - - - - - - - - - - - SYMBOL TABLE - - - - - - - - - - - - - - - -" << endl;
    file << std::setw(15) << "symbolName | "
            << std::setw(15) << "sectionNumber | "
            << std::setw(8) << "value | "
            << std::setw(10) << "isGlobal | "
            << std::setw(5)  << "id | "
            << std::setw(8) << "size | "
            << std::setw(10) << "isExtern | " << endl;
    file << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -" << endl;
    for (const auto& s: table)
    {
      file << std::setw(15) << left << " " + s.second.symbolName 
      << std::setw(15) << "     " + to_string(s.second.sectionNumber) 
      << std::setw(8) << "  " + to_string(s.second.value) 
      << std::setw(10) << "    " + to_string(s.second.isGlobal) 
      << std::setw(5) << "  " + to_string(s.second.id) 
      << std::setw(8) << "   " + to_string(s.second.size) 
      << std::setw(10) << "    " + to_string(s.second.isExtern) << endl;
      file << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -" << endl;
    }
    // file << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -" << endl << endl << endl;
    /* file << "- - - - - - - - - - - - - - - - SYMBOL USE - - - - - - - - - - - - - - - - -" << endl;
    
    for(const auto& s: table) {
      if(s.second.usedHere.size() > 0) {
        file << "---> SYMBOL: " << s.second.symbolName << endl;
        file  <<  std::setw(10) << "address | "
              << std::setw(10) << "sectionName | "
              << std::setw(15) << "  type  | " << endl;
        for (const auto& s: s.second.usedHere)
        {
          file << std::setw(10) << std::left << to_string(s.address)
             << std::setw(10) << s.sectionName
             << std::setw(15) << ((s.type == 0) ? "   pc_relative" : "   absolute" ) << endl;
        }
        file << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -" << endl;
      }
      
    } */
    file << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -" << endl << endl << endl;
    file.close();
  }
}

map<string, SymbolTableEntry> SymbolTable::getAllSymbols() {
  return table;
}

/* vector<Info> SymbolTable::getInfoListForSymbol(string symName) {
  auto ste = table.find(symName);
  return ste->second.usedHere;
}*/

// setters
  void SymbolTable::setSize(int id, long size) {
    for (auto& s: table) {
      if(s.second.id == id) {
        s.second.size = size;
        break;
      }
    }
  }
  void SymbolTable::setSectionNumber(string symName, int secNum) {
    auto find = table.find(symName);
    if(find != table.end())
      find->second.sectionNumber = secNum;
  }
  void SymbolTable::setValue(string symName, int value) {
    auto find = table.find(symName);
    if(find != table.end())
      find->second.value = value;
  }
  void SymbolTable::setIsGlobal(string symName, bool isGlobal) {
    auto find = table.find(symName);
    if(find != table.end())
      find->second.isGlobal = isGlobal;
  }
  void SymbolTable::setSize(string symName, long size) {
    auto find = table.find(symName);
    if(find != table.end())
      find->second.size = size;
  }
  void SymbolTable::setIsExtern(string symName, bool isExtern) {
    auto find = table.find(symName);
    if(find != table.end())
      find->second.isExtern = isExtern;
  }
  bool SymbolTable::exists(string symName) {
    auto find = table.find(symName);
    if(find != table.end())
      return true;
    return false;
  }
  // getters
  long SymbolTable::getSize(int id) {
    for (const auto& s: table) {
      if(s.second.id == id) {
        return s.second.size;
      }
    }
    return -1;
  }
  bool SymbolTable::getIsExtern(string symName) {
    auto find = table.find(symName);
    return find->second.isExtern;
  }
  int SymbolTable::getSectionNumber(string symName) {
    auto s = table.find(symName);
    if(s != table.end())
      return s->second.sectionNumber;
    cout << "SymbolTable::getSectionNumber: symbol does not exists!" << endl;
    return -1;
  }
  int SymbolTable::getValue(string symName) {
    auto s = table.find(symName);
    if(s != table.end())
      return s->second.value;
    cout << "SymbolTable::getValue: symbol does not exists!" << endl;
    return -1;
  }
 
  long SymbolTable::getSize(string symName) {
    auto s = table.find(symName);
    if(s != table.end())
      return s->second.size;
    cout << "SymbolTable::getSize: symbol does not exists!" << endl;
    return -1;
  }
  bool SymbolTable::getIsGlobal(string symName) {
    auto s = table.find(symName);
    if(s != table.end())
      return s->second.isGlobal;
    cout << "SymbolTable::getIsGlobal: symbol does not exists!" << endl;
    return false;
  }
  
  int SymbolTable::getId(string symName) {
    auto s = table.find(symName);
    if(s != table.end())
      return s->second.id;
    cout << "SymbolTable::getId: symbol does not exists!" << endl;
    return -1;
  } 