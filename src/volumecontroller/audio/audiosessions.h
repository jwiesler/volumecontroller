#ifndef AUDIOSESSIONS_H
#define AUDIOSESSIONS_H
#include "volumecontroller/comptr.h"
#include "volumecontroller/info/programminformation.h"

#define NOMINMAX
#include <mmdeviceapi.h>
#include <vector>
#include <optional>
#include <QObject>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <optional>

class AudioSession;

template<typename Derived, typename Base>
class IUnknownBase : public Base {
	static_assert(std::is_base_of_v<IUnknown, Base>, "Base has to inherit from IUnknown");
	LONG _refCount;

protected:
	IUnknownBase() : _refCount(1) {}

	Base *basePtr() {
		return static_cast<Base *>(this);
	}

	Derived *derivedPtr() {
		return static_cast<Derived *>(this);
	}

public:
	ULONG STDMETHODCALLTYPE AddRef() override {
		return InterlockedIncrement(&_refCount);
	}

	ULONG STDMETHODCALLTYPE Release() override {
		ULONG ulRef = InterlockedDecrement(&_refCount);
		if (0 == ulRef)
			delete derivedPtr();
		return ulRef;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface) override {
		if (IID_IUnknown == riid) {
			AddRef();
			*ppvInterface = (IUnknown *)basePtr();
		} else if (__uuidof(Base) == riid) {
			AddRef();
			*ppvInterface = (Base *)basePtr();
		} else {
			*ppvInterface = nullptr;
			return E_NOINTERFACE;
		}
		return S_OK;
	}
};

class AudioSessionEvents final : public IUnknownBase<AudioSessionEvents, IAudioSessionEvents> {
	AudioSession &session;

	bool isApplicationEvent(LPCGUID context);

public:
	AudioSessionEvents(AudioSession &session) : session(session) {}

	// Notification methods for audio session events

	HRESULT STDMETHODCALLTYPE OnDisplayNameChanged(LPCWSTR NewDisplayName,
																  LPCGUID EventContext);

	HRESULT STDMETHODCALLTYPE OnIconPathChanged(LPCWSTR NewIconPath,
															  LPCGUID EventContext);

	HRESULT STDMETHODCALLTYPE OnSimpleVolumeChanged(float NewVolume, BOOL NewMute,
																	LPCGUID EventContext);

	HRESULT STDMETHODCALLTYPE
	OnChannelVolumeChanged(DWORD ChannelCount, float NewChannelVolumeArray[],
								  DWORD ChangedChannel, LPCGUID EventContext);

	HRESULT STDMETHODCALLTYPE OnGroupingParamChanged(LPCGUID NewGroupingParam,
																	 LPCGUID EventContext);

	HRESULT STDMETHODCALLTYPE OnStateChanged(AudioSessionState NewState);

	HRESULT STDMETHODCALLTYPE
	OnSessionDisconnected(AudioSessionDisconnectReason DisconnectReason);
};

class AudioSessionNotification final : public QObject, public IUnknownBase<AudioSessionNotification, IAudioSessionNotification> {
	Q_OBJECT

public:
	AudioSessionNotification(QObject *parent);

	HRESULT STDMETHODCALLTYPE OnSessionCreated(IAudioSessionControl *NewSession) override;

signals:
	void sessionCreated(AudioSession *NewSession);
};

class IAudioControl {
protected:
	IAudioControl() = default;

public:
	virtual ~IAudioControl() = default;

	virtual std::optional<float> volume() const = 0;
	virtual bool setVolume(float v) = 0;

	virtual std::optional<bool> muted() const = 0;
	virtual bool setMuted(bool muted) = 0;

	virtual std::optional<float> peakValue() const = 0;
};

class DeviceAudioControl final : public QObject, public IAudioControl {
	Q_OBJECT

public:
	friend class DeviceAudioEvents;

	DeviceAudioControl(ComPtr<IAudioEndpointVolume> &&vol, ComPtr<IAudioMeterInformation> &&audioMeterInfo);

	~DeviceAudioControl();

	std::optional<float> volume() const override;
	bool setVolume(float v) override;

	std::optional<bool> muted() const override;
	bool setMuted(bool muted) override;

	std::optional<float> peakValue() const override;

	const GUID &eventContext() const { return _eventContext; }

private:
	void onVolumeChangedEvent(float volume, bool muted);

signals:
	void volumeChanged(float volume, bool muted);

private:
	const GUID _eventContext;
	ComPtr<IAudioEndpointVolume> volumeControl;
	ComPtr<IAudioMeterInformation> audioMeterInfo;
	ComPtr<DeviceAudioEvents> volumeEvents;
	float volumeMindB;
	float volumeMaxdB;
	float volumeIncrementdB;
};

class DeviceAudioEvents final : public IUnknownBase<DeviceAudioEvents, IAudioEndpointVolumeCallback > {
	DeviceAudioControl &session;

	bool isApplicationEvent(LPCGUID context);

public:
	DeviceAudioEvents(DeviceAudioControl &session) : session(session) {}

	HRESULT STDMETHODCALLTYPE OnNotify(AUDIO_VOLUME_NOTIFICATION_DATA *pNotify) override;
};

