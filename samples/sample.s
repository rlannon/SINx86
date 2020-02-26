section .text
main:

	mov ax, 23
	mov [rbp - 20], eax

	mov eax, 4
	mov [rbp - 16], eax

	mov [rbp - 16], eax

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

section .data

section .bss

