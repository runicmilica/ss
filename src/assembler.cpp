#include "../inc/assembler.hpp"
#include <regex>
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <vector>

using namespace std;
int Assembler::id = 0;

Assembler::Assembler(string output, string input){
  inputFile = input;
  outputFile = output;
  
  locationCounter = 0;
  currentSectionNumber = -1;

  assemble();

}

bool Assembler::checkStart(int argc, char *argv[]) {

  if(argc < 4) return false;  // not enough arguments
  regex input("^.+\\.s$");    // at least one character + \ + .s
  regex output("^.+\\.o$");   // at least one character + \ + .o
  regex option("^-o$");       // -o


  if(regex_match(argv[1], option) && regex_match(argv[2], output) && regex_match(argv[3], input)) {
    // -o _\.o _\.s
    cout << "Argv[1] = " << argv[1] << ", Argv[2] = " << argv[2] << ", Argv[3] = " << argv[3] << endl;
    return true;
  }
  else {
    cout << "Error:checkStart: args not valid." << endl;
  }
  return false;
}

int Assembler::assemble() {
  // cout << "assemble" << endl;
  ifstream fileInput(inputFile, ios::in);   // open for reading
  if(!fileInput.is_open()) {
    std::cout << "Error: Could not open the file " << inputFile << "!" << endl;
    return -1;
  }
  // file successfully opened
  string line;
  int i = 0;
  bool endFlag = false, errorFlag = false;
  while(getline(fileInput, line)) {
    i++;
    /*  One directive per line*
      1.  Comment - starts with '#' and is inline
      2.  Label - at the beginning (behind white spaces*) + ?directive
      3.  Section [.section <section_name>], previous section ends 
      4.  End [.end] 
      5.  Global [.global <symbol_list>] (<symbol_list> a,...)
      6.  Empty line
      7.  Extern [.extern <symbol_list>]
      8.  Skip [.skip <literal>] num_of_bytes filled with 0
      9.  Word [.word <symbol_or_literal_list>], 4B for each, initialize with their values
      10. Instruction
    */
    cout << "Assembler:Line: " + line << endl;

    removeComment(line);  // deletes the comment leaving the rest
    if(labelCheck(line) < 0) {
      errorFlag = true;
      break;    // removes the label leaving the rest
    }
    // cout << "line after labelcheck in assemble" + line << endl;
    if(sectionCheck(line)) {
      cout << "Assembler:assemble: sectionCheck -> true, section name: " << line << endl;
      // in line is section_name
      // process .section with section_name
      // TO DO
      if(section(line) < 0) {
        cout << "Error:Assembler:Assemble: error while assembling. Line: " << i << endl; 
        errorFlag = true;
        break;
      } else continue;
    }
    if(endCheck(line)) {
      cout << "Assembler:assemble: endCheck -> true" << endl;
      // process .end
      // TO DO
      // URADITI DODATNE PROVERE !!!!
      endFlag = true;
      symbolTable.setSize(currentSectionNumber, locationCounter);
      setSectionSize(currentSectionNumber, locationCounter);
      currentSectionNumber = 0;
      locationCounter = 0;
      for(const auto&sym: symbolTable.getAllSymbols()) {
        if(sym.second.value == -1 && sym.second.isGlobal == true && !sym.second.isExtern) {
          cout << "Error:Assembler::assemle: symbol " + sym.second.symbolName + " not defined. " << endl;
          errorFlag = true;
          break;
        }  
      }
      if(fillRelocationTables() < 0) {
        errorFlag = true;
        break;
      }

      for(auto&sec:sectionTable) {
        int size = 0;
        for(const auto& l: sec.second.literalTable) {
          size += l.size;
        }
        cout << "size of lit table: " << size << endl;
        sec.second.size += size;
        symbolTable.setSize(sec.second.number, sec.second.size);
      }
      // printSymbolTableIntoFile();
      // printSectionTableIntoFile();
      // printSymbolUseInfoTableIntoFile();
      printForLinkerIntoFile();
      break;
    }
    if(globalCheck(line)) {
      cout << "Assembler:assemble: globalCheck -> true, line: " << line << endl;
      // process .global
      // TO DO
      int res = global(line);
      if(res < 0) {
        cout << "Error:Assembler:Assemble: error while assembling [global]. Line: " << i << endl; 
        errorFlag = true;
        break;
      }
      else continue;
    }
    if(emptyLineCheck(line)) { // DONE
      cout << "Assembler:assemble: emptyLineCheck -> true" << endl;
      // nothing to do
      continue;
    }
    if(externCheck(line)) {
      cout << "Assembler:assemble: externCheck -> true, line: " << line  << endl;
      // process .extern
      // TO DO
      int res = processExtern(line);
      // cout << "res from assemble for externcheck is " << res << endl;
      if(res < 0) {
        cout << "Error:Assembler:Assemble: error while assembling [extern]. Line: " << i << endl; 
        errorFlag = true;
        break;
      }
      continue;
    }
    if(skipCheck(line)) {
      cout << "Assembler:assemble: skipCheck -> true, line: " << line  << endl;
      // process .skip
      // TO DO
      if(skip(line) < 0) {
        cout << "Error:Assembler:Assemble: error while assembling [skip]. Line: " << i << endl; 
        errorFlag = true;
        break;
      } else continue;
    }
    if(wordCheck(line)) {
      cout << "Assembler:assemble: wordCheck -> true, line: " << line << endl;
      // process .word
      // TO DO
      if(word(line) < 0) {
        cout << "Error:Assembler:Assemble: error while assembling [word]. Line: " << i << endl; 
        errorFlag = true;
        break;
      } else continue;
    }
    int res = instructionCheck(line);
    if(res >= 0) {
      cout << "Assembler:assemble: instructionCheck -> true, line: " << line << endl;
      // process instruction_type
      // TO DO
      // errorFlag = true;
      continue;
    } 
    else if(res == -10) {
      // not instruction
    }
    else {
      cout << "Error:Assembler:Assemble: error while assembling [instruction]. Line: " << i << endl; 
      errorFlag = true;
      break;
    }

    cout << "Error:Assembler:Assemble: error while assembling. Line: " << i << endl;
    errorFlag = true;
    break;
  }
  // TO DO
  if(errorFlag) {
    ofstream file; 
    file.open ("p_" + outputFile);
    file << "Error::Assembler:: check console for more information!" << endl;
    file.close();
    return -1;
  }
  if(!endFlag) {
    cout << "Error::Assembler:: End not found." << endl;
    return -2;
  }
  // ok
  return 0;
}

