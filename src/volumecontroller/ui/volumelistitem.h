#ifndef VOLUMELISTITEM_H
#define VOLUMELISTITEM_H
#include "volumecontroller/audio/audiosessions.h"

#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QLineEdit>
#include <QIcon>
#include <QPushButton>

#include <volumecontroller/ui/theme.h>

class VolumeIcons;

class PeakSlider : public QSlider{
public:
	PeakSlider(QWidget *parent, const PeakSliderTheme &theme);

	void paintEvent(QPaintEvent *ev) override;

	void updateTheme(const PeakSliderTheme &theme);

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
	const PeakSliderTheme *_theme;
};

class VolumeItemBase : public QObject {
	Q_OBJECT

public:
	Q_DISABLE_COPY_MOVE(VolumeItemBase);

	VolumeItemBase(QWidget *parent, IAudioControl &control, const VolumeItemTheme &theme);

	~VolumeItemBase();

	void setIcon(QPixmap bitmap);

	void setVolume(int volume);
	void setVolumeFAndMute(float volume, bool mute);

	bool muted() const;

	void updatePeak();

	void setIcon(const QIcon &icon);
	void setInfo(const std::optional<QIcon> &icon, const QString &identifier);

	const IAudioControl &control() const { return _control; }

	void show();
	void hide();

	const QString &identifier() const { return _identifier; }

	QPushButton *descriptionButton() { return _descriptionButton; }
	PeakSlider *volumeSlider() { return _volumeSlider; }
	QLabel *volumeLabel() { return _volumeLabel; }

	void updateTheme(const VolumeItemTheme &theme);

protected:
	void setMuted(bool muted);
	void setPeak(int volume);

	virtual void volumeChangedEvent(int value);
	virtual void muteChangedEvent(bool mute);

	void setVolumeText(int volume);
	void setVolumeSlider(int volume);

	void setVolumeTextNoEvent(int volume);
	void setVolumeSliderNoEvent(int volume);

signals:
	void volumeChanged(int value);
	void muteChanged(bool mute);

private:
	void setMutedInternal(bool muted);
	void setVolumeInternal(int volume);

	QString _identifier;
	QPushButton *_descriptionButton;
	PeakSlider *_volumeSlider;
	QLabel *_volumeLabel;

	QIcon *icon = nullptr;
	bool mutedValue;

	IAudioControl &_control;
};

class SessionVolumeItem : public VolumeItemBase {
public:
	SessionVolumeItem(QWidget *parent, AudioSession &control, const VolumeItemTheme &theme);

	const AudioSession &control() const { return _control; }
private:
	AudioSession &_control;
};

class DeviceVolumeItem : public VolumeItemBase {
public:
	DeviceVolumeItem(QWidget *parent, DeviceAudioControl &control, const VolumeIcons &icons, const QString &deviceName, const VolumeItemTheme &theme);

	void setVolumeFAndMute(float volume, bool muted);
	void updateThemeAndIcon(const VolumeItemTheme &theme);

protected:
	void volumeChangedEvent(int value) override;

private:
	void updateIcon(int volume);

	DeviceAudioControl &control;
	const VolumeIcons &icons;
};

#endif // VOLUMELISTITEM_H
