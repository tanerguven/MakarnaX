	.global _start
	.extern main
	.extern exit
_start:
	jmp main
	call exit
.wait: hlt
	jmp .wait