#ifndef WINDOWFADEANIMATION_H
#define WINDOWFADEANIMATION_H

#include <QPropertyAnimation>



class WindowFadeAnimation
{
public:
	WindowFadeAnimation(QWidget *target);

	void fadeIn();
	void fadeOut();
	void finishAnimation();

private:
	QWidget * const target;

	QPropertyAnimation fadeInAnimation;
	QPropertyAnimation fadeOutAnimation;
	QPropertyAnimation * currentAnimation = nullptr;
};

#endif // WINDOWFADEANIMATION_H
