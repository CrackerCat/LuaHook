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

print("------------")
local classes = ue.get_classes()
print(#classes)
for _, class in pairs(classes) do
	print(ue.nameof(class))
end

