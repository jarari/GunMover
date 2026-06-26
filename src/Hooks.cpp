#include "Hooks.h"
#include "Globals.h"
#include "EquipWatcher.h"
#include "MenuWatcher.h"
#include "Configs.h"
#include "EditorUI.h"
#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

REL::Relocation<uintptr_t> DrawWorld_Set1stPersonCameraLocationOrig{ REL::ID{ 598404, 2318327 } };
REL::Relocation<uintptr_t> DrawWorld_Move1stPersonToOriginOrig{ REL::ID{ 76526, 2318293 } };
REL::Relocation<uintptr_t> MuzzleFlash_Position3D{ REL::ID{ 1311341, 2236918 } };
REL::Relocation<RE::NiCamera**> ptr_1stPersonCamera{ REL::ID{ 380177, 2712915 } };
const static float transitionDelay = 0.5f;

using FnSet1stPersonCameraLocation = void (*)(RE::NiPoint3&);
using FnMove1stPersonToOrigin = void (*)();
using FnPosition3D = void (*)(RE::MuzzleFlash*, RE::TESObjectREFR*, bool);

FnSet1stPersonCameraLocation OriginalSet1stPersonCameraLocation;
FnMove1stPersonToOrigin OriginalMove1stPersonToOrigin;
FnPosition3D OriginalPosition3D;

static float transitionTimer;
static RE::NiPoint3 adjustRot;
static RE::NiMatrix3 adjustRotMat;

bool Hooks::shouldAdjust = false;
RE::NiPoint3 Hooks::cachedProjectileNodeLocal;

float easePercentage = 1.f, currentTime = 0.f, duration = 0.f;
std::string clipName;
std::string animationInfo;
RE::NiPoint3 adjustDiff;

namespace
{
	std::shared_mutex resourceProbeLock;
	std::unordered_map<std::string, bool> resourceProbeCache;

	struct RipRel32Patch
	{
		std::size_t instructionOffset;
		std::size_t displacementOffset;
		std::size_t instructionSize;
	};

	std::int32_t MakeRel32Displacement(const std::uintptr_t sourceNext, const std::uintptr_t destination)
	{
		const auto displacement = static_cast<std::int64_t>(destination) - static_cast<std::int64_t>(sourceNext);
		if (displacement < (std::numeric_limits<std::int32_t>::min)() ||
			displacement > (std::numeric_limits<std::int32_t>::max)()) {
			REX::FAIL("rel32 displacement out of range sourceNext={:x} destination={:x}", sourceNext, destination);
		}

		return static_cast<std::int32_t>(displacement);
	}

	void WriteBranch5(const std::uintptr_t source, const std::uintptr_t destination)
	{
		auto& trampoline = REL::GetTrampoline();
		const auto branch = trampoline.allocate_branch5(destination);
		const REL::ASM::JMP5 assembly{ MakeRel32Displacement(source + sizeof(REL::ASM::JMP5), branch) };
		REL::WriteSafeData(source, assembly);
	}

	template <class T>
	T CreateBranchGateway5(
		const char* name,
		REL::Relocation<std::uintptr_t>& target,
		const std::size_t prologueSize,
		void* hook,
		std::initializer_list<RipRel32Patch> ripPatches = {})
	{
		const auto targetAddress = target.address();
		auto* targetBytes = reinterpret_cast<const std::byte*>(targetAddress);
		auto& trampoline = REL::GetTrampoline();

		auto* gateway = static_cast<std::byte*>(trampoline.allocate(prologueSize + sizeof(REL::ASM::JMP14)));
		std::memcpy(gateway, targetBytes, prologueSize);

		for (const auto& patch : ripPatches) {
			std::int32_t oldDisp = 0;
			std::memcpy(std::addressof(oldDisp), targetBytes + patch.displacementOffset, sizeof(oldDisp));
			const auto originalTarget = targetAddress + patch.instructionOffset + patch.instructionSize + oldDisp;
			const auto gatewayNext = reinterpret_cast<std::uintptr_t>(gateway) + patch.instructionOffset + patch.instructionSize;
			const auto newDisp = MakeRel32Displacement(gatewayNext, originalTarget);
			std::memcpy(gateway + patch.displacementOffset, std::addressof(newDisp), sizeof(newDisp));
		}

		const REL::ASM::JMP14 jumpBack{ targetAddress + prologueSize };
		std::memcpy(gateway + prologueSize, std::addressof(jumpBack), sizeof(jumpBack));
		WriteBranch5(targetAddress, reinterpret_cast<std::uintptr_t>(hook));
		logger::info("{} branch hook installed at {:x}", name, targetAddress);
		return reinterpret_cast<T>(gateway);
	}

