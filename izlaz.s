.section sekcija1     # 1
halt                  # 2
b:
call 0xfffF            # 3
.word a               # 4
a:                    # 5
call a                # 6
                   # 8
call b                # 9
.global b
.end
