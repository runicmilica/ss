#include "../inc/emulator.hpp"
#include <regex>
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <vector>
#include <sstream>

using namespace std;

Machine* Emulator::machine = nullptr;

Emulator::Emulator(string filename) {
  this->filename = filename;
  Emulator::machine = new Machine();
  if(fillMemory() < 0) return;
  execute();
  writeState();
}

int Emulator::fillMemory() {
  fileOUT.open("izlaz_emulatora.txt");
  fileOUT << "- - - - - fillMemory() - - - - -" << endl;
  fstream file;
  file.open(filename);
  if(!file.is_open()) {
    fileOUT << "Error:Emulator: Could not open file " << filename << endl;
    return -1;
  }

  string line;
  while(getline(file, line))  {

    regex r(R"(^([0-9A-Fa-f]{8}):\s+(([0-9A-Fa-f]{2}\s)+[0-9A-Fa-f]{2})\s*$)");
    smatch sm;

    if(regex_match(line, sm, r)) {
        long address = hexadecimalToDecimal(sm[1]);
        std::istringstream iss(sm[2]);
        std::string byte;
        int i = 0;
        while (iss >> byte) {
            machine->memory.insert(pair<long, string>(address + i++, byte));
        }
    }
  }

fileOUT << "- - - - - MEMORY - - - - -" << endl;
  for(const auto&m: machine->memory) {
    fileOUT << "address: " << decimalToHexadecimal(m.first, 8) << ", byte:" << m.second << endl;
  }
  return 0;
}

int Emulator::execute() {
  string byte1, byte2, byte3, byte4;
  while(machine->stop != true) {  // until halt instruction
  fileOUT << "PC from execute before reading instr: " << decimalToHexadecimal(machine->registers[pc]) << endl;
  for(int i = 0; i < 16; i++) {
    fileOUT << "r" << i << " = " << decimalToHexadecimal(machine->registers[i]) << "; ";
  } 
  for(int i = 0; i < 3; i++) {
    fileOUT << "csr" << i << " = " << decimalToHexadecimal(machine->csr[i]) << "; ";
  }
  fileOUT << endl;
    // get 4B from memory
    // fileOUT << "PC: " << decimalToHexadecimal(machine->registers[pc]) << endl;
    auto b = machine->memory.find(machine->registers[pc]++);
    byte4 = b->second;

    // fileOUT << "PC: " << decimalToHexadecimal(machine->registers[pc]) << endl;
    b = machine->memory.find(machine->registers[pc]++);
    byte3 = b->second;

    // fileOUT << "PC: " << decimalToHexadecimal(machine->registers[pc]) << endl;
    b = machine->memory.find(machine->registers[pc]++);
    byte2 = b->second;

    // fileOUT << "PC: " << decimalToHexadecimal(machine->registers[pc]) << endl;
    b = machine->memory.find(machine->registers[pc]++);
    byte1 = b->second;

    fileOUT << "PC from execute after reading instr: " << decimalToHexadecimal(machine->registers[pc]) << endl;

    // fileOUT << "Execute: " << byte1 << " " << byte2 << " " << byte3 << " " << byte4 << endl;

    if(byte1 == "00") instrHALT();
    else if(byte1 == "10") instrINT();
    else if(byte1 == "20" || byte1 == "21") instrCALL(byte1, byte2, byte3, byte4);
    else if(byte1 == "30" || byte1 == "31" ||byte1 == "32" || byte1 == "33" || byte1 == "38" || byte1 == "39" || byte1 == "3A" || byte1 == "3B") instrJMP(byte1, byte2, byte3, byte4);
    else if(byte1 == "40") instrXCHG(byte1, byte2, byte3, byte4);
    else if(byte1 == "50" || byte1 == "51" ||byte1 == "52" || byte1 == "53") instrARITM(byte1, byte2, byte3, byte4);
    else if(byte1 == "60" || byte1 == "61" ||byte1 == "62" || byte1 == "63") instrLOGIC(byte1, byte2, byte3, byte4);
    else if(byte1 == "70" || byte1 == "71") instrSH(byte1, byte2, byte3, byte4);
    else if(byte1 == "80" || byte1 == "81" || byte1 == "82") instrSTORE(byte1, byte2, byte3, byte4);
    else if(byte1 == "90" || byte1 == "91" ||byte1 == "92" || byte1 == "93" || 
            byte1 == "94" || byte1 == "95" || byte1 == "96" || byte1 == "97") instrLOAD(byte1, byte2, byte3, byte4);
    else {
      // wrong op code
      fileOUT << "Execute: wrong op code! op code: " << byte1 << endl;
      break;
    }
    
  }
  return 0;
}

