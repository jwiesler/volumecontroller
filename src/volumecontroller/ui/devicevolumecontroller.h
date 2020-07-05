#ifndef DEVICEVOLUMECONTROLLER_H
#define DEVICEVOLUMECONTROLLER_H

#include <QWidget>
#include <QGridLayout>
#include "volumecontroller/ui/volumecontrollist.h"
#include "volumecontroller/audio/audiodevicemanager.h"
#include "volumecontroller/ui/volumeicons.h"

class DeviceVolumeController : public QWidget
{
	Q_OBJECT

public:
	DeviceVolumeController(QWidget *parent, AudioDeviceManager &&m);
	~DeviceVolumeController();

	const DeviceAudioControl &deviceControl() const { return _deviceControl; }
	const DeviceVolumeItem &deviceVolumeItem() const { return *deviceItem; }
	DeviceVolumeItem &deviceVolumeItem() { return *deviceItem; }

	void resizeEvent(QResizeEvent *event) override;
private:
	void addSession(AudioSession* session);

	void createDeviceItem();
	void createLineSeperator();

	AudioDeviceManager manager;
	AudioSessionGroups sessionGroups;
	ComPtr<AudioSessionNotification> audioSessionNotification;
	VolumeControlList *controlList = nullptr;

	DeviceAudioControl _deviceControl;
	std::unique_ptr<DeviceVolumeItem> deviceItem;

	QGridLayout gridLayout;
	QFrame *separator = nullptr;

	VolumeIcons volumeIcons;
};

#endif // DEVICEVOLUMECONTROLLER_H
