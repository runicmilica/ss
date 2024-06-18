#include "../inc/linker.hpp"
#include <regex>
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <vector>
#include <map>

using namespace std;

int main(int argc, char *argv[]) {
  if(argc < 5) {
    cout << "Error:Linker: There is not enough arguments." << endl;
    return -1; 
  }

  vector<string> inputFiles;
  string outputFile = "";
  map<string, string> sectionAddr;

  regex place(R"(^\-(place)=(\w+)\@0x([0-9a-fA-F]{8})$)");
  smatch sm;

  bool hexFound = false;
  bool optionsDone = false;

  for(int i = 1; i < argc; i++) {
    string arg = argv[i];
    if(strcmp(argv[i], "-hex") == 0) {
      if(optionsDone) {
        cout << "Error:Linker: Options must be before input files!" << endl;
        return -1;
      }
      hexFound = true;
    } 
    else if(strcmp(argv[i], "-o") == 0) {
      if(optionsDone) {
        cout << "Error:Linker: Options must be before input files!" << endl;
        return -1;
      }
      outputFile = argv[i++ + 1];
    }
    else if(regex_match(arg, sm, place)) {
      if(optionsDone) {
        cout << "Error:Linker: Options must be before input files!" << endl;
        return -1;
      }
      cout << "place: " << sm[1] << " " << sm[2] << " " << sm[3] << endl;
      string addr = sm[3];
      /* if(addr < "40000000") {
        cout << "Error:Linker: Address 0x" << addr << " < 0x40000000!" << endl;
        return -1;
      } else */ if(addr >= "FFFFFF00") {
        cout << "Error:Linker: Address 0x" << addr << " > 0xFFFFFF00!" << endl;
        return -1;
      }
      auto find = sectionAddr.find(sm[2]);
      if(find == sectionAddr.end())
        sectionAddr.insert(pair<string, string>(sm[2], sm[3]));
      else {
        cout << "Error:Linker: -place already defined for section " << sm[2] << "!" << endl;
        return -1;
      }
    }
    else {
      optionsDone = true;
      inputFiles.push_back(argv[i]);
    }
  }

  if(!hexFound) {
    cout << "Error:Linker: There is no option -hex." << endl;
    return -1;
  }

  /* for(const auto& i: inputFiles)
    cout << "inputFile: " << i << endl;
  for(const auto& i: sectionAddr)
    cout << "section: " << i.first << ", address: " << i.second << endl;
  cout << "outputFile: " << outputFile << endl;*/ 

  new Linker(inputFiles, outputFile, sectionAddr);
  return 0;
}