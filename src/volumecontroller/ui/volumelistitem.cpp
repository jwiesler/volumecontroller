#include "volumecontroller.h"
#include "volumelistitem.h"
#include <QApplication>
#include <QDebug>
#include <QGraphicsScene>
#include <QPaintEvent>
#include <QPainter>
#include <QStyleOptionSlider>

PeakSlider::PeakSlider(QWidget *parent, const PeakSliderTheme &theme) : QSlider(parent), theme(theme) {}

void PeakSlider::paintEvent(QPaintEvent *ev) {
	QSlider::paintEvent(ev);

	QStyleOptionSlider opt;
	initStyleOption(&opt);
	QPainter painter(this);

	const auto sliderRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
	const auto fullGrooveRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);

	const auto grooveRect = QRect(opt.rect.left(), fullGrooveRect.center().y() - 2, opt.rect.width(), 4);
	const auto innerRect = QRect(grooveRect.left() + 1, grooveRect.top() + 1, grooveRect.width() - 2, grooveRect.height() - 2);

	if(opt.state & QStyle::State_Enabled) {
		QRect r{innerRect.left(), innerRect.top(), _peakValue * (sliderRect.left() - 1 - innerRect.x()) / maximum(), innerRect.height()};
		if(r.left() < sliderRect.x()) {
			painter.fillRect(r, theme.peakMeter);
		}
	}

	painter.end();
}

int PeakSlider::peakValue() const {
	return _peakValue;
}

void PeakSlider::setPeakValue(int value) {
	if(_peakValue == value)
		return;
	_peakValue = value;
	repaint();
}

QWheelEvent CreateScrollEvent(QWheelEvent *e, QPoint angleDelta, Qt::KeyboardModifier modifiers) {
	return QWheelEvent(
				e->position(), e->globalPosition(), e->pixelDelta(), angleDelta,
				e->buttons(), modifiers, e->phase(), e->inverted(), e->source());
}

void PeakSlider::wheelEvent(QWheelEvent *e){
	const bool vertical = e->angleDelta().y() != 0;
	const int angle = vertical ? e->angleDelta().y() : e->angleDelta().x();
	const int scrollLines = QApplication::wheelScrollLines();

	int multiplier;
	// update value depending on the modifiers
	if(e->modifiers() & Qt::ControlModifier) {
		multiplier = controlScrollStepMultiplier();
	} else if(e->modifiers() & Qt::ShiftModifier) {
		multiplier = shiftScrollStepMultiplier();
	} else {
		multiplier = scrollStepMultiplier();
	}

	const int scrollSteps = multiplier * singleStep();
	const int newAngle = (angle * scrollSteps) / scrollLines;

	const auto newAngleDelta = vertical ? QPoint(0, newAngle) : QPoint(newAngle, 0);
	QWheelEvent event = CreateScrollEvent(e, newAngleDelta, Qt::KeyboardModifier::NoModifier);

//	qDebug() << "Scrolled" << angle << "should be" << scrollSteps << "scrollLines" << scrollLines << "new angle is" << newAngle;
	QSlider::wheelEvent(&event);
}

VolumeItemBase::VolumeItemBase(QWidget *parent, IAudioControl &ctrl, const VolumeItemTheme &theme) : QObject(parent), icon(nullptr), _control(ctrl) {
	_descriptionButton = new QPushButton(parent);
	_descriptionButton->setFlat(true);
	_descriptionButton->setCheckable(true);
	_descriptionButton->setIconSize(QSize(32, 32));
	_descriptionButton->setFixedSize(40, 40);

	_volumeSlider = new PeakSlider(parent, theme.slider);
	_volumeSlider->setOrientation(Qt::Horizontal);
	_volumeSlider->setMaximum(100);

	_volumeLabel = new QLabel(parent);
	QFont font = _volumeLabel->font();
	font.setPointSize(12);
	_volumeLabel->setFont(font);
	_volumeLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
	QFontMetrics metrics(font);
	_volumeLabel->setFixedWidth(metrics.horizontalAdvance("100"));

	QObject::connect(_volumeSlider, &QSlider::valueChanged, [this](int value) {
		setVolumeText(value);
		volumeChangedEvent(value);
	});

	QObject::connect(_descriptionButton, &QPushButton::clicked, [this](bool checked) {
		setMuted(checked);
	});

	setMuted(control().muted().value_or(true));
	setVolume(control().volume().value_or(0.0f) * 100.0f);
}

