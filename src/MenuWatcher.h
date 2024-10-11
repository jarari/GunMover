#pragma once

class MenuWatcher : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
protected:
	static MenuWatcher* instance;

public:
	static bool isInPipboyMenu;

	MenuWatcher() = default;
	MenuWatcher(MenuWatcher&) = delete;
	void operator=(const MenuWatcher&) = delete;
	static MenuWatcher* GetSingleton()
	{
		if (!instance)
			instance = new MenuWatcher();
		return instance;
	}
	virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>* src) override;
};
