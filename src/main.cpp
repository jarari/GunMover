#include <fstream>
#include <shared_mutex>
#include <wtypes.h>
#include <Globals.h>
#include <Hooks.h>
#include <Configs.h>
#include <EditorUI.h>

void OnInit(F4SE::MessagingInterface::Message* msg)
{
	if (msg->type == F4SE::MessagingInterface::kGameDataReady) {
		Globals::InitializeGlobals();
		Hooks::InitializeHooks();
		Configs::LoadConfigs();
	} else if (msg->type == F4SE::MessagingInterface::kPostLoadGame) {
		Configs::LoadConfigs();
	} else if (msg->type == F4SE::MessagingInterface::kNewGame) {
		Configs::LoadConfigs();
	}
}

F4SEPluginLoad(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se, {
		.log = true,
		.logName = Version::PROJECT.data(),
		.trampoline = true,
		.trampolineSize = 1024,
	});

	EditorUI::HookD3D11();

	const F4SE::MessagingInterface* message = F4SE::GetMessagingInterface();
	message->RegisterListener(OnInit);

	return true;
}

extern "C"
{
	F4SE_EXPORT bool F4SEPlugin_Query(const F4SE::QueryInterface*, F4SE::PluginInfo* a_info)
	{
		a_info->name = Version::PROJECT.data();
		a_info->infoVersion = F4SE::PluginInfo::kVersion;
		a_info->version = Version::MAJOR;
		return true;
	}
}
