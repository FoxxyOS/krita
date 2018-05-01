/*
    Copyright (C) 2011 Silvio Heinrich <plassy@web.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef H_KIS_COLOR_SELECTOR_H
#define H_KIS_COLOR_SELECTOR_H

#include <QWidget>
#include <QVector>
#include <QImage>
#include <QPainterPath>

#include "kis_color.h"
#include "kis_radian.h"
#include "kis_acs_types.h"
#include "kis_signal_compressor_with_param.h"

class QPainter;
class QPainter;

class KisColorSelector: public QWidget
{
    Q_OBJECT
    
    typedef KisRadian<float> Radian;
    
    struct ColorRing
    {
        ColorRing(): angle(0) { }
        
        Radian getPieceAngle() const { return RAD_360 / float(pieced.size()); }
        Radian getShift     () const { return angle % getPieceAngle();        }
        Radian getMovedAngel() const { return angle - tmpAngle;               }
        
        void setTemporaries(const KisColor& color) {
            tmpAngle = angle;
            tmpColor = color;
        }
        
        KisColor              tmpColor;
        Radian                tmpAngle;
        Radian                angle;
        float                 saturation;
        float                 outerRadius;
        float                 innerRadius;
        QVector<QPainterPath> pieced;
    };
    
public:
    KisColorSelector(QWidget* parent, KisColor::Type type=KisColor::HSL);
    
    void setColorSpace(KisColor::Type type);
    void setNumPieces(int num);
    void setNumLightPieces(int num);
    void setNumRings(int num);
    void resetRings();
    void resetSelectedRing();
    void resetLight();
    void setLight(float light=0.0f, bool relative=true);
    void setInverseSaturation(bool inverse);
    void selectColor(const KisColor& color);
    void setFgColor(const KisColor& fgColor);
    void setBgColor(const KisColor& bgColor);
    
    void saveSettings();
    void loadSettings();
    
    KisColor::Type getColorSpace       () const { return m_colorSpace;        }
    qint32         getNumRings         () const { return m_colorRings.size(); }
    qint32         getNumPieces        () const { return m_numPieces;         }
    qint32         getNumLightPieces   () const { return m_numLightPieces;    }
    qreal          getLight            () const { return m_light;             }
    bool           isSaturationInverted() const { return m_inverseSaturation; }
    bool           islightRelative     () const { return m_relativeLight;     }

Q_SIGNALS:
    void sigFgColorChanged(const KisColor& color);
    void sigBgColorChanged(const KisColor& color);
    
private:
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void paintEvent(QPaintEvent* event);

    void requestUpdateColorAndPreview(const KisColor &color, Acs::ColorRole role);

    void recalculateAreas(quint8 numLightPieces);
    void recalculateRings(quint8 numRings, quint8 numPieces);
    void createRing(ColorRing& wheel, quint8 numPieces, qreal innerRadius, qreal outerRadius);

    void drawRing(QPainter& painter, ColorRing& wheel, const QRect& rect);
    void drawOutline(QPainter& painter, const QRect& rect);
    void drawLightStrip(QPainter& painter, const QRect& rect);

    qint8 getHueIndex(Radian hue, Radian shift=0.0f) const;
    qreal getHue(int hueIdx, Radian shift=0.0f) const;
    qint8 getLightIndex(const QPointF& pt) const;
    qint8 getLightIndex(qreal light) const;
    qreal getLight(qreal light, qreal hue, bool relative) const;
    qreal getLight(const QPointF& pt) const;
    qint8 getSaturationIndex(const QPointF& pt) const;
    qint8 getSaturationIndex(qreal saturation) const;
    qreal getSaturation(int saturationIdx) const;
    
    QPointF mapCoord(const QPointF& pt, const QRectF& rect) const;

public:
    // This is a private interface for signal compressor, don't use it.
    // Use requestUpdateColorAndPreview() instead
    void slotUpdateColorAndPreview(QPair<KisColor, Acs::ColorRole> color);

private:
    KisColor::Type     m_colorSpace;
    quint8             m_numPieces;
    quint8             m_numLightPieces;
    bool               m_inverseSaturation;
    bool               m_relativeLight;
    float              m_light;
    qint8              m_selectedRing;
    qint8              m_selectedPiece;
    qint8              m_selectedLightPiece;
    KisColor           m_selectedColor;
    KisColor           m_fgColor;
    KisColor           m_bgColor;
    QImage             m_renderBuffer;
    QRect              m_renderArea;
    QRect              m_lightStripArea;
    bool               m_mouseMoved;
    Acs::ColorRole     m_selectedColorRole;
    QPointF            m_clickPos;
    qint8              m_clickedRing;
    QVector<ColorRing> m_colorRings;
    Qt::MouseButtons   m_pressedButtons;

    typedef KisSignalCompressorWithParam<QPair<KisColor, Acs::ColorRole>> ColorCompressorType;
    QScopedPointer<ColorCompressorType> m_updateColorCompressor;
};

#endif // H_KIS_COLOR_SELECTOR_H
