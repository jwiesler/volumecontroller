#include "audiodevicemanager.h"
#include <QDebug>
#include <QString>
#include <Functiondiscoverykeys_devpkey.h>
#include <optional>
#include <Objbase.h>
#include "endpointvolume.h"

#include "volumecontroller/hresulterrors.h"

template<typename T>
struct ComMemoryRelease {
	void operator()(T *ptr) const {
		CoTaskMemFree(ptr);
	}
};

template<typename T>
using ComMemoryPtr = std::unique_ptr<T, ComMemoryRelease<T>>;

struct Prop : PROPVARIANT {
	Q_DISABLE_COPY_MOVE(Prop);

	Prop() {
		PropVariantInit(this);
	}

	~Prop() {
		PropVariantClear(this);
	}
};

template<typename F>
std::optional<QString> GetString(F && f) {
	ComMemoryPtr<WCHAR> value;
	GET_INTO_COMMEMORYPTR(WCHAR, value, ptr, RET_EMPTY(f(&ptr)));
	return QString::fromWCharArray(value.get());
}

template<typename Functor>
HRESULT Foreach(IMMDeviceCollection *collection, Functor && f) {
	UINT count;
	RET_FAILED(collection->GetCount(&count));

	for(UINT i = 0; i < count; ++i) {
		IMMDevice *value;
		RET_FAILED(collection->Item(i, &value));
		RET_FAILED(f(i, ComPtr<IMMDevice>(value)));
	}
	return S_OK;
}

template<typename Functor>
HRESULT Foreach(IAudioSessionEnumerator *collection, Functor && f) {
	int count;
	RET_FAILED(collection->GetCount(&count));

	for(int i = 0; i < count; ++i) {
		IAudioSessionControl *value;
		RET_FAILED(collection->GetSession(i, &value));
		RET_FAILED(f(i, ComPtr<IAudioSessionControl>(value)));
	}
	return S_OK;
}

AudioDeviceManager::AudioDeviceManager(AudioDevice &&dev) : _device(std::move(dev)), _manager(_device.manager()) {}

std::optional<AudioDeviceManager> AudioDeviceManager::Default(EDataFlow flow, ERole role)
{
	ComPtr<IMMDeviceEnumerator> enumerator;
	GET_INTO_COMPTR(IMMDeviceEnumerator, enumerator, pEnumerator, RET_EMPTY(CoCreateInstance(__uuidof(MMDeviceEnumerator),
																										  NULL, CLSCTX_INPROC_SERVER,
																										  __uuidof(IMMDeviceEnumerator),
																										  (void**)&pEnumerator)));

	ComPtr<IMMDevice> device;
	GET_INTO_COMPTR(IMMDevice, device, pDevice, RET_EMPTY(enumerator->GetDefaultAudioEndpoint(flow, role, &pDevice)));
	return AudioDeviceManager(AudioDevice(std::move(device)));

//	ComPtr<IMMDeviceCollection> devices;
//	GET_INTO_COMPTR(IMMDeviceCollection, devices, pDevices, RET_FAILED(enumerator->EnumAudioEndpoints(
//																								 eRender, DEVICE_STATE_ACTIVE,
//																								 &pDevices)));

//	RET_FAILED(Foreach(devices.get(), [](const UINT index, ComPtr<IMMDevice> ptr) {
//		Device device {std::move(ptr)};
//		const auto opId = device.id();
//		const auto opName = device.name();
//		if(!opId.has_value() || ! opName.has_value())
//			return S_FALSE;


//		qInfo().nospace() << "Endpoint " << index << ": " << *opName << " (" << *opId << ")";
//		return S_OK;
	//	}));
}

std::unique_ptr<AudioSession> CreateSession(IAudioSessionControl * ptr) {
	ComPtr<IAudioSessionControl2> control;
	GET_INTO_COMPTR(IAudioSessionControl2, control, pControl, RET_EMPTY(ptr->QueryInterface(&pControl)));

	ComPtr<ISimpleAudioVolume> volume;
	GET_INTO_COMPTR(ISimpleAudioVolume, volume, pVolume, RET_EMPTY(ptr->QueryInterface(&pVolume)));

	ComPtr<IAudioMeterInformation> meter;
	GET_INTO_COMPTR(IAudioMeterInformation, meter, pMeter, RET_EMPTY(ptr->QueryInterface(&pMeter)));

	return std::make_unique<AudioSession>(std::move(control), std::move(volume), std::move(meter));
}

bool InsertIntoGroup(std::unique_ptr<AudioSession> &&session, AudioSessionGroups &groups) {
	DWORD pid;
	if(FAILED(session->control().GetProcessId(&pid)))
		return false;

	GUID guid;
	if(FAILED(session->control().GetGroupingParam(&guid)))
		return false;

	groups.insert(std::move(session), pid, guid);
	return true;
}

std::optional<AudioSessionGroups> AudioDeviceManager::createSessionGroups()
{
	AudioSessionGroups groups;
	ComPtr<IAudioSessionEnumerator> sessions;
	GET_INTO_COMPTR(IAudioSessionEnumerator, sessions, pSessions, RET_EMPTY(_manager->GetSessionEnumerator(&pSessions)));

	RET_EMPTY(Foreach(sessions.get(), [&](UINT, ComPtr<IAudioSessionControl> ptr) {
		auto session = CreateSession(ptr.get());
		if(!session)
			return S_FALSE;
		if(!InsertIntoGroup(std::move(session), groups))
			return S_FALSE;
		return S_OK;
	}));

	return groups;
}

std::unique_ptr<DeviceAudioControl> AudioDeviceManager::createDeviceControl()
{
	auto volumeControl = _device.volume();
	if(!volumeControl)
		return {};
	auto meterInfo = _device.meter();
	if(!meterInfo)
		return {};

	return std::make_unique<DeviceAudioControl>(std::move(volumeControl), std::move(meterInfo));
}

std::optional<QString> AudioDevice::id() {
	return GetString([&](auto val) { return device->GetId(val); });
}

std::optional<QString> AudioDevice::name() {
	ComPtr<IPropertyStore> propertyStore;
	GET_INTO_COMPTR(IPropertyStore, propertyStore, pPropertyStore, RET_EMPTY(device->OpenPropertyStore(STGM_READ, &pPropertyStore))) ;
	// Get the endpoint's friendly-name property.
	Prop nameProp;
	RET_EMPTY(propertyStore->GetValue(PKEY_Device_FriendlyName, &nameProp));

	return QString::fromWCharArray(nameProp.pwszVal);
}

ComPtr<IAudioSessionManager2> AudioDevice::manager() {
	return Activate<IAudioSessionManager2>(device.get());
}

ComPtr<IAudioEndpointVolume> AudioDevice::volume()  {
	return Activate<IAudioEndpointVolume>(device.get());
}

ComPtr<IAudioMeterInformation> AudioDevice::meter() {
	return Activate<IAudioMeterInformation>(device.get());
}
