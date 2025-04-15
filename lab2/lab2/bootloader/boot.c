#include "boot.h"

#define SECTSIZE 512


void bootMain(void) {
	int i = 0;
	int phoff = 0x34; // program header offset
	int offset = 0x1000; // .text section offset
	unsigned int elf = 0x100000; // physical memory addr to load
	void (*kMainEntry)(void);
	kMainEntry = (void(*)(void))0x100000; // entry address of the program
	for (i = 0; i < 200; i++) {
		readSect((void*)(elf + i*512), 1+i);
	}
	// TODO: 阅读boot.h查看elf相关信息，填写kMainEntry、phoff、offset
	// // 首先，将elf地址转换为ELFHeader结构体指针
	// struct ELFHeader *elf_header = (struct ELFHeader *)elf;
	// // 读取ELF文件头中的entry字段，即程序的入口地址
	// // 并将其转换为函数指针类型，赋值给kMainEntry
	// kMainEntry = (void(*)(void))elf_header->entry;
	// // 读取ELF文件头中的phoff字段，即程序头表在文件中的偏移量
	// // 赋值给phoff
	// phoff = elf_header->phoff;
	// // 计算程序头表的起始地址
	// struct ProgramHeader *program_header = (struct ProgramHeader *)(elf + phoff);
	// // 读取程序头表中的off字段，即.text节在文件中的偏移量
	// // 赋值给offset
	// offset = program_header->off;
	// kMainEntry();
	// for (i = 0; i < 200 * 512; i++) {
	// 		*(unsigned char *)(elf + i) = *(unsigned char *)(elf + i + offset);
	// 	}
	// kMainEntry();

	ELFHeader *elfHeader = (void *)elf;
	kMainEntry = (void(*)(void))elfHeader->entry; 
	phoff = elfHeader->phoff; 
	//offset = 0x1000;

	for (i = 0; i < 200 * 512; i++) {
			*(unsigned char *)(elf + i) = *(unsigned char *)(elf + i + offset);
		}

	kMainEntry();
}

void waitDisk(void) { // waiting for disk
	while((inByte(0x1F7) & 0xC0) != 0x40);
}

void readSect(void *dst, int offset) { // reading a sector of disk
	int i;
	waitDisk();
	outByte(0x1F2, 1);
	outByte(0x1F3, offset);
	outByte(0x1F4, offset >> 8);
	outByte(0x1F5, offset >> 16);
	outByte(0x1F6, (offset >> 24) | 0xE0);
	outByte(0x1F7, 0x20);

	waitDisk();
	for (i = 0; i < SECTSIZE / 4; i ++) {
		((int *)dst)[i] = inLong(0x1F0);
	}
}
