# Lab 02 - [Linux Shellcoding](https://0x00sec.org/t/linux-shellcoding-part-1-0/289)


**Compiling assembly code**
```console
$ nasm -f elf -o shell.o shell.asm
$ ld -m elf_i386 -o shell.out shell.o
```

**Objdump**
```console
$ objdump -M intel -d shell
```

**Compiling test.c**

`test.c` needs to be compiled to target 32bit machines with stack protection turned off
```console
$ gcc shell_codetest.c -o shell_codetest.out -m32 -fno-stack-protector -z execstack -no-pie
```


---
https://0x00sec.org/t/linux-shellcoding-part-1-0/289
