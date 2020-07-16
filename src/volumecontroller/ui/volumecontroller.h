#ifndef VOLUMECONTROLLER_H
#define VOLUMECONTROLLER_H

#include "volumecontrollist.h"
#include "volumelistitem.h"
#include "devicevolumecontroller.h"
#include "windowfadeanimation.h"

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
	VolumeController(QWidget *parent, const Theme &theme);
	~VolumeController();

	void onApplicationInactive(const QWidget *activeWindow);

	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;

	void fadeOut();
	void fadeIn();

	void resizeEvent(QResizeEvent *event) override;

	void setShowInactive(bool value);

private:
	void createActions();
	void createAnimations();
	void createTray();

	void loadSettings();
	void saveSettings();

	void onDeviceVolumeChanged(int volume);
	void updateTray(int volume);

	void reposition();

	DeviceVolumeController *deviceVolumeController = nullptr;
	WindowFadeAnimation windowFadeAnimation;

	QMenu * trayMenu = nullptr;
	QSystemTrayIcon *trayIcon = nullptr;
	QAction *showAction = nullptr;
	QAction *showInactiveAction = nullptr;
	QAction *exitAction = nullptr;
	VolumeIcons trayVolumeIcons;

	QString settingsPath;
};
#endif // VOLUMECONTROLLER_H
