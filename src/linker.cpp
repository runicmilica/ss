#include "../inc/linker.hpp"
#include <regex>
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <vector>
#include <sstream>

using namespace std;

Linker::Linker(vector<string> iFiles, string oFile, map<string, string> sa) {
  cout << "Linker constructor." << endl;
  inputFiles = iFiles; outputFile = oFile; sectionAddr = sa;
  bool error = false;
  if(parseInputFiles() < 0) error = true; // also calculates sections total size
  if(error) {
    ofstream outfile; 
    outfile.open(outputFile);

    outfile << "Error while linking." << endl;
    outfile.close();
    return;
  }
  if(mapSections() < 0) error = true;
  if(error) {
    ofstream outfile; 
    outfile.open(outputFile);

    outfile << "Error while linking." << endl;
    outfile.close();
    return;
  }
  if(fillSymbolTableLinker() < 0) error = true;
  if(error) {
    ofstream outfile; 
    outfile.open(outputFile);

    outfile << "Error while linking." << endl;
    outfile.close();
    return;
  }
  if(fillSectionTableLinker() < 0) error = true;
  if(error) {
    ofstream outfile; 
    outfile.open(outputFile);

    outfile << "Error while linking." << endl;
    outfile.close();
    return;
  }
  fixRelocations();
  printCode();
  /* for(const auto&i: inputFiles)
    cout << "i: " << i << endl;
  cout << "o: " << outputFile << endl;
  for(const auto&s:sectionAddr)
    cout << s.first << " " << s.second << endl;*/

}


string Linker::decimalToHexadecimal(long number, int width) {
  stringstream ss;
  ss << setfill('0') << setw(width) << std::hex << number;
  return ss.str();
}

long Linker::hexadecimalToDecimal(string hexad) {
    stringstream ss;

    // Insert the hexadecimal string into the stringstream
    ss << std::hex << hexad;

    // Initialize the result variable
    long result;
    ss >> result;
    return result;
}

