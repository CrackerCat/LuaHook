--[[
struct FName
{
	int index;
	int number;
};

struct TArray
{
	void*	AllocatorInstance;
	int		ArrayNum;
	int		ArrayMax;
};

struct UObject
{
	/**V Table**/
	void *vt;			+0
	/** Flag **/
	int	ObjectFlags;	+4
	/** Index into GObjectArray...very private. */
	int	InternalIndex;	+8
	/** Class the object belongs to. */
	void*	ClassPrivate;	+12
	/** Name of this object */
	FName	NamePrivate;	+16
	/** Object this object resides in. */
	UObject*	OuterPrivate;	+24
};

struct UField : UObject
{
	UField *Next;		+28
};

struct UProperty : UField
{
	int			ArrayDim;			+32
	int			ElementSize;		+36
	uint64_t	PropertyFlags;		+40
	int			RepIndex;			+48
	FName		RepNotifyFunc;		+52
	int			Unknow;				+60
	int			Offset_Internal;	+64
	/** In memory only: Linked list of properties from most-derived to base **/
	UProperty*	PropertyLinkNext;	+68
	/** In memory only: Linked list of object reference properties from most-derived to base **/
	UProperty*  NextRef;			+72
	/** In memory only: Linked list of properties requiring destruction. Note this does not include things that will be destroyed byt he native destructor **/
	UProperty*	DestructorLinkNext;	+76
	/** In memory only: Linked list of properties requiring post constructor initialization.**/
	UProperty*	PostConstructLinkNext;	+80
};

struct UBoolProperty : UProperty
{
	/** Size of the bitfield/bool property. Equal to ElementSize but used to check if the property has been properly initialized (0-8, where 0 means uninitialized). */
	char FieldSize;					+84
	/** Offset from the memeber variable to the byte of the property (0-7). */
	char ByteOffset;				+85
	/** Mask of the byte byte with the property value. */
	char ByteMask;					+86
	/** Mask of the field with the property value. Either equal to ByteMask or 255 in case of 'bool' type. */
	char FieldMask;					+87
};

struct UArrayProperty : UProperty
{
	UProperty* Property;			+84
};

struct UStruct : UField
{
	UStruct*	SuperStruct;		+32
	UField*		Children;			+36
	int			PropertiesSize;		+40
	int			MinAlignment;		+44
	TArray		Script;				+48
	/** In memory only: Linked list of properties from most-derived to base **/
	UProperty*	PropertyLink;		+60
	/** In memory only: Linked list of object reference properties from most-derived to base **/
	UProperty*	RefLink;			+64
	/** In memory only: Linked list of properties requiring destruction. Note this does not include things that will be destroyed byt he native destructor **/
	UProperty*	DestructorLink;		+68
	/** In memory only: Linked list of properties requiring post constructor initialization.**/
	UProperty*	PostConstructLink;	+72
	/** Array of object references embedded in script code. Mirrored for easy access by realtime garbage collection code */
	TArray		ScriptObjectReferences;	+76
};


struct UScriptStruct : UStruct
{
	int Size;						+88
	/** ALIGNOF() of the structure **/
	int Alignment;					+92
};

struct UStructProperty : UProperty
{
	UScriptStruct* ScriptStruct;	+84
};

struct FUObjectItem
{
	// Pointer to the allocated object
	UObject *Object;				+0
	// Internal flags
	int Flags;						+4
	// UObject Owner Cluster Index
	int ClusterIndex;				+8
	// Weak Object Pointer Serial number associated with the object
	int SerialNumber;				+12
};
--]]

local env = {}
setmetatable(env, {__index = _G})
_ENV = env

local offset = 
{
	["ClassPrivate"] = 12,
	["NamePrivate"]= 16,
	["SuperStruct"] = 32,
	["PropertyLink"] = 60,
	["PropertyLinkNext"] = 68,
	["Offset_Internal"] = 64,
	["ScriptStruct"] = 84,
	["Property"] = 84,
	["ArrayDim"] = 32,
	["ElementSize"] = 36,
	["ByteOffset"] = 85,
	["ByteMask"] = 86,
	["FieldMask"] = 87,
	["ObjObjects"] = 16
}

local get_debug_name = x.dlsym("libUE4.so", "_Z10DebugFNameR5FName")
local gobj_array = x.dlsym("libUE4.so", "GUObjectArray")

function get_fname_name(fname_addr)
	wchar_name_addr = x.call(get_debug_name, fname_addr)
	return x.rwstr(wchar_name_addr)
end

function get_obj_name(obj)
	return get_fname_name(obj+offset.NamePrivate)
end

function get_obj_type(obj)
	return x.rint32(obj+offset.ClassPrivate)
end

function get_obj_type_name(obj)
	return get_obj_name(get_obj_type(obj))
end

function get_super_class(class)
	local result = {}
	local super = class
	repeat
		table.insert(result, super)
		super = x.rint32(super+offset.SuperStruct)
	until(super==0)
	
	return result
end

function get_class_props(class)
	local result = {}
	local prop = x.rint32(class+offset.PropertyLink)
	while (prop~=0) do
		table.insert(result, prop)
		prop = x.rint32(prop+offset.PropertyLinkNext)
	end
	
	return result
end

function get_class_inst(class)
	local result = {}
	if gobj_array~=0 then
		local objs_start_addr = x.rint32(gobj_array+offset.ObjObjects)
		local objs_count = x.rint32(gobj_array+offset.ObjObjects+8)
		for i=0, objs_count-1 do
			local obj_item_addr =  objs_start_addr + 16*i
			local obj = x.rint32(obj_item_addr)
			local obj_type = get_obj_type(obj)
			if obj_type==class then
				table.insert(result, obj)
			end
		end
	end
	
	return result
end

function get_prop_offset(prop)
	return x.rint32(prop+offset.Offset_Internal)
end

function get_prop_struct(prop)
	return x.rint32(prop+offset.ScriptStruct)
end

function get_prop_array(prop)
	return x.rint32(prop+offset.Property)
end

function get_prop_array_dim(prop)
	return x.rint32(prop+offset.ArrayDim)
end

function get_prop_elem_size(prop)
	return x.rint32(prop+offset.ElementSize)
end

function get_bool_prop_value(prop, addr)
	local bytevalue_addr = addr + x.rint8(prop+offset.ByteOffset)
	return (x.rint8(bytevalue_addr) & x.rint8(prop+offset.FieldMask))~=0 and 1 or 0
end

function set_bool_prop_value(prop, addr, value)
	local bytevalue_addr = addr + x.rint8(prop+offset.ByteOffset)
	x.wint8(bytevalue_addr, (x.rint8(bytevalue_addr) & ~x.rint8(prop+offset.FieldMask))|(value~=0 and x.rint8(prop+offset.ByteMask) or 0))
end

return env