section .text
main:
	mov al, 1

	jz sinl_ite_else_0

	mov ax, 23
	mov [rbp - 20], eax

	mov eax, 4
	mov [rbp - 16], eax

	mov [rbp - 16], eax

	jmp sinl_ite_done_0
sinl_ite_else_0:
sinl_ite_done_0:
	mov al, 0

	jz sinl_ite_else_1
	pushfq
	push rbp
	mov rbp, rsp
	mov rax, strc_0
	call print
	mov rsp, rbp
	pop rbp
	popfq

	jmp sinl_ite_done_1
sinl_ite_else_1:
	pushfq
	push rbp
	mov rbp, rsp
	mov rax, strc_1
	call print
	mov rsp, rbp
	pop rbp
	popfq

sinl_ite_done_1:
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
strc_0 .dd 6 `error!`
strc_1 .dd 8 `success!`

section .bss

