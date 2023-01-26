.global _start

.text

_start:

        mov     $1, %rax
        mov     $1, %rdi
        mov     $msg, %rsi
        mov     $12, %rdx
        syscall

msg:
        .ascii  "Hello world\n"
        