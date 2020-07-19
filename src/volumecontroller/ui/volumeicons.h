#ifndef VOLUMEICONS_H
#define VOLUMEICONS_H
#include "theme.h"

#include <QIcon>
#include <QFont>
#include <array>

class VolumeIcons {
public:
	static const QFont IconFont;

	static const QChar Mute;
	static const QChar Volume1;
	static const QChar Volume2;
	static const QChar Volume3;

	static const QColor Gray;
	static const QColor LightGray;

	VolumeIcons() = default;
	VolumeIcons(QSize size, const IconTheme &theme);

	const QIcon &selectIcon(int volume) const;

private:
	std::array<std::unique_ptr<QIcon>, 4> icons {};
};

#endif // VOLUMEICONS_H
