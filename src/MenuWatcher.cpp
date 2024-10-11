#include "Configs.h"
#include "MenuWatcher.h"

MenuWatcher* MenuWatcher::instance = nullptr;
bool MenuWatcher::isInPipboyMenu = false;
	RE::BSEventNotifyControl MenuWatcher::ProcessEvent(const RE::MenuOpenCloseEvent& evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>* src) {
	if (!evn.opening) {
		if (evn.menuName == "LoadingMenu") {
			Configs::SetAdjustmentForEquipped();
		}
	}
	if (evn.menuName == "PipboyMenu") {
		isInPipboyMenu = evn.opening;
	}
	return RE::BSEventNotifyControl::kContinue;
}
