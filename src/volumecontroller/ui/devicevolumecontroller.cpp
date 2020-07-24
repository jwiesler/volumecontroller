#include "devicevolumecontroller.h"

#include <QTimer>
#include <QDebug>

constexpr QSize deviceVolumeIconSize = QSize(32, 32);

DeviceVolumeController::DeviceVolumeController(QWidget *parent, AudioDeviceManager &&m, const DeviceVolumeControllerTheme &theme, bool showInactive)
	: QWidget(parent),
	  manager(std::move(m)),
	  gridLayout(this),
	  volumeIcons(deviceVolumeIconSize, theme.icon())
{
	setObjectName(QString::fromUtf8("DeviceVolumeController"));
	QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
	sizePolicy1.setHorizontalStretch(0);
	sizePolicy1.setVerticalStretch(0);
	sizePolicy1.setHeightForWidth(sizePolicy().hasHeightForWidth());
	setSizePolicy(sizePolicy1);
	setAutoFillBackground(true);
//	auto p = palette();
//	p.setColor(QPalette::Window, QColor(Qt::green));
//	setPalette(p);

	gridLayout.setSpacing(12);
	gridLayout.setSizeConstraint(QLayout::SetDefaultConstraint);
	gridLayout.setContentsMargins(12, 12, 12, 12);
	gridLayout.setAlignment(Qt::AlignTop);

	qDebug() << "Creating device control.";
	auto deviceControlPtr = manager.createDeviceControl();
	Q_ASSERT(deviceControlPtr);
	_deviceControl = std::move(deviceControlPtr);

	qDebug() << "Creating session groups.";
	auto optSessionGroups = manager.createSessionGroups();
	Q_ASSERT(optSessionGroups.has_value());
	sessionGroups = std::move(*optSessionGroups);

	_deviceName = manager.device().name().value_or("Lautsprecher");

	qDebug() << "Creating device item.";
	createDeviceItem(theme.volumeItem());
	VolumeControlList::addItem(gridLayout, *deviceItem, 0);

	createLineSeperator();
	gridLayout.addWidget(separator, 1, 0, 1, 3);

	qDebug() << "Creating VolumeControlList.";
	_controlList = new VolumeControlList(this, this->sessionGroups, theme.volumeItem(), showInactive);
	gridLayout.addWidget(_controlList, 2, 0, 1, 3);

	qDebug() << "Starting peak update timer.";
	QTimer *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, [this]() {
		controlList().updatePeaks();
		deviceItem->updatePeak();
	});
	timer->start(15);

	qDebug() << "Start listening on audio session notifications.";
	audioSessionNotification = ComPtr<AudioSessionNotification>(new AudioSessionNotification(this));
	connect(audioSessionNotification.get(), &AudioSessionNotification::sessionCreated,
			  this, &DeviceVolumeController::addSession, Qt::ConnectionType::QueuedConnection);
	manager.manager().RegisterSessionNotification(audioSessionNotification.get());
}

DeviceVolumeController::~DeviceVolumeController() {
	manager.manager().UnregisterSessionNotification(audioSessionNotification.get());
}

void DeviceVolumeController::resizeEvent(QResizeEvent *) {
//	qDebug() << "Resize DeviceVolumeController";
	parentWidget()->adjustSize();
}

void DeviceVolumeController::changeTheme(const DeviceVolumeControllerTheme &theme) {
	volumeIcons = VolumeIcons(deviceVolumeIconSize, theme.icon());
	deviceItem->updateThemeAndIcon(theme.volumeItem());
	controlList().changeTheme(theme.volumeItem());
}

void DeviceVolumeController::addSession(AudioSession *sessionPtr) {
	controlList().addSession(std::unique_ptr<AudioSession>(sessionPtr));
}

static const QIcon &GetVolumeIcon(const VolumeIcons &volumeIcons, const IAudioControl &control) {
	return volumeIcons.selectIcon(control.volume().value_or(0.0f) * 100.0f);
}

void DeviceVolumeController::createDeviceItem(const VolumeItemTheme &theme) {
	deviceItem = std::make_unique<DeviceVolumeItem>(this, deviceControl(), volumeIcons, deviceName(), theme);
	connect(&deviceControl(), &DeviceAudioControl::volumeChanged, deviceItem.get(), &DeviceVolumeItem::setVolumeFAndMute, Qt::ConnectionType::QueuedConnection);
}

void DeviceVolumeController::createLineSeperator() {
	separator = new QFrame(this);
	separator->setFrameShape(QFrame::HLine);
	separator->setFrameShadow(QFrame::Sunken);
}
