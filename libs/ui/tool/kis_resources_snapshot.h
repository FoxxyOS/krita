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

#ifndef __KIS_RESOURCES_SNAPSHOT_H
#define __KIS_RESOURCES_SNAPSHOT_H

#include "kis_shared.h"
#include "kis_shared_ptr.h"
#include "kis_types.h"
#include "kritaui_export.h"
#include "kis_painter.h"
#include "kis_default_bounds.h"

class KoCanvasResourceManager;
class KoCompositeOp;
class KisPainter;
class KisPostExecutionUndoAdapter;
class KisRecordedPaintAction;
class KoPattern;

/**
 * @brief The KisResourcesSnapshot class takes a snapshot of the various resources
 * like colors and settings used at the begin of a stroke or a recording so subsequent
 * changes don't impact the running stroke. The main reason for the snapshot is that the
 * user can *change* the options while the stroke is being executed in the background.
 */
class KRITAUI_EXPORT KisResourcesSnapshot : public KisShared
{
public:
    KisResourcesSnapshot(KisImageSP image, KisNodeSP currentNode, KoCanvasResourceManager *resourceManager, KisDefaultBoundsBaseSP bounds = 0);
    ~KisResourcesSnapshot();

    void setupPainter(KisPainter *painter);
    // XXX: This was marked as KDE_DEPRECATED, but no althernative was
    //      given in the apidox.
    void setupPaintAction(KisRecordedPaintAction *action);

    KisPostExecutionUndoAdapter* postExecutionUndoAdapter() const;
    void setCurrentNode(KisNodeSP node);
    void setStrokeStyle(KisPainter::StrokeStyle strokeStyle);
    void setFillStyle(KisPainter::FillStyle fillStyle);

    KisNodeSP currentNode() const;
    KisImageSP image() const;
    bool needsIndirectPainting() const;
    QString indirectPaintingCompositeOp() const;

    /**
     * \return currently active selection. Note that it will return
     *         null if current node *is* the current selection. This
     *         is done to avoid recursive selection application when
     *         painting on selectgion masks.
     */
    KisSelectionSP activeSelection() const;

    bool needsAirbrushing() const;
    int airbrushingRate() const;

    void setOpacity(qreal opacity);
    quint8 opacity() const;
    const KoCompositeOp* compositeOp() const;
    QString compositeOpId() const;

    KoPattern* currentPattern() const;
    KoColor currentFgColor() const;
    KoColor currentBgColor() const;
    KisPaintOpPresetSP currentPaintOpPreset() const;

    /// @return the channel lock flags of the current node with the global override applied
    QBitArray channelLockFlags() const;

    qreal effectiveZoom() const;
    bool presetAllowsLod() const;

    void setFGColorOverride(const KoColor &color);
    void setBGColorOverride(const KoColor &color);
    void setSelectionOverride(KisSelectionSP selection);
    void setBrush(const KisPaintOpPresetSP &brush);

private:
    struct Private;
    Private * const m_d;
};

typedef KisSharedPtr<KisResourcesSnapshot> KisResourcesSnapshotSP;


#endif /* __KIS_RESOURCES_SNAPSHOT_H */
