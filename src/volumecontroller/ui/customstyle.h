#ifndef CUSTOMSTYLE_H
#define CUSTOMSTYLE_H

#include "theme.h"

#include <QProxyStyle>

class CustomStyle : public QProxyStyle {
public:
	CustomStyle(QStyle *style);

	void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget = nullptr) const override;

	void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
	int pixelMetric(PixelMetric pm, const QStyleOption *opt, const QWidget *widget) const override;

	SliderTheme sliderTheme;
	PushButtonTheme buttonTheme;
};

#endif // CUSTOMSTYLE_H