void Assembler::removeComment(string& line) {
  regex r(R"(^(.*)\#.*$)");
  std::smatch commDir;
  if(regex_match(line, commDir, r)) {
    // in line remains everything but comment
    line = commDir.str(1);
  }
}
int Assembler::labelCheck(string& label) {
  regex r(R"(^\s*([a-zA-Z][a-zA-Z_0-9]*):\s*([a-zA-Z_\.][a-zA-Z_0-9\s]*)\s$)");
  regex r2(R"(^\s*([a-zA-Z][a-zA-Z_0-9]*):\s*$)");
  smatch labDir; 
  smatch lab;
  string labelName;
  if(std::regex_match(label, labDir, r)){
    // label with dir
    labelName = labDir.str(1);  // label
    label = labDir.str(2);  // remaining in line
    int res = processLabel(labelName);
    cout << "Assembler::labelCheck: " << symbolTable.getPrintRow(labelName) << endl;
    return res;
  }
  else if(std::regex_match(label, lab, r2)) {
    // just label
    labelName = lab.str(1);  // label
    label = " ";
    int res = processLabel(labelName);
    // cout << "Assembler::labelCheck: " << symbolTable.getPrintRow(labelName) << endl;
    return res;
  }
  else {
    return 0;
  }
}
bool Assembler::emptyLineCheck(string line) {
  regex r(R"(^\s*$)");
  return regex_match(line, r);
}
bool Assembler::sectionCheck(string& line) {
  regex r(R"(^\s*\.section\s+([a-zA-Z_][a-zA-Z_0-9]*)\s*$)");
  smatch section;
  if(regex_match(line, section, r)) {
    line = section.str(1);  // returns section name for processing
    return true;
  }
  return false;
}
bool Assembler::endCheck(string line) {
  regex r(R"(^\s*\.end\s*$)");
  return regex_match(line, r);
}
bool Assembler::wordCheck(string& line) {
  regex r(R"(^\s*\.word\s+((?:\s*(?:[a-zA-Z_][a-zA-Z_0-9]*|(?:\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]{1,8})\s*,)*\s*(?:[a-zA-Z_][a-zA-Z_0-9]*|(?:\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]{1,8}))\s*$)");
  smatch word;
  if(regex_match(line, word, r)){
    line = word.str(1);
    return true;
  }
  return false;
}
bool Assembler::globalCheck(string& line) {
  regex r(R"(^\s*\.global\s+((?:\s*[a-zA-Z_][a-zA-Z_0-9]*\s*,)*\s*[a-zA-Z_][a-zA-Z_0-9]*)\s*$)");
  smatch global;
  if(regex_match(line, global, r)){
    line = global.str(1);
    return true;
  }
  return false;
}
bool Assembler::externCheck(string& line) {
  regex r(R"(^\s*\.extern\s+((?:\s*[a-zA-Z_][a-zA-Z_0-9]*\s*,)*\s*[a-zA-Z_][a-zA-Z_0-9]*)\s*$)");
  smatch e;
  if(regex_match(line, e, r)){
    line = e.str(1);
    return true;
  }
  return false;
}
bool Assembler::skipCheck(string& line) {
  regex r(R"(\s*\.skip\s+(-?\d+|0[xX][0-9a-fA-F]+)\s*$)");
  smatch skip;
  if(regex_match(line, skip, r)) {
    line = skip.str(1);
    return true;
  }
  return false;
}
int Assembler::instructionCheck(string instruction) {
  regex halt_int_iret_ret(R"(^\s*(halt|int|iret|ret)\s*$)"); // 1, 2, 3, 5
  regex call(R"(^\s*(call)\s+((?:[a-zA-Z_][a-zA-Z_0-9]*)|(0x[0-9a-fA-F]+)|(\d+))\s*$)");  // 4
  regex jmp(R"(^\s*(jmp)\s+((?:[a-zA-Z_][a-zA-Z_0-9]*)|(0x[0-9a-fA-F]+)|(\d+))\s*$)");   // 6
  regex b_jump(R"(^\s*(beq|bne|bgt)\s+%(r(?:[0-9]|1[0-5])|pc|sp)\s*,\s*%(r(?:[0-9]|1[0-5])|pc|sp)\s*,\s*([a-zA-Z_][a-zA-Z_0-9]*|0x[0-9a-fA-F]+|\d+)\s*$)");  // beq, bne, bgt - 7, 8, 9
  regex push_pop_not(R"(^\s*(push|pop|not)\s+%(r(?:[0-9]|1[0-5])|pc|sp)\s*$)");  // 10, 11, 17
  regex aritm(R"(^\s*(xchg|add|sub|mul|div)\s+%(r(?:[0-9]|1[0-5])|pc|sp)\s*,\s*%(r(?:[0-9]|1[0-5])|pc|sp)\s*$)"); // 12, 13, 14, 15, 16
  regex logic(R"(^\s*(and|or|xor|shl|shr)\s+\%(r(?:[0-9]|1[0-5])|pc|sp)\s*,\s*\%(r(?:[0-9]|1[0-5])|pc|sp)\s*$)"); // 18, 19, 20, 21, 22

  regex load_immed(R"(^\s*(ld)\s+\$(0x[0-9a-fA-F]+|\d+|[a-zA-Z_][a-zA-Z_0-9]*)\s*,\s*%(r(?:[0-9]|1[0-5])|pc|sp)\s*$)");  // 23
  // regex store_immed(R"(^\s*st\s+\%(r(?:[0-9]|1[0-5])|pc|sp)\s*,\s*(?:\$0x[0-9a-fA-F]+|\$\d+|\$[a-zA-Z_][a-zA-Z_0-9]*)\s*$)"); // 24

  regex load_mem(R"(^\s*(ld)\s+(0x[0-9a-fA-F]+|\d+|[a-zA-Z_][a-zA-Z_0-9]*)\s*,\s*%(r(?:[0-9]|1[0-5])|pc|sp)\s*$)");  // 23
  regex store_mem(R"(^\s*(st)\s+%(r(?:[0-9]|1[0-5])|pc|sp)\s*,\s*(0x[0-9a-fA-F]+|\d+|[a-zA-Z_][a-zA-Z_0-9]*)\s*$)"); // 24

  regex load_reg(R"(^\s*(ld)\s+%(r(?:[0-9]|1[0-5])|pc|sp)\s*,\s*%(r(?:[0-9]|1[0-5])|pc|sp)\s*$)");  // 23
  // regex store_reg(R"(^\s*st\s+%(r(?:[0-9]|1[0-5])|pc|sp)\s*,\s*%(r(?:[0-9]|1[0-5])|pc|sp)\s*$)"); // 24

  regex load_mem_reg(R"(^\s*(ld)\s+\[%(r(?:[0-9]|1[0-5])|pc|sp)\]\s*,\s*\%(r(?:[0-9]|1[0-5])|pc|sp)\s*$)");  // 23
  regex store_mem_reg(R"(^\s*(st)\s+\%(r(?:[0-9]|1[0-5])|pc|sp)\s*,\s*\[%(r(?:[0-9]|1[0-5])|pc|sp)\]\s*$)"); // 24

  // regex load_mem_reg_pom (R"(^\s*(ld)\s+\[%(r(?:[0-9]|1[0-5])|pc|sp)\s*\+\s*(?:[a-zA-Z_][a-zA-Z_0-9]*|0x[0-9a-fA-F]+|(\d+))\]\s*,\s*\%(r(?:[0-9]|1[0-5])|pc|sp)\s*$)");  // 23
  regex load_mem_reg_pom (R"(^\s*(ld)\s+\[%\s*(r(?:[0-9]|1[0-5])|pc|sp)\s*\+\s*(?:([a-zA-Z_][a-zA-Z_0-9]*|0x[0-9a-fA-F]+|\d+))\]\s*,\s*%\s*(r(?:[0-9]|1[0-5])|pc|sp)\s*$)");

  // regex store_mem_reg_pom(R"(^\s*(st)\s+\%(r(?:[0-9]|1[0-5])|pc|sp)\s*,\s*\[%(r(?:[0-9]|1[0-5])|pc|sp)\s*\+\s*(?:[a-zA-Z_][a-zA-Z_0-9]*|0x[0-9a-fA-F]+|(\d+))\]\s*$)"); // 24
  regex store_mem_reg_pom(R"(^\s*(st)\s+%\s*(r(?:[0-9]|1[0-5])|pc|sp)\s*,\s*\[%\s*(r(?:[0-9]|1[0-5])|pc|sp)\s*\+\s*(?:([a-zA-Z_][a-zA-Z_0-9]*|0x[0-9a-fA-F]+|\d+))\]\s*$)");

  regex csrrd(R"(^\s*(csrrd)\s+%(handler|status|cause)\s*,\s+%(r(?:[0-9]|1[0-5])|pc|sp)\s*$)"); // 25
  regex csrwr(R"(^\s*(csrwr)\s+%(r(?:[0-9]|1[0-5])|pc|sp)\s*,\s+%(handler|status|cause)\s*$)"); // 26

  // is it inside section
  if(currentSectionNumber == -1) {
    cout << "Error:Assembler:Instruction: instruction is not inside section!" << endl;
    return -1;
  }
  // find current section
  SectionTableEntry* section = getCurrentSectionTableEntry(currentSectionNumber);
  smatch oper;
  string pc = getRegNum("pc");
  string sp = getRegNum("sp");
  // HALT INT IRET RET
  if(regex_match(instruction, halt_int_iret_ret))  {
    cout << "HALT_INT_IRET_RET" << endl;
    // remove white spaces
    instruction.erase(remove(instruction.begin(), instruction.end(), ' '), instruction.end());
    cout << "instruction: " << instruction << endl;
    if(instruction == "halt") {   // 00 00 00 00
      for (int i = 0; i < 4; i++) section->code.push_back("00");
      locationCounter += 4;
      return 0;
    } else if(instruction == "int") {   // 10 00 00 00 -> little endian: 00 00 00 10
      section->code.push_back("00"); section->code.push_back("00"); 
      section->code.push_back("00"); section->code.push_back("10");
      locationCounter += 4;
      return 0;
    } else if(instruction == "iret") {  
      // pop pc; pop status;
      // pop pc; ==> pop -> gpr <= mem32[sp]; sp <= sp + 4;
      // 93 FE 00 04
      section->code.push_back("04"); section->code.push_back("00");
      section->code.push_back("FE"); section->code.push_back("93"); 
      // pop status; ==> pop -> gpr <= mem32[sp]; sp <= sp + 4;
      // 97 0E 00 04
      section->code.push_back("04"); section->code.push_back("00");
      section->code.push_back("0E"); section->code.push_back("97"); 
      locationCounter += 8; // ! ! !
      return 0;
    } else if(instruction == "ret") {
      // pop pc;
      section->code.push_back("04"); section->code.push_back("00");
      section->code.push_back("FE"); section->code.push_back("93"); 
      locationCounter += 4;
      return 0;
    }
    return -1;
  }
  // CALL & JMP
  if(regex_match(instruction, oper, call) || regex_match(instruction, oper, jmp))  {
    cout << "CALL | JUMP - ";
    string instr = oper.str(1);
    string operand = oper.str(2);
    cout << "operand: " << operand << ", instruction: " + instr << endl;

  // LITERAL
    long literal;
    if(convertStringToNumber(operand, &literal)) {  // LITERAL
      string literalString = decimalToHexadecimalNoFill(literal);
      if(literalString.length() <= 3) {
        // no need for literal pool
        cout << "section: " << section->sectionName << endl;
        if(literalString.length() == 3) {
          section->code.push_back(literalString.substr(1,1) + literalString.substr(2,1));
          section->code.push_back("0" + literalString.substr(0,1));
        } else if(literalString.length() == 2) {
          section->code.push_back(literalString.substr(0,1) + literalString.substr(1,1));
          section->code.push_back("00");
        } else {
          section->code.push_back("0" + literalString);
          section->code.push_back("00");
        }
        if(instr == "call") {
          section->code.push_back("00"); section->code.push_back("20");
        }
        else {  // jmp
          section->code.push_back("00"); section->code.push_back("30");
        }
        locationCounter += 4;
        return 0;
      } 
      else if(literalString.length() > 8) {
        cout << "Error::Assembler:: literal " + literalString + " too big!!!" << endl;
        return -2;
      }
      else {
      // literal pool
      bool exists = false;
      int address;
      for(auto& lit:section->literalTable) {
        if(lit.value == literal) {
          exists = true;
          address = lit.address;
        }
      }
      if(!exists) { // literal does not exists, add it to the literal table
        cout << "Assembler::instructionCheck: call literal " + to_string(literal) + " adding to the literal table" << endl;
        address = section->literalTable.at(section->literalTable.size()-1).address + 
                      section->literalTable.at(section->literalTable.size()-1).size;
        LiteralTableEntry lte(literal, decimalToHexadecimal(literal), false, "", address, 4);
        section->literalTable.push_back(lte);
      }
      literalUse.push_back(UseInfo(locationCounter, currentSectionNumber, literal, 1, ""));
      // offset goes to instruction
      section->code.push_back("00"); section->code.push_back("00");
      if(instr == "call") {
        section->code.push_back("F0"); section->code.push_back("21");
      }
      else {  // jmp
        section->code.push_back("F0"); section->code.push_back("38");
      }
      // for(auto& c: section->code)
        // cout << " " << c << endl;
      locationCounter += 4;
      return 0;
    }
  // SYMBOL
    } 
    else {    
      if(symbolTable.exists(operand)) {   // symbol is in the table
        if(symbolTable.getId(operand) == symbolTable.getSectionNumber(operand)) {
          cout << "Error:Assembler:instructionCheck: call <symbol> -> call tried for section!!!" << endl;
          return -2;
        }
      } else {      // symbol is NOT in the table
        cout << "Symbol: '" + operand+ "' from " + instr + " added to the symbol table" << endl;
        symbolTable.add(operand);
      }
      addSymbolToLiteralTableIfNotAlreadyIn(operand);
      // displacement set to 0x000
      // offset goes to instruction
      
      section->code.push_back("00"); section->code.push_back("00");
      if(instr == "call") {
        section->code.push_back("F0"); section->code.push_back("21");
      }
      else {  // jmp
        section->code.push_back("F0"); section->code.push_back("38");
      }
      literalUse.push_back(UseInfo(locationCounter, currentSectionNumber, 0, 1, operand));

      locationCounter += 4;
      return 0;
    }
    
    return -1;
  }
  // BEQ BNE BGT
  if(regex_match(instruction, oper, b_jump))  {
    cout << "b_jump" << endl;
    // cout  << "instr: " << oper[1] << ", reg1: " << decimalToHexadecimalNoFill(getRegNum(oper[2])) 
          // << ", reg2: " << decimalToHexadecimalNoFill(getRegNum(oper[3])) << ", operand: " << oper[4] << endl;
    // if(gpr1 == gpr2) pc <= operand
    // if(gpr[B] == gpr[C]) pc <= gpr[A] + D
    string instr = oper[1];
    string reg1 = getRegNum(oper[2]);
    string reg2 = getRegNum(oper[3]);
    long o;     

    // LITERAL
    if(convertStringToNumber(oper[4], &o)) {
      string operand = decimalToHexadecimalNoFill(o);
      if(operand.length() < 4) {
        // NO NEED FOR POOL
        if(operand.length() == 3) {
          section->code.push_back(operand.substr(1,1) + operand.substr(2,1));
          section->code.push_back(reg2 + operand.substr(0,1));
        } else if(operand.length() == 2) {
          section->code.push_back(operand.substr(0,1) + operand.substr(1,1));
          section->code.push_back(reg2 + "0");
        } else {
          section->code.push_back("0" + operand);
          section->code.push_back(reg2 + "0");
        }

        section->code.push_back("0" + reg1);
        if(instr == "beq") section->code.push_back("31");
        else if(instr == "bne") section->code.push_back("32");
        else section->code.push_back("33");
        locationCounter += 4;
        return 0;
      }
      else if(operand.length() > 8) {
        cout << "Error::Assembler:: literal " + operand + " too big!!!" << endl;
        return -2;
      }
      else {
        // LITERAL POOL
        bool exists = false;
        int address;
        for(auto& lit:section->literalTable) {
          // literal is already in the pool
          if(lit.value == o) {
            exists = true;
            address = lit.address;
          }
        }
        if(!exists) { // literal does not exists, add it to the literal table
          cout << "Assembler::instructionCheck: " + instr +" literal " + to_string(o) + " adding to the literal table" << endl;
          address = section->literalTable.at(section->literalTable.size()-1).address + 
                        section->literalTable.at(section->literalTable.size()-1).size;
          LiteralTableEntry lte(o, decimalToHexadecimal(o), false, "", address, 4);
          section->literalTable.push_back(lte);
        }
        literalUse.push_back(UseInfo(locationCounter, currentSectionNumber, o, 1, ""));
        // offset goes to instruction
        section->code.push_back("00"); 
        section->code.push_back(reg2 + "0");
        section->code.push_back(pc + reg1);
        if(instr == "beq") section->code.push_back("39");
        else if(instr == "bne") section->code.push_back("3a");
        else section->code.push_back("3b");
        
        locationCounter += 4;
        return 0;
      }
    }

    // SYMBOL
    else {
      string operand = oper[4];
      if(symbolTable.exists(operand)) {   // symbol is in the table
        if(symbolTable.getId(operand) == symbolTable.getSectionNumber(operand)) {
          cout << "Error:Assembler:instructionCheck: " + instr + " <symbol> -> call tried for section!!!" << endl;
          return -2;
        }
      } else {      // symbol is NOT in the table
        cout << "Symbol: '" + operand+ "' from " + instr + " added to the symbol table" << endl;
        symbolTable.add(operand);
      }
      addSymbolToLiteralTableIfNotAlreadyIn(operand);
      // displacement set to 0x000
      // offset goes to instruction
      
      section->code.push_back("00"); 
      section->code.push_back( reg2 + "0");
      section->code.push_back(pc + reg1);
      if(instr == "beq") section->code.push_back("39");
      else if(instr == "bne") section->code.push_back("3a");
      else section->code.push_back("3b");
      
      literalUse.push_back(UseInfo(locationCounter, currentSectionNumber, 0, 1, operand));
      locationCounter += 4;
      return 0;
    }
    return -1;
  }
  // PUSH POP NOT
  if(regex_match(instruction, oper, push_pop_not))  {
    cout << "PUSH_POP_NOT" << endl;
    cout << "instr: " << oper[1] << ", reg: " <<  getRegNum(oper[2]) << endl;
    string reg = getRegNum(oper[2]);
    string sp = getRegNum("sp");
    // 04 00 reg+sp 93    POP
    // FC reg+F sp+0 81   PUSH 
    // 00 00 reg+reg 60   NOT
    if(oper[1] == "pop") {
      section->code.push_back("04");
      section->code.push_back("00");
      section->code.push_back(reg + sp);
      section->code.push_back("93");
      locationCounter += 4;
      return 0;
    } else if(oper[1] == "push") {
      section->code.push_back("fc");
      section->code.push_back(reg + "f");
      section->code.push_back(sp + "0");
      section->code.push_back("81");
      locationCounter += 4;
      return 0;
    } else if(oper[1] == "not") {
      section->code.push_back("00");
      section->code.push_back("00");
      section->code.push_back(reg + reg);
      section->code.push_back("60");
      locationCounter += 4;
      return 0;
    }
    return -1;
  }
  // XCHG ADD SUB MUL DIV
  if(regex_match(instruction, oper, aritm))  {
    cout << "ARITM" << endl;
    cout << "instr: " << oper[1] << ", reg1: " << oper[2] << ", reg2: " << oper[3] << endl;
    string reg1 = getRegNum(oper[2]);
    string reg2 = getRegNum(oper[3]);
    string instr = oper[1];
    if(instr == "xchg") {
      // 40 0+reg2 reg1+0 00
      section->code.push_back("00");
      section->code.push_back(reg1 + "0");
      section->code.push_back("0" + reg2);
      section->code.push_back("40");
      locationCounter += 4;
      return 0;
    } else if(instr == "add") {
      // 50 reg2+reg2 reg1+0 00
      section->code.push_back("00");
      section->code.push_back(reg1 + "0");
      section->code.push_back(reg2 + reg2);
      section->code.push_back("50");
      locationCounter += 4;
      return 0;
    } else if(instr == "sub") {
      // 51 reg2+reg2 reg1+0 00
      section->code.push_back("00");
      section->code.push_back(reg1 + "0");
      section->code.push_back(reg2 + reg2);
      section->code.push_back("51");
      locationCounter += 4;
      return 0;
    } else if(instr == "mul") {
      // 52 reg2+reg2 reg1+0 00
      section->code.push_back("00");
      section->code.push_back(reg1 + "0");
      section->code.push_back(reg2 + reg2);
      section->code.push_back("52");
      locationCounter += 4;
      return 0;
    } else if(instr == "div") {
      // 53 reg2+reg2 reg1+0 00
      section->code.push_back("00");
      section->code.push_back(reg1 + "0");
      section->code.push_back(reg2 + reg2);
      section->code.push_back("53");
      locationCounter += 4;
      return 0;
    }
    return -1;
  }
  // AND OR XOR SHL SHR
  if(regex_match(instruction, oper, logic))  {
    cout << "LOGIC" << endl;
    cout << "instr: " << oper[1] << ", reg1: " << oper[2] << ", reg2: " << oper[3] << endl;
    string reg1 = getRegNum(oper[2]);
    string reg2 = getRegNum(oper[3]);
    string instr = oper[1];
    section->code.push_back("00");
    section->code.push_back(reg1 + "0");
    section->code.push_back(reg2 + reg2);
    if(instr == "and") {
      section->code.push_back("61");
      locationCounter += 4;
      return 0;
    } else if(instr == "or") {
      section->code.push_back("62");
      locationCounter += 4;
      return 0;
    } else if(instr == "xor") {
      section->code.push_back("63");
      locationCounter += 4;
      return 0;
    } else if(instr == "shl") {
      section->code.push_back("70");
      locationCounter += 4;
      return 0;
    } else if(instr == "shr") {
      section->code.push_back("71");
      locationCounter += 4;
      return 0;
    }
    return -1;
  }
  // LOAD IMMED
  if(regex_match(instruction, oper, load_immed))  {
    cout << "LOAD_IMMED" << endl;
    cout << "instr: " << oper[1] << ", operand: " << oper[2] << ", reg: " << oper[3] << endl;
    string reg = getRegNum(oper[3]);
    long literal;
    // LITERAL
    // 91(2) reg+0 0l ll
    if(convertStringToNumber(oper[2], &literal)) {
      string literalString = decimalToHexadecimalNoFill(literal);
      if(literalString.length() < 4) {
        // NO NEED FOR POOL
        if(literalString.length() == 3) {
          section->code.push_back(literalString.substr(1,1) + literalString.substr(2,1));
          section->code.push_back("0" + literalString.substr(0,1));
        } else if(literalString.length() == 2) {
          section->code.push_back(literalString.substr(0,1) + literalString.substr(1,1));
          section->code.push_back("00");
        } else {
          section->code.push_back("0" + literalString);
          section->code.push_back("00");
        }

        section->code.push_back(reg + "0"); section->code.push_back("91");
        
      }
      else if(literalString.length() > 8) {
        cout << "Error::Assembler:: literal " + literalString + " too big!!!" << endl;
        return -2;
      }
      else {
        // LITERAL POOL
        bool exists = false;
        int address;
        for(auto& lit:section->literalTable) {
          // literal is already in the pool
          if(lit.value == literal) {
            exists = true;
            address = lit.address;
          }
        }
        if(!exists) { // literal does not exists, add it to the literal table
          cout << "Assembler::instructionCheck:  literal " + to_string(literal) + " adding to the literal table" << endl;
          address = section->literalTable.at(section->literalTable.size()-1).address + 
                        section->literalTable.at(section->literalTable.size()-1).size;
          LiteralTableEntry lte(literal, decimalToHexadecimal(literal), false, "", address, 4);
          section->literalTable.push_back(lte);
        }
        literalUse.push_back(UseInfo(locationCounter, currentSectionNumber, literal, 1, ""));
        // offset goes to instruction
        // 92 reg+PC 00 00
        section->code.push_back("00"); section->code.push_back("00");
        section->code.push_back(reg + pc);
        section->code.push_back("92");
      }
    }
    // SYMBOL 
    else {
      string operand = oper[2];
      if(symbolTable.exists(operand)) {   // symbol is in the table
        if(symbolTable.getId(operand) == symbolTable.getSectionNumber(operand)) {
          cout << "Error:Assembler:instructionCheck: ld $<symbol>, gpr -> tried for section!!!" << endl;
          return -2;
        }
      } else {      // symbol is NOT in the table
        cout << "Symbol: '" + operand+ "' from "  + " added to the symbol table" << endl;
        symbolTable.add(operand);
      }
      addSymbolToLiteralTableIfNotAlreadyIn(operand);
      // displacement set to 0x000
      // offset goes to instruction + PC!!!
      
      section->code.push_back("00"); section->code.push_back("00");
      section->code.push_back(reg + pc); section->code.push_back("92");
      
      literalUse.push_back(UseInfo(locationCounter, currentSectionNumber, 0, 1, operand));
    }
    locationCounter += 4;
    return 0;
  }
 // LOAD MEM
  if(regex_match(instruction, oper, load_mem))  {
    // 2 instructions for literal > 12b and symbol!!! 
    cout << "LOAD_MEM" << endl;
    cout << "instr: " << oper[1] << ", operand: " << oper[2] << ", reg: " << oper[3] << endl;
    string reg = getRegNum(oper[3]);
    long literal;
    // LITERAL
    // 92 reg+0 0l ll + 92 reg+reg 00 00
    if(convertStringToNumber(oper[2], &literal)) {
      string literalString = decimalToHexadecimalNoFill(literal);
      if(literalString.length() < 4) {
        // NO NEED FOR POOL
        if(literalString.length() == 3) {
          section->code.push_back(literalString.substr(1,1) + literalString.substr(2,1));
          section->code.push_back("0" + literalString.substr(0,1));
        } else if(literalString.length() == 2) {
          section->code.push_back(literalString.substr(0,1) + literalString.substr(1,1));
          section->code.push_back("00");
        } else {
          section->code.push_back("0" + literalString);
          section->code.push_back("00");
        }

        section->code.push_back(reg + "0"); section->code.push_back("92");
        
      }
      else if(literalString.length() > 8) {
        cout << "Error::Assembler:: literal " + literalString + " too big!!!" << endl;
        return -2;
      }
      else {
        // LITERAL POOL
        bool exists = false;
        int address;
        for(auto& lit:section->literalTable) {
          // literal is already in the pool
          if(lit.value == literal) {
            exists = true;
            address = lit.address;
          }
        }
        if(!exists) { // literal does not exists, add it to the literal table
          cout << "Assembler::instructionCheck:  literal " + to_string(literal) + " adding to the literal table" << endl;
          address = section->literalTable.at(section->literalTable.size()-1).address + 
                        section->literalTable.at(section->literalTable.size()-1).size;
          LiteralTableEntry lte(literal, decimalToHexadecimal(literal), false, "", address, 4);
          section->literalTable.push_back(lte);
        }
        literalUse.push_back(UseInfo(locationCounter, currentSectionNumber, literal, 1, ""));
        // offset goes to instruction
        section->code.push_back("00"); section->code.push_back("00");
        section->code.push_back(reg + pc);
        section->code.push_back("92");
        // mem[mem[D]]
        section->code.push_back("00"); section->code.push_back("00");
        section->code.push_back(reg + reg);
        section->code.push_back("92");
        locationCounter += 4;
      }
    }
    // SYMBOL 
    else {
      string operand = oper[2];
      if(symbolTable.exists(operand)) {   // symbol is in the table
        if(symbolTable.getId(operand) == symbolTable.getSectionNumber(operand)) {
          cout << "Error:Assembler:instructionCheck: ld <symbol>, gpr -> tried for section!!!" << endl;
          return -2;
        }
      } else {      // symbol is NOT in the table
        cout << "Symbol: '" + operand+ "' from "  + " added to the symbol table" << endl;
        symbolTable.add(operand);
      }
      addSymbolToLiteralTableIfNotAlreadyIn(operand);
      // displacement set to 0x000
      // offset goes to instruction + pc!!!
      
      section->code.push_back("00"); section->code.push_back("00");
      section->code.push_back(reg + pc); section->code.push_back("92");
      
      literalUse.push_back(UseInfo(locationCounter, currentSectionNumber, 0, 1, operand));
      // mem[mem[D]]
      section->code.push_back("00"); section->code.push_back("00");
      section->code.push_back(reg + reg);
      section->code.push_back("92");
      locationCounter += 4;
    }
    locationCounter += 4;
    return 0;
  }
  // STORE MEM
  if(regex_match(instruction, oper, store_mem))  {
    cout << "STORE_MEM" << endl;
    cout << "instr: " << oper[1] << ", operand: " << oper[3] << ", reg: " << oper[2] << endl;
    string reg = getRegNum(oper[2]);
    
    long literal;
    // LITERAL
    // 80 00 reg+l ll
    // OR
    // 82 pc0 reg+l ll for pool
    if(convertStringToNumber(oper[3], &literal)) {
      string literalString = decimalToHexadecimalNoFill(literal);
      if(literalString.length() < 4) {
        // NO NEED FOR POOL
        if(literalString.length() == 3) {
          section->code.push_back(literalString.substr(1,1) + literalString.substr(2,1));
          section->code.push_back(reg + literalString.substr(0,1));
        } else if(literalString.length() == 2) {
          section->code.push_back(literalString.substr(0,1) + literalString.substr(1,1));
          section->code.push_back(reg + "0");
        } else {
          section->code.push_back("0" + literalString);
          section->code.push_back(reg + "0");
        }

        section->code.push_back("00"); section->code.push_back("80");
        
      }
      else if(literalString.length() > 8) {
        cout << "Error::Assembler:: literal " + literalString + " too big!!!" << endl;
        return -2;
      }
      else {
        // LITERAL POOL
        bool exists = false;
        int address;
        for(auto& lit:section->literalTable) {
          // literal is already in the pool
          if(lit.value == literal) {
            exists = true;
            address = lit.address;
          }
        }
        if(!exists) { // literal does not exists, add it to the literal table
          cout << "Assembler::instructionCheck:  literal " + to_string(literal) + " adding to the literal table" << endl;
          address = section->literalTable.at(section->literalTable.size()-1).address + 
                        section->literalTable.at(section->literalTable.size()-1).size;
          LiteralTableEntry lte(literal, decimalToHexadecimal(literal), false, "", address, 4);
          section->literalTable.push_back(lte);
        }
        literalUse.push_back(UseInfo(locationCounter, currentSectionNumber, literal, 1, ""));
        // offset goes to instruction + PC!!!
        section->code.push_back("00"); 
        section->code.push_back(reg + "0");
        section->code.push_back(pc + "0");
        // POOL -> 82
        section->code.push_back("82");
      }
    }
    // SYMBOL 
    else {
      string operand = oper[3];
      if(symbolTable.exists(operand)) {   // symbol is in the table
        if(symbolTable.getId(operand) == symbolTable.getSectionNumber(operand)) {
          cout << "Error:Assembler:instructionCheck: ld $<symbol>, gpr -> tried for section!!!" << endl;
          return -2;
        }
      } else {      // symbol is NOT in the table
        cout << "Symbol: '" + operand+ "' from "  + " added to the symbol table" << endl;
        symbolTable.add(operand);
      }
      addSymbolToLiteralTableIfNotAlreadyIn(operand);
      // displacement set to 0x000
      // offset goes to instruction
      
      section->code.push_back("00"); 
      section->code.push_back(reg + "0");
      section->code.push_back(pc + "0");
      section->code.push_back("82");
      
      literalUse.push_back(UseInfo(locationCounter, currentSectionNumber, 0, 1, operand));
    }
    locationCounter += 4;
    return 0;
  }
  // LOAD REG
  if(regex_match(instruction, oper, load_reg))  {
    cout << "LOAD_REG" << endl;
    cout << "instr: " << oper[1] << ", operand: " << oper[2] << ", reg: " << oper[3] << endl;
    string reg1 = getRegNum(oper[2]);
    string reg2 = getRegNum(oper[3]);
    // 91 reg2+reg1 00 00
    section->code.push_back("00");
    section->code.push_back("00");
    section->code.push_back(reg2 + reg1);
    section->code.push_back("91");
    locationCounter += 4;
    return 0;
  }
  // LOAD MEM REG
  if(regex_match(instruction, oper, load_mem_reg))  {
    cout << "LOAD_MEM_REG" << endl;
    cout << "instr: " << oper[1] << ", operand: " << oper[2] << ", reg: " << oper[3] << endl;
    string reg1 = getRegNum(oper[2]);
    string reg2 = getRegNum(oper[3]);
    // 92 reg2+reg1 00 00
    section->code.push_back("00");
    section->code.push_back("00");
    section->code.push_back(reg2 + reg1);
    section->code.push_back("92");
    locationCounter += 4;
    return 0;
  }
  // STORE MEM REG
  if(regex_match(instruction, oper, store_mem_reg))  {
    cout << "STORE_MEM_REG" << endl;
    cout << "instr: " << oper[1] << ", operand: " << oper[2] << ", reg: " << oper[3] << endl;
    string reg1 = getRegNum(oper[2]); // gpr[C]
    string reg2 = getRegNum(oper[3]); // gpr[A]
    // 80 reg2+0 reg1+0 00
    section->code.push_back("00");
    section->code.push_back(reg1 + "0");
    section->code.push_back(reg2 + "0");
    section->code.push_back("80");
    locationCounter += 4;
    return 0;
  }
  // LOAD MEM REG POM
  if(regex_match(instruction, oper, load_mem_reg_pom))  {
    cout << "LOAD_MEM_REG_POM" << endl;
    cout << "instr: " << oper[1] << ", operand: " << oper[2] << ", literal/symbol: " << oper[3] << ", reg: " << oper[4] << endl;
    // check if literal or symbol
    string reg1 = getRegNum(oper[2]);
    string reg2 = getRegNum(oper[4]);
    long literal;
    // LITERAL
    if(convertStringToNumber(oper[3], &literal)) {
      string literalString = decimalToHexadecimalNoFill(literal);
      if(literalString.length() > 3) {
        cout << "Error::Assembler:: literal in LOAD_MEM_REG_POM instruction too big!!!" << endl;
        return -1;
      }
      // 92 reg2+reg1 0l ll
      if(literalString.length() == 3) {
        section->code.push_back(literalString.substr(1, 1) + literalString.substr(2, 1));
        section->code.push_back("0" + literalString.substr(0, 1));
      } else if(literalString.length() == 2) {
        section->code.push_back(literalString.substr(0, 1) + literalString.substr(1, 1));
        section->code.push_back("00");
      } else {
        section->code.push_back("0" + literalString.substr(0, 1));
        section->code.push_back("00");
      }
      section->code.push_back(reg2 + reg1);
      section->code.push_back("92");
    }
    // SYMBOL 
    else {
      /* string symbol = oper[3];
      // does symbol exists
      if(symbolTable.exists(symbol)) {   // symbol is in the table
        if(symbolTable.getId(symbol) == symbolTable.getSectionNumber(symbol)) {
          cout << "Error:Assembler:instructionCheck: ld <symbol> -> call tried for section!!!" << endl;
          return -2;
        }
      } else {      // symbol is NOT in the table
        cout << "Symbol: '" + symbol+ "' from " + oper[1] + " added to the symbol table" << endl;
        symbolTable.add(symbol);
      }
      section->code.push_back("00");        section->code.push_back("00");
      section->code.push_back(reg2 + reg1); section->code.push_back("92");

      // type 2 - check if symbol defined-if not ERROR
      //        - check if symbol value can be written in 12b-if not ERROR
      literalUse.push_back(UseInfo(locationCounter, currentSectionNumber, 0, 2, symbol)); */
      cout << "Error:Assembler:instructionCheck: ld [%<reg> + <simbol>], gpr !!!" << endl;
      return -2;
    }
    
    locationCounter += 4;
    return 0;
  }
  // STORE MEM REG POM
  if(regex_match(instruction, oper, store_mem_reg_pom))  {
    cout << "STORE_MEM_REG_POM" << endl;
    cout << "instr: " << oper[1] << ", reg: " << oper[2] << ", operand: " << oper[3] << ", literal/symbol: " << oper[4]<< endl;
    // check if literal or symbol
    string reg1 = getRegNum(oper[2]);
    string reg2 = getRegNum(oper[3]);
    long literal;
    // LITERAL
    if(convertStringToNumber(oper[4], &literal)) {
      string literalString = decimalToHexadecimalNoFill(literal);
      if(literalString.length() > 3) {
        cout << "Error::Assembler:: literal in STORE_MEM_REG_POM instruction too big!!!" << endl;
        return -1;
      }
      // 80 reg2+0 reg1+l ll
      if(literalString.length() == 3) {
        section->code.push_back(literalString.substr(1, 1) + literalString.substr(2, 1));
        section->code.push_back(reg1 + literalString.substr(0, 1));
      } else if(literalString.length() == 2) {
        section->code.push_back(literalString.substr(0, 1) + literalString.substr(1, 1));
        section->code.push_back(reg1 + "0");
      } else {
        section->code.push_back("0" + literalString.substr(0, 1));
        section->code.push_back(reg1 + "0");
      }
      section->code.push_back(reg2 + "0");
      section->code.push_back("80");
    }
    // SYMBOL 
    else {
      /* string symbol = oper[4];
      // does symbol exists
      if(symbolTable.exists(symbol)) {   // symbol is in the table
        if(symbolTable.getId(symbol) == symbolTable.getSectionNumber(symbol)) {
          cout << "Error:Assembler:instructionCheck: st <symbol> -> st tried for section!!!" << endl;
          return -2;
        }
      } else {      // symbol is NOT in the table
        cout << "Symbol: '" + symbol+ "' from " + oper[1] + " added to the symbol table" << endl;
        symbolTable.add(symbol);
      }
      section->code.push_back("00");       section->code.push_back(reg1 + "0");
      section->code.push_back(reg2 + "0"); section->code.push_back("80");

      // type 2 - check if symbol defined-if not ERROR
      //        - check if symbol value can be written in 12b-if not ERROR
      literalUse.push_back(UseInfo(locationCounter, currentSectionNumber, 0, 2, symbol)); */
      cout << "Error:Assembler:instructionCheck: st gpr, [%<reg> + <simbol>] !!!" << endl;
      return -2;
    }

    locationCounter += 4;
    return 0;
  }
  // CSRRD CSRWR
  if(regex_match(instruction, oper, csrrd))  {
    cout << "CSRRD" << endl;
    // cout << "instr: " << oper[1] << ", csr: " << oper[2] << ", reg: " << oper[3] << endl;
    string reg1 = (oper[2] == "status") ? "0" : ((oper[2] == "handler") ? "1" : "2");
    string reg2 = getRegNum(oper[3]);
    cout << reg1 << endl;
    section->code.push_back("00");
    section->code.push_back("00");
    section->code.push_back(reg2 + reg1);
    section->code.push_back("90");
    locationCounter += 4;
    return 0;
  }
  if(regex_match(instruction, oper, csrwr))  {
    cout << "CSRWR" << endl;
    // cout << "instr: " << oper[1] << ", reg: " << oper[2] << ", csr: " << oper[3] << endl;
    string reg1 = getRegNum(oper[2]);
    string reg2 = (oper[3] == "status") ? "0" : ((oper[3] == "handler") ? "1" : "2");
    string instr = oper[1];
    section->code.push_back("00");
    section->code.push_back("00");
    section->code.push_back(reg2 + reg1);
    section->code.push_back("94");
    locationCounter += 4;
    return 0;
  }
  return -10;
}

