#pragma once
#include <vector>

void hexdump(void *data, unsigned int len);
void splitstring(char *str, char split, std::vector<char *> &ret);
void get_self_process_name(char* name, int len);