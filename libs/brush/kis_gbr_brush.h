/*
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
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
#ifndef KIS_GBR_BRUSH_
#define KIS_GBR_BRUSH_

#include <QImage>
#include <QVector>

#include "kis_scaling_size_brush.h"
#include <kis_types.h>
#include <kis_shared.h>
#include <brushengine/kis_paint_information.h>

#include "kritabrush_export.h"

class KisQImagemask;
typedef KisSharedPtr<KisQImagemask> KisQImagemaskSP;

class QString;
class QIODevice;

class BRUSH_EXPORT KisGbrBrush : public KisScalingSizeBrush
{

protected:

public:

    /// Construct brush to load filename later as brush
    KisGbrBrush(const QString& filename);

    /// Load brush from the specified data, at position dataPos, and set the filename
    KisGbrBrush(const QString& filename,
                const QByteArray & data,
                qint32 & dataPos);

    /// Load brush from the specified paint device, in the specified region
    KisGbrBrush(KisPaintDeviceSP image, int x, int y, int w, int h);

    /// Load brush as a copy from the specified QImage (handy when you need to copy a brush!)
    KisGbrBrush(const QImage& image, const QString& name = QString());

    virtual ~KisGbrBrush();

    virtual bool load();
    virtual bool loadFromDevice(QIODevice *dev);
    virtual bool save();
    virtual bool saveToDevice(QIODevice* dev) const;

    /**
     * @return a preview of the brush
     */
    virtual QImage brushTipImage() const;
    /**
     * If the brush image data are colorful (e.g. you created the brush from the canvas with custom brush)
     * and you want to paint with it as with masks, set to true.
     */
    virtual void setUseColorAsMask(bool useColorAsMask);

    virtual bool useColorAsMask() const;

    /**
     * Convert the mask to inverted gray scale, so it is alpha mask.
     * It can be used as MASK brush type. This operates on the date of the brush,
     * so it destruct the original brush data
     */
    virtual void makeMaskImage();

    virtual enumBrushType brushType() const;

    /**
     * Makes a copy of this brush.
     */
    virtual KisBrush* clone() const;

    /**
     * @return default file extension for saving the brush
     */
    virtual QString defaultFileExtension() const;

protected:
    /**
     * save the content of this brush to an IO device
     */
    friend class KisImageBrushesPipe;


    KisGbrBrush(const KisGbrBrush& rhs);

    void setBrushType(enumBrushType type);
    virtual void setBrushTipImage(const QImage& image);

    void toXML(QDomDocument& d, QDomElement& e) const;

private:

    bool init();
    bool initFromPaintDev(KisPaintDeviceSP image, int x, int y, int w, int h);

    struct Private;
    Private* const d;
};

#endif // KIS_GBR_BRUSH_