int Assembler::skip(string literal) {
  if(currentSectionNumber == -1) {
    cout << "Error::Assembler:skip: not in section!" << endl;
    return -1;
  }
  regex rHEX("^0[xX][0-9a-fA-F]+$");
  regex rDEC("^[0-9]+$");
  int num;
  if(regex_match(literal, rDEC)) {
    // decimal number
    // cout << "literal: _" + literal + "_ : " << to_string(stoi(literal, nullptr, 10)) << endl;
    num = stol(literal, nullptr, 10);
  } else if(regex_match(literal, rHEX)) {
    // hexadecimal number
    // cout << "literal: _" + literal + "_ : " << to_string(stoi(literal, nullptr, 16))<< endl;
    num = stol(literal, nullptr, 16);
  }
  SectionTableEntry* s = getCurrentSectionTableEntry(currentSectionNumber);
  for (int i = 0; i < num; i++)
  {
    s->code.push_back("00");
    locationCounter += 1;
  }
  return 0;
}

int Assembler::processLabel(string labelName) {
  if(symbolTable.exists(labelName)) {
    // symbol in table
    // check if symbol is extern
    if(symbolTable.getIsExtern(labelName)) {
      cout << "found" << endl;
      cout << "Error:Assembler:processLabel: definition of extern symbol: " << labelName << "!" << endl;
      return -1;
    }
    symbolTable.setSectionNumber(labelName, currentSectionNumber);
    symbolTable.setValue(labelName, locationCounter);
  } else {
    // symbol not in table
    cout << "Symbol: '" + labelName + "' from label added to the symbol table" << endl;
    symbolTable.add(labelName);
    symbolTable.setSectionNumber(labelName, currentSectionNumber);
    symbolTable.setValue(labelName, locationCounter);
  }
    /* SectionTableEntry* section = getCurrentSectionTableEntry(currentSectionNumber);
    bool exists = false;
    int addr;
    for(auto& lit:section->literalTable) {
      if(lit.symbolName == labelName) {
        // symbol already used so it is in literal table
        cout << "Definition of symbol: " + labelName << endl;
        exists = true;
        lit.value = locationCounter;
        lit.hexValue = decimalToHexadecimal((int)locationCounter);
      }
    }
    if(!exists) {
      int address = section->literalTable.at(section->literalTable.size()-1).address + 
                      section->literalTable.at(section->literalTable.size()-1).size;
      LiteralTableEntry lte(locationCounter, decimalToHexadecimal((int)locationCounter), true, labelName, address, 4);
      section->literalTable.push_back(lte);
    }*/
    // for situations where definition is after usage of symbol
    // if symbol is used in other sections, pool needs to be fixed
    // symbol value has to update 
    /*for(auto& s: sectionTable) {
      if(s.second.number != section->number) {
        for(auto& lit:s.second.literalTable) {
          if(lit.symbolName == labelName) {
            // symbol already used so it is in literal table
            lit.value = locationCounter;
            lit.hexValue = decimalToHexadecimal((int)locationCounter);
          }
        }
      }
    }*/
  return 0;
}

