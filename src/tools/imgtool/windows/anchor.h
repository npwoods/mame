//============================================================
//
//  anchor.h - Component to implement anchoring
//
//============================================================

#ifndef MAME_TOOLS_IMGTOOL_WINDOWS_ANCHOR
#define MAME_TOOLS_IMGTOOL_WINDOWS_ANCHOR

#include <windows.h>
#include <stdint.h>
#include <vector>

class anchor_manager
{
public:
	enum class anchor : uint8_t
	{
		LEFT	= 0x01,
		TOP		= 0x02,
		RIGHT	= 0x04,
		BOTTOM	= 0x08
	};

	struct entry
	{
		uint16_t control;
		anchor_manager::anchor anchor;
	};

	anchor_manager(std::vector<entry> &&entries);
	void setup(HWND window);
	void resize();

private:
	void get_dimensions(LONG &width, LONG &height);

	HWND				m_window;
	std::vector<entry>	m_entries;
	LONG				m_old_width;
	LONG				m_old_height;
};

inline constexpr anchor_manager::anchor operator|(anchor_manager::anchor a, anchor_manager::anchor b)
{
	return (anchor_manager::anchor) ((uint8_t)a | (uint8_t)b);
}

inline constexpr anchor_manager::anchor operator&(anchor_manager::anchor a, anchor_manager::anchor b)
{
	return (anchor_manager::anchor) ((uint8_t)a & (uint8_t)b);
}

#endif // MAME_TOOLS_IMGTOOL_WINDOWS_ANCHOR
