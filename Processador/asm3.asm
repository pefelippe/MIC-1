bipush 34
istore a
bipush 12
istore b
iload a
iload b
iadd
istore r
bipush 46
istore s
iload r
iload s
if_icmpeq l1
bipush 22
istore c
l1 bipush 12
goto l2 
bipush 15
l2 bipush 89
istore d
istore c
iload c