int Assembler::global(string symbols) {
  vector<string> result;
  regex re("\\s*,\\s*");  // Regex to match commas with optional surrounding whitespace
  sregex_token_iterator it(symbols.begin(), symbols.end(), re, -1);
  sregex_token_iterator end;
  for (; it != end; ++it) {
      result.push_back(it->str());
  }
  for(int i = 0; i < result.size(); i++) {
    // for each symbol in <symbol_list>
    // check if symbol exists in symbol table
    if(symbolTable.exists(result[i])) {
      // exists
      // check if symbol is sectionName
      if(symbolTable.getSectionNumber(result[i]) == symbolTable.getId(result[i])) {
        cout << "Error:Assembler::global: .global for section -> " + result[i] + "!" << endl;
        return -1;
      }
      // check if symbol is extern
      if(symbolTable.getIsExtern(result[i])) {
        cout << "Error:Assembler::global: .global for extern symbol -> " + result[i] + "!" << endl;
        return -2;
      }
      // set isGlobal = true
      symbolTable.setIsGlobal(result[i], true);
    } else {
      // does not exists
      // add symbol and set isGlobal = true
      cout << "Symbol: '" + result[i] + "' from .global added to the symbol table" << endl;
      symbolTable.add(result[i]);
      symbolTable.setIsGlobal(result[i], true);
      // addSymbolToLiteralTableIfNotAlreadyIn(result[i]);
    }
  }
  return 0;
}