	const char* SafeString(const char* str)
	{
		return str ? str : "";
	}

	std::string NormalizeSeparators(std::string path)
	{
		for (auto& ch : path) {
			if (ch == '/') {
				ch = '\\';
			}
		}
		while (!path.empty() && path.back() == '\\') {
			path.pop_back();
		}
		return path;
	}

	const char* ReadTaggedString(std::uintptr_t address)
	{
		const auto raw = *reinterpret_cast<std::uintptr_t*>(address) & ~static_cast<std::uintptr_t>(1);
		const auto* str = reinterpret_cast<const char*>(raw);
		return str[0] != '\0' ? str : nullptr;
	}

	std::string GetClipLeaf(const char* authoredPath)
	{
		auto path = NormalizeSeparators(SafeString(authoredPath));
		const auto slash = path.rfind('\\');
		if (slash != std::string::npos) {
			path = path.substr(slash + 1);
		}
		const auto dot = path.rfind('.');
		if (dot != std::string::npos) {
			path = path.substr(0, dot);
		}
		return path;
	}

	std::string ToLowerNormalized(std::string value)
	{
		value = NormalizeSeparators(std::move(value));
		std::ranges::transform(value, value.begin(), [](unsigned char ch) {
			return static_cast<char>(std::tolower(ch));
		});
		return value;
	}

	bool ResourcePathExists(const std::string& path)
	{
		if (path.empty()) {
			return false;
		}

		const auto normalized = NormalizeSeparators(path);
		const auto key = ToLowerNormalized(normalized);
		{
			std::shared_lock lock(resourceProbeLock);
			const auto iter = resourceProbeCache.find(key);
			if (iter != resourceProbeCache.end()) {
				return iter->second;
			}
		}

		bool exists = false;
		{
			RE::BSResourceNiBinaryStream stream(normalized.c_str());
			exists = static_cast<bool>(stream);
		}
		if (!exists && !ToLowerNormalized(normalized).starts_with("meshes\\")) {
			auto meshesPath = std::string("Meshes\\") + normalized;
			RE::BSResourceNiBinaryStream stream(meshesPath.c_str());
			exists = static_cast<bool>(stream);
		}

		{
			std::unique_lock lock(resourceProbeLock);
			resourceProbeCache[key] = exists;
		}

		return exists;
	}

	std::string BuildSubgraphAnimationCandidate(const char* entryPath, const std::string& leaf)
	{
		if (leaf.empty()) {
			return {};
		}

		auto entry = NormalizeSeparators(SafeString(entryPath));
		if (entry.empty()) {
			return {};
		}

		const auto lowerEntry = ToLowerNormalized(entry);
		if (lowerEntry.ends_with(".hkx") || lowerEntry.ends_with(".hkt")) {
			if (ToLowerNormalized(GetClipLeaf(entry.c_str())) == ToLowerNormalized(leaf)) {
				const auto dot = entry.rfind('.');
				return NormalizeSeparators(entry.substr(0, dot) + ".hkx");
			}
			return {};
		}

		auto directory = entry;
		while (!directory.empty() && directory.back() == '\\') {
			directory.pop_back();
		}
		if (directory.empty()) {
			return {};
		}

		return NormalizeSeparators(directory + "\\" + leaf + ".hkx");
	}

	std::uintptr_t ReadCurrentBehaviorRootId(std::uintptr_t behaviorGraph)
	{
		if (!behaviorGraph) {
			return 0;
		}

		return *reinterpret_cast<std::uintptr_t*>(behaviorGraph + 0x30);
	}

	std::uintptr_t ResolveOwningGraphFromBehaviorGraph(std::uintptr_t behaviorGraph, std::uintptr_t fallbackGraph)
	{
		const auto graph = ReadCurrentBehaviorRootId(behaviorGraph);
		if (!graph) {
			return fallbackGraph;
		}

		const auto vtbl = *reinterpret_cast<std::uintptr_t*>(graph);
		if (vtbl != REL::Relocation<uintptr_t>{ RE::VTABLE::BShkbAnimationGraph[0] }.address()) {
			return fallbackGraph;
		}

		return graph;
	}