int Emulator::instrHALT() {
  fileOUT << "\nHALT" << endl;
  machine->stop = true;
  return 0;
}
int Emulator::instrINT() {
  fileOUT << "\nINT" << endl;
  // push status; push pc; cause<=4; status<=status&(~0x1); pc<=handler;
  fileOUT << "INT: push status;" << endl;
  push(status, true);
  fileOUT << "INT: push pc;" << endl;
  push(pc);
  machine->csr[cause] = 4;
  machine->csr[status] &= (~0x1);
  machine->registers[pc] = machine->csr[handler];
  fileOUT << "PC from INT went to : " << decimalToHexadecimal(machine->registers[pc]) << endl;
  return 0;
}

int Emulator::push(int reg, bool isCsr) { // 81
  fileOUT << "push" << endl;
  machine->registers[sp] -= 4;

  string regValue;
  if(isCsr) regValue = decimalToHexadecimal(machine->csr[reg], 8);
  else regValue = decimalToHexadecimal(machine->registers[reg], 8);


  if(machine->memory.find(machine->registers[sp]) != machine->memory.end()) 
    machine->memory.find(machine->registers[sp])->second = regValue.substr(6,2);
  else machine->memory.insert({machine->registers[sp], regValue.substr(6,2)});

  if(machine->memory.find(machine->registers[sp] + 1) != machine->memory.end()) 
    machine->memory.find(machine->registers[sp] + 1)->second = regValue.substr(4,2);
  else machine->memory.insert({machine->registers[sp] + 1, regValue.substr(4,2)});

  if(machine->memory.find(machine->registers[sp] + 2) != machine->memory.end()) 
    machine->memory.find(machine->registers[sp] + 2)->second = regValue.substr(2,2);
  else machine->memory.insert({machine->registers[sp] + 2, regValue.substr(2,2)});

  if(machine->memory.find(machine->registers[sp] + 3) != machine->memory.end()) 
    machine->memory.find(machine->registers[sp] + 3)->second = regValue.substr(0,2);
  else machine->memory.insert({machine->registers[sp] + 3, regValue.substr(0,2)});

  fileOUT << "Na adresi " << decimalToHexadecimal(machine->memory.find(machine->registers[sp] + 0)->first )<< " se nalazi " << machine->memory.find(machine->registers[sp] + 0)->second << ", a ";
  fileOUT << "na adresi " << decimalToHexadecimal(machine->memory.find(machine->registers[sp] + 1)->first )<< " se nalazi " << machine->memory.find(machine->registers[sp] + 1)->second << ", a ";
  fileOUT << "na adresi " << decimalToHexadecimal(machine->memory.find(machine->registers[sp] + 2)->first )<< " se nalazi " << machine->memory.find(machine->registers[sp] + 2)->second << ", a ";
  fileOUT << "na adresi " << decimalToHexadecimal(machine->memory.find(machine->registers[sp] + 3)->first )<< " se nalazi " << machine->memory.find(machine->registers[sp] + 3)->second << endl;
  return 0;
}

