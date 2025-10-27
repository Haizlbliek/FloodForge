#pragma once

#include <string>
#include <unordered_map>

#include "math/Color.hpp"

enum class ThemeColor {
	Background,
	Grid,
	Header,
	Border,
	BorderHighlight,
	Popup,
	PopupHeader,
	Button,
	ButtonDisabled,
	Text,
	TextDisabled,
	TextHighlight,
	SelectionBorder,
	RoomBorder,
	RoomBorderHighlight,
	RoomAir,
	RoomSolid,
	RoomPole,
	RoomPlatform,
	RoomShortcutEnterance,
	RoomShortcutDot,
	RoomShortcutRoom,
	RoomShortcutDen,
	RoomConnection,
	RoomConnectionHover
};

extern std::unordered_map<ThemeColor, Color> currentTheme;

void loadTheme(std::string theme);

void setThemeColor(ThemeColor color);