int Assembler::processExtern(string symbols) {
  vector<string> result;
  regex re("\\s*,\\s*");  // Regex to match commas with optional surrounding whitespace
  sregex_token_iterator it(symbols.begin(), symbols.end(), re, -1);
  sregex_token_iterator end;
  for (; it != end; ++it) {
      result.push_back(it->str());
  }
  for(int i = 0; i < result.size(); i++) {
    // for each symbol in <symbol_list>
    // check if symbol exists in symbol table
    if(symbolTable.exists(result[i])) {
      // exists
      // check if symbol is sectionName
      if(symbolTable.getSectionNumber(result[i]) == symbolTable.getId(result[i])) {
        cout << "Error:Assembler::processExtern: .extern for section -> " + result[i] + "!" << endl;
        return -1;
      }
      // check if symbol is global
      if(symbolTable.getIsGlobal(result[i]) && !symbolTable.getIsExtern(result[i])) {
        cout << "Error:Assembler::processExtern: .extern for global symbol -> " + result[i] + "!" << endl;
        return -2;
      }
      // check if already defined
      if(symbolTable.getSectionNumber(result[i]) != 0) {
        cout << "Error:Assembler::processExtern: .extern for defined symbol -> " + result[i] + "!" << endl;
        return -3;
      }
      // set isExtern = true
      symbolTable.setIsExtern(result[i], true);
      symbolTable.setIsGlobal(result[i], true); // for linker
    } else {
      // does not exists
      // add symbol and set isGlobal = true and set isExtern = true
      cout << "Symbol: '" + result[i] + "' from .extern added to the symbol table" << endl;
      symbolTable.add(result[i]);
      symbolTable.setIsExtern(result[i], true);
      symbolTable.setIsGlobal(result[i], true); // for linker
      // addSymbolToLiteralTableIfNotAlreadyIn(result[i]);
    }
  }
  return 0;
}

