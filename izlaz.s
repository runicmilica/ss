.section sekcija1     # 1
call 0x0A0B0C            # 2
.word a       # 3
.word b      # 4
call a         # 5
a:
.section sekcija2   # 6
call a        # 7
b:              # 8
.extern c     # 9
jmp c         # 10
.word b
halt
.end

