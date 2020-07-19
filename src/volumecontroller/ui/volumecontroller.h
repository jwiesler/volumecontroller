#ifndef VOLUMECONTROLLER_H
#define VOLUMECONTROLLER_H

#include "volumecontrollist.h"
#include "volumelistitem.h"
#include "devicevolumecontroller.h"
#include "windowfadeanimation.h"
#include "customstyle.h"

#include <QSystemTrayIcon>
#include <QMenu>
#include <QPropertyAnimation>

#include <array>

#include "volumeicons.h"
#include "volumecontroller/ui/theme.h"

QT_BEGIN_NAMESPACE
namespace Ui { class VolumeController; }
QT_END_NAMESPACE



class VolumeController : public QWidget {
	Q_OBJECT

public:
	VolumeController(QWidget *parent, CustomStyle &style);
	~VolumeController();

	void onApplicationInactive(const QWidget *activeWindow);

	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;

	void fadeOut();
	void fadeIn();

	void resizeEvent(QResizeEvent *event) override;

	void setShowInactive(bool value);

	void changeTheme(const Theme &theme);

private:
	void setBaseTheme(const BaseTheme &theme);
	void setStyleTheme(const Theme &theme);

	void createActions(bool showInactiveInitial, bool darkThemeInitial, bool opaqueInitial);
	void createAnimations();
	void createTray();

	void saveSettings();

	void setDarkTheme(bool value);
	void setOpaqueTheme(bool value);

	void onDeviceVolumeChanged(int volume);
	void updateTray();
	void updateTray(int volume);

	void reposition();

	DeviceVolumeController *deviceVolumeController = nullptr;
	WindowFadeAnimation windowFadeAnimation;
	CustomStyle &_style;

	QMenu * trayMenu = nullptr;
	QSystemTrayIcon *trayIcon = nullptr;
	QAction *showAction = nullptr;
	QAction *showInactiveAction = nullptr;
	QAction *toggleOpaqueAction = nullptr;
	QAction *toggleDarkThemeAction = nullptr;
	QAction *exitAction = nullptr;
	VolumeIcons trayVolumeIcons;

	QString settingsPath;
};
#endif // VOLUMECONTROLLER_H
