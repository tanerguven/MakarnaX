	.global _start
	.extern main
	.extern _exit
_start:
	/*
	* MakarnaX exec yaparken stacke return eip olarak 0xffffffff basiyor
	* sahte return eip degerini geri al
	*/
	pop %eax
	call main
	pushl $0
	call _exit
.wait: hlt
	jmp .wait
