#include <map>
#include <vector>
#include <string>
#include <lua.hpp>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <dlfcn.h>
#include <core/logger.h>
#include <core/md5.h>
#include <core/special-hook.h>
#include <core/armhook.h>
#include <core/common-help.h>
#include <core/ue.h>

using namespace std;

static lua_State* L = NULL;
static char lua_root_path[] = "/data/local/tmp/luahook";
static int lua_argv = 10;
map<char *, char *> dic_loaded;
map<void *, HookData *> dic_hooked;

/*****************hot load***************/

int load_lua_script(char *base_path, char *script)
{
	char *md5 = (char *)malloc(33);
	char sztmp[256] = { 0x00 };

	memset(md5, 0x00, 33);
	snprintf(sztmp, 256, "%s/%s.lua", base_path, script);
	compute_file_md5(sztmp, md5);

	if (dic_loaded.find(script)!=dic_loaded.end())
	{
		if (strcmp(dic_loaded[script], md5)==0)
		{
			free(md5);
			return 0;
		}
		else
		{
			//LOGD("unload %s %s", script, dic_loaded[script]);
			snprintf(sztmp, 256, "package.loaded['%s']=nil", script);
			luaL_dostring(L, sztmp);
			free(dic_loaded[script]);
		}
	}

	//LOGD("load %s %s", script, md5);
	snprintf(sztmp, 256, "require ('%s')", script);
	luaL_dostring(L, sztmp);
	dic_loaded[script] = md5;

	return 0;
}

int loop_load_lua(char *base_path)
{
	DIR *dir;
	struct dirent *ptr;
	char base[256];

	if ((dir = opendir(base_path)) == NULL)
	{
		LOGD("open dir %s error...", base_path);
		return -1;
	}

	while ((ptr = readdir(dir)) != NULL)
	{
		if (ptr->d_type == 8 || ptr->d_type == 10)    ///file
		{
			load_lua_script(base_path, strtok(ptr->d_name, "."));
		}
		else if (ptr->d_type == 4)    ///dir
		{
			if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)    ///current dir OR parrent dir
			{
				continue;
			}

			memset(base, 0x00, sizeof(base));
			strcpy(base, base_path);
			strcat(base, "/");
			strcat(base, ptr->d_name);
			loop_load_lua(base);
		}
	}

	closedir(dir);
	return 0;
}

/*****************hook******************/

static int lua_call_function(lua_State* l)
{
	int argv = lua_gettop(l);
	if (argv < 1)
		return 1;

	void *addr = (void *)(int)luaL_checkinteger(l, 1);
	if (dic_hooked.find(addr) != dic_hooked.end())
		addr = dic_hooked[addr]->jmp_addr_bak;

	int *args = new int[argv - 1];
	for (int i = 2; i <= argv; i++)
	{
		int arg = lua_tointeger(l, i);
		args[i - 2] = arg;
	}

	int ret = call_function(addr, argv - 1, (void *)args);
	lua_pushinteger(l, ret);
	return 1;
}

static int call_lua_function(char *lua_func, int *params)
{
	int n = lua_gettop(L);

	vector<char *> vec_fileds;
	char sztmp[128] = { 0x00 };
	strncpy(sztmp, lua_func, 128);
	splitstring(sztmp, '.', vec_fileds);
	lua_getglobal(L, vec_fileds[0]);
	for (int i=1; i<vec_fileds.size(); i++)
	{
		lua_getfield(L, -1, vec_fileds[i]);
	}

	for (int i=0; i<lua_argv; i++)
	{
		lua_pushinteger(L, *params);
		params++;
	}

	lua_call(L, lua_argv, 1);
	int ret = lua_tointeger(L, -1);

	lua_settop(L, n);
	return ret;
}

static void common_func(HookData *hook_data, void *params, ArmRegs *regs)
{
	regs->r0 = call_lua_function(hook_data->lua_func_name, (int *)params);
	hook_data->tramp->jmp_addr = (void *)regs->pc;
}