	std::uintptr_t FindSelectedSubgraphData(std::uintptr_t graph, std::uintptr_t behaviorGraph)
	{
		const auto rootId = ReadCurrentBehaviorRootId(behaviorGraph);
		if (!graph || !rootId) {
			return 0;
		}

		const auto swapArray = *reinterpret_cast<std::uintptr_t*>(graph + 0x3A0);
		if (!swapArray) {
			return 0;
		}

		const auto entries = *reinterpret_cast<std::uintptr_t*>(swapArray);
		const auto count = *reinterpret_cast<std::uint32_t*>(swapArray + 0x10);
		if (!entries || count == 0) {
			return 0;
		}

		for (std::uint32_t i = 0; i < count; ++i) {
			const auto entry = entries + static_cast<std::uintptr_t>(i) * 0x48;
			const auto sharedData = *reinterpret_cast<std::uintptr_t*>(entry + 0x08);
			if (!sharedData) {
				continue;
			}

			if (*reinterpret_cast<std::uintptr_t*>(sharedData + 0xC0) == rootId) {
				return sharedData - 0x40;
			}
		}

		return 0;
	}


	struct ResolvedClipPath
	{
		std::string path;
	};

	ResolvedClipPath ResolveFromSelectedSubgraphFiles(std::uintptr_t graph, std::uintptr_t behaviorGraph, const char* authoredPath)
	{
		const auto data = FindSelectedSubgraphData(graph, behaviorGraph);
		if (!data) {
			return {};
		}

		const auto leaf = GetClipLeaf(authoredPath);
		if (leaf.empty()) {
			return {};
		}

		const auto scanArray = [&](std::uintptr_t arrayOffset) -> ResolvedClipPath {
			const auto array = data + arrayOffset;
			const auto entries = *reinterpret_cast<std::uintptr_t*>(array);
			const auto size = *reinterpret_cast<std::uint32_t*>(array + 0x10);
			if (!entries || size == 0) {
				return {};
			}

			for (std::uint32_t i = 0; i < size; ++i) {
				const auto* entry = reinterpret_cast<const RE::BSFixedString*>(entries + static_cast<std::uintptr_t>(i) * sizeof(RE::BSFixedString));
				const auto* entryPath = entry->c_str();
				if (!entryPath || entryPath[0] == '\0') {
					continue;
				}

				const auto candidate = BuildSubgraphAnimationCandidate(entryPath, leaf);
				if (!candidate.empty() && ResourcePathExists(candidate)) {
					return { candidate };
				}
			}

			return {};
		};

		if (auto resolved = scanArray(0x178); !resolved.path.empty()) {
			return resolved;
		}
		if (auto resolved = scanArray(0x160); !resolved.path.empty()) {
			return resolved;
		}

		return {};
	}

	ResolvedClipPath ResolveClipDisplayPath(std::uintptr_t graph, std::uintptr_t behaviorGraph, const char* authoredPath)
	{
		const auto leaf = GetClipLeaf(authoredPath);
		if (leaf.empty()) {
			return { SafeString(authoredPath) };
		}

		if (auto resolved = ResolveFromSelectedSubgraphFiles(graph, behaviorGraph, authoredPath); !resolved.path.empty()) {
			return resolved;
		}

		return { SafeString(authoredPath) };
	}

	bool IsClipGenerator(std::uintptr_t candidate)
	{
		if (!candidate) {
			return false;
		}

		return *reinterpret_cast<std::uintptr_t*>(candidate) == RE::VTABLE::hkbClipGenerator[0].address();
	}

	std::uintptr_t SelectActiveClip(std::uintptr_t activeNodeEntry)
	{
		const auto entryAsClip = IsClipGenerator(activeNodeEntry) ? activeNodeEntry : 0;
		std::uintptr_t nestedClip = 0;
		if (activeNodeEntry) {
			const auto candidate = *reinterpret_cast<std::uintptr_t*>(activeNodeEntry + 0x08);
			nestedClip = IsClipGenerator(candidate) ? candidate : 0;
		}

		if (entryAsClip) {
			return entryAsClip;
		}
		if (nestedClip) {
			return nestedClip;
		}
		return entryAsClip ? entryAsClip : nestedClip;
	}

	std::uintptr_t ReadNestedBehaviorGraph(std::uintptr_t activeNodeEntry)
	{
		if (!activeNodeEntry) {
			return 0;
		}
		return *reinterpret_cast<std::uintptr_t*>(activeNodeEntry + 0x10);
	}

}

