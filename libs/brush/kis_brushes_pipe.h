/*
 *  Copyright (c) 2012 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __KIS_BRUSHES_PIPE_H
#define __KIS_BRUSHES_PIPE_H

#include <kis_fixed_paint_device.h>

template<class BrushType>
class KisBrushesPipe
{
public:
    KisBrushesPipe() {
    }

    KisBrushesPipe(const KisBrushesPipe &rhs) {
        qDeleteAll(m_brushes);
        m_brushes.clear();
        Q_FOREACH (BrushType * brush, rhs.m_brushes) {
            BrushType *clonedBrush = dynamic_cast<BrushType*>(brush->clone());
            KIS_ASSERT_RECOVER(clonedBrush) {continue;}

            m_brushes.append(clonedBrush);
        }
    }

    virtual ~KisBrushesPipe() {
        qDeleteAll(m_brushes);
    }

    virtual void clear() {
        qDeleteAll(m_brushes);
        m_brushes.clear();
    }

    BrushType* firstBrush() const {
        return m_brushes.first();
    }

    BrushType* lastBrush() const {
        return m_brushes.last();
    }

    BrushType* currentBrush(const KisPaintInformation& info) {
        return !m_brushes.isEmpty() ? m_brushes.at(chooseNextBrush(info)) : 0;
    }

    int brushIndex(const KisPaintInformation& info) {
        return chooseNextBrush(info);
    }

    qint32 maskWidth(KisDabShape const& shape, double subPixelX, double subPixelY, const KisPaintInformation& info) {
        BrushType *brush = currentBrush(info);
        return brush ? brush->maskWidth(shape, subPixelX, subPixelY, info) : 0;
    }

    qint32 maskHeight(KisDabShape const& shape, double subPixelX, double subPixelY, const KisPaintInformation& info) {
        BrushType *brush = currentBrush(info);
        return brush ? brush->maskHeight(shape, subPixelX, subPixelY, info) : 0;
    }

    void setAngle(qreal angle) {
        Q_FOREACH (BrushType * brush, m_brushes) {
            brush->setAngle(angle);
        }
    }

    void setScale(qreal scale) {
        Q_FOREACH (BrushType * brush, m_brushes) {
            brush->setScale(scale);
        }
    }

    void setSpacing(double spacing) {
        Q_FOREACH (BrushType * brush, m_brushes) {
            brush->setSpacing(spacing);
        }
    }

    bool hasColor() const {
        Q_FOREACH (BrushType * brush, m_brushes) {
            if (brush->hasColor()) return true;
        }
        return false;
    }

    void notifyCachedDabPainted(const KisPaintInformation& info) {
        updateBrushIndexes(info);
    }

    void generateMaskAndApplyMaskOrCreateDab(KisFixedPaintDeviceSP dst, KisBrush::ColoringInformation* coloringInformation,
            KisDabShape const& shape,
            const KisPaintInformation& info,
            double subPixelX , double subPixelY,
            qreal softnessFactor) {

        BrushType *brush = currentBrush(info);
        if (!brush) return;


        brush->generateMaskAndApplyMaskOrCreateDab(dst, coloringInformation, shape, info, subPixelX, subPixelY, softnessFactor);
        updateBrushIndexes(info);
    }

    KisFixedPaintDeviceSP paintDevice(const KoColorSpace * colorSpace,
                                      KisDabShape const& shape,
                                      const KisPaintInformation& info,
                                      double subPixelX, double subPixelY) {

        BrushType *brush = currentBrush(info);
        if (!brush) return 0;

        KisFixedPaintDeviceSP device = brush->paintDevice(colorSpace, shape, info, subPixelX, subPixelY);
        updateBrushIndexes(info);
        return device;
    }

    QVector<BrushType*> brushes() {
        return m_brushes;
    }

    void testingSelectNextBrush(const KisPaintInformation& info) {
        (void) chooseNextBrush(info);
        updateBrushIndexes(info);
    }

    /**
     * Is called by the paint op when a paintop starts a stroke. The
     * brushes are shared among different strokes, so sometimes the
     * brush should be reset.
     */
    virtual void notifyStrokeStarted() = 0;

protected:
    void addBrush(BrushType *brush) {
        m_brushes.append(brush);
    }

    /**
     * Returns the index of the brush that corresponds to the current
     * values of \p info. This method is called *before* the dab is
     * actually painted.
     *
     * The method is const, so no internal counters of the brush should
     * change during its execution
     */
    virtual int chooseNextBrush(const KisPaintInformation& info) = 0;

    /**
     * Updates internal counters of the brush *after* a dab has been
     * painted on the canvas. Some incremental switching of the brushes
     * may me implemented in this method.
     */
    virtual void updateBrushIndexes(const KisPaintInformation& info) = 0;

protected:
    QVector<BrushType*> m_brushes;
};

#endif /* __KIS_BRUSHES_PIPE_H */
