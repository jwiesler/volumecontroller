#ifndef AUDIODEVICEMANAGER_H
#define AUDIODEVICEMANAGER_H
#include "volumecontroller/audio/audiosessions.h"
#include "volumecontroller/hresulterrors.h"

#include <optional>

template<typename T, typename From>
ComPtr<T> Activate(From *from) {
	T *ptr;
	RET_EMPTY(from->Activate(__uuidof(T),
									 CLSCTX_INPROC_SERVER, NULL,
									 (void**)&ptr));
	return ComPtr<T>(ptr);
}

class AudioDevice {
public:
	AudioDevice(ComPtr<IMMDevice> &&device) : device(std::move(device)) {}

	Q_DISABLE_COPY(AudioDevice);
	AudioDevice(AudioDevice &&) = default;
	AudioDevice &operator=(AudioDevice &&) = default;

	std::optional<QString> id();
	std::optional<QString> name();

	ComPtr<IAudioSessionManager2> manager();
	ComPtr<IAudioEndpointVolume> volume();
	ComPtr<IAudioMeterInformation> meter();

private:
	ComPtr<IMMDevice> device;
};

std::unique_ptr<AudioSession> CreateSession(IAudioSessionControl * ptr);
bool InsertIntoGroup(std::unique_ptr<AudioSession> &&session, AudioSessionGroups &groups);

class AudioDeviceManager
{
	AudioDevice _device;
	ComPtr<IAudioSessionManager2> _manager;
public:
	AudioDeviceManager(AudioDevice && device);

	static std::optional<AudioDeviceManager> Default(EDataFlow flow = EDataFlow::eRender, ERole role = ERole::eMultimedia);

	std::optional<AudioSessionGroups> createSessionGroups();

	std::optional<DeviceAudioControl> createDeviceControl();

	AudioDevice &device() { return _device; }

	IAudioSessionManager2 &manager() { return *_manager; }
};

#endif // AUDIODEVICEMANAGER_H
