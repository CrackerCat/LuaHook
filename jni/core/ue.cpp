#include <map>
#include <algorithm>
#include <dlfcn.h>
#include <core/common-help.h>
#include <core/ue.h>
#include <core/logger.h>

using namespace std;

static void *g_objs = NULL;
static wchar_t* (*get_debug_name)(void *) = 0;
static int(*get_objs_addr)(void *) = 0;
static void(*perform_query)(int, int, int, int, int) = 0;

void get_obj_name(int obj, char *name, int size)
{
	if (obj)
	{
		UObject *uobj = (UObject *)obj;
		wchar_t *wname = get_debug_name(&uobj->NamePrivate);
		if (wname)
		{
			wcstombs(name, wname, size);
		}
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

bool sort_inner_objs_by_type(const int &obj1, const int &obj2)
{
	int type1 = get_obj_type(obj1);
	int type2 = get_obj_type(obj2);
	return type1 > type2;
}

void get_inner_obj(int target_obj, vector<int>& vec_inner_obj)
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
				UObject *outer = (UObject *)obj;
				do
				{
					if ((int)outer == target_obj)
					{
						vec_inner_obj.push_back(obj);
						break;
					}
					outer = outer->OuterPrivate;
				} while (outer != NULL);
			}
		}
	}
	
	sort(vec_inner_obj.begin(), vec_inner_obj.end(), sort_inner_objs_by_type);

}

void get_super_class(int type, vector<int>& vec_super_class)
{
	if (type)
	{
		vec_super_class.push_back(type);
		UStruct *ustruct = (UStruct *)type;
		UStruct *super = ustruct->SuperStruct;
		while (super!=NULL)
		{
			vec_super_class.push_back((int)super);
			super = super->SuperStruct;
		}
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

int search_prop(int inst, const char *prop_name)
{
	// build outer map
	map<int, vector<int> > dic_outer;
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
				int outer = (int)objs[i].Object->OuterPrivate;
				if (outer)
				{
					if (dic_outer.find(outer) == dic_outer.end())
					{
						vector<int> node;
						dic_outer[outer] = node;
					}
					dic_outer[outer].push_back(obj);
				}
			}
		}
	}

	// search
	if (inst)
	{
		vector<int> vec_super;
		get_super_class(get_obj_type(inst), vec_super);
		for (int i = 0; i < vec_super.size(); i++)
		{
			map<int, vector<int> >::iterator itr = dic_outer.find(vec_super[i]);
			if (itr != dic_outer.end())
			{
				for (int j = 0; j < itr->second.size(); j++)
				{
					char sztmp[256] = { 0x00 };
					get_obj_name(itr->second[j], sztmp, 256);
					if (strcmp(sztmp, prop_name) == 0)
					{
						return itr->second[j];
					}
				}
			}
		}
	}

	return 0;
}

long long get_prop_value(int inst, int prop)
{
	if (!inst || !prop)
	{
		return 0;
	}

	char sztmp[256] = { 0x00 };
	get_obj_name(get_obj_type(prop), sztmp, 256);
	int addr = inst + ((UProperty*)prop)->Offset_Internal;

	if (strcmp(sztmp, "Int64Property") == 0 || strcmp(sztmp, "UInt64Property") == 0 || strcmp(sztmp, "DoubleProperty") == 0)
	{
		return *(long long *)addr;
	}

	if (strcmp(sztmp, "IntProperty") == 0 || strcmp(sztmp, "FloatProperty") == 0 || strcmp(sztmp, "UInt32Property") == 0 || strcmp(sztmp, "ObjectProperty")==0)
	{
		return *(int *)addr;
	}

	if (strcmp(sztmp, "Int16Property") == 0 || strcmp(sztmp, "UInt16Property") == 0)
	{
		return *(short *)addr;
	}

	if (strcmp(sztmp, "UInt8Property") == 0)
	{
		return *(char *)addr;
	}

	if (strcmp(sztmp, "ByteProperty") == 0)
	{
		char* ByteValue = (char*)addr + ((UBoolProperty*)prop)->ByteOffset;
		return !!(*ByteValue & ((UBoolProperty*)prop)->FieldMask);
	}

	return 0;
}

void set_prop_value(int inst, int prop, long long value)
{
	if (!inst || !prop)
	{
		return;
	}

	char sztmp[256] = { 0x00 };
	get_obj_name(get_obj_type(prop), sztmp, 256);
	int addr = inst + ((UProperty*)prop)->Offset_Internal;

	if (strcmp(sztmp, "Int64Property") == 0 || strcmp(sztmp, "UInt64Property") == 0 || strcmp(sztmp, "DoubleProperty") == 0)
	{
		*(long long *)addr=value;
	}

	if (strcmp(sztmp, "IntProperty") == 0 || strcmp(sztmp, "FloatProperty") == 0 || strcmp(sztmp, "UInt32Property") == 0 || strcmp(sztmp, "ObjectProperty") == 0)
	{
		*(int *)addr = value;
	}

	if (strcmp(sztmp, "Int16Property") == 0 || strcmp(sztmp, "UInt16Property") == 0)
	{
		*(short *)addr = value;
	}

	if (strcmp(sztmp, "UInt8Property") == 0)
	{
		*(char *)addr = value;
	}

	if (strcmp(sztmp, "ByteProperty") == 0)
	{
		char* ByteValue = (char*)addr + ((UBoolProperty*)prop)->ByteOffset;
		*ByteValue = ((*ByteValue) & ~((UBoolProperty*)prop)->FieldMask) | (value ? ((UBoolProperty*)prop)->ByteMask : 0);
	}
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