#include "windowfadeanimation.h"
#include <QWidget>
#include <QDebug>

WindowFadeAnimation::WindowFadeAnimation(QWidget *target)
	: target(target),
	  fadeInAnimation(target, "windowOpacity"),
	  fadeOutAnimation(target, "windowOpacity") {
	fadeInAnimation.setDuration(250);
	fadeInAnimation.setStartValue(qreal(0));
	fadeInAnimation.setEndValue(qreal(1));
	fadeInAnimation.setEasingCurve(QEasingCurve::InExpo);

	fadeOutAnimation.setDuration(250);
	fadeOutAnimation.setStartValue(qreal(1));
	fadeOutAnimation.setEndValue(qreal(0));
	fadeOutAnimation.setEasingCurve(QEasingCurve::OutExpo);

	QObject::connect(&fadeOutAnimation, &QPropertyAnimation::finished, target, &QWidget::hide);
}

void WindowFadeAnimation::fadeIn()
{
	target->show();
	currentAnimation = &fadeInAnimation;
	currentAnimation->start();
}

void WindowFadeAnimation::fadeOut()
{
	currentAnimation = &fadeOutAnimation;
	currentAnimation->start();
}

void WindowFadeAnimation::finishAnimation()
{
	if(currentAnimation != nullptr) {
		currentAnimation->stop();
		const auto isFadeIn = currentAnimation == &fadeInAnimation;
		currentAnimation = nullptr;
		target->setWindowOpacity(isFadeIn ? 1.0 : 0.0);
		target->repaint();
	}
}

void WindowFadeAnimation::setTargetValue(qreal value) {
	fadeOutAnimation.setStartValue(value);
	fadeInAnimation.setEndValue(value);
}
