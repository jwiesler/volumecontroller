#ifndef PROGRAMMINFORMATION_H
#define PROGRAMMINFORMATION_H

#include <optional>
#include <QIcon>

class ProgrammInformation
{
public:
	ProgrammInformation(QString title, std::optional<QIcon> icon);

	static std::unique_ptr<ProgrammInformation> forProcess(unsigned long pid, bool isSystemSound, QSize imgSize);

	const QString &title() const { return _title; }
	const std::optional<QIcon> &icon() const { return _icon; }

private:
	QString _title;
	std::optional<QIcon> _icon;
};

#endif // PROGRAMMINFORMATION_H
