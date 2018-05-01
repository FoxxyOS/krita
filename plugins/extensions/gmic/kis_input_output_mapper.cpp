/*
 * Copyright (c) 2013 Lukáš Tvrdý <lukast.dev@gmail.com
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

#include <kis_input_output_mapper.h>
#include <kis_image.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>

KisInputOutputMapper::KisInputOutputMapper(KisImageWSP image, KisNodeSP activeNode):m_image(image),m_activeNode(activeNode)
{

}


KisNodeListSP KisInputOutputMapper::inputNodes(InputLayerMode inputMode)
{
/*
    ACTIVE_LAYER,
    ALL_LAYERS,
    ACTIVE_LAYER_BELOW_LAYER,
    ACTIVE_LAYER_ABOVE_LAYER,
    ALL_VISIBLE_LAYERS,
    ALL_INVISIBLE_LAYERS,
    ALL_VISIBLE_LAYERS_DECR,
    ALL_INVISIBLE_DECR,
    ALL_DECR
*/

    KisNodeListSP result(new QList< KisNodeSP >());
    switch (inputMode)
    {
        case ACTIVE_LAYER:
        {
            result->append(m_activeNode);
            break;// drop down in case of one more layer modes
        }
        case ACTIVE_LAYER_BELOW_LAYER:
        {
            result->append(m_activeNode);
            result->append(m_activeNode->prevSibling());
            break;
        }
        case ACTIVE_LAYER_ABOVE_LAYER:
        {
            result->append(m_activeNode);
            result->append(m_activeNode->nextSibling());
            break;
        }
        case NONE:
        case ALL_VISIBLE_LAYERS:
        case ALL_INVISIBLE_LAYERS:
        case ALL_VISIBLE_LAYERS_DECR:
        case ALL_INVISIBLE_DECR:
        {
            dbgPlugins << "Not implemented";
            break;
        }
        case ALL_LAYERS:
        {
            allLayers(result);
            break;
        }
        case ALL_DECR:
        {
            allInversedOrderedLayers(result);
            break;
        }
        default:
        {
            Q_ASSERT(false); // why here??
            break;
        }
    }
    return result;
}


void KisInputOutputMapper::allLayers(KisNodeListSP result)
{
    //TODO: hack ignores hierarchy introduced by group layers
    KisNodeSP root = m_image->rootLayer();
    KisNodeSP item = root->lastChild();
    while (item)
    {
        KisPaintLayer * paintLayer = dynamic_cast<KisPaintLayer*>(item.data());
        if (paintLayer)
        {
            result->append(item);
        }
        item = item->prevSibling();
    }
}


void KisInputOutputMapper::allInversedOrderedLayers(KisNodeListSP result)
{
    Q_UNUSED(result);
}
