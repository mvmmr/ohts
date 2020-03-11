section .text
  global _start

_start:
  xor eax, eax     ; set eax to 0
  push eax         ; push 0 `\x00` to stack![](img/test_str_1.png)
  push 0x68732f2f  ; push `hs//`
  push 0x6e69622f  ; push `nib/`
  mov ebx, esp     ; move the string to ebx
  mov ecx, eax     ; set ecx as 0
  mov al, 0xb      ; set eax as 0xb
  int 0x80         ; execute the syscall
