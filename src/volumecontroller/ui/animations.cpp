#include "animations.h"
#include <QWidget>
#include <QDebug>

void FadeAnimation::beforeIn() {
	InOutAnimation::beforeIn();
	target()->show();
}

void FadeAnimation::afterOut() {
	InOutAnimation::afterOut();
	target()->hide();
}

void FadeAnimation::afterCancelAnimation(bool in) {
	target()->setWindowOpacity(in ? endValue() : startValue());
	target()->repaint();
}

FadeAnimation::FadeAnimation(QWidget *target)
	: InOutAnimation(target, "windowOpacity", 200, QEasingCurve::OutQuint, QEasingCurve::InQuad, 0.0, 1.0) {}

void FlyAnimation::afterCancelAnimation(bool in) {
	target()->move(in ? endValue() : startValue());
}

FlyAnimation::FlyAnimation(QWidget *target, const QPoint &inStart, const QPoint &inEnd)
	: InOutAnimation(target, "pos", 200, QEasingCurve::OutQuint, QEasingCurve::OutQuint, inStart, inEnd) {}
