#include "programminformation.h"

#include <QString>
#include <QDebug>

#include "processdata.h"

ProgrammInformation::ProgrammInformation(QString title, std::optional<QIcon> icon) : _title(std::move(title)), _icon(std::move(icon)) {}

std::unique_ptr<ProgrammInformation> ProgrammInformation::forProcess(const unsigned long pid, const bool isSystemSound, const QSize imgSize)
{
	QString title;
	if(isSystemSound) {
		title = "Systemsounds";
	} else {
		auto strOpt = ProcessData::GetDisplayName(pid);
		if(strOpt.has_value()) {
			title = std::move(*strOpt);
		} else {
			HWND window = ProcessData::FindMainWindow(pid);
			title = ProcessData::GetWindowTitle(window);
		}
	}

	auto optImg = ProcessData::GetProcessImage(pid, imgSize.width(), imgSize.height());

	qDebug() << "ProgrammInformation for pid" << pid << "has title" << title << "and an icon:" << optImg.has_value();
	if(!optImg.has_value())
		return std::make_unique<ProgrammInformation>(title, std::optional<QPixmap>());

	auto img = optImg->toImage();
	img.convertTo(QImage::Format_RGBA8888);
	auto icon = QIcon(QPixmap::fromImage(std::move(img)));
	return std::make_unique<ProgrammInformation>(std::move(title), std::move(icon));
}
