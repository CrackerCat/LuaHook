#include <stdio.h>
#include <cstring>
#include <string.h> 
#include <unistd.h>
#include <core/logger.h>
#include <core/common-help.h>

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