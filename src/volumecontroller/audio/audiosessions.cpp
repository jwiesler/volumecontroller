#include "volumecontroller/audio/audiosessions.h"
#include "volumecontroller/hresulterrors.h"
#include <algorithm>
#include <QDebug>

class AudioSessionEvents : public IAudioSessionEvents {
	LONG _cRef;
	AudioSession &session;

	bool isApplicationEvent(LPCGUID context) {
		return context != nullptr && *context == session.eventContext();
	}

public:
	AudioSessionEvents(AudioSession &session) : _cRef(1), session(session) {}

	~AudioSessionEvents() {}

	// IUnknown methods -- AddRef, Release, and QueryInterface

	ULONG STDMETHODCALLTYPE AddRef() { return InterlockedIncrement(&_cRef); }

	ULONG STDMETHODCALLTYPE Release() {
		ULONG ulRef = InterlockedDecrement(&_cRef);
		if (0 == ulRef)
			delete this;
		return ulRef;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface) {
		if (IID_IUnknown == riid) {
			AddRef();
			*ppvInterface = (IUnknown *)this;
		} else if (__uuidof(IAudioSessionEvents) == riid) {
			AddRef();
			*ppvInterface = (IAudioSessionEvents *)this;
		} else {
			*ppvInterface = NULL;
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	// Notification methods for audio session events

	HRESULT STDMETHODCALLTYPE OnDisplayNameChanged(LPCWSTR NewDisplayName,
																  LPCGUID EventContext) {
		if(isApplicationEvent(EventContext))
			return S_OK;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnIconPathChanged(LPCWSTR NewIconPath,
															  LPCGUID EventContext) {
		if(isApplicationEvent(EventContext))
			return S_OK;

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnSimpleVolumeChanged(float NewVolume, BOOL NewMute,
																	LPCGUID EventContext) {
		if(isApplicationEvent(EventContext))
			return S_OK;
		if (NewMute) {
			printf("MUTE\n");
		} else {
			printf("Volume = %d percent\n", (UINT32)(100 * NewVolume + 0.5));
		}

		session.onVolumeChangedEvent(NewVolume, NewMute);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE
	OnChannelVolumeChanged(DWORD ChannelCount, float NewChannelVolumeArray[],
								  DWORD ChangedChannel, LPCGUID EventContext) {
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnGroupingParamChanged(LPCGUID NewGroupingParam,
																	 LPCGUID EventContext) {
		if(isApplicationEvent(EventContext))
			return S_OK;
		session.onGroupingParamChangedEvent(NewGroupingParam);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnStateChanged(AudioSessionState NewState) {
		switch (NewState) {
		case AudioSessionStateActive:
			break;
		case AudioSessionStateInactive:
			break;
		case AudioSessionStateExpired:
			break;
		}

		session.onStateChangedEvent(NewState);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE
	OnSessionDisconnected(AudioSessionDisconnectReason DisconnectReason) {
		auto pszReason = "?????";

		switch (DisconnectReason) {
		case DisconnectReasonDeviceRemoval:
			pszReason = "device removed";
			break;
		case DisconnectReasonServerShutdown:
			pszReason = "server shut down";
			break;
		case DisconnectReasonFormatChanged:
			pszReason = "format changed";
			break;
		case DisconnectReasonSessionLogoff:
			pszReason = "user logged off";
			break;
		case DisconnectReasonSessionDisconnected:
			pszReason = "session disconnected";
			break;
		case DisconnectReasonExclusiveModeOverride:
			pszReason = "exclusive-mode override";
			break;
		}
		qDebug() << QString("Audio session disconnected :") << QString(pszReason);

		return S_OK;
	}
};

GUID CreateGuid() {
	GUID guid;
	CoCreateGuid(&guid);
	return guid;
}

AudioSession::AudioSession(ComPtr<IAudioSessionControl2> &&ctrl, ComPtr<ISimpleAudioVolume> &&vol, ComPtr<IAudioMeterInformation> &&audioMeterInfo)
	: _eventContext(CreateGuid()),
	  _sessionControl(std::move(ctrl)),
	  volumeControl(std::move(vol))
	, audioMeterInfo(std::move(audioMeterInfo)), sessionEvents(new AudioSessionEvents(*this))
{
	_sessionControl->RegisterAudioSessionNotification(sessionEvents.get());
}

AudioSession::~AudioSession()
{
	_sessionControl->UnregisterAudioSessionNotification(sessionEvents.get());
}

std::optional<float> AudioSession::volume() const {
	float vol;
	RET_EMPTY(volumeControl->GetMasterVolume(&vol));
	return vol;
}

bool AudioSession::setVolume(float v) {
	return SUCCEEDED(volumeControl->SetMasterVolume(v, &eventContext()));
}

std::optional<bool> AudioSession::muted() const {
	BOOL muted;
	RET_EMPTY(volumeControl->GetMute(&muted));
	return muted == TRUE;
}

bool AudioSession::setMuted(bool muted) {
	return SUCCEEDED(volumeControl->SetMute(muted, &eventContext()));
}

std::optional<AudioSessionState> AudioSession::state() const {
	AudioSessionState state;
	RET_EMPTY(_sessionControl->GetState(&state));
	return state;
}

bool AudioSession::isSystemSound() const {
	return _sessionControl->IsSystemSoundsSession() == S_OK;
}

void AudioSession::onVolumeChangedEvent(float newVolume, bool newMute)
{
	emit volumeChanged(newVolume, newMute);
}

void AudioSession::onStateChangedEvent(AudioSessionState newState)
{
	emit stateChanged(newState);
}

void AudioSession::onGroupingParamChangedEvent(const GUID *newGroupingParam)
{
	emit groupingParamChanged(newGroupingParam);
}

std::optional<float> AudioSession::peakValue() const
{
	float value;
	RET_EMPTY(audioMeterInfo->GetPeakValue(&value));
	return std ::min(value, 1.0f);
}

void AudioSessionGroup::insert(std::unique_ptr<AudioSession> &&session) {
	members.emplace_back(std::move(session));
}

std::optional<float> AudioSessionGroup::volume() const {
	return members.empty() ? std::optional<float>(1.0f) : members.front()->volume();
}

bool AudioSessionGroup::setVolume(float v) {
	return std::find_if_not(members.begin(), members.end(), [=](std::unique_ptr<AudioSession> &member) {
		return member->setVolume(v);
	}) == members.end();
}

bool AudioSessionGroup::isSystemSound() const {
	return std::any_of(members.begin(), members.end(), [](const std::unique_ptr<AudioSession> &session){
		return session->isSystemSound();
	});
}

std::vector<std::unique_ptr<AudioSessionGroup>>::iterator AudioSessionPidGroup::findGroup(const GUID &guid) {
	return std::find_if(groups.begin(), groups.end(), [&](const std::unique_ptr<AudioSessionGroup> &g) {
		return guid == g->groupingGuid;
	});
}

AudioSessionGroup &AudioSessionPidGroup::findGroupOrCreate(const GUID &guid) {
	auto it = findGroup(guid);
	return it == groups.end() ? *groups.emplace_back(std::make_unique<AudioSessionGroup>(guid)) : **it;
}

void AudioSessionPidGroup::insert(std::unique_ptr<AudioSession> &&session, const GUID &guid) {
	auto &group = findGroupOrCreate(guid);
	group.insert(std::move(session));
}

std::optional<float> AudioSessionPidGroup::volume() const {
	return groups.empty() ? std::optional<float>(1.0f) : groups.front()->volume();
}

bool AudioSessionPidGroup::setVolume(float v) {
	return std::find_if_not(groups.begin(), groups.end(), [=](std::unique_ptr<AudioSessionGroup> &group) {
		return group->setVolume(v);
	}) == groups.end();
}

bool AudioSessionPidGroup::isSystemSound() const {
	return std::any_of(groups.begin(), groups.end(), [](const std::unique_ptr<AudioSessionGroup> &group){ return group->isSystemSound(); });
}

std::vector<std::unique_ptr<AudioSessionPidGroup>>::iterator AudioSessionGroups::findPidGroup(DWORD pid) {
	return std::find_if(groups.begin(), groups.end(), [&](const std::unique_ptr<AudioSessionPidGroup> &g) {
		return pid == g->pid;
	});
}

AudioSessionPidGroup &AudioSessionGroups::findPidGroupOrCreate(DWORD pid) {
	auto it = findPidGroup(pid);
	return it == groups.end() ? *groups.emplace_back(std::make_unique<AudioSessionPidGroup>(pid)) : **it;
}

void AudioSessionGroups::insert(std::unique_ptr<AudioSession> &&session, DWORD pid, const GUID &guid) {
	auto &group = findPidGroupOrCreate(pid);
	group.insert(std::move(session), guid);
}

DeviceAudioControl::DeviceAudioControl(ComPtr<IAudioEndpointVolume> &&vol, ComPtr<IAudioMeterInformation> &&audioMeterInfo)
	: volumeControl(std::move(vol)),
	  audioMeterInfo(std::move(audioMeterInfo)) {}

std::optional<float> DeviceAudioControl::volume() const
{
	float vol;
	RET_EMPTY(volumeControl->GetMasterVolumeLevelScalar(&vol));
	return vol;
}

bool DeviceAudioControl::setVolume(float v)
{
	return SUCCEEDED(volumeControl->SetMasterVolumeLevelScalar(v, nullptr));
}

std::optional<bool> DeviceAudioControl::muted() const
{
	BOOL muted;
	RET_EMPTY(volumeControl->GetMute(&muted));
	return muted;
}

bool DeviceAudioControl::setMuted(bool muted)
{
	return SUCCEEDED(volumeControl->SetMute(muted, nullptr));
}

std::optional<float> DeviceAudioControl::peakValue() const
{
	float peak;
	RET_EMPTY(audioMeterInfo->GetPeakValue(&peak));
	return peak;
}

AudioSessionNotification::AudioSessionNotification(QObject *parent) : QObject(parent) {}

ULONG AudioSessionNotification::AddRef() { return InterlockedIncrement(&_cRef); }

ULONG AudioSessionNotification::Release() {
	ULONG ulRef = InterlockedDecrement(&_cRef);
	if (0 == ulRef)
		delete this;
	return ulRef;
}

HRESULT AudioSessionNotification::QueryInterface(const IID &riid, void **ppvInterface) {
	if (IID_IUnknown == riid) {
		AddRef();
		*ppvInterface = static_cast<IUnknown *>(this);
	} else if (__uuidof(IAudioSessionNotification) == riid) {
		AddRef();
		*ppvInterface = static_cast<IAudioSessionNotification *>(this);
	} else {
		*ppvInterface = NULL;
		return E_NOINTERFACE;
	}
	return S_OK;
}

#include "audiodevicemanager.h"

HRESULT AudioSessionNotification::OnSessionCreated(IAudioSessionControl *NewSession) {
	auto session = CreateSession(NewSession);
	emit sessionCreated(session.release());
	return S_OK;
}
