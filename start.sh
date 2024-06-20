ASSEMBLER=./assembler
LINKER=./linker
EMULATOR=./emulator

${ASSEMBLER} -o tests/main.o tests/main.s
${ASSEMBLER} -o tests/math.o tests/math.s
${ASSEMBLER} -o tests/handler.o tests/handler.s
${ASSEMBLER} -o tests/isr_timer.o tests/isr_timer.s
${ASSEMBLER} -o tests/isr_terminal.o tests/isr_terminal.s
${ASSEMBLER} -o tests/isr_software.o tests/isr_software.s
${LINKER} -hex \
  -place=my_code@0x40000000 -place=math@0xF0000000 \
  -o tests/program.hex \
  tests/handler.o tests/math.o tests/main.o tests/isr_terminal.o tests/isr_timer.o tests/isr_software.o
${EMULATOR} tests/program.hex