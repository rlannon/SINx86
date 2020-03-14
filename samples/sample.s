section .text
main:

	mov ax, 0
	mov edx, eax
	mul eax, 0
	mov ebx, eax
	add ebx, 4
	mov rcx, rbp
	sub rcx, 16
	mov eax, [rcx]
	cmp edx, eax
	sub rcx, rbx
	mov eax, [rcx]
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

section .rodata
	sp_mask dd 0x80000000
	dp_mask dq 0x8000000000000000

section .data

section .bss

