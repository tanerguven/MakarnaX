#ifndef ELF_H_
#define ELF_H_

#include <types.h>

typedef uint32_t	Elf32_Addr;
typedef uint16_t	Elf32_Half;
typedef uint32_t	Elf32_Off;
typedef int32_t	Elf32_Sword;
typedef uint32_t	Elf32_Word;

#define EI_NIDENT	16

/** Elf Header */
typedef struct elf32_hdr {
	unsigned char	e_ident[EI_NIDENT];
	/** Elf type */
	Elf32_Half	type;
	/** The required architecture for the file */
	Elf32_Half	machine;
	/** Version */
	Elf32_Word	version;
	/* Programın başlangıcının virtual adresi */
	Elf32_Addr	entry;
	/** Program header'larının başlangıcı */
	Elf32_Off	phoff;
	/** Section header'larının başlangıcı */
	Elf32_Off	shoff;
	/** processor-specific flags */
	Elf32_Word	flags;
	/** Size of this header */
	Elf32_Half	ehsize;
	/** Size of program headers */
	Elf32_Half	phentsize;
	/** Number of program headers */
	Elf32_Half	phnum;
	/** Size of section headers */
	Elf32_Half	shentsize;
	/** Number of section headers */
	Elf32_Half	shnum;
	/** Section header string table index */
	Elf32_Half	shstrndx;

	enum Type {
		Type_NONE = 0,
		Type_REL = 1,
		Type_EXEC = 2,
		Type_DYN = 3,
		Type_CORE = 4,
		Type_LOPROC = 0xff00,
		Type_HIPROC = 0xffff,
	};

	enum Machine {
		Machine_NONE = 0, /* No machine */
		Machine_M32 = 1, /* AT&T WE 32100 */
		Machine_SPARC = 2, /* SPARC */
		Machine_386 = 3, /* Intel 80386 */
		Machine_86K = 4, /* Motorola 68000 */
		Machine_88K = 5, /* Motorola 88000 */
		Machine_860 = 7, /* Intel 80860 */
		Machine_MIPS = 8, /* MIPS RS3000 */
	};
} Elf32_Ehdr;

/** Program header table */
struct Elf32_Phdr {
	Elf32_Word	type;
	Elf32_Off	offset;
	Elf32_Addr	vaddr;
	Elf32_Addr	paddr;
	Elf32_Word	filesz;
	Elf32_Word	memsz;
	Elf32_Word	flags;
	Elf32_Word	align;

	enum Type {
		Type_NULL = 0,
		Type_LOAD = 1,
		Type_DYNAMIC = 2,
		Type_INTERP = 3,
		Type_NOTE = 4,
		Type_SHLIB = 5,
		Type_PHDR = 6,
		Type_LOPROC = 0x70000000,
		Type_HIPROC = 0x7fffffff,
	};
};

/** Section header table */
typedef struct elf32_shdr {
	Elf32_Word	sh_name;
	Elf32_Word	sh_type;
	Elf32_Word	sh_flags;
	Elf32_Addr	sh_addr;
	Elf32_Off	sh_offset;
	Elf32_Word	sh_size;
	Elf32_Word	sh_link;
	Elf32_Word	sh_info;
	Elf32_Word	sh_addralign;
	Elf32_Word	sh_entsize;
} Elf32_Shdr;

#endif /* ELF_H_ */
