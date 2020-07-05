#ifndef VOLUMELISTITEM_H
#define VOLUMELISTITEM_H
#include "volumecontroller/audio/audiosessions.h"

#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QLineEdit>
#include <QIcon>
#include <QPushButton>

class VolumeIcons;

class PeakSlider : public QSlider{
public:
	PeakSlider(QWidget *parent);

	void paintEvent(QPaintEvent *ev) override;

	int peakValue() const;
	void setPeakValue(int value);

	int scrollStepMultiplier() const { return _scrollStepMultiplier; }
	int setScrollStepMultiplier(int value) { _scrollStepMultiplier = value; }

	int controlScrollStepMultiplier() const { return _controlScrollStepMultiplier; }
	int setControlScrollStepMultiplier(int value) { _controlScrollStepMultiplier = value; }

	int setShiftScrollStepMultiplier(int value) { _shiftScrollStepMultiplier = value; }
	int shiftScrollStepMultiplier() const { return _shiftScrollStepMultiplier; }

protected:
	void wheelEvent(QWheelEvent *e) override;

private:
	int _peakValue = 0;

	// steps per scroll are multiplier * singleStep(), priority is in descending listing order
	// max scroll steps are pageStep()!
	int _controlScrollStepMultiplier = 1;
	int _shiftScrollStepMultiplier = 5;
	int _scrollStepMultiplier = 2;
};

class VolumeItemBase : public QObject {
	Q_OBJECT

public:
	Q_DISABLE_COPY_MOVE(VolumeItemBase);

	VolumeItemBase(QWidget *parent, IAudioControl &control);

	~VolumeItemBase();

	void setIcon(QPixmap bitmap);

	void setVolume(int volume);
	void setVolumeFAndMute(float volume, bool mute);

	bool muted() const;

	void updatePeak();

	void setIcon(const QIcon &icon);

	const IAudioControl &control() const { return _control; }

protected:
	void setMuted(bool muted);
	void setPeak(int volume);

private:
	void setMutedInternal(bool muted);
	void setVolumeInternal(int volume);

protected:
	virtual void volumeChangedEvent(int value);
	virtual void muteChangedEvent(bool mute);

	void setVolumeText(int volume);
	void setVolumeSlider(int volume);

	void setVolumeTextNoEvent(int volume);
	void setVolumeSliderNoEvent(int volume);

signals:
	void volumeChanged(int value);
	void muteChanged(bool mute);

public:
	QString identifier;
	QPushButton *descriptionButton;
	PeakSlider *volumeSlider;
	QLabel *volumeLabel;

private:
	QIcon *icon = nullptr;
	bool mutedValue;

	IAudioControl &_control;
};

class SessionVolumeItem : public VolumeItemBase {
public:
	SessionVolumeItem(QWidget *parent, AudioSession &control);

	const AudioSession &control() const { return _control; }
private:
	AudioSession &_control;
};

class DeviceVolumeItem : public VolumeItemBase {
public:
	DeviceVolumeItem(QWidget *parent, DeviceAudioControl &control, const VolumeIcons &icons);

protected:
	void volumeChangedEvent(int value) override;

private:
	void updateIcon(int volume);

	DeviceAudioControl &control;
	const VolumeIcons &icons;
};

#endif // VOLUMELISTITEM_H
