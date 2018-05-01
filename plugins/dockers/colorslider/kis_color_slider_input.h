/*
 *  Copyright (c) 2008 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2014 Wolthera van Hövell <griffinvalley@gmail.com>    
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _KIS_COLOR_SLIDER_INPUT_H_
#define _KIS_COLOR_SLIDER_INPUT_H_

#include <QWidget>
#include "KoColorDisplayRendererInterface.h"
#include "kis_canvas2.h"


class KoColor;
class QWidget;
class QDoubleSpinBox;
class KisDoubleParseSpinBox;
class KisHSVSlider;
class KisDisplayColorConverter;



class KisColorSliderInput : public QWidget
{
    Q_OBJECT
public:
    KisColorSliderInput(QWidget* parent, KoColor* color, const int type, KoColorDisplayRendererInterface *displayRenderer, KisCanvas2* canvas);
protected:
    void init();
    virtual QWidget* createInput() = 0;
Q_SIGNALS:
    void updated();
    
protected:
    const int m_type;
    KoColor* m_color;
    KoColorDisplayRendererInterface *m_displayRenderer;
    KisCanvas2* m_canvas;
    KisHSVSlider* m_hsvSlider;
};

class KisHSXColorSliderInput : public KisColorSliderInput
{
    Q_OBJECT
public:
    KisHSXColorSliderInput(QWidget* parent, const int type, KoColor* color, KoColorDisplayRendererInterface *displayRenderer, KisCanvas2* canvas);
    
    KisDisplayColorConverter* converter() const;
protected:
    virtual QWidget* createInput();
    KisCanvas2* m_canvas;
public Q_SLOTS:
    void setValue(double);
    void update();
    void hueUpdate(int h);
    void satUpdate(int s, int type);
    void toneUpdate(int l, int type);
    void sliderChanged(int i);
    void sliderIn();
    void sliderOut();
    void numInputChanged(double v);
Q_SIGNALS:
    void hueUpdated(int);
    void satUpdated(int, int);
    void toneUpdated(int, int);
private:
    KisDoubleParseSpinBox* m_NumInput;
    qreal m_hue;
    qreal m_sat;
    qreal m_val;
    qreal R, G, B, Gamma;
    bool m_hueupdating;
    bool m_satupdating;
    bool m_toneupdating;
    bool m_sliderisupdating;
};

#endif
