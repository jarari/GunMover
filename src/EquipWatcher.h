#pragma once

class EquipWatcher : public RE::BSTEventSink<RE::TESEquipEvent>
{
protected:
	static EquipWatcher* instance;

public:
	EquipWatcher() = default;
	EquipWatcher(EquipWatcher&) = delete;
	void operator=(const EquipWatcher&) = delete;
	static EquipWatcher* GetSingleton()
	{
		if (!instance)
			instance = new EquipWatcher();
		return instance;
	}
	virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent& evn, RE::BSTEventSource<RE::TESEquipEvent>* src) override;
};
