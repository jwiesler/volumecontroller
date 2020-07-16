#ifndef DEVICEVOLUMECONTROLLER_H
#define DEVICEVOLUMECONTROLLER_H

#include <QWidget>
#include <QGridLayout>
#include "volumecontroller/ui/volumecontrollist.h"
#include "volumecontroller/audio/audiodevicemanager.h"
#include "volumecontroller/ui/volumeicons.h"
#include "volumecontroller/ui/theme.h"

class DeviceVolumeController : public QWidget
{
	Q_OBJECT

public:
	DeviceVolumeController(QWidget *parent, AudioDeviceManager &&m, const DeviceVolumeControllerTheme &theme);
	~DeviceVolumeController();

	DeviceAudioControl &deviceControl() { return *_deviceControl; }
	const DeviceAudioControl &deviceControl() const { return *_deviceControl; }

	DeviceVolumeItem &deviceVolumeItem() { return *deviceItem; }
	const DeviceVolumeItem &deviceVolumeItem() const { return *deviceItem; }

	void resizeEvent(QResizeEvent *event) override;

	VolumeControlList &controlList() { return *_controlList; }
	const VolumeControlList &controlList() const { return *_controlList; }

	AudioDeviceManager &deviceManager() { return manager; }

	const QString &deviceName() const { return _deviceName; }

private:
	void addSession(AudioSession* session);

	void createDeviceItem(const VolumeItemTheme &theme);
	void createLineSeperator();

	AudioDeviceManager manager;
	AudioSessionGroups sessionGroups;
	ComPtr<AudioSessionNotification> audioSessionNotification;
	VolumeControlList *_controlList = nullptr;

	std::unique_ptr<DeviceAudioControl> _deviceControl;
	std::unique_ptr<DeviceVolumeItem> deviceItem;

	QGridLayout gridLayout;
	QFrame *separator = nullptr;

	VolumeIcons volumeIcons;
	QString _deviceName;
};

#endif // DEVICEVOLUMECONTROLLER_H