void Assembler::printSectionTableIntoFile() {
  ofstream file; 
  file.open ("p_" + outputFile, ios::app);

  // literal use
    file << "- - - - - - - - - - - - - - - - literal use - - - - - - - - - - - - - - -" << endl;
      file 
          << std::setw(10) << "section | "
          << std::setw(15) << "  value   | "
          << std::setw(15) << "  symbolName | "
          << std::setw(10) << "address | "
          << std::setw(8) << "type | "  << endl;
      file << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -" << endl;
      
      for (const auto& c: literalUse) {
          file 
            << left << std::setw(10) << to_string(c.section) 
            << std::setw(15) << " 0x" + decimalToHexadecimal(c.value)
            << std::setw(15) << setfill(' ')<< c.symbolName
            << std::setw(10) << " " + to_string(c.address)
            << std::setw(8) << " " + to_string(c.type)  << endl;
      }
  
  for (const auto& s: sectionTable)
  {
    file << "- - - - - - - - - - - - - - - - SECTION TABLE - - - - - - - - - - - - - - - -" << endl;
    file << std::setw(15) << "sectionName | "
          << std::setw(15) << "startAddress | "
          << std::setw(8) << "number | "
          << std::setw(8) << "size | "  << endl;
    file << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -" << endl;
    file << std::setw(15) << std::left << s.second.sectionName 
    << std::setw(15) << "    " + to_string(s.second.startAddress) 
    << std::setw(8) << "  " + to_string(s.second.number) 
    << std::setw(8) << "  " + to_string(s.second.size) << endl;
    int i = 0;
    // CODE
    if(s.second.code.empty()) {
      file << "No code in section: " + s.second.sectionName << endl;
    } else {
      file << "- - - - - - - - - - - - - - - - code - - - - - - - - - - - - - - - -" << endl;
      for (const auto& c: s.second.code) {
        file << c << " ";
        i++;
        if(i%8 == 0) file << endl;
      }
      file << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -" << endl << endl;
    }
    file << endl;
    
    i = 0;
    // LITERAL POOL
    if(s.second.literalTable.size() == 1) {
      file << "No literals in section: " + s.second.sectionName << endl;
    } else {
      file << "- - - - - - - - - - - - - - - - literal pool - - - - - - - - - - - - - - -" << endl;
      file << std::setw(15) << "     value   | "
          << std::setw(15) << "  symbolName | "
          << std::setw(10) << "address | "
          << std::setw(8) << "size | "  << endl;
      file << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -" << endl;
      bool first = true;
      for (const auto& c: s.second.literalTable) {
        if(first) first = false;
        else {
          file << std::setw(15) << " 0x" + c.hexValue
            << std::setw(15) << setfill(' ')<< c.symbolName
            << std::setw(10) << " " + to_string(c.address)
            << std::setw(8) << " " + to_string(c.size)  << endl;
          i++;
          if(i%8 == 0) file << endl;
        }
      }
    }
    file << endl;
    file << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -" << endl << endl;
    
    s.second.relocTable->printRelocationTableForSectionIntoFile(outputFile);
  }
  
  file.close();
}

void Assembler::printSymbolTableIntoFile() {
  symbolTable.printSymbolTable(outputFile, true);
}

void Assembler::printForLinkerIntoFile() {
  symbolTable.printSymbolTableForLinker(outputFile);
  ofstream file; 
  file.open(outputFile, ios::app);
  int i = 0;

  int max = symbolTable.getMaxId();
  // to keep same order from input file
  for(int id = 0; id < max; i++) {
    for(const auto& sec:sectionTable) {
      if(sec.second.number == id) {
          i++;
          SectionTableEntry section = sec.second;
          file << ":SECTION:" << endl;
          file << section.sectionName << endl;

          // relocations
          section.relocTable->printForLinker(outputFile);
          
          // code
          file << ":CODE:" << endl;
          if(section.code.size() < 1)
            file << "NOTHING" << endl;
          else {
            for(const auto& c: section.code)
              file << c << " ";
            file << endl;
          }
          
          // pool
          file << ":LITERAL POOL:" << endl;
          if(section.literalTable.size() < 2)
            file << "NOTHING" << endl;
          else {
            file << "hexValue:symName:address:size:isSym:" << endl;
            bool first = true;
            for(const auto& l:section.literalTable) {
              if(first) first = false;
              else {
                file << setw(8) << l.hexValue << ":"
                << setw(7) << l.symbolName << ":"
                << setw(7) << decimalToHexadecimalNoFill(l.address) << ":"
                << setw(4) << decimalToHexadecimalNoFill(l.size) << ":"
                << setw(5) << l.isSymbol << ":" << endl;
              }
            }
          }
          file << ":END:" << endl;
        }
    }
    id++;
  }

  file.close();
}

void Assembler::setSectionSize(int num, long size) {
  for(auto& s: sectionTable) {
    if(s.second.number == num) {
      s.second.size = size;
      break;
    }
  }
}

SectionTableEntry* Assembler::getCurrentSectionTableEntry(int num) {
  for(auto& s: sectionTable) {
    if(s.second.number == num) {
      return &s.second;
    }
  }
  return nullptr;
}

