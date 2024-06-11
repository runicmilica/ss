
.section sekcija1     # 1
beq %r11, %r15, 45           # 2
bne     %sp   , %pc,   5
bgt %r0, %pc,  a33
.word 0xffffffff
.section nova
.word 0x1234
a33:
.end

