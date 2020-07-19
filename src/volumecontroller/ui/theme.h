#ifndef THEME_H
#define THEME_H
#include <QColor>

constexpr QColor Gray = QColor(55, 55, 55);
constexpr QColor Gray2 = QColor(65,65,65);
constexpr QColor Gray3 = QColor(76,76,76);
constexpr QColor Gray4 = QColor(86,86,86);
constexpr QColor Gray5 = QColor(96,96,96);
constexpr QColor Gray6 = QColor(106,106,106);
constexpr QColor Gray7 = QColor(117,117,117);
constexpr QColor LightGray = QColor(127, 127, 127);
constexpr QColor White = QColor(255, 255, 255);
constexpr QColor Black = QColor(0, 0, 0);

constexpr QColor ColorFromHex(uint hex) {
	return QColor((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF);
}

struct SliderTheme {
	QColor slider;
	QColor sliderFocused;
	QColor sliderDisabled;
	QColor grooveBackground;
	QColor grooveBorder;
};

struct PushButtonTheme {
	QColor normal;
	QColor normalBorder;
	QColor hovered;
	QColor hoveredBorder;
	QColor down;
	QColor downBorder;
	QColor disabled;
	QColor disabledBorder;
};

constexpr SliderTheme DefaultSliderTheme {
	QColor(0, 122, 217), // slider
	QColor(23, 23, 23), // sliderFocused
	QColor(204, 204, 204), // sliderDisabled
	QColor(231, 234, 234), // grooveBackground
	QColor(214, 214, 214) // grooveBorder
};

constexpr SliderTheme DarkSliderTheme {
	QColor(0, 120, 215), // slider
	White, // sliderFocused
	QColor(204, 204, 204), // sliderDisabled
	LightGray, // grooveBackground
	Gray3 // grooveBorder
};

constexpr PushButtonTheme DefaultButtonTheme {
	ColorFromHex(0xe1e1e1), // normal
	ColorFromHex(0xadadad), // normalBorder
	ColorFromHex(0xe5f1fb), // hovered
	ColorFromHex(0x0078d7), // hoveredBorder
	ColorFromHex(0xcce4f7), // down
	ColorFromHex(0x005499), // downBorder
	Gray5, // disabled
	Gray6, // disabledBorder
};

constexpr PushButtonTheme DarkButtonTheme {
	Gray, // normal
	Gray2, // normalBorder
	Gray3, // hovered
	Gray2, // hoveredBorder
	Gray5, // down
	Gray3, // downBorder
	Gray6, // disabled
	Gray5, // disabledBorder
};

struct PeakSliderTheme {
	QColor peakMeter;
};

constexpr PeakSliderTheme DefaultVolumeSliderTheme {
	QColor(75, 191, 63) // peakMeter
};

constexpr PeakSliderTheme DarkVolumeSliderTheme {
	//QColor(58, 217, 42) // peakMeter
	ColorFromHex(0x38A9FF) // peakMeter
};


struct VolumeItemTheme {
	PeakSliderTheme slider;
};

constexpr VolumeItemTheme DefaultVolumeItemTheme {
	DefaultVolumeSliderTheme
};

constexpr VolumeItemTheme DarkVolumeItemTheme {
	DarkVolumeSliderTheme
};

struct IconTheme {
	QColor foreground;
	QColor background;
};

constexpr IconTheme DefaultTrayIconTheme {
	White,
	LightGray
};

constexpr IconTheme DefaultIconTheme {
	Gray,
	LightGray
};

constexpr IconTheme DarkIconTheme {
	QColor(214, 214, 214),
	LightGray
};

struct BaseTheme {
	QColor background;
	QColor text;
	QColor light;
	QColor dark;
};

constexpr BaseTheme DefaultBaseTheme {
	QColor(240, 240, 240), // background
	Black, // text
	White, // light
	QColor(160, 160, 160), // dark
};

constexpr BaseTheme DarkBaseTheme {
	QColor(55, 55, 55), // background
	White, // text
	Gray6, // light
	LightGray, // dark
};

constexpr BaseTheme DefaultOpaqueBaseTheme {
	QColor(240, 240, 240, 245), // background
	Black, // text
	White, // light
	QColor(160, 160, 160), // dark
};

constexpr BaseTheme DarkOpaqueBaseTheme {
	QColor(55, 55, 55, 245), // background
	White, // text
	Gray6, // light
	LightGray, // dark
};

struct DeviceVolumeControllerTheme {
	const IconTheme *_icon;
	const VolumeItemTheme *_volumeItem;

	constexpr DeviceVolumeControllerTheme(const IconTheme &i, const VolumeItemTheme &v) : _icon(&i), _volumeItem(&v) {}

	constexpr void setIcon(const IconTheme &theme) { _icon = &theme; }
	constexpr const IconTheme &icon() const { return *_icon; }

	constexpr void setVolumeItem(const VolumeItemTheme &theme) { _volumeItem = &theme; }
	constexpr const VolumeItemTheme &volumeItem() const { return *_volumeItem; }
};

class Theme {
	const BaseTheme *_base;
	const IconTheme *_icon;
	const SliderTheme *_slider;
	const PushButtonTheme *_button;
	DeviceVolumeControllerTheme _device;

public:
	constexpr Theme(
			const BaseTheme &base, const IconTheme &icon,
			const SliderTheme &sliderTheme, const PushButtonTheme &buttonTheme,
			DeviceVolumeControllerTheme device)
		: _base(&base), _icon(&icon), _slider(&sliderTheme), _button(&buttonTheme), _device(device) {}

	constexpr void setBase(const BaseTheme &theme) { _base = &theme; }
	constexpr const BaseTheme &base() const { return *_base; }

	constexpr void setIcon(const IconTheme &theme) { _icon = &theme; }
	constexpr const IconTheme &icon() const { return *_icon; }

	constexpr const SliderTheme &slider() const { return *_slider; }
	constexpr const PushButtonTheme &button() const { return *_button; }

	constexpr DeviceVolumeControllerTheme &device() { return _device; }
	constexpr const DeviceVolumeControllerTheme &device() const { return _device; }
};

constexpr Theme DefaultTheme {
	DefaultBaseTheme,
	DefaultTrayIconTheme,
	DefaultSliderTheme,
	DefaultButtonTheme,
	DeviceVolumeControllerTheme {
		DefaultIconTheme,
		DefaultVolumeItemTheme,
	}
};

constexpr Theme DefaultOpaqueTheme {
	DefaultOpaqueBaseTheme,
	DefaultTrayIconTheme,
	DefaultSliderTheme,
	DefaultButtonTheme,
	DeviceVolumeControllerTheme {
		DefaultIconTheme,
		DefaultVolumeItemTheme,
	}
};


constexpr Theme DarkTheme {
	DarkBaseTheme,
	DefaultTrayIconTheme,
	DarkSliderTheme,
	DarkButtonTheme,
	DeviceVolumeControllerTheme {
		DarkIconTheme,
		DarkVolumeItemTheme,
	}
};

constexpr Theme DarkOpaqueTheme {
	DarkOpaqueBaseTheme,
	DefaultTrayIconTheme,
	DarkSliderTheme,
	DarkButtonTheme,
	DeviceVolumeControllerTheme {
		DarkIconTheme,
		DarkVolumeItemTheme,
	}
};

constexpr const Theme &SelectTheme(bool dark, bool opaque) {
	if(dark)
		return opaque ? DarkOpaqueTheme : DarkTheme;
	return opaque ? DefaultOpaqueTheme : DefaultTheme;
}

#endif // THEME_H