class AudioSessionPidGroup;

class AudioSession final : public QObject, public IAudioControl {
	Q_OBJECT
public:
	enum class State {
		Inactive = 0,
		Active = 1,
		Expired = 2
	};

	friend class AudioSessionEvents;

	Q_DISABLE_COPY_MOVE(AudioSession);

	AudioSession(ComPtr<IAudioSessionControl2> &&ctrl, ComPtr<ISimpleAudioVolume> &&vol, ComPtr<IAudioMeterInformation> &&audioMeterInfo);

	~AudioSession();

	std::optional<float> volume() const override;
	bool setVolume(float v) override;

	std::optional<bool> muted() const override;
	bool setMuted(bool muted) override;

	std::optional<float> peakValue() const override;

	std::optional<State> state() const;

	bool isSystemSound() const;

	std::optional<DWORD> pid() const;

	const GUID &eventContext() const { return _eventContext; }

	IAudioSessionControl2 &control() { return *_sessionControl; }

	const AudioSessionPidGroup *parent() const { return _parent; }
	AudioSessionPidGroup *parent() { return _parent; }

private:
	void onVolumeChangedEvent(float newVolume, bool newMute);
	void onStateChangedEvent(AudioSessionState newState);
	void onGroupingParamChangedEvent(LPCGUID newGroupingParam);

signals:
	void volumeChanged(float newVolume, bool newMute);
	void stateChanged(int newState);
	void groupingParamChanged(const GUID *newGroupingParam);

private:
	const GUID _eventContext;
	AudioSessionPidGroup *_parent;
	ComPtr<IAudioSessionControl2> _sessionControl;
	ComPtr<ISimpleAudioVolume> volumeControl;
	ComPtr<IAudioMeterInformation> audioMeterInfo;
	ComPtr<IAudioSessionEvents> sessionEvents;
};

constexpr const char* ToString(AudioSession::State state) {
	switch(state) {
	case AudioSession::State::Inactive:
		return u8"Inactive";
	case AudioSession::State::Active:
		return u8"Active";
	case AudioSession::State::Expired:
		return u8"Expired";
	}
	return u8"Undefined";
}

class AudioSessionGroup {
public:
	Q_DISABLE_COPY_MOVE(AudioSessionGroup);

	AudioSessionGroup(const GUID &groupingGuid) : _groupingGuid(groupingGuid) {}

	void insert(std::unique_ptr<AudioSession> &&session);

	std::optional<float> volume() const;
	bool setVolume(float v);

	bool isSystemSound() const;

	const GUID &groupingGuid() const { return _groupingGuid; }

	const std::vector<std::unique_ptr<AudioSession>> &members() const { return _members; }
	std::vector<std::unique_ptr<AudioSession>> &members() { return _members; }

private:
	std::vector<std::unique_ptr<AudioSession>> _members;
	GUID _groupingGuid;
};

class AudioSessionPidGroup {
public:
	Q_DISABLE_COPY(AudioSessionPidGroup);

	AudioSessionPidGroup(AudioSessionPidGroup &&) = default;
	AudioSessionPidGroup &operator=(AudioSessionPidGroup &&) = default;

	AudioSessionPidGroup(DWORD pid) : _pid(pid) {}

	std::vector<std::unique_ptr<AudioSessionGroup>>::iterator findGroup(const GUID &guid);

	AudioSessionGroup &findGroupOrCreate(const GUID &guid);

	void insert(std::unique_ptr<AudioSession> &&session, const GUID &guid);
	void remove(AudioSession &session);

	std::optional<float> volume() const;
	bool setVolume(float v);

	bool isSystemSound() const;

	DWORD pid() const { return _pid; }

	ProgrammInformation* infoPtr() { return _info.get(); }
	const ProgrammInformation* infoPtr() const { return _info.get(); }

	void setInfoPtr(std::unique_ptr<ProgrammInformation> &&info) { _info = std::move(info); }

	const std::vector<std::unique_ptr<AudioSessionGroup>> &groups() const { return _groups; }
	std::vector<std::unique_ptr<AudioSessionGroup>> &groups() { return _groups; }

private:
	DWORD _pid;
	std::vector<std::unique_ptr<AudioSessionGroup>> _groups;
	std::unique_ptr<ProgrammInformation> _info;
};

class AudioSessionGroups {
public:
	AudioSessionGroups() = default;
	Q_DISABLE_COPY(AudioSessionGroups);

	AudioSessionGroups(AudioSessionGroups &&) = default;
	AudioSessionGroups &operator=(AudioSessionGroups &&) = default;

	std::vector<std::unique_ptr<AudioSessionPidGroup>>::iterator findPidGroup(DWORD pid);

	AudioSessionPidGroup &findPidGroupOrCreate(DWORD pid);

	void insert(std::unique_ptr<AudioSession> &&session, DWORD pid, const GUID &guid);

	const std::vector<std::unique_ptr<AudioSessionPidGroup>> &groups() const { return _groups; }
	std::vector<std::unique_ptr<AudioSessionPidGroup>> &groups() { return _groups; }

private:
	std::vector<std::unique_ptr<AudioSessionPidGroup>> _groups;
};

#endif // AUDIOSESSIONS_H
