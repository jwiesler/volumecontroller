#include "customstyle.h"

#include <QStyleOptionSlider>
#include <QDebug>
#include <QPainterPath>

#include <QPainter>

CustomStyle::CustomStyle(QStyle *style) : QProxyStyle(style) {}

void FillRectInset(const QRect &rect, QPainter *painter, const QColor &color) {
	const auto innerRect = QRect(rect.left() + 1, rect.top() + 1, rect.width() - 2,
											rect.height() - 2);
	painter->fillRect(innerRect, color);
}

void FillRectWithBorder(const QRect &rect, QPainter *painter, const QColor &inside, const QColor border) {
	painter->fillRect(rect, border);
	FillRectInset(rect, painter, inside);
}

void CustomStyle::drawComplexControl(QStyle::ComplexControl control, const QStyleOptionComplex *option,
												QPainter *painter, const QWidget *widget) const {
	const State flags = option->state;
	const SubControls sub = option->subControls;

	switch(control) {
		case CC_Slider:
			if(const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
				const QRect slrect = slider->rect;

				if(sub & SC_SliderGroove) {
					const auto fullGrooveRect = proxy()->subControlRect(CC_Slider, option, SC_SliderGroove, widget);
					QRect grooveRect;
					if(slider->orientation == Qt::Horizontal) {
						grooveRect = QRect(slrect.left(), fullGrooveRect.center().y() - 2, slrect.width(), 4);
					} else {
						grooveRect = QRect(fullGrooveRect.center().x() - 2, slrect.top(), 4, slrect.height());
					}
					FillRectWithBorder(grooveRect, painter, sliderTheme.grooveBackground, sliderTheme.grooveBorder);
				}

				if(sub & SC_SliderTickmarks)
					qDebug() << "Unsupported option SC_SliderTickmarks used";

				if(sub & SC_SliderHandle) {
					const auto sliderRect = proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget);

					const QColor &color = [&]() -> decltype(auto) {
						if(!(slider->state & State_Enabled))
							return sliderTheme.sliderDisabled;
						else if(slider->activeSubControls & SC_SliderHandle && (slider->state & State_Sunken))
							return sliderTheme.sliderFocused;
						else if(slider->activeSubControls & SC_SliderHandle && (slider->state & State_MouseOver))
							return sliderTheme.sliderFocused;
						else if(flags & State_HasFocus)
							return sliderTheme.slider;
						else
							return sliderTheme.slider;
					}();

					qreal radius = sliderRect.width() / 2;
					QPainterPath path(QPointF(sliderRect.left(), sliderRect.top() + radius));
					path.arcTo(QRectF(sliderRect.left(), sliderRect.top(), sliderRect.width(), sliderRect.width()), 180, -180);
					path.arcTo(QRectF(sliderRect.left(), sliderRect.bottom() - 2 * radius, sliderRect.width(), 2 * radius), 0, -180);
					painter->save();
					painter->setRenderHint(QPainter::Antialiasing);
					painter->fillPath(path, color);
					painter->restore();
				}
				if(slider->state & State_HasFocus) {
					QStyleOptionFocusRect fropt;
					fropt.QStyleOption::operator=(*slider);
					fropt.rect = subElementRect(SE_SliderFocusRect, slider, widget);
					proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
				}
			}
			break;

		default:
			QProxyStyle::drawComplexControl(control, option, painter, widget);

	}
}

void CustomStyle::drawControl(QStyle::ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const {
	const auto flags = option->state;
	switch(element) {
		case CE_PushButtonBevel:
			if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
				bool justFlat = (!(flags & State_MouseOver) && (btn->features & QStyleOptionButton::Flat) && !(flags & (State_On|State_Sunken)))
									 || ((btn->features & QStyleOptionButton::CommandLinkButton)
										  && !(flags & State_MouseOver)
										  && !(btn->features & QStyleOptionButton::DefaultButton));
				const auto disabled = !(flags & State_Enabled) && !(btn->features & QStyleOptionButton::Flat);

				const QColor *inside;
				const QColor *border;

				if (disabled) {
					inside = &buttonTheme.disabled;
					border = &buttonTheme.disabledBorder;
				} else if (justFlat) {
					inside = nullptr;
					border = nullptr;
				} else if (flags & (State_Sunken | State_On)) {
					inside = &buttonTheme.down;
					border = &buttonTheme.downBorder;
				} else if (flags & State_MouseOver) {
					inside = &buttonTheme.hovered;
					border = &buttonTheme.hoveredBorder;
				} else {
					inside = &buttonTheme.normal;
					border = &buttonTheme.normalBorder;
				}

				if(border) {
					painter->fillRect(btn->rect, *border);
					if(inside)
						FillRectInset(btn->rect, painter, *inside);
				} else if(inside) {
					painter->fillRect(btn->rect, *inside);
				}
			}
			break;

		case CE_PushButtonLabel:
			if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
				painter->save();
				if(!btn->icon.isNull())
					painter->setRenderHint(QPainter::Antialiasing);
				QProxyStyle::drawControl(element, option, painter, widget);
				painter->restore();
			}
			break;
		default:
			QProxyStyle::drawControl(element, option, painter, widget);
	}
}

int CustomStyle::pixelMetric(QStyle::PixelMetric pm, const QStyleOption *opt, const QWidget *widget) const {
	if(pm == QStyle::PM_SliderLength)
		return 8;
	return QProxyStyle::pixelMetric(pm, opt, widget);
}
