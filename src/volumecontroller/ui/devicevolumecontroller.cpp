#include "devicevolumecontroller.h"

#include <QTimer>
#include <QDebug>

DeviceVolumeController::DeviceVolumeController(QWidget *parent, AudioDeviceManager &&m)
	: QWidget(parent),
	  manager(std::move(m)),
	  gridLayout(this),
	  volumeIcons(QSize(32, 32), VolumeIcons::Gray, VolumeIcons::LightGray)
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

	auto optDeviceControl = manager.createDeviceControl();
	Q_ASSERT(optDeviceControl.has_value());
	_deviceControl = std::move(*optDeviceControl);

	auto optSessionGroups = manager.createSessionGroups();
	Q_ASSERT(optSessionGroups.has_value());
	sessionGroups = std::move(*optSessionGroups);

	audioSessionNotification = ComPtr<AudioSessionNotification>(new AudioSessionNotification(this));
	connect(audioSessionNotification.get(), &AudioSessionNotification::sessionCreated,
			  this, &DeviceVolumeController::addSession, Qt::ConnectionType::QueuedConnection);

	manager.manager().RegisterSessionNotification(audioSessionNotification.get());


	createDeviceItem();
	VolumeControlList::addItem(gridLayout, *deviceItem, 0);

	createLineSeperator();
	gridLayout.addWidget(separator, 1, 0, 1, 3);

	_controlList = new VolumeControlList(this, this->sessionGroups);
	gridLayout.addWidget(_controlList, 2, 0, 1, 3);

	QTimer *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, [this]() {
		controlList().updatePeaks();
		deviceItem->updatePeak();
	});
	timer->start(15);
}

DeviceVolumeController::~DeviceVolumeController() {
	manager.manager().UnregisterSessionNotification(audioSessionNotification.get());
}

void DeviceVolumeController::resizeEvent(QResizeEvent *) {
//	qDebug() << "Resize DeviceVolumeController";
	parentWidget()->adjustSize();
}

void DeviceVolumeController::addSession(AudioSession *sessionPtr) {
	controlList().addSession(std::unique_ptr<AudioSession>(sessionPtr));
}

static const QIcon &GetVolumeIcon(const VolumeIcons &volumeIcons, const IAudioControl &control) {
	return volumeIcons.selectIcon(control.volume().value_or(0.0f) * 100.0f);
}

void DeviceVolumeController::createDeviceItem() {
	deviceItem = std::make_unique<DeviceVolumeItem>(this, _deviceControl, volumeIcons);
	deviceItem->setIcon(GetVolumeIcon(volumeIcons, _deviceControl));
}

void DeviceVolumeController::createLineSeperator() {
	separator = new QFrame(this);
	separator->setFrameShape(QFrame::HLine);
	separator->setFrameShadow(QFrame::Sunken);
}