int Linker::parseInputFiles() {
  // cout << "Goes through all lines of each file." << endl;
  // calculate section sizes
  for(const auto& f:inputFiles) { // goes through all files
    ifstream file(f, ios::in);   // open for reading
    if(!file.is_open()) {
      std::cout << "Error: Could not open the file " << f << "!" << endl;
      return -1;
    }

    string line;
    regex symbolTableRegex(R"(^\s*([a-zA-Z0-9_]+)\s*:\s*([0-9a-fA-F]+)\s*:\s*([0-9a-fA-F]+)\s*:\s*(-?[0-9a-fA-F]+)\s*:\s*([0-9a-fA-F]+)\s*:\s*$)");
    regex relocRegex(R"(^\s*([0-9a-fA-F]+)\s*:\s*([0-9a-fA-F]+)\s*:\s*([0-9a-fA-F]+)\s*:\s*([0-9]+)\s*:\s*$)");
    regex literalPoolRegex(R"(^\s*([0-9a-fA-F]+)\s*:\s*([a-zA-Z0-9_]*)\s*:\s*([0-9a-fA-F]+)\s*:\s*([0-9]+)\s*:\s*([0-9]+)\s*:\s*$)");
    smatch match;

    FileInfo fileInfo;
    fileInfo.filename = f;

     while (getline(file, line)) {
        if (line == ":SYMBOL TABLE:") {
            getline(file, line); // Skip header line
            while (getline(file, line) && line != ":SECTION:") {
                if (regex_match(line, match, symbolTableRegex)) {
                    SymbolTableEntry entry(match[1].str(), 
                            hexadecimalToDecimal(match[2].str()), 
                            hexadecimalToDecimal(match[3].str()), 
                            hexadecimalToDecimal(match[4].str()), 
                            hexadecimalToDecimal(match[5].str()));
                    fileInfo.symTab[match[1].str()] = entry;

                    // calculate size of sections
                    if(hexadecimalToDecimal(match[2].str()) == hexadecimalToDecimal(match[3].str())) {  // section
                      auto s = sectionSize.find(match[1].str());
                      if(s != sectionSize.end()) {  // found
                        long newSize = s->second + hexadecimalToDecimal(match[5].str());
                        sectionSize[match[1].str()] = newSize;
                      } else {  // not found
                        sectionSize.insert(pair<string, long>(match[1].str(), hexadecimalToDecimal(match[5].str())));
                      }
                    }
                }
            }
        } if (line == ":SECTION:") {
            getline(file, line);
            string sectionName = line;
            // to keep order of sections for output
            bool alreadyExists = false;
            for(auto& s:sectionOrder) {
              if(s == sectionName) {
                alreadyExists = true;
                break;
              }
            }
            if(!alreadyExists) sectionOrder.push_back(sectionName);
            // 
            SectionTableEntry section;
            section.secName = sectionName;
            auto s = fileInfo.symTab.find(sectionName); 
            if(fileInfo.symTab.end() != s) {
              section.size = s->second.size;
              section.id = s->second.id;
            }

            getline(file, line); // Should be ":RELOC:"
            if (line == ":RELOC:") {
                getline(file, line);
                if (line != "NOTHING") {
                    do {
                        if (regex_match(line, match, relocRegex)) {
                            RelocationEntry entry(
                                  hexadecimalToDecimal(match[1].str()), 
                                  hexadecimalToDecimal(match[2].str()), 
                                  hexadecimalToDecimal(match[3].str()), 
                                  hexadecimalToDecimal(match[4].str()));
                            section.relocs.push_back(entry);
                        }
                    } while (getline(file, line) && line != ":CODE:" && line != ":LITERAL POOL:" && line != ":END:");
                }  else getline(file, line);
            }

            if (line == ":CODE:") {
                getline(file, line);
                if (line != "NOTHING") {
                    do {
                      istringstream iss(line);
                      string c;
                      while (iss >> c) {
                          section.code.push_back(c);
                      }
                    } while (getline(file, line) && line != ":LITERAL POOL:" && line != ":END:");
                } else getline(file, line);
            }

            if (line == ":LITERAL POOL:") {
                getline(file, line);
                if (line != "NOTHING") {
                    do {
                        if (regex_match(line, match, literalPoolRegex)) {
                            LiteralPoolEntry entry(
                                  hexadecimalToDecimal(match[1].str()), 
                                  match[2].str(), hexadecimalToDecimal(match[3].str()), 
                                  hexadecimalToDecimal(match[4].str()), 
                                  hexadecimalToDecimal(match[5].str()));
                            section.pool.push_back(entry);
                        }
                    } while (getline(file, line) && line != ":END:");
                } else getline(file, line);
            }
            fileInfo.secs[sectionName] = section;
        }
    }

    fileInfos.push_back(fileInfo);
    file.close();
  }

  cout << "- - - - - - - - - PRINT FROM parseInputFiles - - - - - - - - - " << endl << endl;
  for(const auto& s:sectionSize) {
    cout << "section: " << s.first << ", size: " << s.second << endl;
  }
  /* PRINT */
  for(const auto&fileInfo:fileInfos) {
    cout << "filename: " << fileInfo.filename << endl;
    cout << "Symbol Table:" << endl;
        for (const auto& s : fileInfo.symTab) {

            cout << "Symbol: " << s.second.symbolName << ", ID: " << s.second.id
                 << ", Sec: " << s.second.sec << ", Value: " << s.second.value  << ", Size: " << s.second.size << endl;
        }

        for (const auto& s : fileInfo.secs) {
            cout << "\nSection: " << s.second.secName << endl;

            cout << "Relocation Entries:" << endl;
            for (const auto& entry : s.second.relocs) {
                cout << "Offset: " << entry.offset << ", SymTabRef: " << entry.symTabRef
                     << ", Addend: " << entry.addend << ", Type: " << entry.type << endl;
            }

            cout << "Code:" << endl;
            for (const auto& line : s.second.code) {
                cout << line;
            }
            cout<< endl;

            cout << "Literal Pool Entries:" << endl;
            for (const auto& entry : s.second.pool) {
                cout << "HexValue: " << entry.hexValue << ", SymName: " << entry.symName
                     << ", Address: " << entry.address << ", Size: " << entry.size
                     << ", IsSym: " << entry.isSym << endl;
            }
            cout << endl;
        }
  }
  cout << "- - - - - - - - - PRINT END parseInputFiles - - - - - - - - - " << endl << endl;
  return 0;
}

