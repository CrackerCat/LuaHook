#include <stdio.h> 
#include <string.h>
#include <unistd.h>
#include "android-injector.h"

int main(int argc, char* argv[])
{
	if (argc<2)
	{
		printf("useage: %s process\n\n", argv[0]);
		return 0;
	}

	char *proc_name = argv[1];
	char *so_path = argv[2];

	if (access(so_path, F_OK) == -1)
	{
		printf("err %s not exists!\n\n", so_path);
		return 0;
	}

	ANDROID_INJECTOR injector;
	pid_t target_pid = -1;
	do
	{
		target_pid = injector.find_pid_of(proc_name);
		usleep(500000);
		printf("wait for %s...\n", proc_name);
	} while (target_pid == -1);

	int lib_gameso = -1;
	do
	{
		lib_gameso=injector.find_injected_so_of(target_pid, (char *)"libUE4.so");
		usleep(100000);
	} while (lib_gameso == -1);
	
	int ret = injector.inject_remote_process(target_pid, so_path, "so_main", NULL, 0, 0);
	if (ret==-1)
	{
		printf("err inject failed!\n\n");
		return 0;
	}
	
	printf("inject success!\n\n");
	return 1;
}