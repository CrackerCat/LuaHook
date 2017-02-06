local ue = require("ue")

local actor_rpc = x.dlsym("libUE4.so", "_ZN6AActor18CallRemoteFunctionEP9UFunctionPvP11FOutParmRecP6FFrame")

function my_actor_rpc(a1, a2, a3, a4, a5)
	print("-------------")
	print("rpc: "..ue.nameof(a1).."->"..ue.nameof(a2)..":")
	local params = ue.get_params(a2, a3)
	for _, param in pairs(params) do
		print(param.name..":"..param.value)
	end
	
	return x.call(actor_rpc, a1, a2, a3, a4, a5)
end

x.hook(actor_rpc, "my_actor_rpc")

--[[
local classes = ue.get_classes()
for _, class in pairs(classes) do
	print(ue.nameof(class))
end
--]]


print("-----------------")
local class_localplayer=ue.find_class("HTSkillParts")
local inst_localplayer = ue.get_inst(class_localplayer)
for _, inst in pairs(inst_localplayer) do
	print("inst "..ue.nameof(inst))
	if ue.nameof(inst)=="HTSkillParts_0" then
		local ctrl = ue.find_prop(inst, "BattleModeControl")
		print(ctrl.name)
		local props = ue.get_props(ctrl.value)
		for _, prop in pairs(props) do
			print(prop.type.."->"..prop.name..":"..prop.value)
		end
	end
end