VolumeItemBase::~VolumeItemBase() {
	delete _volumeSlider;
	delete _volumeLabel;
	delete _descriptionButton;
}

void VolumeItemBase::setVolumeText(int volume) {
	auto str = QString::number(volume);
	if(str == _volumeLabel->text())
		return;
	_volumeLabel->setText(str);
}

void VolumeItemBase::setVolumeSlider(int volume) {
	if(_volumeSlider->value() == (int)volume)
		return;
	_volumeSlider->setValue(volume);
}

void VolumeItemBase::setMuted(bool muted) {
	setMutedInternal(muted);
	muteChangedEvent(muted);
}

void VolumeItemBase::setMutedInternal(bool muted) {
	mutedValue = muted;
	_descriptionButton->setChecked(muted);
	_volumeSlider->setDisabled(muted);
	_volumeLabel->setDisabled(muted);
}

void VolumeItemBase::setPeak(int volume) {
	if(mutedValue)
		volume = 0;
	_volumeSlider->setPeakValue(volume);
}

bool VolumeItemBase::muted() const {
	return mutedValue;
}

void VolumeItemBase::updatePeak() {
	int value = 0;
	if(!muted())
		value = control().peakValue().value_or(0.0f) * 100.0f;
	setPeak(value);
}

void VolumeItemBase::setIcon(const QIcon &icon) {
	_descriptionButton->setIcon(icon);
}

void VolumeItemBase::setInfo(const std::optional<QIcon> &icon, const QString &identifier) {
	_identifier = identifier;
	if(icon.has_value()) {
		_descriptionButton->setToolTip(identifier);
		setIcon(*icon);
	} else {
		_descriptionButton->setText(identifier);
	}
}

void VolumeItemBase::show() {
	_descriptionButton->show();
	_volumeSlider->show();
	_volumeLabel->show();
}

void VolumeItemBase::hide() {
	_descriptionButton->hide();
	_volumeSlider->hide();
	_volumeLabel->hide();
}

void VolumeItemBase::volumeChangedEvent(const int value) {
	if(!_control.setVolume(value / 100.0f))
		qDebug() << "Failed to set volume for" << identifier();

	emit volumeChanged(value);
}

void VolumeItemBase::muteChangedEvent(const bool mute) {
	if(!_control.setMuted(mute))
		qDebug() << "Failed to set mute for" << identifier();

	emit muteChanged(mute);
}

void VolumeItemBase::setVolume(const int volume) {
	setVolumeText(volume);
	setVolumeSlider(volume);
}

void VolumeItemBase::setVolumeFAndMute(float volume, bool mute) {
	setVolume(volume * 100.0f);
	setMuted(mute);
}

SessionVolumeItem::SessionVolumeItem(QWidget *parent, AudioSession &control, const VolumeItemTheme &theme) : VolumeItemBase(parent, control, theme), _control(control) {}

DeviceVolumeItem::DeviceVolumeItem(QWidget *parent, DeviceAudioControl &control, const VolumeIcons &icons, const QString &deviceName, const VolumeItemTheme &theme) : VolumeItemBase(parent, control, theme), control(control), icons(icons) {
	const auto volume = control.volume().value_or(0.0f) * 100.0f;
	setInfo(icons.selectIcon(volume), deviceName);
}

void DeviceVolumeItem::setVolumeFAndMute(float volume, bool muted) {
	VolumeItemBase::setVolumeFAndMute(volume, muted);
	updateIcon(volume * 100.0f);
}

void DeviceVolumeItem::volumeChangedEvent(const int value) {
	VolumeItemBase::volumeChangedEvent(value);
	updateIcon(value);
}

void DeviceVolumeItem::updateIcon(const int volume) {
	setIcon(icons.selectIcon(volume));
}
