local send = x.dlsym("libUE4.so", "_ZN12GSBaseClient4SendEjPvi")
local fetch_email = x.dlsym("libUE4.so", "_ZN19GShooterEMailSystem20FetchEmailAttachmentEj")
local signin = x.dlsym("libUE4.so", "_ZN20GShooterSignInSystem6SignInEjh")
local basepawn_takedamage = x.dlsym("libUE4.so", "_ZN22AGShooterBaseCharacter10TakeDamageEfRK12FDamageEventP11AControllerP6AActor")

GSBaseClient = GSBaseClient or nil
EMailSystem = EMailSystem or nil
SignInSystem = SignInSystem or nil

function print_prop(class)
	local super = x.super(class)
	for k, v in pairs(super) do
		print("Class: "..x.name(v))
		local props = x.inner(v)
		for _, m in pairs(props) do
			print("		"..x.name(m).." : "..x.name(x.type(m)))
		end
	end
end

local toggle=0

function my_send(a1, a2, a3, a4)
	--print("send cmd "..a2)
	--x.dump(a3, a4)
	GSBaseClient = a1
	return x.call(send, a1, a2, a3, a4)
end

function my_fetch_email(a1, a2)
	print("fetch email id "..a2)
	a2=-1
	EMailSystem=a1
	return x.call(fetch_email, a1, a2)
end

function my_signin(a1, a2, a3)
	print("signin "..a2.." "..a3)
	SignInSystem=a1
	return x.call(signin, a1, a2, a3)
end

function my_basepawn_takedamage(a1, a2, a3, a4, a5)
	if toggle==0 then
		toggle=1
		--[[
		print("=====================")
		local ammo_max_prop = x.search(a5, "CurrentAmmo")
		local ammo_left_prop = x.search(a5, "CurrentAmmoInClip")
		print("Ammo "..x.value(a5, ammo_left_prop).." / "..x.value(a5, ammo_max_prop))
		x.value(a5, ammo_max_prop, 999)
		x.value(a5, ammo_left_prop, 888)
		--]]
		
		local profile = x.search(a5, "WeaponProfile")
		x.value(a5, profile)
	end
	
	return x.call(basepawn_takedamage, a1, a2, a3, a4, a5)
end

x.hook(send, "my_send")
x.hook(fetch_email, "my_fetch_email")
x.hook(signin, "my_signin")
x.hook(basepawn_takedamage, "my_basepawn_takedamage")