int Assembler::section(string sectionName) {
  // check if section already exists in symbol table
  if(symbolTable.exists(sectionName)) {
    // is it section or symbol?
    if(symbolTable.getId(sectionName) == symbolTable.getSectionNumber(sectionName)) {
      // section
      // end previous section
      symbolTable.setSize(currentSectionNumber, locationCounter);
      setSectionSize(currentSectionNumber, locationCounter);
      // cout << "Current section ends: " + to_string(currentSectionNumber) << ", size is " + to_string(newSize) << endl;
      // reopen section
      locationCounter = symbolTable.getSize(sectionName); // size of reopened section
      currentSectionNumber = symbolTable.getId(sectionName);
      // cout << "Section starts: " + to_string(currentSectionNumber) << ", size is " + to_string(locationCounter) << endl;
      
    } else {
      // symbol -> ERROR
      cout << "Assembler::section: symbol '" + sectionName + "' already exists in symbol table!" << endl;
      return -1;
    }
  } else {
    // end previous section
    symbolTable.setSize(currentSectionNumber, locationCounter);
    setSectionSize(currentSectionNumber, locationCounter);
    // cout << "Current section ends: " + to_string(currentSectionNumber) << ", size is " + to_string(newSize) << endl;
    // start new section and add it to the symbol table
    cout << "Symbol: '" + sectionName + "' from .section added to the symbol table" << endl;
    symbolTable.add(sectionName);
    currentSectionNumber = symbolTable.getId(sectionName);
    symbolTable.setSectionNumber(sectionName, currentSectionNumber);
    symbolTable.setSize(currentSectionNumber, 0);
    locationCounter = 0;

    SectionTableEntry ste;
    ste.sectionName = sectionName;
    ste.number = currentSectionNumber;
    sectionTable.insert(pair<string, SectionTableEntry>(sectionName, ste));
    // cout << "Section starts: " + to_string(currentSectionNumber) << ", size is " + to_string(locationCounter) << endl;
    // create new relocation table for this section
    RelocationTable* relTab = new RelocationTable(sectionName);
    for(auto& sec:sectionTable) {
      if(sec.second.sectionName == sectionName) {
        sec.second.relocTable = relTab;
        sec.second.literalTable.push_back(LiteralTableEntry(0, "", false, "", 0, 0));
        break;
      }
    }
    // create literal table for this section
    
  }
  return 0;
}

int Assembler::word(string params) { // not done, just for testing
  if(currentSectionNumber == -1) {
    cout << "Error::Assembler:word: not in section!" << endl;
    return -1;
  }
  vector<string> result;
  regex re("\\s*,\\s*");  // Regex to match commas with optional surrounding whitespace
  sregex_token_iterator it(params.begin(), params.end(), re, -1);
  sregex_token_iterator end;
  for (; it != end; ++it) {
      result.push_back(it->str());
  }
  for(int i = 0; i < result.size(); i++) {
    // cout << result[i] << " location counter after increment " << locationCounter << endl;
    // decimal, hexadecimal, symbol
    regex rHEX("^0[xX][0-9a-fA-F]+$");
    regex rDEC("^[0-9]+$");
    // NUMBER
    if(regex_match(result[i], rHEX) || regex_match(result[i], rDEC)) {
      cout << "NUMBER" << endl;
      
      int num;
      if(regex_match(result[i], rDEC))  num = stol(result[i], nullptr, 10);
      else if(regex_match(result[i], rHEX)) num = stol(result[i], nullptr, 16);
      
      string numString = decimalToHexadecimal(num);
      if(numString.length() > 8) {
        cout << "Error::Assembler:word: literal " + numString + " too big." << endl;
        return -2;
      }
      cout << "Number from .word: " + numString << endl;
      // write 4B into code
      SectionTableEntry* s = getCurrentSectionTableEntry(currentSectionNumber);
      s->code.push_back(numString.substr(6,1) + "" + numString.substr(7,1));
      s->code.push_back(numString.substr(4,1) + "" + numString.substr(5,1));
      s->code.push_back(numString.substr(2,1) + "" + numString.substr(3,1));
      s->code.push_back(numString.substr(0,1) + "" + numString.substr(1,1));
    } 
    // SYMBOL
    else {
      cout << "SYMBOL" << endl;
      if(!symbolTable.exists(result[i])) { // symbol not in symbol table
        cout << "Symbol: '" + result[i] + "' from .word added to the symbol table" << endl;
        symbolTable.add(result[i]);
        // addSymbolToLiteralTableIfNotAlreadyIn(result[i]);
      } 
      // new info about using symbol here
      /* Info newInfo;
      newInfo.address = locationCounter;
      newInfo.sectionName = getCurrentSectionTableEntry(currentSectionNumber)->sectionName;
      newInfo.type = 1; // absolute
      symbolTable.addInfoToSymbol(result[i], newInfo); // info -> this symbol used here  */


      // CHANGE - use only literalUse
      literalUse.push_back(UseInfo(locationCounter, currentSectionNumber, 0, 0, result[i]));

      // add 4B of code to the section
      SectionTableEntry* s = getCurrentSectionTableEntry(currentSectionNumber);
      for (int i = 0; i < 4; i++) s->code.push_back("00");
    }
    locationCounter += 4;
  }
  return 0;
}