namespace RE
{
	class EquipEventSource :
		public RE::BSTEventSource<RE::TESEquipEvent>
	{
	public:
		[[nodiscard]] static EquipEventSource* GetSingleton()
		{
			REL::Relocation<EquipEventSource*> singleton{ REL::ID{ 485633, 2691240, 4798533 } };
			return singleton.get();
		}
	};
}

void Hooks::InitializeHooks()
{
	animationInfo.reserve(1024);
	Globals::ui->GetEventSource<RE::MenuOpenCloseEvent>()->RegisterSink(MenuWatcher::GetSingleton());
	RE::EquipEventSource::GetSingleton()->RegisterSink(EquipWatcher::GetSingleton());

	const auto isOG = REX::FModule::IsRuntimeOG();
	OriginalSet1stPersonCameraLocation =
		CreateBranchGateway5<FnSet1stPersonCameraLocation>(
			"DrawWorld::Set1stPersonCameraLocation",
			DrawWorld_Set1stPersonCameraLocationOrig,
			0x11,
			reinterpret_cast<void*>(&HookedSet1stPersonCameraLocation),
			{ { 0x4, 0x8, 0x8 } });
	OriginalMove1stPersonToOrigin =
		CreateBranchGateway5<FnMove1stPersonToOrigin>(
			"DrawWorld::Move1stPersonToOrigin",
			DrawWorld_Move1stPersonToOriginOrig,
			isOG ? 0x5 : 0x9,
			reinterpret_cast<void*>(&HookedMove1stPersonToOrigin));
	OriginalPosition3D =
		CreateBranchGateway5<FnPosition3D>(
			"MuzzleFlash::Position3D",
			MuzzleFlash_Position3D,
			0xB,
			reinterpret_cast<void*>(&HookedPosition3D));
}

namespace RE
{
	class hkbClipGenerator;

	struct hkbBehaviorGraph
	{
		struct hkbNodeInfo
		{
			void* unk00;                       // 00
			hkbClipGenerator* node;            // 08
			hkbBehaviorGraph* behaviorGraph;   // 10
		};

		// members
		std::uint64_t unk00[0xE0 >> 3];            // 000
		hkArray<hkbNodeInfo*>* activeNodes;        // 0E0
		std::uint64_t unkE8[(0x1A8 - 0xE8) >> 3];  // 0E8
		std::uint8_t unk1A8;                       // 1A8
		std::uint8_t unk1A9;                       // 1A9
		bool isActive;                             // 1AA
		bool isLinked;                             // 1AB
		bool updateActiveNodes;                    // 1AC
		bool stateOrTransitionChanged;             // 1AD
	};

