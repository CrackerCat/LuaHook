#pragma once
#include <stdio.h>

struct _ArmRegs {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;
	uint32_t sp;
	uint32_t lr;
	uint32_t pc;
};
typedef struct _ArmRegs ArmRegs;

struct _Trampoline {
	void *common_func_addr;			// common_func_addr
	void *jmp_addr;					// 退出时将跳往的地址
	void *user_data;				// userdata
	char *common_code;
};
typedef struct _Trampoline Trampoline;

struct _HookData {
	void *jmp_addr_bak;
	Trampoline *tramp;
	char lua_func_name[128];
};
typedef struct _HookData HookData;

HookData *special_hook(void *org, void *func);