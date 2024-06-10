#include "../inc/assembler.hpp"
#include <string.h>

using namespace std;

int main(int argc, char *argv[]) {
  Assembler* as;
  bool start = Assembler::checkStart(argc, argv);
  if(start) {
    string p1 = argv[2];
    string p2 = argv[3];
    as = new Assembler(p1, p2);
  }

  return 0;
}