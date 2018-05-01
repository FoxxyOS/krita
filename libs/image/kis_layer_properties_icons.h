/*
 *  Copyright (c) 2015 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_LAYER_PROPERTIES_ICONS_H
#define __KIS_LAYER_PROPERTIES_ICONS_H

#include <QScopedPointer>
#include <KoID.h>

#include <kis_base_node.h>
#include "kritaimage_export.h"

class KRITAIMAGE_EXPORT KisLayerPropertiesIcons
{
public:
    KisLayerPropertiesIcons();
    ~KisLayerPropertiesIcons();

    static const KoID locked;
    static const KoID visible;
    static const KoID layerStyle;
    static const KoID inheritAlpha;
    static const KoID alphaLocked;
    static const KoID onionSkins;
    static const KoID passThrough;
    static const KoID selectionActive;
    static const KoID colorLabelIndex;
    static const KoID colorizeNeedsUpdate;
    static const KoID colorizeEditKeyStrokes;
    static const KoID colorizeShowColoring;

    static KisLayerPropertiesIcons* instance();

    static KisBaseNode::Property getProperty(const KoID &id, bool state);
    static KisBaseNode::Property getProperty(const KoID &id, bool state,
                                              bool isInStasis, bool stateInStasis);

    /**
     * Sets the specified property of the node and updates it
     */
    static void setNodeProperty(KisNodeSP node, const KoID &id, const QVariant &value, KisImageSP image);

    static void setNodeProperty(KisBaseNode::PropertyList *props, const KoID &id, const QVariant &value);

    /**
     * Gets the specified property of the node
     */
    static QVariant nodeProperty(KisNodeSP node, const KoID &id, const QVariant &defaultValue);

private:
    void updateIcons();

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif /* __KIS_LAYER_PROPERTIES_ICONS_H */