	class hkbClipGenerator
	{
	public:
		// members
		std::uint64_t unk00[0x38 >> 3];           // 00
		const char* animName;                     // 38
		std::uint32_t id;						  // 40
		std::uint8_t cloneState;                  // 42
		std::uint8_t pad43;                       // 43
		std::uint32_t pad44;                      // 44
		std::uint64_t unk40[(0x90 - 0x48) >> 3];  // 40
		const char* animPath;                     // 90
	};
}
void GetClipInfo(float& _currentTime, float& _duration, std::string& _clipName) {
	if (Globals::p->currentProcess) {
		RE::BSAnimationGraphManager* graphManager = Globals::p->currentProcess->middleHigh->animationGraphManager.get();
		if (graphManager) {
			RE::BShkbAnimationGraph* graph = graphManager->variableCache.graphToCacheFor.get();
			if (graph) {
				RE::hkbBehaviorGraph* hkGraph = *(RE::hkbBehaviorGraph**)((uintptr_t)graph + 0x378);
				if (hkGraph->activeNodes && hkGraph->activeNodes->size() > 0 && !hkGraph->updateActiveNodes && !hkGraph->stateOrTransitionChanged) {
					for (int i = 0; i < hkGraph->activeNodes->size(); ++i) {
						const auto activeNodeEntry = reinterpret_cast<std::uintptr_t>((*hkGraph->activeNodes)[i]);
						if (const auto clip = SelectActiveClip(activeNodeEntry)) {
							uintptr_t animCtrl = *(uintptr_t*)(clip + 0xD0);
							if (animCtrl) {
								uintptr_t animBinding = *(uintptr_t*)(animCtrl + 0x38);
								uintptr_t anim = *(uintptr_t*)(animBinding + 0x18);
								_currentTime = *(float*)(clip + 0x140);
								_duration = *(float*)(anim + 0x14);
								_clipName = std::string(SafeString(ReadTaggedString(clip + 0x38)));
								return;
							}
						}
					}
				}
			}
		}
	}
}
void GetAllClipInfo(std::string& clipInfo)
{
	if (!Globals::p || !Globals::p->currentProcess || !Globals::p->currentProcess->middleHigh) {
		return;
	}

	RE::BSAnimationGraphManager* graphManager = Globals::p->currentProcess->middleHigh->animationGraphManager.get();
	if (!graphManager) {
		return;
	}

	RE::BShkbAnimationGraph* graph = graphManager->variableCache.graphToCacheFor.get();
	if (!graph) {
		return;
	}

	auto* hkGraph = *reinterpret_cast<RE::hkbBehaviorGraph**>(reinterpret_cast<std::uintptr_t>(graph) + 0x378);
	if (!hkGraph) {
		return;
	}

	if (!hkGraph->activeNodes) {
		return;
	}

	if (hkGraph->activeNodes->empty()) {
		return;
	}

	if (hkGraph->updateActiveNodes || hkGraph->stateOrTransitionChanged) {
		return;
	}

	for (int i = 0; i < hkGraph->activeNodes->size(); ++i) {
		const auto activeNodeEntry = reinterpret_cast<std::uintptr_t>((*hkGraph->activeNodes)[i]);
		if (const auto clip = SelectActiveClip(activeNodeEntry)) {
			const auto* name = SafeString(ReadTaggedString(clip + 0x38));
			const auto* authoredPath = SafeString(ReadTaggedString(clip + 0x90));
			const auto behaviorGraph = ReadNestedBehaviorGraph(activeNodeEntry);
			const auto graphForClip = ResolveOwningGraphFromBehaviorGraph(
				behaviorGraph,
				reinterpret_cast<std::uintptr_t>(graph));
			auto displayPath = ResolveClipDisplayPath(graphForClip, behaviorGraph, authoredPath);

			clipInfo.append("Clip: ");
			clipInfo.append(name);
			clipInfo.append("\n");
			clipInfo.append(displayPath.path);
			clipInfo.append("\n");
		}
	}
}

