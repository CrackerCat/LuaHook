# LuaHook
LuaHook android native function, alse with base help lib for ue4


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
