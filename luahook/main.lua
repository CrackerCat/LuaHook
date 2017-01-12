local ue = require("ue")
local send = x.dlsym("libUE4.so", "_ZN12GSBaseClient4SendEjPvi")
local fetch_email = x.dlsym("libUE4.so", "_ZN19GShooterEMailSystem20FetchEmailAttachmentEj")
local signin = x.dlsym("libUE4.so", "_ZN20GShooterSignInSystem6SignInEjh")
local basepawn_takedamage = x.dlsym("libUE4.so", "_ZN22AGShooterBaseCharacter10TakeDamageEfRK12FDamageEventP11AControllerP6AActor")
local purchase = x.dlsym("libUE4.so", "_ZN20GShooterLogicSession20PurchaseGoodsRequestEjhhjb")
local search_friend = x.dlsym("libUE4.so", "_ZN20GShooterFriendSystem19SearchFriendRequestERK7FString")
local activity_req = x.dlsym("libUE4.so", "_ZN22GShooterActivitySystem24GetActivityRewardRequestEjh")
local mission_req = x.dlsym("libUE4.so", "_ZN15GShooterMission24GetLivenessRewardRequestEj")
local chat_req = x.dlsym("libUE4.so", "_ZN18GShooterChatSystem14SendMsgRequestEN24EGShooterChatChannelType4TypeE7FStringjRK5FTextj")
local localplayer_beginplay = x.dlsym("libUE4.so", "_ZN25AGShooterPlayerController9BeginPlayEv")
local start_reload = x.dlsym("libUE4.so", "_ZN15AGShooterWeapon11StartReloadEb")
local server_reload = x.dlsym("libUE4.so", "_ZN15AGShooterWeapon17ServerStartReloadEv")
local pvp_god = x.dlsym("libUE4.so", "_ZN25AGShooterPlayerController12ServerPvpGodEbf")
local moveto = x.dlsym("libUE4.so", "_ZN27UCharacterMovementComponent21ReplicateMoveToServerEfRK7FVector")
local server_sucide = x.dlsym("libUE4.so", "_ZN17APlayerController5ResetEv")

GSBaseClient = GSBaseClient or nil
EMailSystem = EMailSystem or nil
SignInSystem = SignInSystem or nil
LogicSession = LogicSession or nil
ActivitySystem = ActivitySystem or nil
LocalPlayer = LocalPlayer or nil
Attacker = Attacker or nil

function my_send(a1, a2, a3, a4)
	GSBaseClient = a1
	
	--print("send cmd "..a2)
	--x.dump(a3, a4)
	return x.call(send, a1, a2, a3, a4)
end

function my_fetch_email(a1, a2)
	print("fetch email id "..a2)
	EMailSystem=a1
	return x.call(fetch_email, a1, a2)
end

function my_signin(a1, a2, a3)
	print("signin "..a2.." "..a3)
	SignInSystem=a1
	return x.call(signin, a1, a2, a3)
end

function my_basepawn_takedamage(a1, a2, a3, a4, a5)
	--[[
	local attacker_name = x.name(a5)
	if string.find(attacker_name, "_npc_") or string.find(attacker_name, "_NPC_") then
		a2=x.f2i(0)
	else
		a2=x.f2i(99999)
	end
	--]]
	Attacker=a5
	return x.call(basepawn_takedamage, a1, a2, a3, a4, a5)
end

function my_purchase(a1, a2,  a3, a4, a5, a6)
	LogicSession = a1
	print("purchase "..a2.." "..a3.." "..a4.." "..a5.." "..a6)
	return x.call(purchase, a1, a2, a3, a4, a5, a6)
end

function my_search_friend(a1, a2)
	return x.call(search_friend, a1, a2)
end

function my_activity_req(a1, a2, a3)
	ActivitySystem = a1
	print("activity_req " .. a2 .. " " .. a3)
	return x.call(activity_req, a1, a2, a3)
end

function my_mission_req(a1, a2)
	print("mission_req "..a2)
	return x.call(mission_req, a1, a2)
end

function my_chat_req(a1, a2, a3, a4, a5, a6)
	return x.call(chat_req, a1, a2, a3, a4, a5, a6)
end

function my_localplayer_beginplay(a1)
	LocalPlayer = a1
	return x.call(localplayer_beginplay, a1)
end

function my_start_reload(a1, a2)
	print("reloadx...")
	return x.call(start_reload, a1, a2)
end

function my_moveto(a1, a2, a3)
	--print("moveto "..x.i2f(a2))
	a2 = x.f2i(0.9)
	return x.call(moveto, a1, a2, a3)
end

x.hook(send, "my_send")
x.hook(fetch_email, "my_fetch_email")
x.hook(signin, "my_signin")
x.hook(basepawn_takedamage, "my_basepawn_takedamage")
x.hook(purchase, "my_purchase")
x.hook(search_friend, "my_search_friend")
x.hook(activity_req, "my_activity_req")
x.hook(mission_req, "my_mission_req")
x.hook(chat_req, "my_chat_req")
x.hook(localplayer_beginplay, "my_localplayer_beginplay")
x.hook(start_reload, "my_start_reload")
x.hook(moveto, "my_moveto")


if LocalPlayer then
	print("god")
	x.call(server_sucide, LocalPlayer)
end


print("-------cccc------")
if Attacker then
	print("Attacker:")
	local props = ue.enum_props(Attacker)
	for _, prop in pairs(props) do
		print(prop.name..":"..prop.type..":"..prop.value)
	end
end
