#include "types.h"
#include "powerpc.h"
#include "hollywood.h"
#include "utils.h"
#include "start.h"
#include "gecko.h"
#include "ff.h"
#include "powerpc_elf.h"
#include "elf.h"
#include "string.h"

#define PHDR_MAX 10

Elf32_Ehdr elfhdr;
Elf32_Phdr phdrs[PHDR_MAX];

int powerpc_load_file(const char *path)
{
	FIL fd;
	u32 read;
	FRESULT fres;

	fres = f_open(&fd, path, FA_READ);
	if(fres != FR_OK)
		return -fres;

	fres = f_read(&fd, &elfhdr, sizeof(elfhdr), &read);
	
	if(fres != FR_OK)
		return -fres;
	if(read != sizeof(elfhdr))
		return -100;

	if(memcmp("\x7F" "ELF\x01\x02\x01\x00\x00",elfhdr.e_ident,9)) {
		gecko_printf("Invalid ELF header! 0x%02x 0x%02x 0x%02x 0x%02x\n",elfhdr.e_ident[0], elfhdr.e_ident[1], elfhdr.e_ident[2], elfhdr.e_ident[3]);
		return -101;
	}

	if(elfhdr.e_phoff == 0 || elfhdr.e_phnum == 0) {
		gecko_puts("ELF has no program headers!\n");
		return -102;
	}
	if(elfhdr.e_phnum > PHDR_MAX) {
		gecko_printf("ELF has too many (%d) program headers!\n", elfhdr.e_phnum);
		return -102;
	}
	
	fres = f_lseek(&fd, elfhdr.e_phoff);
	if(fres != FR_OK)
		return -fres;
	
	fres = f_read(&fd, phdrs, sizeof(phdrs[0])*elfhdr.e_phnum, &read);
	if(fres != FR_OK)
		return -fres;
	if(read != sizeof(phdrs[0])*elfhdr.e_phnum)
		return -103;

	int count = elfhdr.e_phnum;
	Elf32_Phdr *phdr = phdrs;

	powerpc_hang();

	while(count--)
	{
		if(phdr->p_type != PT_LOAD) {
			gecko_printf("Skipping PHDR of type %d\n",phdr->p_type);
		} else {
			void *dst = phdr->p_paddr;
			dst = (void*)((u32)dst & ~0xC0000000);
			
			gecko_printf("LOAD 0x%x -> %p [0x%x]\n", phdr->p_offset, phdr->p_paddr, phdr->p_filesz);
			fres = f_lseek(&fd, phdr->p_offset);
			if(fres != FR_OK)
				return -fres;
			fres = f_read(&fd, dst, phdr->p_filesz, &read);
			if(fres != FR_OK)
				return -fres;
			if(read != phdr->p_filesz)
				return -104;
		}
		phdr++;
	}
	gecko_puts("ELF load done, booting PPC...\n");
	powerpc_upload_stub(NULL,0);
	powerpc_reset();
	gecko_puts("PPC booted!\n");

	return 0;
}
