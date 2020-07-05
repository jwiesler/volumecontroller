#include "volumeicons.h"
#include <QPainter>

const QChar VolumeIcons::Mute = L'\uE74F';
const QChar VolumeIcons::Volume1 = L'\uE993';
const QChar VolumeIcons::Volume2 = L'\uE994';
const QChar VolumeIcons::Volume3 = L'\uE995';

const QFont VolumeIcons::IconFont = QFont("Segoe MDL2 Assets", 22);

const QColor VolumeIcons::Gray = QColor::fromRgb(55, 55, 55);
const QColor VolumeIcons::LightGray = QColor::fromRgb(127, 127, 127);

void DrawVolumeIcon(QPainter &painter, QChar c, const QRect &rect, QColor foreground, QColor background) {
	painter.setPen(background);
	painter.drawText(rect, Qt::AlignCenter, VolumeIcons::Volume3);
	painter.setPen(foreground);
	painter.drawText(rect, Qt::AlignCenter, c);
}

VolumeIcons::VolumeIcons(QSize size, QColor foreground, QColor background) {
	QImage image(size, QImage::Format::Format_RGBA8888);
	const auto rect = QRect(0, 0, size.width(), size.height());

	QPainter painter(&image);
	painter.setFont(IconFont);

	image.fill(0);
	painter.setPen(foreground);
	painter.drawText(rect, Qt::AlignCenter, Mute);
	icons[0] = new QIcon(QPixmap::fromImage(image));

	image.fill(0);
	DrawVolumeIcon(painter, Volume1, rect, foreground, background);
	icons[1] = new QIcon(QPixmap::fromImage(image));

	image.fill(0);
	DrawVolumeIcon(painter, Volume2, rect, foreground, background);
	icons[2] = new QIcon(QPixmap::fromImage(image));

	image.fill(0);
	painter.setPen(foreground);
	painter.drawText(rect, Qt::AlignCenter, Volume3);
	icons[3] = new QIcon(QPixmap::fromImage(image));
}

const QIcon &VolumeIcons::selectIcon(int volume) const {
	Q_ASSERT(0 <= volume && volume <= 100);
	size_t idx;
	if(volume == 0)
		idx = 0;
	else if(volume < 33)
		idx = 1;
	else if(volume < 66)
		idx = 2;
	else
		idx = 3;
	return *icons[idx];
}
