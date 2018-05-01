/*
 *  Copyright (c) 2014 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef KIS_LS_BEVEL_EMBOSS_FILTER_H
#define KIS_LS_BEVEL_EMBOSS_FILTER_H

#include <QSharedPointer>

#include "kis_layer_style_filter.h"
#include <kritaimage_export.h>

struct psd_layer_effects_bevel_emboss;


class KRITAIMAGE_EXPORT KisLsBevelEmbossFilter : public KisLayerStyleFilter
{
public:
    KisLsBevelEmbossFilter();

    void processDirectly(KisPaintDeviceSP src,
                         KisMultipleProjection *dst,
                         const QRect &applyRect,
                         KisPSDLayerStyleSP style,
                         KisLayerStyleFilterEnvironment *env) const;

    QRect neededRect(const QRect & rect, KisPSDLayerStyleSP style, KisLayerStyleFilterEnvironment *env) const;
    QRect changedRect(const QRect & rect, KisPSDLayerStyleSP style, KisLayerStyleFilterEnvironment *env) const;

private:
    void applyBevelEmboss(KisPaintDeviceSP srcDevice,
                          KisMultipleProjection *dst,
                          const QRect &applyRect,
                          const psd_layer_effects_bevel_emboss *config,
                          KisLayerStyleFilterEnvironment *env) const;
};

#endif
