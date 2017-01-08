#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <deque>
#include <map>
#include <vector>
#include<algorithm>
#include <core/armhook.h>
#include <core/logger.h>
#include <core/special-hook.h>

/*全局变量*/
static pthread_mutex_t hooked_mutex = PTHREAD_MUTEX_INITIALIZER;
std::map<uint32_t, HookData*> hooked_method_dict;


#define SP_BLOCK_SIZE 108
static std::deque<char*> mem_cache;
static pthread_mutex_t alloc_mutex = PTHREAD_MUTEX_INITIALIZER;
static char *alloc_specific_trampo() {
	pthread_mutex_lock(&alloc_mutex);
	if (mem_cache.empty()) {
		size_t map_size = 10240 * SP_BLOCK_SIZE;
		char *p = (char*)mmap(0, map_size, PROT_EXEC | PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		for (int i = 0; i < map_size / SP_BLOCK_SIZE; i++) {
			mem_cache.push_back(p + i * SP_BLOCK_SIZE);
		}
	}
	char *r = mem_cache.front();
	mem_cache.pop_front();
	pthread_mutex_unlock(&alloc_mutex);
	return r;
}

static std::deque<char*> mem_cache_userdata;
static pthread_mutex_t alloc_mutex_userdata = PTHREAD_MUTEX_INITIALIZER;
static char *alloc_userdata() {
	static int DATASIZE = sizeof(HookData);
	pthread_mutex_lock(&alloc_mutex_userdata);
	if (mem_cache_userdata.empty()) {
		size_t map_size = 10240 * DATASIZE;
		char *p = (char*)mmap(0, map_size, PROT_EXEC | PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		for (int i = 0; i < map_size / DATASIZE; i++) {
			mem_cache_userdata.push_back(p + i * DATASIZE);
		}
	}
	char *r = mem_cache_userdata.front();
	mem_cache_userdata.pop_front();
	pthread_mutex_unlock(&alloc_mutex_userdata);
	return r;
}


HookData *special_hook(void *org, void *func) {
	/*  ______________
	* |__common_func_ 0
	* |___old_func___ 4
	* |__monomethod__ 8
	* |_____code[]___ 12
	*/
	/*Fixme : 每次更新这段跳板代码都是蛋疼, 有没有方便高效稳定的方法?*/
	unsigned char code[96] = {
		0x0F, 0x00, 0x2D, 0xE9, 0xFF, 0xFF, 0x2D, 0xE9, 0x50, 0x00, 0x4D, 0xE2, 0x34, 0x00, 0x8D, 0xE5,
		0x3C, 0xE0, 0x8D, 0xE5, 0x40, 0x10, 0x8D, 0xE2, 0x30, 0x40, 0xA0, 0xE3, 0x04, 0x40, 0x4F, 0xE0,
		0x08, 0x00, 0x94, 0xE5, 0x0D, 0x20, 0xA0, 0xE1, 0x0F, 0xE0, 0xA0, 0xE1, 0x00, 0xF0, 0x94, 0xE5,
		0xFF, 0x1F, 0xBD, 0xE8, 0x04, 0xE0, 0x9D, 0xE5, 0x1C, 0xD0, 0x8D, 0xE2, 0x01, 0x80, 0x2D, 0xE9,
		0x58, 0x00, 0xA0, 0xE3, 0x00, 0x00, 0x4F, 0xE0, 0x04, 0x00, 0x90, 0xE5, 0x04, 0x00, 0x8D, 0xE5,
		0x01, 0x80, 0xBD, 0xE8, 0x00, 0xF0, 0x20, 0xE3, 0x00, 0xF0, 0x20, 0xE3, 0x00, 0xF0, 0x20, 0xE3
	};
	/*
	* stmfd sp!, {r0-r3}
	* stmfd sp!, {r0-r12, sp, lr, pc}
	* sub r0, sp, #80      //函数调用前的sp                 |
	* str r0, [sp, #52]    //设置保存的reg.sp为原来的sp     |-> 都是为了堆栈回溯做准备
	* str lr, [sp, #60]    //设置保存的reg.pc为lr           |
	* add r1, sp, #64      //2.参数数组
	* ldr r4, =48
	* sub r4, pc, r4
	* ldr r0, [r4, #8]     //1.method ptr
	* mov r2, sp           //3.保存的reg环境块, 用于堆栈回溯
	* mov lr, pc
	* ldr pc, [r4]
	* ldmfd sp!, {r0-r12}
	* ldr lr, [sp, #4]     //恢复lr, 但不恢复sp, pc
	* add sp, sp, #28      //清除掉栈中的 {sp, lr, pc} 以及 {r0-r3}
	* stmfd sp!, {r0, pc}
	* ldr r0, =88
	* sub r0, pc, r0
	* ldr r0, [r0, #4]
	* str r0, [sp, #4]     //设置old_func域中的指针为继续执行的PC
	* ldmfd sp!, {r0, pc}
	*/
	char *p = alloc_specific_trampo();
	char *data = alloc_userdata();

	memcpy(p + 12, code, sizeof(code));
	memcpy(p, &func, 4);
	memcpy(p + 8, &data, 4);

	if (arm_hook(org, p + 12, (void**)(p + 4)) == 0) {
		return NULL;
	}

	cache_flush((int)org, (int)org + 8);
	cache_flush((int)p, (int)p + SP_BLOCK_SIZE);

	((HookData *)data)->jmp_addr_bak = *(void**)(p + 4);
	((HookData *)data)->tramp = (Trampoline *)p;

	return (HookData *)data;
}