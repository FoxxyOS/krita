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

#ifndef __KIS_COMPOSITE_PROGRESS_PROXY_H
#define __KIS_COMPOSITE_PROGRESS_PROXY_H

#include <QList>
#include <KoProgressProxy.h>
#include "kritaimage_export.h"


class KRITAIMAGE_EXPORT KisCompositeProgressProxy : public KoProgressProxy
{
public:
    void addProxy(KoProgressProxy *proxy);
    void removeProxy(KoProgressProxy *proxy);

    int maximum() const;
    void setValue(int value);
    void setRange(int minimum, int maximum);
    void setFormat(const QString &format);

private:
    QList<KoProgressProxy*> m_proxies;
    QList<KoProgressProxy*> m_uniqueProxies;
};

#endif /* __KIS_COMPOSITE_PROGRESS_PROXY_H */
