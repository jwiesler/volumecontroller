#ifndef VOLUMECONTROLLER_H
#define VOLUMECONTROLLER_H

#include "volumecontrollist.h"
#include "volumelistitem.h"
#include "devicevolumecontroller.h"
#include "animations.h"
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

	void createActions(bool showInactiveInitial, bool darkThemeInitial, bool transparentInitial);
	void createAnimations();
	void createTray();

	void saveSettings();

	void setDarkTheme(bool value);
	void setTransparentTheme(bool value);

	void onDeviceVolumeChanged(int volume);
	void updateTray();
	void updateTray(int volume);

	void reposition();

	DeviceVolumeController *deviceVolumeController = nullptr;
	FadeAnimation windowFadeAnimation;
	FlyAnimation windowFlyAnimation;
	CustomStyle &_style;

	bool transparentTheme = false;
	qreal transparentThemeAlpha = qreal(1);

	QMenu * trayMenu = nullptr;
	QSystemTrayIcon *trayIcon = nullptr;
	QAction *showAction = nullptr;
	QAction *showInactiveAction = nullptr;
	QAction *toggleTransparentAction = nullptr;
	QAction *toggleDarkThemeAction = nullptr;
	QAction *exitAction = nullptr;
	VolumeIcons trayVolumeIcons;

	QString settingsPath;
};
#endif // VOLUMECONTROLLER_H