static bool hook_inner(void *addr, const char *lua_func_name)
{
	if (dic_hooked.find(addr) == dic_hooked.end())
	{
		HookData *data = special_hook(addr, (void *)common_func);
		if (data != NULL)
		{
			strncpy(data->lua_func_name, lua_func_name, sizeof(data->lua_func_name));
			dic_hooked[addr] = data;
			return true;
		}
	}
	else
	{
		strncpy(dic_hooked[addr]->lua_func_name, lua_func_name, sizeof(dic_hooked[addr]->lua_func_name));
		return true;
	}

	return false;
}

static int lua_hook(lua_State* l)
{
	if (lua_gettop(l) != 2)
		return 1;

	if (!hook_inner((void *)(int)luaL_checkinteger(l, 1), luaL_checkstring(l, 2)))
		return 1;

	lua_pushboolean(l, 1);
	return 1;
}

static int lua_unhook(lua_State* l)
{
	if (lua_gettop(l) != 2)
		return 1;

	void *addr = (void *)(int)luaL_checkinteger(l, 1);	
	map<void *, HookData *>::iterator itr = dic_hooked.find(addr);
	if (itr == dic_hooked.end())
		return 1;

	arm_unhook(addr, itr->second->tramp);
	dic_hooked.erase(itr);

	lua_pushboolean(l, 1);
	return 1;
}

static int lua_set_argv(lua_State *l)
{
	if (lua_gettop(l)!=1)
		return 1;

	lua_argv = luaL_checkinteger(l, 1);
	return 1;
}

static int lua_get_addr(lua_State *l)
{
	if (lua_gettop(l)!=2)
		return 1;

	void *p_lib = dlopen(luaL_checkstring(l, 1), RTLD_LAZY);
	void *addr = dlsym(p_lib, luaL_checkstring(l, 2));
	dlclose(p_lib);
	lua_pushinteger(l, (int)addr);
	return 1;
}

static int lua_dump(lua_State *l)
{
	if (lua_gettop(l) != 2)
		return 1;

	void *addr = (void *)(int)luaL_checkinteger(l, 1);
	int size = luaL_checkinteger(l, 2);
	hexdump(addr, size);
	return 1;
}

static int lua_i2s(lua_State *l)
{
	if (lua_gettop(l) != 1)
		return 1;

	lua_pushstring(l, (char *)(int)luaL_checkinteger(l, 1));
	return 1;
}

static int lua_s2i(lua_State *l)
{
	if (lua_gettop(l) != 1)
		return 1;

	lua_pushinteger(l, (int)luaL_checkstring(l, 1));
	return 1;
}

static int lua_i2f(lua_State *l)
{
	if (lua_gettop(l) != 1)
		return 1;

	int i = luaL_checkinteger(l, 1);
	lua_pushnumber(l, (double)*(float *)&i);
	return 1;
}

static int lua_f2i(lua_State *l)
{
	if (lua_gettop(l) != 1)
		return 1;

	float f = (float)luaL_checknumber(l, 1);
	lua_pushinteger(l, *(int *)&f);
	return 1;
}

static int lua_get_obj_name(lua_State *l)
{
	if (lua_gettop(l) != 1)
		return 1;

	char name[256] = { 0x00 };
	int obj = luaL_checkinteger(l, 1);
	get_obj_name(obj, name, 256);
	lua_pushstring(l, name);
	return 1;
}

static int lua_get_obj_type(lua_State *l)
{
	if (lua_gettop(l) != 1)
		return 1;

	int obj = luaL_checkinteger(l, 1);
	int type = get_obj_type(obj);
	lua_pushinteger(l, type);
	return 1;
}

static int lua_get_type_inst(lua_State *l)
{
	if (lua_gettop(l) != 1)
		return 1;

	vector<int> vec_inst;
	int type = luaL_checkinteger(l, 1);
	get_type_inst(type, vec_inst);

	lua_newtable(l);
	for (int i=0; i<vec_inst.size(); i++)
	{
		lua_pushinteger(l, vec_inst[i]);
		lua_rawseti(l, -2, i+1);
	}
	return 1;
}

