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

QT_BEGIN_NAMESPACE
namespace Ui { class VolumeController; }
QT_END_NAMESPACE



class VolumeController : public QWidget {
	Q_OBJECT

public:
	VolumeController(QWidget *parent = nullptr);

	void onApplicationInactive(const QWidget *activeWindow);

	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;

	void fadeOut();
	void fadeIn();

	void resizeEvent(QResizeEvent *event) override;

private:
	void createActions();
	void createAnimations();
	void createTray();

	void onDeviceVolumeChanged(int volume);

	void reposition();

public:
	DeviceVolumeController *deviceVolumeController = nullptr;
private:
	WindowFadeAnimation windowFadeAnimation;

	QMenu * trayMenu = nullptr;
	QSystemTrayIcon *trayIcon = nullptr;
	QAction *showAction = nullptr;
	QAction *hideAction = nullptr;
	QAction *exitAction = nullptr;
	VolumeIcons trayVolumeIcons;
};
#endif // VOLUMECONTROLLER_H
