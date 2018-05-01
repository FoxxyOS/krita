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

#ifndef __KIS_TRANSFORM_MASK_ADAPTER_H
#define __KIS_TRANSFORM_MASK_ADAPTER_H

#include <QScopedPointer>
#include "kis_transform_mask_params_interface.h"
#include "kritatooltransform_export.h"

class ToolTransformArgs;


class KRITATOOLTRANSFORM_EXPORT KisTransformMaskAdapter : public KisTransformMaskParamsInterface
{
public:
    KisTransformMaskAdapter(const ToolTransformArgs &args);
    ~KisTransformMaskAdapter();

    QTransform finalAffineTransform() const;
    bool isAffine() const;
    bool isHidden() const;

    void transformDevice(KisNodeSP node, KisPaintDeviceSP src, KisPaintDeviceSP dst) const;

    virtual const ToolTransformArgs& transformArgs() const;

    QString id() const;
    void toXML(QDomElement *e) const;
    static KisTransformMaskParamsInterfaceSP fromXML(const QDomElement &e);

    virtual void translate(const QPointF &offset);

    QRect nonAffineChangeRect(const QRect &rc);
    QRect nonAffineNeedRect(const QRect &rc, const QRect &srcBounds);

    bool isAnimated() const;
    KisKeyframeChannel *getKeyframeChannel(const QString &id, KisDefaultBoundsBaseSP defaultBounds);
    void clearChangedFlag();
    bool hasChanged() const;

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif /* __KIS_TRANSFORM_MASK_ADAPTER_H */
