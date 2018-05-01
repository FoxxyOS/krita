/*
 * Copyright (c) 2016 Boudewijn Rempt <boud@valdyas.org>
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
#include "KisMimeDatabase.h"

#include <QMimeDatabase>
#include <QMimeType>
#include <QFileInfo>
#include <KritaPluginDebug.h>

#include <klocalizedstring.h>

QList<KisMimeDatabase::KisMimeType> KisMimeDatabase::s_mimeDatabase;


QString KisMimeDatabase::mimeTypeForFile(const QString &file)
{
    fillMimeData();

    QFileInfo fi(file);
    QString suffix = fi.suffix().toLower();
    Q_FOREACH(const KisMimeDatabase::KisMimeType &mimeType, s_mimeDatabase) {
        if (mimeType.suffixes.contains("*." + suffix)) {
            debugPlugin << "mimeTypeForFile(). KisMimeDatabase returned" << mimeType.mimeType << "for" << file;
            return mimeType.mimeType;
        }
    }

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(file);
    if (mime.name() != "application/octet-stream") {
        debugPlugin << "mimeTypeForFile(). QMimeDatabase returned" << mime.name() << "for" << file;
        return mime.name();
    }
    return "";
}

QString KisMimeDatabase::mimeTypeForSuffix(const QString &suffix)
{
    fillMimeData();
    QMimeDatabase db;

    QString s = suffix.toLower();
    if (!s.startsWith("*.")) {
        s = "*." + s;
    }

    Q_FOREACH(const KisMimeDatabase::KisMimeType &mimeType, s_mimeDatabase) {
        if (mimeType.suffixes.contains(s)) {
            debugPlugin << "mimeTypeForSuffix(). KisMimeDatabase returned" << mimeType.mimeType << "for" << s;
            return mimeType.mimeType;
        }
    }

    QMimeType mime = db.mimeTypeForFile(s);
    if (mime.name() != "application/octet-stream") {
        debugPlugin << "mimeTypeForSuffix(). QMimeDatabase returned" << mime.name() << "for" << s;
        return mime.name();
    }
    return "";
}

QString KisMimeDatabase::mimeTypeForData(const QByteArray ba)
{
    QMimeDatabase db;
    QMimeType mtp = db.mimeTypeForData(ba);
    debugPlugin << "mimeTypeForData(). QMimeDatabase returned" << mtp.name();
    return mtp.name();
}

QString KisMimeDatabase::descriptionForMimeType(const QString &mimeType)
{
    fillMimeData();

    Q_FOREACH(const KisMimeDatabase::KisMimeType &m, s_mimeDatabase) {
        if (m.mimeType == mimeType) {
            debugPlugin << "descriptionForMimeType. KisMimeDatabase returned" << m.description << "for" << mimeType;
            return m.description;
        }
    }

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(mimeType);
    if (mime.name() != "application/octet-stream") {
        debugPlugin  << "descriptionForMimeType. QMimeDatabase returned" << mime.comment() << "for" << mimeType;
        return mime.comment();
    }

    return mimeType;
}

QStringList KisMimeDatabase::suffixesForMimeType(const QString &mimeType)
{
    fillMimeData();
    Q_FOREACH(const KisMimeDatabase::KisMimeType &m, s_mimeDatabase) {
        if (m.mimeType == mimeType) {
            debugPlugin << "suffixesForMimeType. KisMimeDatabase returned" << m.suffixes;
            return m.suffixes;
        }
    }

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(mimeType);
    if (mime.name() != "application/octet-stream" && !mime.suffixes().isEmpty()) {
        QString preferredSuffix = mime.preferredSuffix();
        if (mimeType == "image/x-tga") {
            preferredSuffix = "tga";
        }
        QStringList suffixes = mime.suffixes();
        if (preferredSuffix != suffixes.first()) {
            suffixes.removeAll(preferredSuffix);
            suffixes.prepend(preferredSuffix);

        }
        debugPlugin << "suffixesForMimeType. QMimeDatabase returned" << suffixes;
        return suffixes;
    }
    return QStringList();
}

QString KisMimeDatabase::iconNameForMimeType(const QString &mimeType)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(mimeType);
    debugPlugin << "iconNameForMimeType" << mime.iconName();
    return mime.iconName();
}

void KisMimeDatabase::fillMimeData()
{
    // This should come from the import/export plugins, but the json files aren't translated,
    // which is bad for the description field
    if (s_mimeDatabase.isEmpty()) {

        KisMimeType mimeType;

        mimeType.mimeType = "image/x-gimp-brush";
        mimeType.description = i18nc("description of a file type", "Gimp Brush");
        mimeType.suffixes = QStringList() << "*.gbr" << "*.vbr";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "image/x-gimp-brush-animated";
        mimeType.description = i18nc("description of a file type", "Gimp Image Hose Brush");
        mimeType.suffixes = QStringList() << "*.gih";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "image/x-adobe-brushlibrary";
        mimeType.description = i18nc("description of a file type", "Adobe Brush Library");
        mimeType.suffixes = QStringList() << "*.abr";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "application/x-krita-paintoppreset";
        mimeType.description = i18nc("description of a file type", "Krita Brush Preset");
        mimeType.suffixes = QStringList() << "*.kpp";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "application/x-krita-assistant";
        mimeType.description = i18nc("description of a file type", "Krita Assistant");
        mimeType.suffixes = QStringList() << "*.paintingassistant";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "image/x-r16";
        mimeType.description = i18nc("description of a file type", "R16 Heightmap");
        mimeType.suffixes = QStringList() <<  "*.r16";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "image/x-r8";
        mimeType.description = i18nc("description of a file type", "R8 Heightmap");
        mimeType.suffixes = QStringList() << "*.r8";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "application/x-spriter";
        mimeType.description = i18nc("description of a file type", "Spriter SCML");
        mimeType.suffixes = QStringList() << "*.scml";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "image/x-svm";
        mimeType.description = i18nc("description of a file type", "Starview Metafile");
        mimeType.suffixes = QStringList() << "*.svm";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "image/openraster";
        mimeType.description = i18nc("description of a file type", "OpenRaster Image");
        mimeType.suffixes = QStringList() << "*.ora";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "application/x-photoshop-style-library";
        mimeType.description = i18nc("description of a file type", "Photoshop Layer Style Library");
        mimeType.suffixes = QStringList() << "*.asl";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "application/x-gimp-color-palette";
        mimeType.description = i18nc("description of a file type", "Color Palette");
        mimeType.suffixes = QStringList() << "*.gpl" << "*.pal" << "*.act" << "*.aco" << "*.colors" << "*.xml";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "application/x-opencolorio-configuration";
        mimeType.description = i18nc("description of a file type", "OpenColorIO Configuration");
        mimeType.suffixes = QStringList() << "*.ocio";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "application/x-krita-recorded-macro";
        mimeType.description = i18nc("description of a file type", "Krita Recorded Action");
        mimeType.suffixes = QStringList() << "*.krarec";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "application/x-gimp-gradient";
        mimeType.description = i18nc("description of a file type", "GIMP Gradients");
        mimeType.suffixes = QStringList() << "*.ggr";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "application/x-gimp-pattern";
        mimeType.description = i18nc("description of a file type", "GIMP Patterns");
        mimeType.suffixes = QStringList() << "*.pat";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "application/x-karbon-gradient";
        mimeType.description = i18nc("description of a file type", "Karbon Gradients");
        mimeType.suffixes = QStringList() << "*.kgr";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "application/x-krita-bundle";
        mimeType.description = i18nc("description of a file type", "Krita Resource Bundle");
        mimeType.suffixes = QStringList() << "*.bundle";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "application/x-krita-workspace";
        mimeType.description = i18nc("description of a file type", "Krita Workspace");
        mimeType.suffixes = QStringList() << "*.kws";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "application/x-krita-taskset";
        mimeType.description = i18nc("description of a file type", "Krita Taskset");
        mimeType.suffixes = QStringList() << "*.kts";
        s_mimeDatabase << mimeType;


        mimeType.mimeType = "image/x-krita-raw";
        mimeType.description = i18nc("description of a file type", "Camera Raw Files");
        mimeType.suffixes = QStringList() << "*.nef" << "*.cr2" << "*.sr2" << "*.crw" << "*.pef" << "*.x3f" << "*.kdc" << "*.mrw" << "*.arw" << "*.k25" << "*.dcr" << "*.orf" << "*.raw" << "*.raw" << "*.raf" << "*.srf" << "*.dng";
        s_mimeDatabase << mimeType;

        mimeType.mimeType = "application/x-extension-exr";
        mimeType.description = i18nc("description of a file type", "OpenEXR (Extended)");
        mimeType.suffixes = QStringList() << "*.exr";
        s_mimeDatabase << mimeType;



        debugPlugin << "Filled mimedatabase with" << s_mimeDatabase.count() << "special mimetypes";
    }
}
