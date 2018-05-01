/* This file is part of the KDE project
 * Copyright 2008 (C) Boudewijn Rempt <boud@valdyas.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef KIS_KRA_SAVER
#define KIS_KRA_SAVER

#include <kis_types.h>

class KisDocument;
class QDomElement;
class QDomDocument;
class KoStore;
class QString;
class QStringList;

class KisKraSaver
{
public:

    KisKraSaver(KisDocument* document);

    ~KisKraSaver();

    QDomElement saveXML(QDomDocument& doc,  KisImageWSP image);

    bool saveKeyframes(KoStore *store, const QString &uri, bool external);

    bool saveBinaryData(KoStore* store, KisImageWSP image, const QString & uri, bool external, bool includeMerge);

    /// @return a list with everthing that went wrong while saving
    QStringList errorMessages() const;

private:
    void saveBackgroundColor(QDomDocument& doc, QDomElement& element, KisImageWSP image);
    void saveWarningColor(QDomDocument& doc, QDomElement& element, KisImageWSP image);
    void saveCompositions(QDomDocument& doc, QDomElement& element, KisImageWSP image);
    bool saveAssistants(KoStore *store,QString uri, bool external);
    bool saveAssistantsList(QDomDocument& doc, QDomElement& element);
    bool saveGrid(QDomDocument& doc, QDomElement& element);
    bool saveGuides(QDomDocument& doc, QDomElement& element);
    bool saveAudio(QDomDocument& doc, QDomElement& element);
    bool saveNodeKeyframes(KoStore *store, QString location, const KisNode *node);
    struct Private;
    Private * const m_d;
};

#endif
