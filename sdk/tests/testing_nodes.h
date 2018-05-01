/*
 *  Copyright (c) 2016 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef TESTING_NODES_H
#define TESTING_NODES_H

#include "kis_node.h"

namespace TestUtil {

struct DefaultNode : public KisNode {
    KisPaintDeviceSP paintDevice() const {
        return 0;
    }

    KisPaintDeviceSP original() const {
        return 0;
    }

    KisPaintDeviceSP projection() const {
        return 0;
    }

    bool allowAsChild(KisNodeSP) const {
        return true;
    }
    const KoColorSpace * colorSpace() const {
        return 0;
    }
    virtual const KoCompositeOp * compositeOp() const {
        return 0;
    }
};

}

#endif // TESTING_NODES_H

