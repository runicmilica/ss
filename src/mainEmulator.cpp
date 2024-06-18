#include "../inc/emulator.hpp"
#include <regex>


int main(int argc, char* argv[]) {

  if(argc != 2) {
    cout << "Wrong number og arguments!" << endl;
    return -1;
  }

  regex file(R"(^.+\.hex$)");
  if(regex_match(argv[1], file))
    new Emulator(argv[1]);
  
  return 0;
}