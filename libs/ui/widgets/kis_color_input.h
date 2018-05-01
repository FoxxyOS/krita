/*
 *  Copyright (c) 2008 Cyrille Berger <cberger@cberger.net>
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

#ifndef _KIS_COLOR_INPUT_H_
#define _KIS_COLOR_INPUT_H_

#include <QWidget>

class KoChannelInfo;
class KoColor;
class QWidget;
class QSpinBox;
class QDoubleSpinBox;
class KisIntParseSpinBox;
class KisDoubleParseSpinBox;
class KoColorSlider;
class QLineEdit;

#include <KoColorDisplayRendererInterface.h>
#include "kritaui_export.h"

class KRITAUI_EXPORT KisColorInput : public QWidget
{
    Q_OBJECT
public:
    KisColorInput(QWidget* parent, const KoChannelInfo*, KoColor* color, KoColorDisplayRendererInterface *displayRenderer = KoDumbColorDisplayRenderer::instance());
protected:
    void init();
    virtual QWidget* createInput() = 0;
Q_SIGNALS:
    void updated();
protected:
    const KoChannelInfo* m_channelInfo;
    KoColor* m_color;
    KoColorSlider* m_colorSlider;
    KoColorDisplayRendererInterface *m_displayRenderer;
};

class KRITAUI_EXPORT KisIntegerColorInput : public KisColorInput
{
    Q_OBJECT
public:
    KisIntegerColorInput(QWidget* parent, const KoChannelInfo*, KoColor* color, KoColorDisplayRendererInterface *displayRenderer = KoDumbColorDisplayRenderer::instance());
protected:
    virtual QWidget* createInput();
public Q_SLOTS:
    void setValue(int);
    void update();
private:
    KisIntParseSpinBox* m_intNumInput;
};


class KRITAUI_EXPORT KisFloatColorInput : public KisColorInput
{
    Q_OBJECT
public:
    KisFloatColorInput(QWidget* parent, const KoChannelInfo*, KoColor* color, KoColorDisplayRendererInterface *displayRenderer = KoDumbColorDisplayRenderer::instance());
protected:
    virtual QWidget* createInput();
public Q_SLOTS:
    void setValue(double);
    void sliderChanged(int);
    void update();
private:
    KisDoubleParseSpinBox* m_dblNumInput;
    qreal m_minValue;
    qreal m_maxValue;
};

class KRITAUI_EXPORT KisHexColorInput : public KisColorInput
{
    Q_OBJECT
public:
    KisHexColorInput(QWidget* parent, KoColor* color, KoColorDisplayRendererInterface *displayRenderer = KoDumbColorDisplayRenderer::instance());
protected:
    virtual QWidget* createInput();
public Q_SLOTS:
    void setValue();
    void update();
private:
    QLineEdit* m_hexInput;
};

#endif
