#ifndef WINDOWFADEANIMATION_H
#define WINDOWFADEANIMATION_H

#include <QPropertyAnimation>

#define IS_METHOD_OVERWRITTEN(method, base, derived) (&base::method != &derived::method);

template<typename T, typename Derived>
class InOutAnimation : public QObject {
protected:
	void beforeIn() {}

	void beforeOut() {}

	void afterIn() {}

	void afterOut() {}

	void afterCancelAnimation(bool in) {}

	Derived &derived() noexcept { return static_cast<Derived &>(*this); }

public:
	explicit InOutAnimation(QWidget *target, const QByteArray &propertyName, int duration, const QEasingCurve &inCurve, const QEasingCurve &outCurve, const T &inStartValue, const T &inEndValue, QObject *parent = nullptr)
		: QObject(parent),
		  target_(target),
		  in_(target, propertyName),
		  out_(target, propertyName) {
		in_.setDuration(duration);
		in_.setStartValue(inStartValue);
		in_.setEndValue(inEndValue);
		in_.setEasingCurve(inCurve);

		out_.setDuration(duration);
		out_.setStartValue(inEndValue);
		out_.setEndValue(inStartValue);
		out_.setEasingCurve(outCurve);

		constexpr bool isAfterOutOverwritten = IS_METHOD_OVERWRITTEN(afterOut, InOutAnimation, Derived);
		constexpr bool isAfterInOverwritten = IS_METHOD_OVERWRITTEN(afterIn, InOutAnimation, Derived);
		if constexpr(isAfterOutOverwritten) {
			QObject::connect(&out_, &QPropertyAnimation::finished, &derived(), &Derived::afterOut);
		}
		if constexpr(isAfterInOverwritten) {
			QObject::connect(&in_, &QPropertyAnimation::finished, &derived(), &Derived::afterIn);
		}
	}

	void in() {
		if(isRunningAnimation(in_))
			return;
		checkNoAnimation();
		derived().beforeIn();
		currentAnimation_ = &in_;
		currentAnimation_->start();
	}

	void out() {
		if(isRunningAnimation(out_))
			return;
		checkNoAnimation();
		derived().beforeOut();
		currentAnimation_ = &out_;
		currentAnimation_->start();
	}

	void finishAnimation() {
		if(currentAnimation_ != nullptr) {
			const bool finished = currentAnimation_->state() == QAbstractAnimation::State::Stopped;
			if(!finished)
				currentAnimation_->stop();

			const auto isIn = currentAnimation_ == &in_;
			currentAnimation_ = nullptr;
			if(!finished)
				derived().afterCancelAnimation(isIn);
		}
	}

	void setStartValue(const T &value) {
		startValue_ = value;
		out_.setEndValue(value);
		in_.setStartValue(value);
	}

	void setTargetValue(const T &value) {
		endValue_ = value;
		out_.setStartValue(value);
		in_.setEndValue(value);
	}

	const T &startValue() const noexcept { return startValue_; }
	const T &endValue() const noexcept { return endValue_; }

	QWidget * target() noexcept { return target_; }

	bool isRunning() const noexcept {
		return currentAnimation_ && isCurrentRunning();
	}

private:
	bool isCurrentRunning() const noexcept {
		return currentAnimation_->state() == QPropertyAnimation::State::Running;
	}

	bool isRunningAnimation(const QPropertyAnimation &animation) const noexcept  {
		return currentAnimation_ == &animation && isCurrentRunning();
	}

	void clearCurrentAnimation() noexcept { currentAnimation_ = nullptr; }

	void checkNoAnimation() {
		if(!currentAnimation_)
			return;

		Q_ASSERT(!isCurrentRunning());
		clearCurrentAnimation();
	}

	QWidget * const target_;

	T startValue_;
	T endValue_;

	QPropertyAnimation in_;
	QPropertyAnimation out_;
	QPropertyAnimation *currentAnimation_ = nullptr;
};

class FadeAnimation : public InOutAnimation<qreal, FadeAnimation>  {
	friend class InOutAnimation<qreal, FadeAnimation>;

protected:
	void beforeIn();
	void afterOut();

	void afterCancelAnimation(bool in);

public:
	FadeAnimation(QWidget *target);
};

class FlyAnimation : public InOutAnimation<QPoint, FlyAnimation> {
public:
	friend class InOutAnimation<QPoint, FlyAnimation>;

protected:
	void afterCancelAnimation(bool in);

public:
	FlyAnimation(QWidget *target, const QPoint &inStart, const QPoint &inEnd);
};

#endif // WINDOWFADEANIMATION_H
