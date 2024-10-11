#include "Globals.h"

RE::PlayerCharacter* Globals::p = nullptr;
RE::PlayerCamera* Globals::pcam = nullptr;
RE::UI* Globals::ui = nullptr;

void Globals::InitializeGlobals() {
	p = RE::PlayerCharacter::GetSingleton();
	pcam = RE::PlayerCamera::GetSingleton();
	ui = RE::UI::GetSingleton();
}
