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

#ifndef __KIS_MODEL_INDEX_CONVERTER_H
#define __KIS_MODEL_INDEX_CONVERTER_H

#include "kis_model_index_converter_base.h"

class KisDummiesFacadeBase;
class KisNodeModel;


/**
 * The class for converting to/from QModelIndex and KisNodeDummy when
 * the root node is *hidden* (ShowRootLayer == *false*). All the selection
 * masks owned by the root layer are hidden as well.
 */

class KRITAUI_EXPORT KisModelIndexConverter : public KisModelIndexConverterBase
{
public:
    KisModelIndexConverter(KisDummiesFacadeBase *dummiesFacade,
                           KisNodeModel *model,
                           bool showGlobalSelection);

    KisNodeDummy* dummyFromRow(int row, QModelIndex parent);
    KisNodeDummy* dummyFromIndex(QModelIndex index);

    QModelIndex indexFromDummy(KisNodeDummy *dummy);
    bool indexFromAddedDummy(KisNodeDummy *parentDummy, int index,
                             const QString &newNodeMetaObjectType,
                             QModelIndex &parentIndex, int &row);

    int rowCount(QModelIndex parent);

private:
    inline bool checkDummyType(KisNodeDummy *dummy);
    inline bool checkDummyMetaObjectType(const QString &type);

private:
    KisDummiesFacadeBase *m_dummiesFacade;
    KisNodeModel *m_model;
    bool m_showGlobalSelection;
};

#endif /* __KIS_MODEL_INDEX_CONVERTER_H */
