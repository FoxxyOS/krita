/*
 *  Copyright (c) 2013 Dmitry Kazakov <dimula73@gmail.com>
 *  Copyright (c) 2013 Lukáš Tvrdý <lukast.dev@gmail.com>
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

#ifndef __KIS_GMIC_COMMAND_H
#define __KIS_GMIC_COMMAND_H

#include <gmic.h>

#include <QSharedPointer>

#include <kundo2command.h>
#include <kis_node.h>
#include "kis_types.h"

#include "kis_gmic_data.h"

#include <gmic.h>

class QString;

class KisGmicCommand : public QObject, public KUndo2Command
{
    Q_OBJECT
public:
    KisGmicCommand(const QString &gmicCommandString, QSharedPointer< gmic_list<float> > images, KisGmicDataSP data, const QByteArray &customCommands = QByteArray());
    virtual ~KisGmicCommand();

    void undo();
    void redo();

    /* @return true if gmic failed in redo () */
    bool isSuccessfullyDone();

Q_SIGNALS:
    void gmicFinished(bool successfully, int miliseconds = -1, const QString &msg = QString());

private:
    static QString gmicDimensionString(const gmic_image<float>& img);

private:
    const QString m_gmicCommandString;
    QSharedPointer<gmic_list<float> > m_images;
    KisGmicDataSP m_data;
    const QByteArray m_customCommands;
    bool m_firstRedo;
    bool m_isSuccessfullyDone;
};

#endif /* __KIS_GMIC_COMMAND_H */