static int lua_get_inner_obj(lua_State *l)
{
	if (lua_gettop(l) != 1)
		return 1;

	vector<int> vec_inner_obj;
	int type = luaL_checkinteger(l, 1);
	get_inner_obj(type, vec_inner_obj);

	lua_newtable(l);
	for (int i = 0; i < vec_inner_obj.size(); i++)
	{
		lua_pushinteger(l, vec_inner_obj[i]);
		lua_rawseti(l, -2, i+1);
	}
	return 1;
}

static int lua_get_super_class(lua_State *l)
{
	if (lua_gettop(l) != 1)
		return 1;

	vector<int> vec_super_class;
	int type = luaL_checkinteger(l, 1);
	get_super_class(type, vec_super_class);

	lua_newtable(l);
	for (int i = 0; i < vec_super_class.size(); i++)
	{
		lua_pushinteger(l, vec_super_class[i]);
		lua_rawseti(l, -2, i+1);
	}
	return 1;
}

static int lua_property_value(lua_State *l)
{
	int argc = lua_gettop(l);

	// get
	if (argc==2)
	{
		lua_pushinteger(l, get_prop_value(luaL_checkinteger(l, 1), luaL_checkinteger(l, 2)));
	}
	
	// set
	if (argc==3)
	{
		set_prop_value(luaL_checkinteger(l, 1), luaL_checkinteger(l, 2), luaL_checkinteger(l, 3));
		lua_pushboolean(l, 1);
	}

	return 1;
}

static int lua_search_property(lua_State *l)
{
	if (lua_gettop(l) != 2)
		return 1;

	lua_pushinteger(l, search_prop(luaL_checkinteger(l, 1), luaL_checkstring(l, 2)));
	return 1;
}

static int lua_write_int64(lua_State *l)
{
	if (lua_gettop(l) != 2)
		return 1;

	int addr = luaL_checkinteger(l, 1);
	long long value = luaL_checkinteger(l, 2);
	if (addr)
	{
		*(long long *)addr = value;
	}
	return 1;
}

static int lua_read_int64(lua_State *l)
{
	if (lua_gettop(l) != 1)
		return 1;

	int addr = luaL_checkinteger(l, 1);
	if (addr)
	{
		lua_pushinteger(l, *(long long *)addr);
	}
	return 1;
}

static int lua_write_int32(lua_State *l)
{
	if (lua_gettop(l) != 2)
		return 1;

	int addr = luaL_checkinteger(l, 1);
	int value = luaL_checkinteger(l, 2);
	if (addr)
	{
		*(int *)addr = value;
	}
	return 1;
}

static int lua_read_int32(lua_State *l)
{
	if (lua_gettop(l) != 1)
		return 1;

	int addr = luaL_checkinteger(l, 1);
	if (addr)
	{
		lua_pushinteger(l, *(int *)addr);
	}
	return 1;
}

static int lua_write_int16(lua_State *l)
{
	if (lua_gettop(l) != 2)
		return 1;

	int addr = luaL_checkinteger(l, 1);
	int value = luaL_checkinteger(l, 2);
	if (addr)
	{
		*(short *)addr = value;
	}
	return 1;
}

static int lua_read_int16(lua_State *l)
{
	if (lua_gettop(l) != 1)
		return 1;

	int addr = luaL_checkinteger(l, 1);
	if (addr)
	{
		lua_pushinteger(l, *(short *)addr);
	}
	return 1;
}

static int lua_write_int8(lua_State *l)
{
	if (lua_gettop(l) != 2)
		return 1;

	int addr = luaL_checkinteger(l, 1);
	int value = luaL_checkinteger(l, 2);
	if (addr)
	{
		*(char *)addr = value;
	}
	return 1;
}

