#pragma once

namespace Hooks
{
	extern bool shouldAdjust;
	extern RE::NiPoint3 cachedProjectileNodeLocal;
	const static char* projCacheNodeName = "_GM_ORIGPROJ";
	void InitializeHooks();
	void HookedSet1stPersonCameraLocation(RE::NiPoint3&);
	void HookedMove1stPersonToOrigin();
	void HookedPosition3D(RE::MuzzleFlash*, RE::TESObjectREFR*, bool);
}