int Assembler::fillRelocationTables() {
  /* map<string, SymbolTableEntry> symbols = symbolTable.getAllSymbols();
  cout << "Assembler::fillRelocationTables: all symbols from symbol table" << endl;
  for(const auto& s:symbols) {
    cout << "Symbol: " << s.second.symbolName + ". ";
  }
  string secName;
  cout << "Assembler::fillRelocationTables: all symbols from symbol table that are not sections: " << endl;
  for(auto& sym:symbols) {
    if(sym.second.sectionNumber != sym.second.id) { // only for symbols, not for sections
      cout << "--> " << sym.second.symbolName;
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      if(sym.second.isGlobal) { // symbol is global
        cout << "-global-" << endl;
          for(auto& sec:sectionTable) {
            for(auto& info: sym.second.usedHere) {
              if(sec.second.sectionName == info.sectionName) {
                if(info.type == 1)
                  sec.second.relocTable->add(sym.second.symbolName, info.address, sym.second.id, info.type);
                else { // pc relative
                    // nadji u tabeli literala gde je smestena vrednost simbola i
                    // odatle uzmi adresu i stavi pomeraj u code
                    // or(auto& i: sec.second.literalTable)
                    int offset;
                    int litAddress;
                    for(auto& lit: sec.second.literalTable)
                      if(lit.symbolName == sym.second.symbolName) {
                        offset = lit.address + sec.second.size - info.address - 4;
                        litAddress = lit.address;
                      }
                    string offsHex = decimalToHexadecimalNoFill(offset);
                    cout << "Assembler::fillRelocationTables: offsHex -> " + offsHex << endl;
                    if(offsHex.length() > 3) {
                      cout << "Assembler:fillRelocationTables: offset to literal in literal table too big. (>12b)" << endl;
                      return;
                    }
                    SectionTableEntry* s = getCurrentSectionTableEntry(sec.second.number);
                    if(offsHex.length() == 3) {
                      s->code.at(info.address) = (offsHex.substr(1,1) + offsHex.substr(2,1));
                      s->code.at(info.address + 1) = ("0" + offsHex.substr(0,1));
                    } else if(offsHex.length() == 2) {
                      s->code.at(info.address) = (offsHex.substr(0,1) + offsHex.substr(1,1));
                      s->code.at(info.address + 1) = ("00");
                    } else {
                      cout << "offsHex.length() == 1" << endl;
                      cout << sec.second.sectionName << endl;
                      s->code.at(info.address) = ("0"+ offsHex);
                      s->code.at(info.address + 1) = ("00");
                      for(const auto&c: s->code)
                        cout << c << " ";
                      cout<< endl;
                    }
                    sec.second.relocTable->add(sym.second.symbolName, litAddress + s->size, sym.second.id, 1);
                }
              }
            }
          }
      } 
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      else { // symbol is local
        cout << "-local-" << endl;
        for(auto& sec:sectionTable) {
            for(auto& info: sym.second.usedHere) {
              if(sec.second.sectionName == info.sectionName) {
                if(info.type == 1) {
                  sec.second.relocTable->add(sym.second.symbolName, info.address, sym.second.sectionNumber, info.type);
                  // fix code - add value of symbol
                
                  int symValue = sym.second.value;
                  
                  string numString = decimalToHexadecimal(symValue);
                  if(numString.length() > 8) {
                    cout << "Error::Assembler:word: literal " + numString + " too big." << endl;
                    return;
                  }
                  cout << "Assembler::fillRelocationTables: Number from .word: " + numString << endl;
                  // write 4B into code
                  SectionTableEntry* s = getCurrentSectionTableEntry(sec.second.number);
                  s->code.at(info.address) = (numString.substr(6,1) + "" + numString.substr(7,1));
                  s->code.at(info.address + 1) = (numString.substr(4,1) + "" + numString.substr(5,1));
                  s->code.at(info.address + 2) = (numString.substr(2,1) + "" + numString.substr(3,1));
                  s->code.at(info.address + 3) = (numString.substr(0,1) + "" + numString.substr(1,1));
                } else { // pc relative
                    // nadji u tabeli literala gde je smestena vrednost simbola i
                    // odatle uzmi adresu i stavi pomeraj u code
                    // or(auto& i: sec.second.literalTable)
                    int offset;
                    int litAddress;
                    for(auto& lit: sec.second.literalTable)
                      if(lit.symbolName == sym.second.symbolName) {
                        offset = lit.address + sec.second.size - info.address - 4;
                        litAddress = lit.address;
                      }
                    string offsHex = decimalToHexadecimalNoFill(offset);
                    cout << "Assembler::fillRelocationTables: offsHex -> " + offsHex << endl;
                    if(offsHex.length() > 3) {
                      cout << "Assembler:fillRelocationTables: offset to literal in literal table too big. (>12b)" << endl;
                      return;
                    }
                    SectionTableEntry* s = getCurrentSectionTableEntry(sec.second.number);
                    if(offsHex.length() == 3) {
                      s->code.at(info.address) = (offsHex.substr(1,1) + offsHex.substr(2,1));
                      s->code.at(info.address + 1) = ("0" + offsHex.substr(0,1));
                    } else if(offsHex.length() == 2) {
                      s->code.at(info.address) = (offsHex.substr(0,1) + offsHex.substr(1,1));
                      s->code.at(info.address + 1) = ("00");
                    } else {
                      cout << "offsHex.length() == 1" << endl;
                      cout << sec.second.sectionName << endl;
                      s->code.at(info.address) = ("0"+ offsHex);
                      s->code.at(info.address + 1) = ("00");
                      for(const auto&c: s->code)
                        cout << c << " ";
                      cout<< endl;
                    }
                    sec.second.relocTable->add(sym.second.symbolName, litAddress + s->size, sym.second.sectionNumber, 1);
                }
            }
          }
        }
      }
    }
  }

  for(const auto& lit: literalUse) {
    SectionTableEntry* s = getCurrentSectionTableEntry(lit.section);
    for(const auto& l: s->literalTable) {
      if(l.value == lit.value && !l.isSymbol) {
        int offset = l.address + s->size - lit.address - 4;
        string offsHex = decimalToHexadecimalNoFill(offset);
        cout << "Assembler::fillRelocationTables: offsHex -> " + offsHex << endl;
        if(offsHex.length() > 3) {
          cout << "Assembler:fillRelocationTables: offset to literal in literal table too big. (>12b)" << endl;
          return;
        }
        
        if(offsHex.length() == 3) {
          s->code.at(lit.address) = (offsHex.substr(1,1) + offsHex.substr(2,1));
          s->code.at(lit.address + 1) = ("0" + offsHex.substr(0,1));
        } else if(offsHex.length() == 2) {
          s->code.at(lit.address) = (offsHex.substr(0,1) + offsHex.substr(1,1));
          s->code.at(lit.address + 1) = ("00");
        } else {
          cout << "offsHex.length() == 1" << endl;
          // cout << sec.second.sectionName << endl;
          s->code.at(lit.address) = ("0"+ offsHex);
          s->code.at(lit.address + 1) = ("00");
          for(const auto&c: s->code)
            cout << c << " ";
          cout<< endl;
        }  
      }
    }
  }*/
  // kroz sve sekcije
  /* for(auto& sec: sectionTable) {
    // kroz njihove tabele literala, da se promeni value za simbole koji su definisani
    for(auto& lit: sec.second.literalTable) {
      if(lit.isSymbol) {
        int val = symbolTable.getValue(lit.symbolName);
        bool isGlobal = symbolTable.getIsGlobal(lit.symbolName);
        bool isExtern = symbolTable.getIsExtern(lit.symbolName);
        if(val >= 0) {
          lit.value = val;
          continue;
        }
        if(val < 0 && isGlobal && !isExtern) {
          cout << "Error:Assembler:: symbol '" + lit.symbolName + "' not defined but set as global!" << endl;
          return -1;  // global symbol but not defined
        }
      }
    }
  }*/
  for(auto& use:literalUse) {
    cout << "FILL RELOCATIONS " << endl;
    if(use.type == 0) { // for .word, put symbol value in code
      SectionTableEntry* sec = getCurrentSectionTableEntry(use.section);
      int value = symbolTable.getValue(use.symbolName);
      string valString = decimalToHexadecimal(value);
      /* sec->code.at(use.address)     = valString[6] + valString[7];
      sec->code.at(use.address + 1) = valString[4] + valString[5];
      sec->code.at(use.address + 2) = valString[2] + valString[3];
      sec->code.at(use.address + 3) = valString[0] + valString[1]; */
      // global symbol
      if(symbolTable.getIsGlobal(use.symbolName)) {
        sec->relocTable->add(use.symbolName, use.address, symbolTable.getId(use.symbolName), 0, 0);
      } 
      // local symbol
      else {
        // if value is -1 that means that symbol is local and it is not defined nor extern
        // mistake!!! 
        if(value < 0) {
          cout << "Error:Assembler: Symbol " << use.symbolName << " not defined!" << endl;
          return -1;
        }
        sec->relocTable->add(use.symbolName, use.address, symbolTable.getSectionNumber(use.symbolName), 0, value);
      }
    } else if(use.type == 1){  // for instruction, put offset [from address to place in literal pool] in code
      SectionTableEntry* sec = getCurrentSectionTableEntry(use.section);
      bool first = true;
      for(auto& lit: sec->literalTable) {
        if(first) { first = false; continue; }
        // cout << "lit: " <<  " " << lit.symbolName << endl;
        // LITERAL
        if(!lit.isSymbol && lit.value == use.value) {
          int offset = lit.address + sec->size - use.address - 4;
          
          stringstream s;
          s << setfill('0') << setw(4) << hex << offset;
          string offsetString = s.str();
          /* cout << "Offset to literal " << lit.value << " in literal pool is " << offsetString 
            << " " + offsetString.substr(2,2) << " " + offsetString.substr(0,2)<< endl;
          cout<< "use.address = " << use.address << endl;
          cout << "sec->code.at(use.address) = " << sec->code.at(use.address) <<endl; */
          
          sec->code.at(use.address)     = offsetString.substr(2,1) + offsetString.substr(3,1);
          // cout << "sec->code.at(use.address) = " << sec->code.at(use.address) <<endl;

          // gpr[C] must be preserved
          char gprC = sec->code.at(use.address + 1).at(0); 
          sec->code.at(use.address + 1) = gprC + offsetString.substr(1,1);
          break;
        }
        // SYMBOL
        if(lit.isSymbol && lit.symbolName == use.symbolName) {
          int offset = lit.address + sec->size - use.address - 4;
          stringstream s;
          s << setfill('0') << setw(4) << hex << offset;
          string offsetString = s.str();
          sec->code.at(use.address)     = offsetString.substr(2,1) + offsetString.substr(3,1);
          
          // gpr[C] must be preserved
          char gprC = sec->code.at(use.address + 1).at(0); 
          sec->code.at(use.address + 1) = gprC + offsetString.substr(1,1);
          // relocation 
          // sec->relocTable->add("", lit.address + sec->size, sec->number, 0, 0);
          if(symbolTable.getIsGlobal(use.symbolName)) {
            // to not have duplicates in relocation table for symbols in literal pool
            if(!sec->relocTable->relocExistsForPoolRelocations(lit.address + sec->size, symbolTable.getId(use.symbolName), 0))
              sec->relocTable->add(use.symbolName, lit.address + sec->size, symbolTable.getId(use.symbolName), 0, 0);
          } 
          // local symbol
          else {
            if(symbolTable.getValue(use.symbolName) < 0) {
              cout << "Error:Assembler: Symbol " << use.symbolName << " not defined!" << endl;
              return -1;
            }
            // to not have duplicates in relocation table for symbols in literal pool
            if(!sec->relocTable->relocExistsForPoolRelocations(lit.address + sec->size, symbolTable.getSectionNumber(use.symbolName), symbolTable.getValue(use.symbolName)))
              sec->relocTable->add(use.symbolName, lit.address + sec->size, symbolTable.getSectionNumber(use.symbolName), 0, symbolTable.getValue(use.symbolName));
          }
          break;
        }
      }
    }
    else {  // for ld/st_mem_reg_pom
      // check if symbol defined -> if not ERROR
      /* int value = symbolTable.getValue(use.symbolName);
      if(value < 0) {  // not defined
        cout << "Error::Assembler: value of symbol '" + use.symbolName + "' is not known at assembly time!!!" << endl;
        return -1;
      }
      // check if value of symbol can fit in 12b -> if not ERROR
      string valueString = decimalToHexadecimalNoFill(value);
      if(valueString.length() > 3) {
        cout << "Error::Assembler: value of symbol '" + use.symbolName + "' can not fit in 12b!!!" << endl;
        return -1;
      }
      SectionTableEntry* sec = getCurrentSectionTableEntry(use.section);
    */
    }
  }
  return 0;
}

void Assembler::addSymbolToLiteralTableIfNotAlreadyIn(string symbolName) {
  cout << "addSymbolToLiteralTableIfNotAlreadyIn: " << symbolName << endl;
  SectionTableEntry* section = getCurrentSectionTableEntry(currentSectionNumber);
  for(const auto& lit:section->literalTable) {
    if(lit.symbolName == symbolName)
      return;
  }
  int address = section->literalTable.at(section->literalTable.size()-1).address + 
                      section->literalTable.at(section->literalTable.size()-1).size;
  string value = "00000000";
  // if symbol is already defined use its value
  /* if(symbolTable.getValue(symbolName) >= 0) {
    value = decimalToHexadecimal(symbolTable.getValue(symbolName));
  } */

  LiteralTableEntry lte(0, value, true, symbolName, address, 4);
  section->literalTable.push_back(lte);
}

string Assembler::getRegNum(string reg) {
  regex ri("^r(?:[0-9]|1[0-5])$");
  regex sp("^sp$");
  regex pc("^pc$");
  long num;
  if(regex_match(reg, ri)) {
    reg = reg.substr(1);
    convertStringToNumber(reg, &num);
  } 
  else if(regex_match(reg, sp)) num = 14;
  else if(regex_match(reg, pc)) num = 15;
  
  stringstream s;
  s << setw(1) << hex << num;
  return s.str();
}

bool Assembler::convertStringToNumber(string stringNum, long* number) {
  regex rHEX("^0[xX][0-9a-fA-F]+$");
  regex rDEC("^[0-9]+$");
  if(regex_match(stringNum, rHEX) || regex_match(stringNum, rDEC)) {    
    if(regex_match(stringNum, rDEC))  *number = stol(stringNum, nullptr, 10);
    else if(regex_match(stringNum, rHEX)) *number = stol(stringNum, nullptr, 16);
    return true;
  }
  return false;
}

string Assembler::decimalToHexadecimalNoFill(long num) {
  stringstream s;
  s << hex << num;
  return s.str();
}

string Assembler::decimalToHexadecimal(long num) {
  stringstream s;
  s << setfill('0') << setw(8) << hex << num;
  return s.str();
}