int Emulator::instrCALL(string byte1, string byte2, string byte3, string byte4){
  fileOUT << "\nCALL" << endl;
  int regA = hexadecimalToDecimal(byte2.substr(0,1));
  int regB = hexadecimalToDecimal(byte2.substr(1,1));
  int regC = hexadecimalToDecimal(byte3.substr(0,1));
  int d;
  if(byte3.at(1) == 'F')
   d = hexadecimalToDecimal("FFFFF" + byte3.substr(1,1) + byte4);
  else 
   d = hexadecimalToDecimal(byte3.substr(1,1) + byte4);
  // push pc; pc<=gpr[A]+gpr[B]+D;
  // push pc; pc<=mem32[gpr[A]+gpr[B]+D];
  // push pc => sp <= sp + 4, mem[sp] <= reg[pc]
  push(pc);

  if(byte1 == "20") machine->registers[pc] = machine->registers[regA] + machine->registers[regB] + d;
  else if(byte1 == "21")  machine->registers[pc] = hexadecimalToDecimal(
                                  machine->memory.find(machine->registers[regA] + machine->registers[regB] + d + 3)->second + 
                                  machine->memory.find(machine->registers[regA] + machine->registers[regB] + d + 2)->second +
                                  machine->memory.find(machine->registers[regA] + machine->registers[regB] + d + 1)->second +
                                  machine->memory.find(machine->registers[regA] + machine->registers[regB] + d + 0)->second);
  fileOUT << "PC from CALL went to : " << decimalToHexadecimal(machine->registers[pc]) << endl;
  return 0;
}
int Emulator::instrJMP(string byte1, string byte2, string byte3, string byte4){
  fileOUT << "\nJMP" << endl;
  int regA = hexadecimalToDecimal(byte2.substr(0,1));
  int regB = hexadecimalToDecimal(byte2.substr(1,1));
  int regC = hexadecimalToDecimal(byte3.substr(0,1));
  int d;
  if(byte3.at(1) == 'F')
   d = hexadecimalToDecimal("FFFFF" + byte3.substr(1,1) + byte4);
  else 
   d = hexadecimalToDecimal(byte3.substr(1,1) + byte4);

  fileOUT << "PC from JMP is : " << decimalToHexadecimal(machine->registers[pc]) << endl;

  if(byte1 == "30") machine->registers[pc] = machine->registers[regA] + d;
  else if(byte1 == "31") {
    if(machine->registers[regB] == machine->registers[regC]) machine->registers[pc] = machine->registers[regA] + d;
  }
  else if(byte1 == "32") {
    if(machine->registers[regB] != machine->registers[regC]) machine->registers[pc] = machine->registers[regA] + d;
  }
  else if(byte1 == "33") {
    if(machine->registers[regB] > machine->registers[regC]) machine->registers[pc] = machine->registers[regA] + d;
  }
  else if(byte1 == "38") machine->registers[pc] = hexadecimalToDecimal(
                                  machine->memory.find(machine->registers[regA] + d + 3)->second + 
                                  machine->memory.find(machine->registers[regA] + d + 2)->second +
                                  machine->memory.find(machine->registers[regA] + d + 1)->second +
                                  machine->memory.find(machine->registers[regA] + d + 0)->second);
  else if(byte1 == "39") {
    if(machine->registers[regB] == machine->registers[regC]) machine->registers[pc] = hexadecimalToDecimal(
                                  machine->memory.find(machine->registers[regA] + d + 3)->second + 
                                  machine->memory.find(machine->registers[regA] + d + 2)->second +
                                  machine->memory.find(machine->registers[regA] + d + 1)->second +
                                  machine->memory.find(machine->registers[regA] + d + 0)->second);
  }
  else if(byte1 == "3A") {
    if(machine->registers[regB] != machine->registers[regC]) machine->registers[pc] = hexadecimalToDecimal(
                                  machine->memory.find(machine->registers[regA] + d + 3)->second + 
                                  machine->memory.find(machine->registers[regA] + d + 2)->second +
                                  machine->memory.find(machine->registers[regA] + d + 1)->second +
                                  machine->memory.find(machine->registers[regA] + d + 0)->second);
  }
  else if(byte1 == "3B") {
    if(machine->registers[regB] > machine->registers[regC]) machine->registers[pc] = hexadecimalToDecimal(
                                  machine->memory.find(machine->registers[regA] + d + 3)->second + 
                                  machine->memory.find(machine->registers[regA] + d + 2)->second +
                                  machine->memory.find(machine->registers[regA] + d + 1)->second +
                                  machine->memory.find(machine->registers[regA] + d + 0)->second);
  }
  
  fileOUT << "PC from JMP went to : " << decimalToHexadecimal(machine->registers[pc]) << endl;
  return 0;
}
int Emulator::instrXCHG(string byte1, string byte2, string byte3, string byte4){
  fileOUT << "\nXCHG" << endl;
  int regB = hexadecimalToDecimal(byte2.substr(1,1));
  int regC = hexadecimalToDecimal(byte3.substr(0,1));
  // temp <= B; B <= C; C <= temp;
  fileOUT << "regB and regC before exchange: " << decimalToHexadecimal(machine->registers[regB]) << " " << decimalToHexadecimal(machine->registers[regC]) << endl;
  int temp = machine->registers[regB];
  machine->registers[regB] = machine->registers[regC];
  machine->registers[regC] = temp;
  machine->registers[0] = 0;
  fileOUT << "regB and regC after exchange: " << decimalToHexadecimal(machine->registers[regB]) << " " << decimalToHexadecimal(machine->registers[regC]) << endl;
  return 0;
}
int Emulator::instrARITM(string byte1, string byte2, string byte3, string byte4){
  fileOUT << "\nARITM" << endl;
  int regA = hexadecimalToDecimal(byte2.substr(0,1));
  int regB = hexadecimalToDecimal(byte2.substr(1,1));
  int regC = hexadecimalToDecimal(byte3.substr(0,1));
  // A <= B op C
  if(byte1 == "50") {         // ADD
    fileOUT << "ADD r" << regA << "<= r" << regB << ", r" << regC << endl;
    fileOUT << "reg[regA] = " << machine->registers[regA] << ", reg[regB] = " << machine->registers[regB] << ", reg[regC] = " << machine->registers[regC] << endl;
    machine->registers[regA] = machine->registers[regB] + machine->registers[regC];
    fileOUT << "reg[regA] = " << machine->registers[regA] << endl;
  } else if(byte1 == "51") {  // SUB
    fileOUT << "SUB r" << regA << "<= r" << regB << ", r" << regC << endl;
    fileOUT << "reg[regA] = " << machine->registers[regA] << ", reg[regB] = " << machine->registers[regB] << ", reg[regC] = " << machine->registers[regC] << endl;
    machine->registers[regA] = machine->registers[regB] - machine->registers[regC];
    fileOUT << "reg[regA] = " << machine->registers[regA] << endl;
  } else if(byte1 == "52") {  // MUL
    fileOUT << "MUL r" << regA << "<= r" << regB << ", r" << regC << endl;
    fileOUT << "reg[regA] = " << machine->registers[regA] << ", reg[regB] = " << machine->registers[regB] << ", reg[regC] = " << machine->registers[regC] << endl;
    machine->registers[regA] = machine->registers[regB] * machine->registers[regC];
    fileOUT << "reg[regA] = " << machine->registers[regA] << endl;
  } else if(byte1 == "53") {  // DIV
    if(machine->registers[regC] != 0) {
      fileOUT << "DIV r" << regA << "<= r" << regB << ", r" << regC << endl;
      fileOUT << "reg[regA] = " << machine->registers[regA] << ", reg[regB] = " << machine->registers[regB] << ", reg[regC] = " << machine->registers[regC] << endl;
      machine->registers[regA] = machine->registers[regB] / machine->registers[regC];
      fileOUT << "reg[regA] = " << machine->registers[regA] << endl;
    } else {
      fileOUT << "DIVISION WITH ZERO" << endl;
      // INTERRUPT
      fileOUT << "push status; push pc; cause <= 1; status &= ~0x1; pc <= handler; " << endl;
      push(status, true);
      push(pc);
      machine->csr[cause] = 1;
      machine->csr[status] &= (~0x1);
      machine->registers[pc] = machine->csr[handler];
      fileOUT << "PC from ARITM went to : " << decimalToHexadecimal(machine->registers[pc]) << endl;
    }
  }
  machine->registers[0] = 0;
  return 0;
}
int Emulator::instrLOGIC(string byte1, string byte2, string byte3, string byte4){
  fileOUT << "\nLOGIC" << endl;
  int regA = hexadecimalToDecimal(byte2.substr(0,1));
  int regB = hexadecimalToDecimal(byte2.substr(1,1));
  int regC = hexadecimalToDecimal(byte3.substr(0,1));
  // A <= B op C
  if(byte1 == "60") {         // NOT
    machine->registers[regA] = ~machine->registers[regB];
  } else if(byte1 == "61") {  // AND
    machine->registers[regA] = machine->registers[regB] & machine->registers[regC];
  } else if(byte1 == "62") {  // OR
    machine->registers[regA] = machine->registers[regB] | machine->registers[regC];
  } else if(byte1 == "63") {  // XOR
    machine->registers[regA] = machine->registers[regB] ^ machine->registers[regC];
  }
  machine->registers[0] = 0;
  return 0;
}
int Emulator::instrSH(string byte1, string byte2, string byte3, string byte4){
  fileOUT << "\nSH" << endl;
  int regA = hexadecimalToDecimal(byte2.substr(0,1));
  int regB = hexadecimalToDecimal(byte2.substr(1,1));
  int regC = hexadecimalToDecimal(byte3.substr(0,1));
  // A <= B op C
  if(byte1 == "70") {         // SHL
    machine->registers[regA] = machine->registers[regB] << machine->registers[regC];
  } else if(byte1 == "71") {  // SHR
    machine->registers[regA] = machine->registers[regB] >> machine->registers[regC];
  } 
  machine->registers[0] = 0;
  return 0;
}
int Emulator::instrSTORE(string byte1, string byte2, string byte3, string byte4){
  fileOUT << "\nSTORE" << endl;
  int regA = hexadecimalToDecimal(byte2.substr(0,1));
  int regB = hexadecimalToDecimal(byte2.substr(1,1));
  int regC = hexadecimalToDecimal(byte3.substr(0,1));
  int d;
  if(byte3.at(1) == 'F')
   d = hexadecimalToDecimal("FFFFF" + byte3.substr(1,1) + byte4);
  else 
   d = hexadecimalToDecimal(byte3.substr(1,1) + byte4);
  fileOUT << "D: " << byte3.substr(1,1) + byte4 << ", " << d << endl;
  long address;
  // mem32[gpr[A]+gpr[B]+D]<=gpr[C];
  if(byte1 == "80") address = machine->registers[regA] + machine->registers[regB] + d;
  // mem32[mem32[gpr[A]+gpr[B]+D]]<=gpr[C];
  else if(byte1 == "82") { 
    address = machine->registers[regA] + machine->registers[regB] + d;
    fileOUT << "***address1: " << decimalToHexadecimal(address);
    address = hexadecimalToDecimal(
              machine->memory.find(address + 3)->second + 
              machine->memory.find(address + 2)->second + 
              machine->memory.find(address + 1)->second +
              machine->memory.find(address + 0)->second);
    fileOUT << "  ***address2: " << decimalToHexadecimal(address);
  } 
  // gpr[A]<=gpr[A]+D; mem32[gpr[A]]<=gpr[C]; 
  else if(byte1 == "81") {  
    fileOUT << "PUSH r" << regC << endl;
    fileOUT << "reg[regA] = " << machine->registers[regA] << endl;
    
    machine->registers[regA] = machine->registers[regA] + d;
    machine->registers[0] = 0;
    address = machine->registers[regA];
    fileOUT << "(address)reg[regA] = " << machine->registers[regA];
  } 
  fileOUT << "ADRESA GDE TREBA DA SE STORUJE VREDNOST JE: 0x" << decimalToHexadecimal(address) << endl;
  string regCstring = decimalToHexadecimal(machine->registers[regC], 8);
  fileOUT << " value to push 0x" << regCstring << endl; 
    if(machine->memory.find(address) != machine->memory.end()) {
      fileOUT << "Ta adresa je pronadjena" << endl;
      fileOUT << "Vrednsot na toj adresi pre upisa: "<< machine->memory.find(address)->second << endl;
      machine->memory.find(address)->second = regCstring.substr(6,2);
      fileOUT << "Vrednsot na toj adresi posle upisa: "<< machine->memory.find(address)->second << endl;
      
    }
    else machine->memory.insert({address, regCstring.substr(6,2)});

    if(machine->memory.find(address + 1) != machine->memory.end()) 
      machine->memory.find(address + 1)->second = regCstring.substr(4,2);
    else machine->memory.insert({address + 1, regCstring.substr(4,2)});

    if(machine->memory.find(address + 2) != machine->memory.end()) 
      machine->memory.find(address + 2)->second = regCstring.substr(2,2);
    else machine->memory.insert({address + 2, regCstring.substr(2,2)});

    if(machine->memory.find(address + 3) != machine->memory.end()) 
      machine->memory.find(address + 3)->second = regCstring.substr(0,2);
    else machine->memory.insert({address + 3, regCstring.substr(0,2)});

  fileOUT << "Na adresi " << decimalToHexadecimal(machine->memory.find(address + 0)->first )<< " se nalazi " << machine->memory.find(address + 0)->second << ", a ";
  fileOUT << "na adresi " << decimalToHexadecimal(machine->memory.find(address + 1)->first )<< " se nalazi " << machine->memory.find(address + 1)->second << ", a ";
  fileOUT << "na adresi " << decimalToHexadecimal(machine->memory.find(address + 2)->first )<< " se nalazi " << machine->memory.find(address + 2)->second << ", a ";
  fileOUT << "na adresi " << decimalToHexadecimal(machine->memory.find(address + 3)->first )<< " se nalazi " << machine->memory.find(address + 3)->second << endl;
  

  machine->registers[0] = 0;
  return 0;
}

