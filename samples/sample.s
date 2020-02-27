section .text
main:
	mov ax, 10
	mov [rbp - 16], eax

	mov ax, 20

	mov [rbp - 24], rax

	mov [rbp - 32], rax

	mov rax, 0


	ret



global _start
_start:
	push rbp
	mov rbp, rsp
	call main
	mov rsp, rbp
	pop rbp
	mov rbx, rax
	mov rax, 0x01
	int 0x80

section .rodata

section .data

section .bss

