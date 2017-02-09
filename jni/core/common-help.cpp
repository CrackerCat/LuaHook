#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string.h> 
#include <unistd.h>
#include <elf.h>
#include <dlfcn.h>
#include <core/logger.h>
#include <core/common-help.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <sys/stat.h>

//输出16进制内存数据
void hexdump(void *data, unsigned int len)
{
	unsigned int i;
	unsigned int r, c;
	char szTmp[4096] = { 0x00 };

	if (!data) return;
	if (len >= 4096) return;

	for (r = 0, i = 0; r < (len / 16 + (len % 16 != 0)); r++, i += 16)
	{
		sprintf(szTmp, "%08x:   ", (int)data+i); /* location of first byte in line */

		for (c = i; c < i + 16; c++) /* left half of hex dump */
			if (c < len)
				sprintf(&szTmp[strlen(szTmp)], "%02X ", ((unsigned char *)data)[c]);
			else
				sprintf(&szTmp[strlen(szTmp)], "   "); /* pad if short line */

		for (c = i; c < i + 16; c++) /* ASCII dump */
			if (c < len)
				if (((unsigned char *)data)[c] >= 32 && ((unsigned char *)data)[c] < 127)
					sprintf(&szTmp[strlen(szTmp)], "%c", ((char const *)data)[c]);
				else
					sprintf(&szTmp[strlen(szTmp)], "."); /* put this for non-printables */

		LOGD("%s", szTmp);
		memset(szTmp, 0x00, 4096);
	}
}

void splitstring(char *str, char split, std::vector<char *> &ret)
{
	bool found = false;
	char *last = str;
	while (true)
	{
		if (*str==0x00)
		{
			ret.push_back(last);
			break;
		}

		if (found)
		{
			ret.push_back(last);
			last = str;
		}

		if (*str==split)
		{
			*str = 0x00;
			found = true;
		}
		else
		{
			found = false;
		}

		str++;
	}
}


void get_self_process_name(char* name, int len)
{
	char sztmp[256] = { 0x00 };
	snprintf(sztmp, 256, "/proc/%d/cmdline", getpid());

	FILE *fp = fopen(sztmp, "r");
	if (fp==NULL)
	{
		LOGD("fopen %s failed", sztmp);
		return;
	}

	fgets(name, len, fp);

	fclose(fp);
}

int get_module_base(char *module_name)
{
	FILE *fp;
	long addr = 0;
	char *pch;
	char line[1024];

	fp = fopen("/proc/self/maps", "r");

	if (fp != NULL) {
		while (fgets(line, sizeof(line), fp)) {
			if (strstr(line, module_name)) {
				pch = strtok(line, "-");
				addr = strtoul(pch, NULL, 16);

				if (addr == 0x8000)
					addr = 0;

				break;
			}
		}

		fclose(fp);
	}

	return addr;
}

void get_module_path(char *module_name, char *module_path)
{
	FILE *fp;
	long addr = 0;
	char *pch;
	char line[1024];

	fp = fopen("/proc/self/maps", "r");

	if (fp != NULL) {
		while (fgets(line, sizeof(line), fp)) {
			if (strstr(line, module_name)) {
				char *path = strstr(line, "/");
				path[strlen(path) - 1] = 0x00;
				strcpy(module_path, path);
				break;
			}
		}

		fclose(fp);
	}
}

int advance_dlsym(const char *libname, const char *find_sym_name)
{
	int addr = 0;

	void *pLib = dlopen(libname, RTLD_LAZY);
	if (pLib != NULL)
	{
		addr = (int)dlsym(pLib, find_sym_name);
		dlclose(pLib);
	}

	if (addr==0)
	{
		void *buff = NULL;
		char module_path[256] = { 0x00 };
		int module_base = get_module_base((char *)libname);
		get_module_path((char *)libname, module_path);

		int fd = open(module_path, O_RDONLY);
		if (fd != -1)
		{
			struct stat stat_data;
			fstat(fd, &stat_data);
			buff = mmap(0, stat_data.st_size, PROT_READ, MAP_SHARED, fd, 0);

			if (buff != MAP_FAILED)
			{
				Elf32_Shdr *sym_table_header = 0;
				Elf32_Shdr *str_table_header = 0;
				Elf32_Ehdr *elf_header = (Elf32_Ehdr *)buff;
				Elf32_Shdr *section_header = (Elf32_Shdr *)((int)buff + elf_header->e_shoff);

				for (int i = 0; i < elf_header->e_shnum; i++)
				{
					if (section_header[i].sh_type == SHT_DYNSYM || section_header[i].sh_type == SHT_SYMTAB)
					{
						sym_table_header = &section_header[i];
						if (sym_table_header->sh_offset>0)
						{
							int str_table_addr = (int)buff + section_header[sym_table_header->sh_link].sh_offset;
							Elf32_Sym *sym = (Elf32_Sym *)((int)buff + sym_table_header->sh_offset);
							int sym_count = sym_table_header->sh_size / sizeof(Elf32_Sym);
							for (int j = 0; j < sym_count; j++)
							{
								char *sym_name = (char *)(str_table_addr + sym->st_name);
								if (strcmp(sym_name, find_sym_name)==0)
								{
									addr = sym->st_value + module_base;
									break;
								}
								sym++;
							}

							if (addr!=0)
							{
								break;
							}
						}
					}
				}

				munmap(buff, stat_data.st_size);
			}

			close(fd);
		}
	}

	return addr;
}