int Emulator::instrLOAD(string byte1, string byte2, string byte3, string byte4){
  int regA = hexadecimalToDecimal(byte2.substr(0,1));
  
  int regB = hexadecimalToDecimal(byte2.substr(1,1));
  int regC = hexadecimalToDecimal(byte3.substr(0,1));
  int d;
  if(byte3.at(1) == 'F')
   d = hexadecimalToDecimal("FFFFF" + byte3.substr(1,1) + byte4);
  else 
   d = hexadecimalToDecimal(byte3.substr(1,1) + byte4);
  fileOUT << "\nLOAD" << endl;
  fileOUT << "regA: " << regA << ", regB: " << regB << ", regC: " << regC << endl;
  fileOUT << "reg[regA]: " << decimalToHexadecimal(machine->registers[regA]) 
  << " reg[regB]: " << decimalToHexadecimal(machine->registers[regB]) 
  << " reg[regC]: " << decimalToHexadecimal(machine->registers[regC]) << endl;
  // fileOUT << "D: " << byte3.substr(1,1) + byte4 << ", " << d << endl;
  long address;

  // gpr[A]<=csr[B];
  if(byte1 == "90") machine->registers[regA] = machine->csr[regB];
  // gpr[A]<=gpr[B]+D;
  else if(byte1 == "91") machine->registers[regA] = machine->registers[regB] + d;
  // gpr[A]<=mem32[gpr[B]+gpr[C]+D];
  else if(byte1 == "92") 
    machine->registers[regA] = hexadecimalToDecimal(
                                  machine->memory.find(machine->registers[regB] + machine->registers[regC] + d + 3)->second + 
                                  machine->memory.find(machine->registers[regB] + machine->registers[regC] + d + 2)->second +
                                  machine->memory.find(machine->registers[regB] + machine->registers[regC] + d + 1)->second +
                                  machine->memory.find(machine->registers[regB] + machine->registers[regC] + d + 0)->second);
  // gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
  else if(byte1 == "93")  {
        fileOUT << "pop" << endl;
        fileOUT << "regA <= mem[regB] => regA: " << regA << ", regB: " << regB << endl;
        // POP PC
        if(byte2 == "FE" && byte3 == "00" && byte4 == "04") { 
          fileOUT << "RET from function" << endl;
          // pop pc;
          long temporary_pc_ = hexadecimalToDecimal( machine->memory.find(machine->registers[regB] + 3)->second + 
                                      machine->memory.find(machine->registers[regB] + 2)->second + 
                                      machine->memory.find(machine->registers[regB] + 1)->second + 
                                      machine->memory.find(machine->registers[regB] + 0)->second );
          // sp += 4;
          machine->registers[regB] = machine->registers[regB] + d;

          // check if next isntruction is pop status;
          byte4 = machine->memory.find(machine->registers[pc])->second;
          byte3 = machine->memory.find(machine->registers[pc] + 1)->second;
          byte2 = machine->memory.find(machine->registers[pc] + 2)->second;
          byte1 = machine->memory.find(machine->registers[pc] + 3)->second;
          if(byte1 == "97") { // POP STATUS
            // pop status;
            fileOUT << "IRET!!!" << endl;
            machine->csr[status] = hexadecimalToDecimal( machine->memory.find(machine->registers[regB] + 3)->second + 
                                      machine->memory.find(machine->registers[regB] + 2)->second + 
                                      machine->memory.find(machine->registers[regB] + 1)->second + 
                                      machine->memory.find(machine->registers[regB] + 0)->second );
            machine->registers[sp] += 4;
          }

          machine->registers[pc] = temporary_pc_;
        } 
        // POP some other register
        else {  
      machine->registers[regA] = hexadecimalToDecimal(
                                    machine->memory.find(machine->registers[regB] + 3)->second + 
                                    machine->memory.find(machine->registers[regB] + 2)->second + 
                                    machine->memory.find(machine->registers[regB] + 1)->second + 
                                    machine->memory.find(machine->registers[regB] + 0)->second );
      machine->registers[0] = 0;
      machine->registers[regB] = machine->registers[regB] + d;
    }
  }


  // csr[A]<=gpr[B];
  else if(byte1 == "94") machine->csr[regA] = machine->registers[regB];
  // csr[A]<=csr[B]|D;
  else if(byte1 == "95") machine->csr[regA] = machine->csr[regB] | d;
  // csr[A]<=mem32[gpr[B]+gpr[C]+D];
  else if(byte1 == "96") machine->csr[regA] = hexadecimalToDecimal(
                                  machine->memory.find(machine->registers[regB] + machine->registers[regC] + d + 3)->second + 
                                  machine->memory.find(machine->registers[regB] + machine->registers[regC] + d + 2)->second +
                                  machine->memory.find(machine->registers[regB] + machine->registers[regC] + d + 1)->second +
                                  machine->memory.find(machine->registers[regB] + machine->registers[regC] + d + 0)->second);
  // csr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
  else if(byte1 == "97") {
    fileOUT << "GRESKA!!!" << endl;
  }
  machine->registers[0] = 0;
  fileOUT << "reg[regA]: " << decimalToHexadecimal(machine->registers[regA]) 
  << " reg[regB]: " << decimalToHexadecimal(machine->registers[regB]) 
  << " reg[regC]: " << decimalToHexadecimal(machine->registers[regC]) << endl;
  return 0;
}

