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

constexpr QSize trayIconSize = QSize(32, 32);

const struct {
	QString darkTheme = "dark-theme";
	QString opaqueTheme = "opaque";
	QString showInactive = "show-inactive";
} settingsKeys;

VolumeController::VolumeController(QWidget *parent, CustomStyle &style)
	: QWidget(parent),
	  windowFadeAnimation(this),
	  _style(style)
{
	settingsPath = QDir::cleanPath(QApplication::applicationDirPath() + QDir::separator() + "settings.ini");
	qDebug().nospace() << "Loading settings from " << settingsPath << ".";
	QSettings settings(settingsPath, QSettings::IniFormat);
	const auto darkTheme = settings.value(settingsKeys.darkTheme, false).toBool();
	const auto opaqueTheme = settings.value(settingsKeys.opaqueTheme, false).toBool();
	const auto showInactive = settings.value(settingsKeys.showInactive, false).toBool();

	const Theme &theme = SelectTheme(darkTheme, opaqueTheme);
	setBaseTheme(theme.base());
	setStyleTheme(theme);
	trayVolumeIcons = VolumeIcons(trayIconSize, theme.icon());

	QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
	sizePolicy1.setHorizontalStretch(0);
	sizePolicy1.setVerticalStretch(0);
	sizePolicy1.setHeightForWidth(sizePolicy().hasHeightForWidth());
	setSizePolicy(sizePolicy1);

	qDebug() << "Creating audio device manager";
	auto optManager = AudioDeviceManager::Default();
	Q_ASSERT(optManager.has_value());

	setMinimumWidth(320);
	QGridLayout *layout = new QGridLayout(this);
	layout->setSizeConstraint(QLayout::SetDefaultConstraint);
	layout->setAlignment(Qt::AlignTop);
	layout->setContentsMargins(0, 0, 0, 0);

	deviceVolumeController = new DeviceVolumeController(this, std::move(*optManager), theme.device(), showInactive);
	layout->addWidget(deviceVolumeController, 0, 0);

	createActions(showInactive, darkTheme, opaqueTheme);
	createTray();
	trayIcon->show();
}

VolumeController::~VolumeController() {
	saveSettings();
	qDebug() << "Destroying.";
}

void VolumeController::createActions(bool showInactiveInitial, bool darkThemeInitial, bool opaqueInitial) {
	qDebug() << "Creating actions";
	showAction = new QAction(tr("Show"), this);
	connect(showAction, &QAction::triggered, this, &VolumeController::fadeIn);

	showInactiveAction = new QAction(tr("Show inactive"), this);
	showInactiveAction->setCheckable(true);
	showInactiveAction->setChecked(showInactiveInitial);
	connect(showInactiveAction, &QAction::toggled, this, &VolumeController::setShowInactive);

	toggleDarkThemeAction = new QAction(tr("Dark theme"), this);
	toggleDarkThemeAction->setCheckable(true);
	toggleDarkThemeAction->setChecked(darkThemeInitial);
	connect(toggleDarkThemeAction, &QAction::toggled, this, &VolumeController::setDarkTheme);

	toggleOpaqueAction = new QAction(tr("Opaque"), this);
	toggleOpaqueAction->setCheckable(true);
	toggleOpaqueAction->setChecked(opaqueInitial);
	connect(toggleOpaqueAction, &QAction::toggled, this, &VolumeController::setOpaqueTheme);

	exitAction = new QAction(tr("Exit"), this);
	connect(exitAction, &QAction::triggered, this, &VolumeController::close);
}

void VolumeController::createTray() {
	qDebug() << "Creating tray";
	trayMenu = new QMenu(this);

	trayMenu->addAction(showAction);
	trayMenu->addSeparator();
	trayMenu->addAction(showInactiveAction);
	trayMenu->addAction(toggleDarkThemeAction);
	trayMenu->addAction(toggleOpaqueAction);
	trayMenu->addSeparator();
	trayMenu->addAction(exitAction);

	trayIcon = new QSystemTrayIcon(this);
	trayIcon->setContextMenu(trayMenu);
	updateTray();

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

void VolumeController::saveSettings() {
	qDebug() << "Saving to " << settingsPath << ".";
	QSettings settings(settingsPath, QSettings::IniFormat);
	settings.setValue(settingsKeys.darkTheme, toggleDarkThemeAction->isChecked());
	settings.setValue(settingsKeys.opaqueTheme, toggleOpaqueAction->isChecked());
	settings.setValue(settingsKeys.showInactive, deviceVolumeController->controlList().showInactive());
	settings.sync();
}

void VolumeController::setDarkTheme(bool value) {
	const bool dark = value;
	const bool opaque = toggleOpaqueAction->isChecked();
	changeTheme(SelectTheme(dark, opaque));
}

void VolumeController::setOpaqueTheme(bool value) {
	const bool dark = toggleDarkThemeAction->isChecked();
	const bool opaque = value;
	changeTheme(SelectTheme(dark, opaque));
}

void VolumeController::onDeviceVolumeChanged(const int volume) {
	qDebug() << "Device volume changed to" << volume;
	updateTray(volume);
}

void VolumeController::updateTray() {
	const auto volume = deviceVolumeController->deviceControl().volume().value_or(0.0f) * 100.0f;
	updateTray(volume);
}

const QString trayToolTipPattern("%1: %2%");
void VolumeController::updateTray(const int volume) {
	trayIcon->setIcon(trayVolumeIcons.selectIcon(volume));
	trayIcon->setToolTip(trayToolTipPattern.arg(deviceVolumeController->deviceName(), QString::number(volume)));
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
	reposition();
}

void VolumeController::setShowInactive(bool value) {
	deviceVolumeController->controlList().setShowInactive(value);
}

void VolumeController::changeTheme(const Theme &theme) {
	setBaseTheme(theme.base());
	setStyleTheme(theme);
	trayVolumeIcons = VolumeIcons(trayIconSize, theme.icon());
	updateTray();
	deviceVolumeController->changeTheme(theme.device());
}

void VolumeController::setBaseTheme(const BaseTheme &theme){
	auto p = palette();
	p.setColor(QPalette::Window, theme.background);
	p.setColor(QPalette::WindowText, theme.text);
	p.setColor(QPalette::Light, theme.light);
	p.setColor(QPalette::Dark, theme.dark);
	setPalette(p);
}

void VolumeController::setStyleTheme(const Theme &theme){
	_style.buttonTheme = theme.button();
	_style.sliderTheme = theme.slider();
}

void VolumeController::reposition() {
	const auto rect = QApplication::primaryScreen()->availableGeometry();
	move(rect.width() - width(), rect.height() - height());
}
