#include <dlfcn.h>
#include <pthread.h>
#include <core/logger.h>
#include <core/armhook.h>
#include <core/common-help.h>

using namespace std;


bool(*old_send)(int pthis, int chanel, char *data, int count) = 0;
bool my_send(int pthis, int chanel, char *data, int count)
{
	LOGD("send %x %x %x", chanel, (int)data, count);
	hexdump(data, count);
	return old_send(pthis, chanel, data, count);
}

int(*old_recv)(int pthis, int a1) = 0;
int my_recv(int pthis, int a1)
{
	char *data = *(char **)(*(int *)a1 + 112);
	int size = *(int *)(*(int *)a1 + 116);
	LOGD("recv %x %x", (int)data, size);
	hexdump(data, size);
	return old_recv(pthis, a1);
}



void *startwork_proto(void *param)
{
	LOGD("startwork_proto");

	void *p_game = dlopen("libUE4.so", RTLD_LAZY);

	void *send = dlsym(p_game, "_ZN12GSBaseClient4SendEjPvi");
	void *recv = dlsym(p_game, "_ZN12GSBaseClient11ReceiveDataERK10TSharedPtrI12FArrayReaderL7ESPMode1EE");

	if (send)
	{
		LOGD("addr send ok");
		arm_hook(send, (void *)my_send, (void **)&old_send);
	}
	if (recv)
	{
		LOGD("addr recv ok");
		arm_hook(recv, (void *)my_recv, (void **)&old_recv);
	}

	dlclose(p_game);
	return NULL;
}

__attribute__((constructor))
static void ctor_ue()
{
	pthread_t tid;
	//pthread_create(&tid, NULL, startwork_proto, NULL);
}