void Emulator::writeState() {
  cout << endl;
  cout << "------------------------------------------------------------------------------------------------------------------------" << endl;
  cout << "Emulated processor executed halt instruction" << endl;
  cout << "Emulated processor state:" << endl;
  cout << " r0=0x"      << decimalToHexadecimal(machine->registers[0], 8);
  cout << "      r1=0x" << decimalToHexadecimal(machine->registers[1], 8);
  cout << "      r2=0x" << decimalToHexadecimal(machine->registers[2], 8);
  cout << "      r3=0x" << decimalToHexadecimal(machine->registers[3], 8) << endl;
  cout << " r4=0x"      << decimalToHexadecimal(machine->registers[4], 8);
  cout << "      r5=0x" << decimalToHexadecimal(machine->registers[5], 8);
  cout << "      r6=0x" << decimalToHexadecimal(machine->registers[6], 8);
  cout << "      r7=0x" << decimalToHexadecimal(machine->registers[7], 8) << endl;
  cout << " r8=0x"      << decimalToHexadecimal(machine->registers[8], 8);
  cout << "      r9=0x" << decimalToHexadecimal(machine->registers[9], 8);
  cout << "     r10=0x" << decimalToHexadecimal(machine->registers[10], 8);
  cout << "     r11=0x" << decimalToHexadecimal(machine->registers[11], 8) << endl;
  cout << "r12=0x"      << decimalToHexadecimal(machine->registers[12], 8);
  cout << "     r13=0x" << decimalToHexadecimal(machine->registers[13], 8);
  cout << "     r14=0x" << decimalToHexadecimal(machine->registers[14], 8);
  cout << "     r15=0x" << decimalToHexadecimal(machine->registers[15], 8) << endl;
}

string Emulator::decimalToHexadecimal(long number, int width) {
  stringstream ss;
  ss << setfill('0') << setw(width) << std::hex << number;
  return ss.str();
}

long Emulator::hexadecimalToDecimal(string hexad) {
    stringstream ss;

    // Insert the hexadecimal string into the stringstream
    ss << std::hex << hexad;

    // Initialize the result variable
    long result;
    ss >> result;
    return result;
}
