namespace EditorUI
{
	void HookD3D11();

	struct Hotkey
	{
		int mainKey = 0;  // Main key (e.g., 'A', 'X', etc.)
		bool ctrl = false;
		bool shift = false;
		bool alt = false;
		short captureState = 0;

		// Custom serialization for ini file
		std::string ToString() const
		{
			return std::to_string(mainKey) + "," + (ctrl ? "1" : "0") + "," + (shift ? "1" : "0") + "," + (alt ? "1" : "0");
		}

		std::string GetKeyName(int virtualKey) const
		{
			switch (virtualKey) {
			case 0x20:
				return "Space";
			case 0x21:
				return "Page Up";
			case 0x22:
				return "Page Down";
			case 0x23:
				return "End";
			case 0x24:
				return "Home";
			case 0x25:
				return "Left Arrow";
			case 0x26:
				return "Up Arrow";
			case 0x27:
				return "Right Arrow";
			case 0x28:
				return "Down Arrow";
			case 0x29:
				return "Select";
			case 0x2A:
				return "Print";
			case 0x2B:
				return "Execute";
			case 0x2C:
				return "Print Screen";
			case 0x2D:
				return "Insert";
			case 0x2E:
				return "Delete";
			case 0x2F:
				return "Help";
			case 0x30:
				return "0";
			case 0x31:
				return "1";
			case 0x32:
				return "2";
			case 0x33:
				return "3";
			case 0x34:
				return "4";
			case 0x35:
				return "5";
			case 0x36:
				return "6";
			case 0x37:
				return "7";
			case 0x38:
				return "8";
			case 0x39:
				return "9";
			case 0x41:
				return "A";
			case 0x42:
				return "B";
			case 0x43:
				return "C";
			case 0x44:
				return "D";
			case 0x45:
				return "E";
			case 0x46:
				return "F";
			case 0x47:
				return "G";
			case 0x48:
				return "H";
			case 0x49:
				return "I";
			case 0x4A:
				return "J";
			case 0x4B:
				return "K";
			case 0x4C:
				return "L";
			case 0x4D:
				return "M";
			case 0x4E:
				return "N";
			case 0x4F:
				return "O";
			case 0x50:
				return "P";
			case 0x51:
				return "Q";
			case 0x52:
				return "R";
			case 0x53:
				return "S";
			case 0x54:
				return "T";
			case 0x55:
				return "U";
			case 0x56:
				return "V";
			case 0x57:
				return "W";
			case 0x58:
				return "X";
			case 0x59:
				return "Y";
			case 0x5A:
				return "Z";
			case 0x5B:
				return "Left Windows";
			case 0x5C:
				return "Right Windows";
			case 0x5D:
				return "Applications";
			case 0x5F:
				return "Sleep";
			case 0x60:
				return "Numpad 0";
			case 0x61:
				return "Numpad 1";
			case 0x62:
				return "Numpad 2";
			case 0x63:
				return "Numpad 3";
			case 0x64:
				return "Numpad 4";
			case 0x65:
				return "Numpad 5";
			case 0x66:
				return "Numpad 6";
			case 0x67:
				return "Numpad 7";
			case 0x68:
				return "Numpad 8";
			case 0x69:
				return "Numpad 9";
			case 0x6A:
				return "Numpad Multiply";
			case 0x6B:
				return "Numpad Add";
			case 0x6C:
				return "Numpad Separator";
			case 0x6D:
				return "Numpad Subtract";
			case 0x6E:
				return "Numpad Decimal";
			case 0x6F:
				return "Numpad Divide";
			case 0x70:
				return "F1";
			case 0x71:
				return "F2";
			case 0x72:
				return "F3";
			case 0x73:
				return "F4";
			case 0x74:
				return "F5";
			case 0x75:
				return "F6";
			case 0x76:
				return "F7";
			case 0x77:
				return "F8";
			case 0x78:
				return "F9";
			case 0x79:
				return "F10";
			case 0x7A:
				return "F11";
			case 0x7B:
				return "F12";
			case 0x7C:
				return "F13";
			case 0x7D:
				return "F14";
			case 0x7E:
				return "F15";
			case 0x7F:
				return "F16";
			case 0x80:
				return "F17";
			case 0x81:
				return "F18";
			case 0x82:
				return "F19";
			case 0x83:
				return "F20";
			case 0x84:
				return "F21";
			case 0x85:
				return "F22";
			case 0x86:
				return "F23";
			case 0x87:
				return "F24";
			case 0x90:
				return "Num Lock";
			case 0x91:
				return "Scroll Lock";
			case 0xA0:
				return "Left Shift";
			case 0xA1:
				return "Right Shift";
			case 0xA2:
				return "Left Control";
			case 0xA3:
				return "Right Control";
			case 0xA4:
				return "Left Alt";
			case 0xA5:
				return "Right Alt";
			case 0xBA:
				return ";";
			case 0xBB:
				return "=";
			case 0xBC:
				return ",";
			case 0xBD:
				return "-";
			case 0xBE:
				return ".";
			case 0xBF:
				return "/";
			case 0xC0:
				return "`";
			case 0xDB:
				return "[";
			case 0xDC:
				return "\\";
			case 0xDD:
				return "]";
			case 0xDE:
				return "'";
			case 0xDF:
				return "OEM_8";
			default:
				return "Unknown";
			}
		}

		std::string ToReadableString() const
		{
			std::string hotkeyStr;

			if (ctrl)
				hotkeyStr += "Ctrl + ";
			if (shift)
				hotkeyStr += "Shift + ";
			if (alt)
				hotkeyStr += "Alt + ";

			hotkeyStr += (mainKey != 0) ? GetKeyName(mainKey) : "None";

			return hotkeyStr;
		}

		static void SaveHotkeyToIni();
		static void CaptureHotkey(Hotkey& hotkey);
		static void LoadHotkeyFromIni();

		// Deserialize from ini format
		static Hotkey FromString(const std::string& str)
		{
			Hotkey hotkey;
			std::stringstream ss(str);
			char delimiter;

			// Parse the mainKey, ctrl, shift, and alt from the string
			ss >> hotkey.mainKey >> delimiter >> hotkey.ctrl >> delimiter >> hotkey.shift >> delimiter >> hotkey.alt;

			return hotkey;
		}

		// Check if a hotkey matches the current key state
		bool MatchesCurrentState() const
		{
			return (GetAsyncKeyState(mainKey) & 0x8000) &&
			       (!ctrl || (GetAsyncKeyState(VK_CONTROL) & 0x8000)) &&
			       (!shift || (GetAsyncKeyState(VK_SHIFT) & 0x8000)) &&
			       (!alt || (GetAsyncKeyState(VK_MENU) & 0x8000));
		}
	};

	class Window
	{
	protected:
		static Window* instance;
		bool shouldDraw = false;
		void HelpTooltip(const char* a_desc);

	public:
		static bool imguiInitialized;
		static void ImGuiInit();
		Window() = default;
		Window(Window&) = delete;
		void operator=(const Window&) = delete;

		static Window* GetSingleton()
		{
			if (!instance)
				instance = new Window();
			return instance;
		}

		// toggles the ImGui window and the Windows cursor
		// also use it to reset data between toggles if needed
		void ToggleEditorUI();
		// handles the drawing of the ImGui interface
		void Draw();

		inline bool GetShouldDraw()
		{
			return shouldDraw;
		}
	};
}