static int lua_read_int8(lua_State *l)
{
	if (lua_gettop(l) != 1)
		return 1;

	int addr = luaL_checkinteger(l, 1);
	if (addr)
	{
		lua_pushinteger(l, *(char *)addr);
	}
	return 1;
}

static int lua_write_string(lua_State *l)
{
	if (lua_gettop(l) != 2)
		return 1;

	int addr = luaL_checkinteger(l, 1);
	const char *value = luaL_checkstring(l, 2);
	if (addr)
	{
		strcpy((char *)addr, value);
	}
	return 1;
}

static int lua_read_string(lua_State *l)
{
	if (lua_gettop(l) != 1)
		return 1;

	int addr = luaL_checkinteger(l, 1);
	if (addr)
	{
		lua_pushstring(l, (const char *)addr);
	}
	return 1;
}

static int lua_write_wstring(lua_State *l)
{
	if (lua_gettop(l) != 2)
		return 1;

	int addr = luaL_checkinteger(l, 1);
	const char *value = luaL_checkstring(l, 2);
	if (addr)
	{
		mbstowcs((wchar_t *)addr, value, strlen(value));
	}
	return 1;
}

static int lua_read_wstring(lua_State *l)
{
	if (lua_gettop(l) != 1)
		return 1;

	int addr = luaL_checkinteger(l, 1);
	if (addr)
	{
		int len = wcslen((wchar_t *)addr) * 4;
		char *sztmp = (char *)malloc(len+1);
		memset(sztmp, 0x00, len + 1);
		wcstombs(sztmp, (wchar_t *)addr, len);
		lua_pushstring(l, sztmp);
		free(sztmp);
	}
	return 1;
}

static int lua_get_wstring_len(lua_State *l)
{
	if (lua_gettop(l) != 1)
		return 1;

	int addr = luaL_checkinteger(l, 1);
	if (addr)
	{
		lua_pushinteger(l, wcslen((wchar_t *)addr));
	}
	return 1;
}


/***************ctor*******************/
static const luaL_Reg XLUA[] = 
{
	{ "hook", lua_hook },
	{ "unhook", lua_unhook },
	{ "call", lua_call_function },
	{ "argv", lua_set_argv },
	{ "dlsym", lua_get_addr },
	{ "dump", lua_dump },
	{ "name", lua_get_obj_name },
	{ "type", lua_get_obj_type },
	{ "inst", lua_get_type_inst },
	{ "inner", lua_get_inner_obj },
	{ "super", lua_get_super_class },
	{ "value", lua_property_value },
	{ "search", lua_search_property },
	{ "i2s", lua_i2s },
	{ "s2i", lua_s2i },
	{ "i2f", lua_i2f },
	{ "f2i", lua_f2i },
	{ "wint64", lua_write_int64 },
	{ "rint64", lua_read_int64 },
	{ "wint", lua_write_int32 },
	{ "rint", lua_read_int32 },
	{ "wint16", lua_write_int16 },
	{ "rint16", lua_read_int16 },
	{ "wint8", lua_write_int8 },
	{ "rint8", lua_read_int8 },
	{ "wstr", lua_write_string },
	{ "rstr", lua_read_string },
	{ "wwstr", lua_write_wstring },
	{ "rwstr", lua_read_wstring },
	{ "wslen", lua_get_wstring_len },
	{ 0, 0 }
};

int luaopen_xlua(lua_State *L) 
{
	luaL_newlib(L, XLUA);
	return 1;
}

static void *startwork_lua(void *param)
{
	LOGD("startwork_lua");

	L = luaL_newstate();
	if (L)
	{
		// 基础库
		luaL_openlibs(L);
		// Xlua库
		luaL_requiref(L, "x", luaopen_xlua, 1);
		lua_pop(L, 1);
		// 循环监控lua脚本，变化后重载
		while (1)
		{
			loop_load_lua(lua_root_path);
			usleep(50000);
		}
	}
}

__attribute__((constructor))
static void ctor_lua()
{
	pthread_t tid;
	pthread_create(&tid, NULL, startwork_lua, NULL);
}