int Linker::mapSections() {
  cout << endl << "START TO FILL SECTIONS WITH PLACE OPTION." << endl;
  long locationCounter = 0;
  // first put sections that have place option defined => sectionAddr
  while(sectionAddr.size() > 0) {   // while there are unresolved place options
    // find minimal address defined
    long min = hexadecimalToDecimal("FFFFFF00");
    string sectionWithMinAddr = "";
    for(auto& s:sectionAddr) {
      if(hexadecimalToDecimal(s.second) < min) {
        min = hexadecimalToDecimal(s.second); 
        sectionWithMinAddr = s.first;
      }
    }
    // remove it from sectionAddr
    sectionAddr.erase(sectionWithMinAddr);
    // order for print 
    sectionOrderForPrint.push_back(sectionWithMinAddr);

    if(locationCounter > min) {
      cout << "Error:Linker: Overlap!!! => section: " << sectionWithMinAddr << ", place: 0x" << decimalToHexadecimal(min, 8) << endl;
      return -1;
    }
    locationCounter = min;
    // put that section to that address
    for(auto& file: fileInfos) {
      for(auto& sec: file.secs) {
        if(sec.second.secName == sectionWithMinAddr) {
          // to mark sections that are placed
          sec.second.itsPlaced = true;
          sec.second.address = locationCounter;
          locationCounter += sec.second.size;
          if(locationCounter > hexadecimalToDecimal("FFFFFF00")) {
             cout << "Error:Linker: Section enters register filed!!! => section: " << sectionWithMinAddr << ", place: 0x" << decimalToHexadecimal(min, 8) << endl;
             return -1;
          }
          cout << "location counter from place: " << decimalToHexadecimal(locationCounter, 8) << endl;
          break;
        }
      }
    }
  }

  // locationCounter = 0;  // 0x00000000
  cout << endl << "START TO FILL SECTIONS WITH NO PLACE OPTION." << endl;
  cout << "location counter: " << decimalToHexadecimal(locationCounter, 8) << endl;

  for(auto& secI:sectionOrder) {  // go in order

    for(auto& section:sectionSize) {
      if(section.first == secI) {
        for(auto& file: fileInfos) {
          for(auto& sec: file.secs) {
            if(sec.second.secName == section.first && !sec.second.itsPlaced) {  // only for sections not placed with -place option
              sec.second.address = locationCounter;
              locationCounter += sec.second.size;
              sec.second.itsPlaced = true;
              bool inOrderForPrint = false;
              for(auto& ord:sectionOrderForPrint)
                if(ord == sec.second.secName) {
                  inOrderForPrint = true;
                  break;
                }
              if(!inOrderForPrint) sectionOrderForPrint.push_back(sec.second.secName);
              cout << "location counter: " << decimalToHexadecimal(locationCounter, 8) << endl;
              break;
            }
          }
        }
      }
    }
  }
  cout << endl;
  cout << "- - - - - - - PRINT FROM mapSections(after) - - - - - - -" << endl << endl;
  for(const auto&fileInfo:fileInfos) {
    cout << "filename: " << fileInfo.filename << endl;

        for (const auto& s : fileInfo.secs) {
            cout << "Section: " << s.second.secName  
                << ", address: " << decimalToHexadecimal(s.second.address)
                << ", size: " << decimalToHexadecimal(s.second.size)
                << endl;

        }
        cout << endl;
  }

  cout << "- - - - - - - PRINT END mapSections - - - - - - -" << endl << endl;
  return 0;
}