void Hooks::HookedSet1stPersonCameraLocation(RE::NiPoint3& a_loc)
{
	if (shouldAdjust && Configs::adjustment) {
		const float deltaTime = *Utils::ptr_deltaTime;
		Configs::adjustment->StepTransition(deltaTime);

		RE::NiNode* fireNode = nullptr;
		if (Globals::p->currentProcess) {
			Globals::p->currentProcess->middleHigh->equippedItemsLock.lock();
			RE::BSTArray<RE::EquippedItem>& equipped = Globals::p->currentProcess->middleHigh->equippedItems;
			if (equipped.size() != 0 && equipped[0].data.get()) {
				RE::EquippedWeaponData* weapData = static_cast<RE::EquippedWeaponData*>(equipped[0].data.get());
				if (weapData->fireNode) {
					fireNode = static_cast<RE::NiNode*>(weapData->fireNode);
				}
			}
			Globals::p->currentProcess->middleHigh->equippedItemsLock.unlock();
		}

		if (cachedProjectileNodeLocal.x == FLT_MAX && fireNode) {
			RE::NiAVObject* projCacheNode = fireNode->GetObjectByName(projCacheNodeName);
			if (projCacheNode) {
				cachedProjectileNodeLocal = projCacheNode->local.translate;
			} else {
				cachedProjectileNodeLocal = fireNode->local.translate;
				projCacheNode = Utils::CreateBone(projCacheNodeName);
				if (projCacheNode) {
					fireNode->AttachChild(projCacheNode, true);
					projCacheNode->local.translate = cachedProjectileNodeLocal;
				}
			}
		}

		RE::NiMatrix3 playerDirMat = Utils::GetRotationMatrix33(0, -Globals::p->data.angle.x, -Globals::p->data.angle.z);
		RE::NiPoint3 adjustWorld = playerDirMat * Configs::adjustment->translation;
		adjustRot = Configs::adjustment->rotation;
		float zoomPercentage = *(float*)((uintptr_t)Globals::pcam->cameraStates[RE::CameraState::kFirstPerson].get() + 0x78);

		currentTime = 0.f, duration = 0.f;
		GetClipInfo(currentTime, duration, clipName);
		if (EditorUI::Window::GetSingleton()->GetShouldDraw()) {
			animationInfo.clear();
			GetAllClipInfo(animationInfo);
		}
		if (MenuWatcher::GetSingleton()->isInPipboyMenu || 
			(Globals::p->currentProcess && Globals::p->currentProcess->middleHigh->weaponCullCounter > 0) ||
			(Configs::adjustment->GetAdjustmentFlag(Configs::REVERT_FLAG::kRevertOnReload) && Globals::p->gunState == RE::GUN_STATE::kReloading && (duration - currentTime) >= duration / 3.f) ||
			(Configs::adjustment->GetAdjustmentFlag(Configs::REVERT_FLAG::kRevertOnMelee) && (Globals::p->meleeAttackState & 0x2) && (duration - currentTime) >= duration / 2.5f) ||
			(Configs::adjustment->GetAdjustmentFlag(Configs::REVERT_FLAG::kRevertOnThrow) && Globals::p->gunState == RE::GUN_STATE::kFire && (clipName == "WPNGrenadeThrow" || clipName == "WPNMineThrow") && (duration - currentTime) >= duration / 3.f) ||
			(Configs::adjustment->GetAdjustmentFlag(Configs::REVERT_FLAG::kRevertOnSprint) && (Globals::p->moveMode & 0x100) == 0x100) ||
			(Configs::adjustment->GetAdjustmentFlag(Configs::REVERT_FLAG::kRevertOnEquip) && clipName == "WPNEquip" && (duration - currentTime) >= duration / 3.f) ||
			(Configs::adjustment->GetAdjustmentFlag(Configs::REVERT_FLAG::kRevertOnFastEquip) && clipName == "WPNEquipFast" && (duration - currentTime) >= duration / 2.f) ||
			(Configs::adjustment->GetAdjustmentFlag(Configs::REVERT_FLAG::kRevertOnUnequip) && Globals::p->weaponState == RE::WEAPON_STATE::kSheathing) ||
			(Configs::adjustment->GetAdjustmentFlag(Configs::REVERT_FLAG::kRevertOnGunDown) && Globals::p->gunState == RE::GUN_STATE::kBlocked)) {
			transitionTimer = min(transitionDelay, transitionTimer + deltaTime);
		} else {
			transitionTimer = max(0.f, transitionTimer - deltaTime);
		}

		float adjustPercentage = 0.f;
		if (zoomPercentage > 0.f || transitionTimer > 0.f) {
			adjustPercentage = min(1.f - zoomPercentage, Utils::easeInOutQuad(1.f - transitionTimer / transitionDelay));
		} else {
			adjustPercentage = 1.f;
		}
		easePercentage = 1.f;
		if (adjustPercentage < 1.f) {
			easePercentage = adjustPercentage;
			adjustWorld = adjustWorld * easePercentage;
			adjustRot = adjustRot * easePercentage;
		}

		adjustRotMat = Utils::GetRotationMatrix33(adjustRot.z, adjustRot.y, adjustRot.x);

		if (fireNode) {
			adjustDiff = Utils::GetRotationMatrix33(adjustRot.y, adjustRot.x, adjustRot.z) * (cachedProjectileNodeLocal + Configs::adjustment->translation * easePercentage) - cachedProjectileNodeLocal;
			fireNode->local.translate = cachedProjectileNodeLocal + adjustDiff;
		}

		a_loc = a_loc - adjustWorld;
	} else {
		transitionTimer = 0.f;
		adjustDiff = RE::NiPoint3();
	}
	if (OriginalSet1stPersonCameraLocation) {
		OriginalSet1stPersonCameraLocation(a_loc);
	}
}

void Hooks::HookedMove1stPersonToOrigin()
{
	if (OriginalMove1stPersonToOrigin) {
		OriginalMove1stPersonToOrigin();
	}

	if (shouldAdjust) {
		if (adjustRot.Length() > 0.f) {
			(*ptr_1stPersonCamera)->world.rotate = adjustRotMat * (*ptr_1stPersonCamera)->world.rotate;
		}
	}
}

void Hooks::HookedPosition3D(RE::MuzzleFlash* a_muz, RE::TESObjectREFR* a_ref, bool a_updateNode)
{
	if (OriginalPosition3D) {
		OriginalPosition3D(a_muz, a_ref, a_updateNode);
		if (shouldAdjust) {
			RE::NiNode* muzNode = *(RE::NiNode**)((uintptr_t)a_muz + 0x10);
			if (muzNode) {
				muzNode->local.translate = muzNode->local.translate - adjustDiff;
			}
		}
	}
}
