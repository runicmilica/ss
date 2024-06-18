#ifndef EMULATOR_HPP_
#define EMULATOR_HPP_

#include <stdio.h>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <fstream>

using namespace std;

const int sp = 14;
const int pc = 15;
const int status = 0;
const int handler = 1;
const int cause = 2;


struct Machine {
  long registers[16] = {0};
  map<long, string> memory;
  // status   - 0
  // handler  - 1
  // cause    - 2
  long csr[3] = {0};
  bool stop = false;
  Machine() {
    registers[pc] = 1073741824; // 0x40000000
    registers[sp] = 1073741824;
  }
};

class Emulator {

private:
  fstream fileOUT;
  string filename;

public:
  static Machine* machine;
  Emulator(string filename);
  int fillMemory();
  int execute();
  
  int instrHALT();
  int instrINT();
  int instrCALL(string byte1, string byte2, string byte3, string byte4);
  int instrJMP(string byte1, string byte2, string byte3, string byte4);
  int instrXCHG(string byte1, string byte2, string byte3, string byte4);
  int instrARITM(string byte1, string byte2, string byte3, string byte4);
  int instrLOGIC(string byte1, string byte2, string byte3, string byte4);
  int instrSH(string byte1, string byte2, string byte3, string byte4);
  int instrSTORE(string byte1, string byte2, string byte3, string byte4);
  int instrLOAD(string byte1, string byte2, string byte3, string byte4);
  // 
  int push(int reg, bool isCsr = false);

  // write 
  void writeState();

  // conversion
  string decimalToHexadecimal(long number, int width = 0);
  long hexadecimalToDecimal(string hexad);

};
#endif