int Linker::fillSymbolTableLinker() {
  cout << "- - - - - - PRINT FROM fillSymbolTableLinker - - - - - - " << endl << endl;
  for (auto &file : fileInfos) {
    for (auto &sym : file.symTab) {
      if(sym.second.id == sym.second.sec) continue; // section
      else {  // only for symbols

        auto find = symbolTableLinker.find(sym.second.symbolName);
        if(find != symbolTableLinker.end()) {
          // symbol is in the table
          // check values
          if(find->second >= 0 && sym.second.sec != 0) {
            // multiple definition
            cout << "Error:Linker: Multiple definition of symbol " << sym.second.symbolName << endl;
            return -1;
          }

          if(find->second >= 0 && sym.second.sec == 0) continue;

          if(find->second < 0 && sym.second.sec != 0) {
            find->second = sym.second.value;
          }
        } else {
          // symbol not in the table until now
          long val = sym.second.sec == 0 ? -1 : sym.second.value;
          symbolTableLinker.insert(pair<string, long>(sym.second.symbolName, val));
        }
        /*
        // if(sym.second.value == -1) continue;  // extern symbol
        // auto find = symbolTableLinker.find(sym.second.symbolName);
        // if(find != symbolTableLinker.end()) {
        //   cout << "Error:Linker: Multiple definition of symbol " << sym.second.symbolName << endl;
        //   return -1;
        // }
        // find address of section that this symbol belongs to
        long addr;
        for (auto &sec : file.secs) {
          cout << "sec.second.id " << sec.second.id << " sym.second.sec" << sym.second.sec << endl;
          if(sec.second.id == sym.second.sec) {
            // cout << "here "<< sec.first << " " << sec.second.address << endl;
            addr = sec.second.address;
          }
        }
        // new value of symbol is address + value
        cout << "before insert " << sym.second.symbolName << " " << decimalToHexadecimal(addr, 8) << " " << decimalToHexadecimal(sym.second.value, 8)  << endl;
        symbolTableLinker.insert(pair<string, long>(sym.second.symbolName, addr + sym.second.value));*/
      }
    }
  }

  /* for (auto &file : fileInfos) {
    for (auto &sym : file.symTab) {

        long addr;
        for (auto &sec : file.secs) {
          cout << "sec.second.id " << sec.second.id << " sym.second.sec" << sym.second.sec << endl;
          if(sec.second.id == sym.second.sec) {
            // cout << "here "<< sec.first << " " << sec.second.address << endl;
            addr = sec.second.address;
          }
        }
        // new value of symbol is address + value
        cout << "before insert " << sym.second.symbolName << " " << decimalToHexadecimal(addr, 8) << " " << decimalToHexadecimal(sym.second.value, 8)  << endl;
        symbolTableLinker.insert(pair<string, long>(sym.second.symbolName, addr + sym.second.value));
    }
  }*/

  for(auto& s:symbolTableLinker) {
    if(s.second < 0) {
      cout << "Error:fillSymbolTableLinker: Symbol " << s.first << " not defined!" << endl;
      return -1;
    } else {
      for (auto &file : fileInfos) {
        auto find = file.symTab.find(s.first);
        if(find != file.symTab.end()) {
          if(find->second.value == s.second) {
            for (auto &sec : file.secs) {
              // cout << "sec.second.id " << sec.second.id << " sym.second.sec" << sym.second.sec << endl;
              if(sec.second.id == find->second.sec) {
                s.second += sec.second.address;
              }
            }
          }
        }
      }
    }
  }



  cout << "SymbolTableLinker: " << endl;
  for(const auto& s: symbolTableLinker) {
    cout << "name: " << s.first << ", value: " << decimalToHexadecimal(s.second, 8) << endl;
  }
  cout << "- - - - - - PRINT END fillSymbolTableLinker - - - - - - " << endl << endl;
  return 0;
}
int Linker::fillSectionTableLinker() {
  cout << "- - - - - - PRINT FROM fillSectionTableLinker - - - - - - " << endl << endl;
  for(auto& s: sectionSize) { // all different sections
    bool found = false;
    for(auto& file: fileInfos) {
      
      // find all sections with that name
      for(auto& section: file.secs) {
        if(s.first == section.second.secName) {
          sectionTableLinker.insert(pair<string, long>(s.first, section.second.address));
          found = true;
          break;
        }
      }
      if(found) break;
    }
  }
  cout << "- - - - - - PRINT END fillSectionTableLinker - - - - - - " << endl << endl;
  return 0;
}

