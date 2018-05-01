/*
 *  Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2014 Sven Langkamp <sven.langkamp@gmail.com>
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

#ifndef KIS_MIRROR_MANAGER_H
#define KIS_MIRROR_MANAGER_H

#include <QObject>
#include <QPointer>

#include "KisView.h"

class KisViewManager;
class KActionCollection;
class KisMirrorAxis;


class KisMirrorManager : public QObject
{
    Q_OBJECT

public:
    KisMirrorManager(KisViewManager* view);
    virtual ~KisMirrorManager();

    void setup(KActionCollection* collection);

    void setView(QPointer<KisView> imageView);

private Q_SLOTS:
    void updateAction();

private:

    QPointer<KisView> m_imageView;
    QAction *m_mirrorCanvas;
    KisMirrorAxis* hasDecoration();

};

#endif // KIS_PAINTING_ASSISTANTS_MANAGER_H
