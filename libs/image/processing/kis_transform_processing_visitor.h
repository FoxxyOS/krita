/*
 *  Copyright (c) 2011 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_TRANSFORM_PROCESSING_VISITOR_H
#define __KIS_TRANSFORM_PROCESSING_VISITOR_H

#include "kis_processing_visitor.h"


#include <kis_types.h>

#include <QPointF>
#include <QTransform>

class KisFilterStrategy;


class KRITAIMAGE_EXPORT KisTransformProcessingVisitor : public KisProcessingVisitor
{
public:
    KisTransformProcessingVisitor(qreal  xscale, qreal  yscale,
                                  qreal  xshear, qreal  yshear, const QPointF &shearOrigin, qreal angle,
                                  qint32  tx, qint32  ty,
                                  KisFilterStrategy *filter,
                                  const QTransform &shapesCorrection = QTransform());

    void visit(KisNode *node, KisUndoAdapter *undoAdapter);
    void visit(KisPaintLayer *layer, KisUndoAdapter *undoAdapter);
    void visit(KisGroupLayer *layer, KisUndoAdapter *undoAdapter);
    void visit(KisAdjustmentLayer *layer, KisUndoAdapter *undoAdapter);
    void visit(KisExternalLayer *layer, KisUndoAdapter *undoAdapter);
    void visit(KisGeneratorLayer *layer, KisUndoAdapter *undoAdapter);
    void visit(KisCloneLayer *layer, KisUndoAdapter *undoAdapter);
    void visit(KisFilterMask *mask, KisUndoAdapter *undoAdapter);
    void visit(KisTransformMask *mask, KisUndoAdapter *undoAdapter);
    void visit(KisTransparencyMask *mask, KisUndoAdapter *undoAdapter);
    void visit(KisSelectionMask *mask, KisUndoAdapter *undoAdapter);
    void visit(KisColorizeMask *mask, KisUndoAdapter *undoAdapter);

private:
    void transformClones(KisLayer *layer, KisUndoAdapter *undoAdapter);
    void transformPaintDevice(KisPaintDeviceSP device, KisUndoAdapter *adapter, const ProgressHelper &helper);
    void transformSelection(KisSelectionSP selection, KisUndoAdapter *adapter, const ProgressHelper &helper);

private:
    qreal m_sx, m_sy;
    qint32 m_tx, m_ty;
    qreal m_shearx, m_sheary;
    QPointF m_shearOrigin;
    KisFilterStrategy *m_filter;
    qreal m_angle;
    QTransform m_shapesCorrection;
};

#endif /* __KIS_TRANSFORM_PROCESSING_VISITOR_H */
