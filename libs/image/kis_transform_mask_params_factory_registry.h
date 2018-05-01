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

#ifndef __KIS_TRANSFORM_MASK_PARAMS_FACTORY_REGISTRY_H
#define __KIS_TRANSFORM_MASK_PARAMS_FACTORY_REGISTRY_H

#include <QMap>

#include <functional>

#include "kis_types.h"
#include "kritaimage_export.h"


class QDomElement;


using KisTransformMaskParamsFactory    = std::function<KisTransformMaskParamsInterfaceSP (const QDomElement &)>;
using KisTransformMaskParamsFactoryMap = QMap<QString, KisTransformMaskParamsFactory>;
using KisAnimatedTransformMaskParamsFactory = std::function<KisTransformMaskParamsInterfaceSP (KisTransformMaskParamsInterfaceSP)>;
using KisTransformMaskKeyframeFactory = std::function<void (KisTransformMaskSP, int, KisTransformMaskParamsInterfaceSP, KUndo2Command*)>;

class KRITAIMAGE_EXPORT KisTransformMaskParamsFactoryRegistry
{

public:
    KisTransformMaskParamsFactoryRegistry();
    ~KisTransformMaskParamsFactoryRegistry();

    void addFactory(const QString &id, const KisTransformMaskParamsFactory &factory);
    KisTransformMaskParamsInterfaceSP createParams(const QString &id, const QDomElement &e);

    void setAnimatedParamsFactory(const KisAnimatedTransformMaskParamsFactory &factory);
    KisTransformMaskParamsInterfaceSP animateParams(KisTransformMaskParamsInterfaceSP params);

    void setKeyframeFactory(const KisTransformMaskKeyframeFactory &factory);
    void autoAddKeyframe(KisTransformMaskSP mask, int time, KisTransformMaskParamsInterfaceSP params, KUndo2Command *parentCommand);

    static KisTransformMaskParamsFactoryRegistry* instance();

private:
    KisTransformMaskParamsFactoryMap m_map;
    KisAnimatedTransformMaskParamsFactory m_animatedParamsFactory;
    KisTransformMaskKeyframeFactory m_keyframeFactory;
};

#endif /* __KIS_TRANSFORM_MASK_PARAMS_FACTORY_REGISTRY_H */
