# LuaHook
LuaHook android native function, alse with base help lib for ue4



1、提供一些hook和常用内存读写函数到lua： 
static const luaL_Reg XLUA[] = 
{ 
	{ "hook", lua_hook },  
	{ "unhook", lua_unhook },  
	{ "call", lua_call_function },  
	{ "argv", lua_set_argv },  
	{ "dlsym", lua_get_addr },  
	{ "dump", lua_dump },  
	{ "i2s", lua_i2s },  
	{ "s2i", lua_s2i },  
	{ "i2f", lua_i2f },  
	{ "f2i", lua_f2i },  
	{ "wint64", lua_write_int64 },  
	{ "rint64", lua_read_int64 },  
	{ "wdouble", lua_write_double },  
	{ "rdouble", lua_read_double },  
	{ "wint32", lua_write_int32 },  
	{ "rint32", lua_read_int32 },  
	{ "wfloat", lua_write_float },  
	{ "rfloat", lua_read_float },  
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


2、提供UE的对象属性解析功能，见ue.lua： 
read_fs(fs) 
write_fs(fs, str) 
enum_props(inst) 
... 

3、lua脚本md5校验，改变后即可重新加载，方便测试代码(lua的print已用LOGD替换)。 