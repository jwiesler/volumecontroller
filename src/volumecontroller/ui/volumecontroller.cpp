#include "volumecontroller.h"
#include "./ui_volumecontroller.h"

#include "volumecontroller/hresulterrors.h"
#include "volumecontroller/info/processdata.h"

#include "volumecontroller/audio/audiodevicemanager.h"
#include <QTimer>
#include <QDebug>
#include <QScreen>
#include <QDir>
#include <QSettings>

VolumeController::VolumeController(QWidget *parent)
	: QWidget(parent),
	  windowFadeAnimation(this),
	  trayVolumeIcons(QSize(32, 32), QColor(Qt::white), VolumeIcons::LightGray)
{
	QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
	sizePolicy1.setHorizontalStretch(0);
	sizePolicy1.setVerticalStretch(0);
	sizePolicy1.setHeightForWidth(sizePolicy().hasHeightForWidth());
	setSizePolicy(sizePolicy1);

	auto optManager = AudioDeviceManager::Default();
	Q_ASSERT(optManager.has_value());

	setMinimumWidth(300);
	QGridLayout *layout = new QGridLayout(this);
	layout->setSizeConstraint(QLayout::SetDefaultConstraint);
	layout->setAlignment(Qt::AlignTop);
	layout->setContentsMargins(0, 0, 0, 0);

	deviceVolumeController = new DeviceVolumeController(this, std::move(*optManager));
	layout->addWidget(deviceVolumeController, 0, 0);

	createActions();

	settingsPath = QDir::cleanPath(QApplication::applicationDirPath() + QDir::separator() + "settings.ini");
	loadSettings();

	createTray();
	trayIcon->show();
}

VolumeController::~VolumeController() {
	saveSettings();
}

void VolumeController::createActions() {
	showAction = new QAction("Show", this);
	connect(showAction, &QAction::triggered, this, &VolumeController::fadeIn);

	showInactiveAction = new QAction("Show inactive", this);
	showInactiveAction->setCheckable(true);
	connect(showInactiveAction, &QAction::toggled, this, &VolumeController::setShowInactive);

	exitAction = new QAction("Exit", this);
	connect(exitAction, &QAction::triggered, this, &VolumeController::close);
}

void VolumeController::createTray() {
	trayMenu = new QMenu(this);

	trayMenu->addAction(showAction);
	trayMenu->addSeparator();
	trayMenu->addAction(showInactiveAction);
	trayMenu->addSeparator();
	trayMenu->addAction(exitAction);

	trayIcon = new QSystemTrayIcon(this);
	trayIcon->setIcon(trayVolumeIcons.selectIcon(deviceVolumeController->deviceControl().volume().value_or(0.0f) * 100.0f));
	trayIcon->setContextMenu(trayMenu);
	trayIcon->setToolTip("Volume Controller");

	connect(trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
		if(reason == QSystemTrayIcon::ActivationReason::Trigger) {
			if(isVisible())
				fadeOut();
			else
				fadeIn();
		}
	});

	connect(&deviceVolumeController->deviceVolumeItem(), &DeviceVolumeItem::volumeChanged, this, &VolumeController::onDeviceVolumeChanged);
}

void VolumeController::loadSettings() {
	QSettings settings(settingsPath, QSettings::IniFormat);
	bool showInactive = settings.value("show-inactive", false).toBool();
	showInactiveAction->setChecked(showInactive);
}

void VolumeController::saveSettings() {
	QSettings settings(settingsPath, QSettings::IniFormat);
	settings.setValue("show-inactive", deviceVolumeController->controlList().showInactive());
	settings.sync();
}

void VolumeController::onDeviceVolumeChanged(const int volume) {
	trayIcon->setIcon(trayVolumeIcons.selectIcon(volume));
}

void VolumeController::onApplicationInactive(const QWidget *activeWindow) {
	if(activeWindow == this)
		fadeOut();
}

void VolumeController::showEvent(QShowEvent *) {
	windowFadeAnimation.finishAnimation();
}

void VolumeController::hideEvent(QHideEvent *) {
	windowFadeAnimation.finishAnimation();
}

void VolumeController::fadeOut() {
	windowFadeAnimation.fadeOut();
}

void VolumeController::fadeIn() {
	windowFadeAnimation.fadeIn();
	activateWindow();
}

void VolumeController::resizeEvent(QResizeEvent *) {
//	adjustSize();
//	qDebug() << "Resize VolumeController";
	reposition();
}

void VolumeController::setShowInactive(bool value) {
	deviceVolumeController->controlList().setShowInactive(value);
}

void VolumeController::reposition() {
	const auto rect = QApplication::primaryScreen()->availableGeometry();
	move(rect.width() - width(), rect.height() - height());
}