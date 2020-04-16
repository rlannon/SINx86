section .text
main:
	sub rsp, 0
	mov rbx, rbp
	sub rbx, 16

	mov rbx, rbp
	sub rbx, 16
	add rbx, 4

	mov rbx, rbp
	sub rbx, 16
	mov [rbp - 16], eax
	sub rsp, 4
	mov rbx, rbp
	sub rbx, 16
	add rbx, 4
	mov [rbp - 20], eax
	sub rsp, 4
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
	sp_mask dd 0x80000000
	dp_mask dq 0x8000000000000000

section .data

section .bss

