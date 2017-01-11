#include <dlfcn.h>
#include <core/common-help.h>
#include <core/ue.h>
#include <core/logger.h>

static void *g_objs = NULL;
static wchar_t* (*get_debug_name)(void *) = 0;
static int(*get_objs_addr)(void *) = 0;
static void(*perform_query)(int, int, int, int, int) = 0;

void get_fname_name(long long fname, char *name, int size)
{
	wchar_t *wname = get_debug_name(&fname);
	if (wname)
	{
		wcstombs(name, wname, size);
	}
}

void get_obj_name(int obj, char *name, int size)
{
	if (obj)
	{
		UObject *uobj = (UObject *)obj;
		get_fname_name(*(long long *)&uobj->NamePrivate, name, size);
	}
}

int get_obj_type(int obj)
{
	if (obj)
	{
		UObject *uobj = (UObject *)obj;
		return (int)uobj->ClassPrivate;
	}

	return 0;
}

void get_super_class(int type, vector<int>& vec_super_class)
{
	vec_super_class.push_back(type);
	UStruct *ustruct = (UStruct *)type;
	UStruct *super = ustruct->SuperStruct;
	while (super != NULL)
	{
		vec_super_class.push_back((int)super);
		super = super->SuperStruct;
	}
}

void get_type_inst(int type, vector<int> &vec_inst)
{
	int objs_array = get_objs_addr(g_objs);
	if (objs_array)
	{
		FUObjectItem *objs = *(FUObjectItem **)objs_array;
		int num = *(int *)(objs_array + 8);
		for (int i = 0; i < num; i++)
		{
			int obj = (int)objs[i].Object;
			if (obj)
			{
				if (get_obj_type(obj) == type)
				{
					vec_inst.push_back(obj);
				}
			}
		}
	}
}

void get_class_props(int type, vector<int> &vec_props)
{
	UStruct *u_struct = (UStruct *)type;
	for (UProperty* u_property = u_struct->PropertyLink; u_property; u_property = u_property->PropertyLinkNext)
	{
		vec_props.push_back((int)u_property);
	}
}

int get_prop_offset(int prop)
{
	return ((UProperty *)prop)->Offset_Internal;
}

int get_struct_prop(int prop)
{
	return (int)((UStructProperty *)prop)->ScriptStruct;
}

int get_bool_prop_value(int prop, int addr)
{
	char* ByteValue = (char*)addr + ((UBoolProperty*)prop)->ByteOffset;

	return !!(*ByteValue & ((UBoolProperty*)prop)->FieldMask);
}

void set_bool_prop_value(int prop, int addr, char value)
{
	char* ByteValue = (char*)addr + ((UBoolProperty*)prop)->ByteOffset;
	*ByteValue = (*ByteValue & ~((UBoolProperty*)prop)->FieldMask) | (value ? ((UBoolProperty*)prop)->ByteMask : 0);
}

int get_array_prop(int prop)
{
	return (int)((UArrayProperty *)prop)->Property;
}

int get_prop_array_dim(int prop)
{
	return (int)((UProperty *)prop)->ArrayDim;
}

int get_prop_elem_size(int prop)
{
	return (int)((UProperty *)prop)->ElementSize;
}

__attribute__((constructor))
static void ctor_ue()
{
	void *p_game = dlopen("libUE4.so", RTLD_LAZY);
	g_objs = dlsym(p_game, "GUObjectArray");
	get_debug_name = (wchar_t* (*)(void *))dlsym(p_game, "_Z10DebugFNameR5FName");
	get_objs_addr = (int(*)(void *))dlsym(p_game, "_ZN13FUObjectArray33GetObjectArrayForDebugVisualizersEv");
	dlclose(p_game);
}