void Linker::fixRelocations() {
  cout << "- - - - - - PRINT FROM fixRelocations - - - - - - " << endl << endl;

  // FIRST: ADD LITERAL POOL TO CODE FOR EASIER MANIPULATION OF RELOCS
  for(auto& file:fileInfos) {
    for(auto& sec:file.secs) {
      for(auto& poolLiteral:sec.second.pool) {
        string value = decimalToHexadecimal(poolLiteral.hexValue, 8);
        sec.second.code.push_back(value.substr(6, 2));
        sec.second.code.push_back(value.substr(4, 2));
        sec.second.code.push_back(value.substr(2, 2));
        sec.second.code.push_back(value.substr(0, 2));
      }
    }
  }

  // all files
  for(auto& file:fileInfos) {
    // all sections
    for(auto& sec:file.secs) {
      // all relocations
      for(auto& reloc:sec.second.relocs) {
        // fix offset to offset + section address
        // reloc.offset += sec.second.address;
        // offset is decimal value
        // everything will be 4B in literal pool
        // find what it refers to in symbol table for this file
        /* if(file.filename == "main.o") {
          cout << "reloc: offset: " << reloc.offset << ", symtabref: " << reloc.symTabRef << ", addend: "<<  reloc.addend << endl;
        }*/
        string symName;
        for(auto& sym:file.symTab){
          if(sym.second.id == reloc.symTabRef)
            symName = sym.second.symbolName;
        }

        // IF IT IS SYMBOL
        // find its new value from new symbol table => long value
        
        auto value = symbolTableLinker.find(symName);
        // cout << "symName: " << symName << ", value->first: " << value->first << ", value->second: " << value->second << endl;
        if(value != symbolTableLinker.end()){
          
          string valueString = decimalToHexadecimal(value->second + reloc.addend, 8);
        
          sec.second.code.at(reloc.offset)     = valueString.substr(6, 2);
          sec.second.code.at(reloc.offset + 1) = valueString.substr(4, 2);
          sec.second.code.at(reloc.offset + 2) = valueString.substr(2, 2);
          sec.second.code.at(reloc.offset + 3) = valueString.substr(0, 2);
          continue;
        }

        // IF IT IS SECTION
        // find its new value from new section table => long value
        value = sectionTableLinker.find(symName);
        if(value != sectionTableLinker.end()){
          string valueString = decimalToHexadecimal(value->second + reloc.addend, 8);
          sec.second.code.at(reloc.offset)     = valueString.substr(6, 2);
          sec.second.code.at(reloc.offset + 1) = valueString.substr(4, 2);
          sec.second.code.at(reloc.offset + 2) = valueString.substr(2, 2);
          sec.second.code.at(reloc.offset + 3) = valueString.substr(0, 2);
          continue;
        }

        // if it is not fined in symbol table nor section table
        // can that happen?
        cout << "Linker:fixRelocations(): SYMBOL FROM RELOCATION NOT FOUND IN SYMBOL TABLE NOR SECTION TABLE!" << endl;
      }
    }
  }
  // PRINT CODE TO CHECK
  for(auto& file:fileInfos) {
      // all sections
      for(auto& sec:file.secs) {
        cout << sec.second.secName << endl;
        int i = 0;
        for(auto& c:sec.second.code) {
          cout << c << " ";
          i++;
          if(i%8 == 0) cout << endl;
        }
        cout << endl;
      }
    }
  cout << "- - - - - - PRINT END fixRelocations - - - - - - " << endl << endl;
}

void Linker::printCode() {
  ofstream outfile; 
  outfile.open(outputFile);
  int i = 0;
  // print symbolTable
  /*for(auto &sym:symbolTableLinker) {
    outfile << setw(10) << left << sym.first << ":" << decimalToHexadecimal(sym.second, 8)  << endl;
  }
  // print section
  for(auto &sec:sectionTableLinker) {
    outfile << setw(10) << left << sec.first << ":" << decimalToHexadecimal(sec.second, 8) << endl;
  }*/

  for(const auto& secI:sectionOrderForPrint) {
    for(const auto& sec: sectionSize) {
      // section order
      if(sec.first == secI) {
        string sectionName = sec.first;
        
        for(auto& file: fileInfos) {
          
          // find all sections with that name
          for(auto& section: file.secs) {
            long lc = 0;
            while(i % 8 != 0) i++;
            if(sectionName == section.second.secName) {
                
              
                // section code
                for(const auto& c: section.second.code) {
                  if((i % 8) == 0) {
                    outfile << endl << decimalToHexadecimal(section.second.address + lc, 8) << ": ";
                  }
                  outfile << (char)toupper(c.at(0)) << (char)toupper(c.at(1)) << " ";
                  i++;
                  lc++;    
                }

                // section literal pool
                /*for(const auto& l: section.second.pool) {
                  string value = decimalToHexadecimal(l.hexValue, 8);
                  // cout << "value: " << value << endl;
                  for(int j = 0; j < 4; j++) {
                    if((i % 8) == 0) {
                      outfile << endl << decimalToHexadecimal(section.second.address + lc, 8) << ": ";
                    }
                    // cout << "value.substr(" << j << "*2,2) = " << value.substr(j*2, 2) << endl;
                    outfile << value.substr(j*2, 2) << " ";
                    i++;
                    lc++;
                  }
                }      */       
            }
          }
        }
      }
    }
  }
  outfile.close();
}
