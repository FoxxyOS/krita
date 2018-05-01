/*
 *  Copyright (c) 2005 Adrian Page <adrian@pagenet.plus.com>
 *  Copyright (c) 2010 Cyrille Berger <cberger@cberger.net>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "exr_converter.h"

#include <half.h>

#include <ImfAttribute.h>
#include <ImfChannelList.h>
#include <ImfInputFile.h>
#include <ImfOutputFile.h>

#include <ImfStringAttribute.h>
#include "exr_extra_tags.h"

#include <QApplication>
#include <QMessageBox>

#include <QFileInfo>

#include <KoColorSpaceRegistry.h>
#include <KoCompositeOpRegistry.h>
#include <KoColorSpaceTraits.h>
#include <KoColorModelStandardIds.h>
#include <KoColor.h>

#include <KisDocument.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kis_transaction.h>
#include "kis_iterator_ng.h"
#include "kra/kis_kra_savexml_visitor.h"
#include <kis_exr_layers_sorter.h>

#include <metadata/kis_meta_data_entry.h>
#include <metadata/kis_meta_data_schema.h>
#include <metadata/kis_meta_data_schema_registry.h>
#include <metadata/kis_meta_data_store.h>
#include <metadata/kis_meta_data_value.h>

// Do not translate!
#define HDR_LAYER "HDR Layer"

template<typename _T_>
struct Rgba {
    _T_ r;
    _T_ g;
    _T_ b;
    _T_ a;
};

struct ExrGroupLayerInfo;

struct ExrLayerInfoBase {
    ExrLayerInfoBase() : colorSpace(0), parent(0) {
    }
    const KoColorSpace* colorSpace;
    QString name;
    const ExrGroupLayerInfo* parent;
};

struct ExrGroupLayerInfo : public ExrLayerInfoBase {
    ExrGroupLayerInfo() : groupLayer(0) {}
    KisGroupLayerSP groupLayer;
};

enum ImageType {
    IT_UNKNOWN,
    IT_FLOAT16,
    IT_FLOAT32,
    IT_UNSUPPORTED
};

struct ExrPaintLayerInfo : public ExrLayerInfoBase {
    ExrPaintLayerInfo()
        : imageType(IT_UNKNOWN)
    {
    }

    ImageType imageType;
    QMap< QString, QString> channelMap; ///< first is either R, G, B or A second is the EXR channel name

    struct Remap {
        Remap(const QString& _original, const QString& _current) : original(_original), current(_current) {
        }
        QString original;
        QString current;
    };

    QList< Remap > remappedChannels; ///< this is used to store in the metadata the mapping between exr channel name, and channels used in Krita
    void updateImageType(ImageType channelType);
};

void ExrPaintLayerInfo::updateImageType(ImageType channelType)
{
    if (imageType == IT_UNKNOWN) {
        imageType = channelType;
    }
    else if (imageType != channelType) {
        imageType = IT_UNSUPPORTED;
    }
}

struct ExrPaintLayerSaveInfo {
    QString name; ///< name of the layer with a "." at the end (ie "group1.group2.layer1.")
    KisPaintLayerSP layer;
    QList<QString> channels;
    Imf::PixelType pixelType;
};

struct exrConverter::Private {
    Private() : doc(0), warnedAboutChangedAlpha(false),
        showNotifications(false) {}

    KisImageSP image;
    KisDocument *doc;

    bool warnedAboutChangedAlpha;
    bool showNotifications;


    template <class WrapperType>
    void unmultiplyAlpha(typename WrapperType::pixel_type *pixel);

    template<typename _T_>
    void decodeData4(Imf::InputFile& file, ExrPaintLayerInfo& info, KisPaintLayerSP layer, int width, int xstart, int ystart, int height, Imf::PixelType ptype);

    template<typename _T_>
    void decodeData1(Imf::InputFile& file, ExrPaintLayerInfo& info, KisPaintLayerSP layer, int width, int xstart, int ystart, int height, Imf::PixelType ptype);


    QDomDocument loadExtraLayersInfo(const Imf::Header &header);
    bool checkExtraLayersInfoConsistent(const QDomDocument &doc, std::set<std::string> exrLayerNames);
    void makeLayerNamesUnique(QList<ExrPaintLayerSaveInfo>& informationObjects);
    void recBuildPaintLayerSaveInfo(QList<ExrPaintLayerSaveInfo>& informationObjects, const QString& name, KisGroupLayerSP parent);
    void reportLayersNotSaved(const QSet<KisNodeSP> &layersNotSaved);
    QString fetchExtraLayersInfo(QList<ExrPaintLayerSaveInfo>& informationObjects);
};

exrConverter::exrConverter(KisDocument *doc, bool showNotifications)
    : m_d(new Private)
{
    m_d->doc = doc;
    m_d->showNotifications = showNotifications;
}

exrConverter::~exrConverter()
{
}

ImageType imfTypeToKisType(Imf::PixelType type)
{
    switch (type) {
    case Imf::UINT:
    case Imf::NUM_PIXELTYPES:
        return IT_UNSUPPORTED;
    case Imf::HALF:
        return IT_FLOAT16;
    case Imf::FLOAT:
        return IT_FLOAT32;
    default:
        qFatal("Out of bound enum");
        return IT_UNKNOWN;
    }
}

const KoColorSpace* kisTypeToColorSpace(QString model, ImageType imageType)
{
    switch (imageType) {
    case IT_FLOAT16:
        return KoColorSpaceRegistry::instance()->colorSpace(model, Float16BitsColorDepthID.id(), "");
    case IT_FLOAT32:
        return KoColorSpaceRegistry::instance()->colorSpace(model, Float32BitsColorDepthID.id(), "");
    case IT_UNKNOWN:
    case IT_UNSUPPORTED:
        return 0;
    default:
        qFatal("Out of bound enum");
        return 0;
    }
}

template <typename T>
static inline T alphaEpsilon()
{
    return static_cast<T>(HALF_EPSILON);
}

template <typename T>
static inline T alphaNoiseThreshold()
{
    return static_cast<T>(0.01); // 1%
}

template <typename T>
struct RgbPixelWrapper
{
    typedef T channel_type;
    typedef Rgba<T> pixel_type;

    RgbPixelWrapper(Rgba<T> &_pixel) : pixel(_pixel) {}

    inline T alpha() const {
        return pixel.a;
    }

    inline bool checkMultipliedColorsConsistent() const {
        return !(pixel.a < alphaEpsilon<T>() &&
                 (pixel.r > 0.0 ||
                  pixel.g > 0.0 ||
                  pixel.b > 0.0));
    }

    inline bool checkUnmultipliedColorsConsistent(const Rgba<T> &mult) const {
        const T alpha = pixel.a;

        return abs(alpha) >= alphaNoiseThreshold<T>() ||
                (pixel.r * alpha == mult.r &&
                 pixel.g * alpha == mult.g &&
                 pixel.b * alpha == mult.b);
    }

    inline void setUnmultiplied(const Rgba<T> &mult, qreal newAlpha) {
        pixel.r = mult.r / newAlpha;
        pixel.g = mult.g / newAlpha;
        pixel.b = mult.b / newAlpha;
        pixel.a = newAlpha;
    }

    Rgba<T> &pixel;
};

template <typename T>
struct GrayPixelWrapper
{
    typedef T channel_type;
    typedef typename KoGrayTraits<T>::Pixel pixel_type;

    GrayPixelWrapper(pixel_type &_pixel) : pixel(_pixel) {}

    inline T alpha() const {
        return pixel.alpha;
    }

    inline bool checkMultipliedColorsConsistent() const {
        return !(pixel.alpha < alphaEpsilon<T>() &&
                 pixel.gray > 0.0);
    }

    inline bool checkUnmultipliedColorsConsistent(const pixel_type &mult) const {
        const T alpha = pixel.alpha;

        return alpha >= alphaNoiseThreshold<T>() ||
                pixel.gray * alpha == mult.gray;
    }

    inline void setUnmultiplied(const pixel_type &mult, qreal newAlpha) {
        pixel.gray = mult.gray / newAlpha;
        pixel.alpha = newAlpha;
    }

    pixel_type &pixel;
};

template <class WrapperType>
void exrConverter::Private::unmultiplyAlpha(typename WrapperType::pixel_type *pixel)
{
    typedef typename WrapperType::pixel_type pixel_type;
    typedef typename WrapperType::channel_type channel_type;

    WrapperType srcPixel(*pixel);

    if (!srcPixel.checkMultipliedColorsConsistent()) {

        bool alphaWasModified = false;
        channel_type newAlpha = srcPixel.alpha();

        pixel_type __dstPixelData;
        WrapperType dstPixel(__dstPixelData);

        /**
         * Division by a tiny alpha may result in an overflow of half
         * value. That is why we use safe iterational approach.
         */
        while (1) {
            dstPixel.setUnmultiplied(srcPixel.pixel, newAlpha);

            if (dstPixel.checkUnmultipliedColorsConsistent(srcPixel.pixel)) {
                break;
            }

            newAlpha += alphaEpsilon<channel_type>();
            alphaWasModified = true;
        }

        *pixel = dstPixel.pixel;

        if (alphaWasModified &&
                !this->warnedAboutChangedAlpha) {

            QString msg =
                    i18nc("@info",
                          "The image contains pixels with zero alpha channel and non-zero "
                          "color channels. Krita will have to modify those pixels to have "
                          "at least some alpha. The initial values will <i>not</i> "
                          "be reverted on saving the image back."
                          "<br/><br/>"
                          "This will hardly make any visual difference just keep it in mind."
                          "<br/><br/>"
                          "<note>Modified alpha will have a range from %1 to %2</note>",
                          alphaEpsilon<channel_type>(),
                          alphaNoiseThreshold<channel_type>());

            if (this->showNotifications) {
                QMessageBox::information(0, i18nc("@title:window", "EXR image will be modified"), msg);
            } else {
                warnKrita << "WARNING:" << msg;
            }

            this->warnedAboutChangedAlpha = true;
        }

    } else if (srcPixel.alpha() > 0.0) {
        srcPixel.setUnmultiplied(srcPixel.pixel, srcPixel.alpha());
    }
}

template <typename T, typename Pixel, int size, int alphaPos>
void multiplyAlpha(Pixel *pixel)
{
    if (alphaPos >= 0) {
        T alpha = pixel->data[alphaPos];

        if (alpha > 0.0) {
            for (int i = 0; i < size; ++i) {
                if (i != alphaPos) {
                    pixel->data[i] *= alpha;
                }
            }

            pixel->data[alphaPos] = alpha;
        }
    }
}

template<typename _T_>
void exrConverter::Private::decodeData4(Imf::InputFile& file, ExrPaintLayerInfo& info, KisPaintLayerSP layer, int width, int xstart, int ystart, int height, Imf::PixelType ptype)
{
    typedef Rgba<_T_> Rgba;

    QVector<Rgba> pixels(width);

    bool hasAlpha = info.channelMap.contains("A");

    for (int y = 0; y < height; ++y) {
        Imf::FrameBuffer frameBuffer;
        Rgba* frameBufferData = (pixels.data()) - xstart - (ystart + y) * width;
        frameBuffer.insert(info.channelMap["R"].toLatin1().constData(),
                Imf::Slice(ptype, (char *) &frameBufferData->r,
                           sizeof(Rgba) * 1,
                           sizeof(Rgba) * width));
        frameBuffer.insert(info.channelMap["G"].toLatin1().constData(),
                Imf::Slice(ptype, (char *) &frameBufferData->g,
                           sizeof(Rgba) * 1,
                           sizeof(Rgba) * width));
        frameBuffer.insert(info.channelMap["B"].toLatin1().constData(),
                Imf::Slice(ptype, (char *) &frameBufferData->b,
                           sizeof(Rgba) * 1,
                           sizeof(Rgba) * width));
        if (hasAlpha) {
            frameBuffer.insert(info.channelMap["A"].toLatin1().constData(),
                    Imf::Slice(ptype, (char *) &frameBufferData->a,
                               sizeof(Rgba) * 1,
                               sizeof(Rgba) * width));
        }

        file.setFrameBuffer(frameBuffer);
        file.readPixels(ystart + y);
        Rgba *rgba = pixels.data();
        KisHLineIteratorSP it = layer->paintDevice()->createHLineIteratorNG(0, y, width);
        do {

            if (hasAlpha) {
                unmultiplyAlpha<RgbPixelWrapper<_T_> >(rgba);
            }

            typename KoRgbTraits<_T_>::Pixel* dst = reinterpret_cast<typename KoRgbTraits<_T_>::Pixel*>(it->rawData());

            dst->red = rgba->r;
            dst->green = rgba->g;
            dst->blue = rgba->b;
            if (hasAlpha) {
                dst->alpha = rgba->a;
            } else {
                dst->alpha = 1.0;
            }


            ++rgba;
        } while (it->nextPixel());
    }

}

template<typename _T_>
void exrConverter::Private::decodeData1(Imf::InputFile& file, ExrPaintLayerInfo& info, KisPaintLayerSP layer, int width, int xstart, int ystart, int height, Imf::PixelType ptype)
{
    typedef typename GrayPixelWrapper<_T_>::channel_type channel_type;
    typedef typename GrayPixelWrapper<_T_>::pixel_type pixel_type;

    KIS_ASSERT_RECOVER_RETURN(
                layer->paintDevice()->colorSpace()->colorModelId() == GrayAColorModelID);

    QVector<pixel_type> pixels(width);

    Q_ASSERT(info.channelMap.contains("G"));
    dbgFile << "G -> " << info.channelMap["G"];

    bool hasAlpha = info.channelMap.contains("A");
    dbgFile << "Has Alpha:" << hasAlpha;


    for (int y = 0; y < height; ++y) {
        Imf::FrameBuffer frameBuffer;
        pixel_type* frameBufferData = (pixels.data()) - xstart - (ystart + y) * width;
        frameBuffer.insert(info.channelMap["G"].toLatin1().constData(),
                Imf::Slice(ptype, (char *) &frameBufferData->gray,
                           sizeof(pixel_type) * 1,
                           sizeof(pixel_type) * width));

        if (hasAlpha) {
            frameBuffer.insert(info.channelMap["A"].toLatin1().constData(),
                    Imf::Slice(ptype, (char *) &frameBufferData->alpha,
                               sizeof(pixel_type) * 1,
                               sizeof(pixel_type) * width));
        }

        file.setFrameBuffer(frameBuffer);
        file.readPixels(ystart + y);

        pixel_type *srcPtr = pixels.data();
        KisHLineIteratorSP it = layer->paintDevice()->createHLineIteratorNG(0, y, width);
        do {

            if (hasAlpha) {
                unmultiplyAlpha<GrayPixelWrapper<_T_> >(srcPtr);
            }

            pixel_type* dstPtr = reinterpret_cast<pixel_type*>(it->rawData());

            dstPtr->gray = srcPtr->gray;
            dstPtr->alpha = hasAlpha ? srcPtr->alpha : channel_type(1.0);

            ++srcPtr;
        } while (it->nextPixel());
    }

}

bool recCheckGroup(const ExrGroupLayerInfo& group, QStringList list, int idx1, int idx2)
{
    if (idx1 > idx2) return true;
    if (group.name == list[idx2]) {
        return recCheckGroup(*group.parent, list, idx1, idx2 - 1);
    }
    return false;
}

ExrGroupLayerInfo* searchGroup(QList<ExrGroupLayerInfo>* groups, QStringList list, int idx1, int idx2)
{
    if (idx1 > idx2) {
        return 0;
    }
    // Look for the group
    for (int i = 0; i < groups->size(); ++i) {
        if (recCheckGroup(groups->at(i), list, idx1, idx2)) {
            return &(*groups)[i];
        }
    }
    // Create the group
    ExrGroupLayerInfo info;
    info.name = list.at(idx2);
    info.parent = searchGroup(groups, list, idx1, idx2 - 1);
    groups->append(info);
    return &groups->last();
}

QDomDocument exrConverter::Private::loadExtraLayersInfo(const Imf::Header &header)
{
    const Imf::StringAttribute *layersInfoAttribute =
            header.findTypedAttribute<Imf::StringAttribute>(EXR_KRITA_LAYERS);

    if (!layersInfoAttribute) return QDomDocument();

    QString layersInfoString = QString::fromUtf8(layersInfoAttribute->value().c_str());

    QDomDocument doc;
    doc.setContent(layersInfoString);

    return doc;
}

bool exrConverter::Private::checkExtraLayersInfoConsistent(const QDomDocument &doc, std::set<std::string> exrLayerNames)
{
    std::set<std::string> extraInfoLayers;

    QDomElement root = doc.documentElement();

    KIS_ASSERT_RECOVER(!root.isNull() && root.hasChildNodes()) { return false; };

    QDomElement el = root.firstChildElement();

    while(!el.isNull()) {
        KIS_ASSERT_RECOVER(el.hasAttribute(EXR_NAME)) { return false; };
        QString layerName = el.attribute(EXR_NAME).toUtf8();
        if (layerName != QString(HDR_LAYER)) {
            extraInfoLayers.insert(el.attribute(EXR_NAME).toUtf8().constData());
        }
        el = el.nextSiblingElement();
    }

    bool result = (extraInfoLayers == exrLayerNames);

    if (!result) {
        dbgKrita << "WARINING: Krita EXR extra layers info is inconsistent!";
        dbgKrita << ppVar(extraInfoLayers.size()) << ppVar(exrLayerNames.size());

        std::set<std::string>::const_iterator it1 = extraInfoLayers.begin();
        std::set<std::string>::const_iterator it2 = exrLayerNames.begin();

        std::set<std::string>::const_iterator end1 = extraInfoLayers.end();

        for (; it1 != end1; ++it1, ++it2) {
            dbgKrita << it1->c_str() << it2->c_str();
        }

    }

    return result;
}

KisImageBuilder_Result exrConverter::decode(const QString &filename)
{
    Imf::InputFile file(QFile::encodeName(filename));

    Imath::Box2i dw = file.header().dataWindow();
    int width = dw.max.x - dw.min.x + 1;
    int height = dw.max.y - dw.min.y + 1;
    int dx = dw.min.x;
    int dy = dw.min.y;

    // Display the attributes of a file
    for (Imf::Header::ConstIterator it = file.header().begin();
         it != file.header().end(); ++it) {
        dbgFile << "Attribute: " << it.name() << " type: " << it.attribute().typeName();
    }

    // fetch Krita's extra layer info, which might have been stored previously
    QDomDocument extraLayersInfo = m_d->loadExtraLayersInfo(file.header());

    // Construct the list of LayerInfo

    QList<ExrPaintLayerInfo> informationObjects;
    QList<ExrGroupLayerInfo> groups;

    ImageType imageType = IT_UNKNOWN;

    const Imf::ChannelList &channels = file.header().channels();
    std::set<std::string> layerNames;
    channels.layers(layerNames);

    if (!extraLayersInfo.isNull() &&
            !m_d->checkExtraLayersInfoConsistent(extraLayersInfo, layerNames)) {

        // it is inconsistent anyway
        extraLayersInfo = QDomDocument();
    }

    // Check if there are A, R, G, B channels

    dbgFile << "Checking for ARGB channels, they can occur in single-layer _or_ multi-layer images:";
    ExrPaintLayerInfo info;
    bool topLevelRGBFound = false;
    info.name = HDR_LAYER;

    for (Imf::ChannelList::ConstIterator i = channels.begin(); i != channels.end(); ++i) {
        const Imf::Channel &channel = i.channel();
        dbgFile << "Channel name = " << i.name() << " type = " << channel.type;

        QString qname = i.name();
        QStringList topLevelChannelNames = QStringList() << "A" << "R" << "G" << "B"  << ".A" << ".R" << ".G" << ".B"  << "A." << "R." << "G." << "B.";;
        if (topLevelChannelNames.contains(qname)) {
            topLevelRGBFound = true;
            dbgFile << "Found top-level channel" << qname;
            info.channelMap[qname] = qname;
            info.updateImageType(imfTypeToKisType(channel.type));
        }
        // Channel names that don't contain a "." or that contain a
        // "." only at the beginning or at the end are not considered
        // to be part of any layer.
        else if (!qname.contains('.')
                 || !qname.mid(1).contains('.')
                 || !qname.left(qname.size() - 1).contains('.')) {
            warnFile << "Found a top-level channel that is not part of the rendered image" << qname << ". Krita will not load this channel.";
        }
    }
    if (topLevelRGBFound) {
        dbgFile << "Toplevel layer" << info.name << ":Image type:" << imageType << "Layer type" << info.imageType;
        informationObjects.push_back(info);
        imageType = info.imageType;
    }

    dbgFile << "Extra layers:" << layerNames.size();

    for (std::set<std::string>::const_iterator i = layerNames.begin();i != layerNames.end(); ++i) {

        info = ExrPaintLayerInfo();

        dbgFile << "layer name = " << i->c_str();
        info.name = i->c_str();
        Imf::ChannelList::ConstIterator layerBegin, layerEnd;
        channels.channelsInLayer(*i, layerBegin, layerEnd);
        for (Imf::ChannelList::ConstIterator j = layerBegin;
             j != layerEnd; ++j) {
            const Imf::Channel &channel = j.channel();
            dbgFile << "\tchannel " << j.name() << " type = " << channel.type;

            info.updateImageType(imfTypeToKisType(channel.type));

            QString qname = j.name();
            QStringList list = qname.split('.');
            QString layersuffix = list.last();

            if (list.size() > 1) {
                info.name = list[list.size()-2];
                info.parent = searchGroup(&groups, list, 0, list.size() - 3);
            }

            info.channelMap[layersuffix] = qname;
        }
        if (info.imageType != IT_UNKNOWN && info.imageType != IT_UNSUPPORTED) {
            informationObjects.push_back(info);
            if (imageType < info.imageType) {
                imageType = info.imageType;
            }
        }
    }

    dbgFile << "File has" << informationObjects.size() << "layer(s)";

    // Set the colorspaces
    for (int i = 0; i < informationObjects.size(); ++i) {
        ExrPaintLayerInfo& info = informationObjects[i];
        QString modelId;

        if (info.channelMap.size() == 1) {
            modelId = GrayAColorModelID.id();
            QString key = info.channelMap.begin().key();
            if (key != "G") {
                info.remappedChannels.push_back(ExrPaintLayerInfo::Remap(key, "G"));
                QString channel =  info.channelMap.begin().value();
                info.channelMap.clear();
                info.channelMap["G"] = channel;
            }
        }
        else if (info.channelMap.size() == 2) {
            modelId = GrayAColorModelID.id();

            QMap<QString,QString>::const_iterator it = info.channelMap.constBegin();
            QMap<QString,QString>::const_iterator end = info.channelMap.constEnd();

            QString failingChannelKey;

            for (; it != end; ++it) {
                if (it.key() != "G" && it.key() != "A") {
                    failingChannelKey = it.key();
                    break;
                }
            }

            info.remappedChannels.push_back(
                        ExrPaintLayerInfo::Remap(failingChannelKey, "G"));

            QString failingChannelValue = info.channelMap[failingChannelKey];
            info.channelMap.remove(failingChannelKey);
            info.channelMap["G"] = failingChannelValue;

        }
        else if (info.channelMap.size() == 3 || info.channelMap.size() == 4) {

            if (info.channelMap.contains("R") && info.channelMap.contains("G") && info.channelMap.contains("B")) {
                modelId = RGBAColorModelID.id();
            }
            else if (info.channelMap.contains("X") && info.channelMap.contains("Y") && info.channelMap.contains("Z")) {
                modelId = XYZAColorModelID.id();
                QMap<QString, QString> newChannelMap;
                if (info.channelMap.contains("W")) {
                    newChannelMap["A"] = info.channelMap["W"];
                    info.remappedChannels.push_back(ExrPaintLayerInfo::Remap("W", "A"));
                    info.remappedChannels.push_back(ExrPaintLayerInfo::Remap("X", "X"));
                    info.remappedChannels.push_back(ExrPaintLayerInfo::Remap("Y", "Y"));
                    info.remappedChannels.push_back(ExrPaintLayerInfo::Remap("Z", "Z"));
                } else if (info.channelMap.contains("A")) {
                    newChannelMap["A"] = info.channelMap["A"];
                }
                // The decode function expect R, G, B in the channel map
                newChannelMap["B"] = info.channelMap["X"];
                newChannelMap["G"] = info.channelMap["Y"];
                newChannelMap["R"] = info.channelMap["Z"];
                info.channelMap = newChannelMap;
            }
            else {
                modelId = RGBAColorModelID.id();
                QMap<QString, QString> newChannelMap;
                QMap<QString, QString>::iterator it = info.channelMap.begin();
                newChannelMap["R"] = it.value();
                info.remappedChannels.push_back(ExrPaintLayerInfo::Remap(it.key(), "R"));
                ++it;
                newChannelMap["G"] = it.value();
                info.remappedChannels.push_back(ExrPaintLayerInfo::Remap(it.key(), "G"));
                ++it;
                newChannelMap["B"] = it.value();
                info.remappedChannels.push_back(ExrPaintLayerInfo::Remap(it.key(), "B"));
                if (info.channelMap.size() == 4) {
                    ++it;
                    newChannelMap["A"] = it.value();
                    info.remappedChannels.push_back(ExrPaintLayerInfo::Remap(it.key(), "A"));
                }

                info.channelMap = newChannelMap;
            }
        }
        else {
            dbgFile << info.name << "has" << info.channelMap.size() << "channels, and we don't know what to do.";
        }
        if (!modelId.isEmpty()) {
            info.colorSpace = kisTypeToColorSpace(modelId, info.imageType);
        }
    }

    // Get colorspace
    dbgFile << "Image type = " << imageType;
    const KoColorSpace* colorSpace = kisTypeToColorSpace(RGBAColorModelID.id(), imageType);

    if (!colorSpace) return KisImageBuilder_RESULT_UNSUPPORTED_COLORSPACE;
    dbgFile << "Colorspace: " << colorSpace->name();

    // Set the colorspace on all groups
    for (int i = 0; i < groups.size(); ++i) {
        ExrGroupLayerInfo& info = groups[i];
        info.colorSpace = colorSpace;
    }

    // Create the image
    m_d->image = new KisImage(m_d->doc->createUndoStore(), width, height, colorSpace, "");

    if (!m_d->image) {
        return KisImageBuilder_RESULT_FAILURE;
    }

    /**
     * EXR semi-transparent images are expected to be rendered on
     * black to ensure correctness of the light model
     */
    m_d->image->setDefaultProjectionColor(KoColor(Qt::black, colorSpace));

    // Create group layers
    for (int i = 0; i < groups.size(); ++i) {
        ExrGroupLayerInfo& info = groups[i];
        Q_ASSERT(info.parent == 0 || info.parent->groupLayer);
        KisGroupLayerSP groupLayerParent = (info.parent) ? info.parent->groupLayer : m_d->image->rootLayer();
        info.groupLayer = new KisGroupLayer(m_d->image, info.name, OPACITY_OPAQUE_U8);
        m_d->image->addNode(info.groupLayer, groupLayerParent);
    }

    // Load the layers
    for (int i = informationObjects.size() - 1; i >= 0; --i) {
        ExrPaintLayerInfo& info = informationObjects[i];
        if (info.colorSpace) {
            dbgFile << "Decoding " << info.name << " with " << info.channelMap.size() << " channels, and color space " << info.colorSpace->id();
            KisPaintLayerSP layer = new KisPaintLayer(m_d->image, info.name, OPACITY_OPAQUE_U8, info.colorSpace);

            layer->setCompositeOpId(COMPOSITE_OVER);

            if (!layer) {
                return KisImageBuilder_RESULT_FAILURE;
            }

            switch (info.channelMap.size()) {
            case 1:
            case 2:
                // Decode the data
                switch (info.imageType) {
                case IT_FLOAT16:
                    m_d->decodeData1<half>(file, info, layer, width, dx, dy, height, Imf::HALF);
                    break;
                case IT_FLOAT32:
                    m_d->decodeData1<float>(file, info, layer, width, dx, dy, height, Imf::FLOAT);
                    break;
                case IT_UNKNOWN:
                case IT_UNSUPPORTED:
                    qFatal("Impossible error");
                }
                break;
            case 3:
            case 4:
                // Decode the data
                switch (info.imageType) {
                case IT_FLOAT16:
                    m_d->decodeData4<half>(file, info, layer, width, dx, dy, height, Imf::HALF);
                    break;
                case IT_FLOAT32:
                    m_d->decodeData4<float>(file, info, layer, width, dx, dy, height, Imf::FLOAT);
                    break;
                case IT_UNKNOWN:
                case IT_UNSUPPORTED:
                    qFatal("Impossible error");
                }
                break;
            default:
                qFatal("Invalid number of channels: %i", info.channelMap.size());
            }
            // Check if should set the channels
            if (!info.remappedChannels.isEmpty()) {
                QList<KisMetaData::Value> values;
                Q_FOREACH (const ExrPaintLayerInfo::Remap& remap, info.remappedChannels) {
                    QMap<QString, KisMetaData::Value> map;
                    map["original"] = KisMetaData::Value(remap.original);
                    map["current"] = KisMetaData::Value(remap.current);
                    values.append(map);
                }
                layer->metaData()->addEntry(KisMetaData::Entry(KisMetaData::SchemaRegistry::instance()->create("http://krita.org/exrchannels/1.0/" , "exrchannels"), "channelsmap", values));
            }
            // Add the layer
            KisGroupLayerSP groupLayerParent = (info.parent) ? info.parent->groupLayer : m_d->image->rootLayer();
            m_d->image->addNode(layer, groupLayerParent);
        } else {
            dbgFile << "No decoding " << info.name << " with " << info.channelMap.size() << " channels, and lack of a color space";
        }
    }

    if (!extraLayersInfo.isNull()) {
        KisExrLayersSorter sorter(extraLayersInfo, m_d->image);
    }

    return KisImageBuilder_RESULT_OK;
}

KisImageBuilder_Result exrConverter::buildImage(const QString &filename)
{
    return decode(filename);

}


KisImageSP exrConverter::image()
{
    return m_d->image;
}

template<typename _T_, int size>
struct ExrPixel_ {
    _T_ data[size];
};

class Encoder
{
public:
    virtual ~Encoder() {}
    virtual void prepareFrameBuffer(Imf::FrameBuffer*, int line) = 0;
    virtual void encodeData(int line) = 0;

};

template<typename _T_, int size, int alphaPos>
class EncoderImpl : public Encoder
{
public:
    EncoderImpl(Imf::OutputFile* _file, const ExrPaintLayerSaveInfo* _info, int width) : file(_file), info(_info), pixels(width), m_width(width) {}
    ~EncoderImpl() override {}
    void prepareFrameBuffer(Imf::FrameBuffer*, int line) override;
    void encodeData(int line) override;
private:
    typedef ExrPixel_<_T_, size> ExrPixel;
    Imf::OutputFile* file;
    const ExrPaintLayerSaveInfo* info;
    QVector<ExrPixel> pixels;
    int m_width;
};

template<typename _T_, int size, int alphaPos>
void EncoderImpl<_T_, size, alphaPos>::prepareFrameBuffer(Imf::FrameBuffer* frameBuffer, int line)
{
    int xstart = 0;
    int ystart = 0;
    ExrPixel* frameBufferData = (pixels.data()) - xstart - (ystart + line) * m_width;
    for (int k = 0; k < size; ++k) {
        frameBuffer->insert(info->channels[k].toUtf8(),
                            Imf::Slice(info->pixelType, (char *) &frameBufferData->data[k],
                                       sizeof(ExrPixel) * 1,
                                       sizeof(ExrPixel) * m_width));
    }
}

template<typename _T_, int size, int alphaPos>
void EncoderImpl<_T_, size, alphaPos>::encodeData(int line)
{
    ExrPixel *rgba = pixels.data();
    KisHLineIteratorSP it = info->layer->paintDevice()->createHLineIteratorNG(0, line, m_width);
    do {
        const _T_* dst = reinterpret_cast < const _T_* >(it->oldRawData());

        for (int i = 0; i < size; ++i) {
            rgba->data[i] = dst[i];
        }

        if (alphaPos != -1) {
            multiplyAlpha<_T_, ExrPixel, size, alphaPos>(rgba);
        }

        ++rgba;
    } while (it->nextPixel());
}

Encoder* encoder(Imf::OutputFile& file, const ExrPaintLayerSaveInfo& info, int width)
{
    dbgFile << "Create encoder for" << info.layer->name() << info.channels << info.layer->colorSpace()->channelCount();
    switch (info.layer->colorSpace()->channelCount()) {
    case 1: {
        if (info.layer->colorSpace()->colorDepthId() == Float16BitsColorDepthID) {
            Q_ASSERT(info.pixelType == Imf::HALF);
            return new EncoderImpl < half, 1, -1 > (&file, &info, width);
        } else if (info.layer->colorSpace()->colorDepthId() == Float32BitsColorDepthID) {
            Q_ASSERT(info.pixelType == Imf::FLOAT);
            return new EncoderImpl < float, 1, -1 > (&file, &info, width);
        }
        break;
    }
    case 2: {
        if (info.layer->colorSpace()->colorDepthId() == Float16BitsColorDepthID) {
            Q_ASSERT(info.pixelType == Imf::HALF);
            return new EncoderImpl<half, 2, 1>(&file, &info, width);
        } else if (info.layer->colorSpace()->colorDepthId() == Float32BitsColorDepthID) {
            Q_ASSERT(info.pixelType == Imf::FLOAT);
            return new EncoderImpl<float, 2, 1>(&file, &info, width);
        }
        break;
    }
    case 4: {
        if (info.layer->colorSpace()->colorDepthId() == Float16BitsColorDepthID) {
            Q_ASSERT(info.pixelType == Imf::HALF);
            return new EncoderImpl<half, 4, 3>(&file, &info, width);
        } else if (info.layer->colorSpace()->colorDepthId() == Float32BitsColorDepthID) {
            Q_ASSERT(info.pixelType == Imf::FLOAT);
            return new EncoderImpl<float, 4, 3>(&file, &info, width);
        }
        break;
    }
    default:
        qFatal("Impossible error");
    }
    return 0;
}

void encodeData(Imf::OutputFile& file, const QList<ExrPaintLayerSaveInfo>& informationObjects, int width, int height)
{
    QList<Encoder*> encoders;
    Q_FOREACH (const ExrPaintLayerSaveInfo& info, informationObjects) {
        encoders.push_back(encoder(file, info, width));
    }

    for (int y = 0; y < height; ++y) {
        Imf::FrameBuffer frameBuffer;
        Q_FOREACH (Encoder* encoder, encoders) {
            encoder->prepareFrameBuffer(&frameBuffer, y);
        }
        file.setFrameBuffer(frameBuffer);
        Q_FOREACH (Encoder* encoder, encoders) {
            encoder->encodeData(y);
        }
        file.writePixels(1);
    }
    qDeleteAll(encoders);
}

KisImageBuilder_Result exrConverter::buildFile(const QString &filename, KisPaintLayerSP layer)
{
    if (!layer)
        return KisImageBuilder_RESULT_INVALID_ARG;

    KisImageSP image = layer->image();
    if (!image)
        return KisImageBuilder_RESULT_EMPTY;

    // Make the header
    qint32 height = image->height();
    qint32 width = image->width();
    Imf::Header header(width, height);

    Imf::PixelType pixelType = Imf::NUM_PIXELTYPES;

    if(layer->colorSpace()->colorDepthId() == Float16BitsColorDepthID) {
        pixelType = Imf::HALF;
    } else if(layer->colorSpace()->colorDepthId() == Float32BitsColorDepthID)
    {
        pixelType = Imf::FLOAT;
    }

    if(pixelType >= Imf::NUM_PIXELTYPES)
    {
        return KisImageBuilder_RESULT_UNSUPPORTED_COLORSPACE;
    }


    header.channels().insert("R", Imf::Channel(pixelType));
    header.channels().insert("G", Imf::Channel(pixelType));
    header.channels().insert("B", Imf::Channel(pixelType));
    header.channels().insert("A", Imf::Channel(pixelType));

    ExrPaintLayerSaveInfo info;
    info.layer = layer;
    info.channels.push_back("R");
    info.channels.push_back("G");
    info.channels.push_back("B");
    info.channels.push_back("A");
    info.pixelType = pixelType;

    // Open file for writing
    Imf::OutputFile file(QFile::encodeName(filename), header);

    QList<ExrPaintLayerSaveInfo> informationObjects;
    informationObjects.push_back(info);

    encodeData(file, informationObjects, width, height);

    return KisImageBuilder_RESULT_OK;
}

QString remap(const QMap<QString, QString>& current2original, const QString& current)
{
    if (current2original.contains(current)) {
        return current2original[current];
    }
    return current;
}

void exrConverter::Private::makeLayerNamesUnique(QList<ExrPaintLayerSaveInfo>& informationObjects)
{
    typedef QMultiMap<QString, QList<ExrPaintLayerSaveInfo>::iterator> NamesMap;
    NamesMap namesMap;

    {
        QList<ExrPaintLayerSaveInfo>::iterator it = informationObjects.begin();
        QList<ExrPaintLayerSaveInfo>::iterator end = informationObjects.end();

        for (; it != end; ++it) {
            namesMap.insert(it->name, it);
        }
    }

    Q_FOREACH (const QString &key, namesMap.keys()) {
        if (namesMap.count(key) > 1) {
            KIS_ASSERT_RECOVER(key.endsWith(".")) { continue; }
            QString strippedName = key.left(key.size() - 1); // trim the ending dot
            int nameCounter = 0;

            NamesMap::iterator it = namesMap.find(key);
            NamesMap::iterator end = namesMap.end();

            for (; it != end; ++it) {
                QString newName =
                        QString("%1_%2.")
                        .arg(strippedName)
                        .arg(nameCounter++);

                it.value()->name = newName;

                QList<QString>::iterator channelsIt = it.value()->channels.begin();
                QList<QString>::iterator channelsEnd = it.value()->channels.end();

                for  (; channelsIt != channelsEnd; ++channelsIt) {
                    channelsIt->replace(key, newName);
                }
            }
        }
    }

}

void exrConverter::Private::recBuildPaintLayerSaveInfo(QList<ExrPaintLayerSaveInfo>& informationObjects, const QString& name, KisGroupLayerSP parent)
{
    QSet<KisNodeSP> layersNotSaved;

    for (uint i = 0; i < parent->childCount(); ++i) {
        KisNodeSP node = parent->at(i);

        if (KisPaintLayerSP paintLayer = dynamic_cast<KisPaintLayer*>(node.data())) {
            QMap<QString, QString> current2original;

            if (paintLayer->metaData()->containsEntry(KisMetaData::SchemaRegistry::instance()->create("http://krita.org/exrchannels/1.0/" , "exrchannels"), "channelsmap")) {

                const KisMetaData::Entry& entry = paintLayer->metaData()->getEntry(KisMetaData::SchemaRegistry::instance()->create("http://krita.org/exrchannels/1.0/" , "exrchannels"), "channelsmap");
                QList< KisMetaData::Value> values = entry.value().asArray();

                Q_FOREACH (const KisMetaData::Value& value, values) {
                    QMap<QString, KisMetaData::Value> map = value.asStructure();
                    if (map.contains("original") && map.contains("current")) {
                        current2original[map["current"].toString()] = map["original"].toString();
                    }
                }

            }

            ExrPaintLayerSaveInfo info;
            info.name = name + paintLayer->name() + '.';
            info.layer = paintLayer;

            if (info.name == QString(HDR_LAYER) + ".") {
                info.channels.push_back("R");
                info.channels.push_back("G");
                info.channels.push_back("B");
                info.channels.push_back("A");
            }
            else {

                if (paintLayer->colorSpace()->colorModelId() == RGBAColorModelID) {
                    info.channels.push_back(info.name + remap(current2original, "R"));
                    info.channels.push_back(info.name + remap(current2original, "G"));
                    info.channels.push_back(info.name + remap(current2original, "B"));
                    info.channels.push_back(info.name + remap(current2original, "A"));
                }
                else if (paintLayer->colorSpace()->colorModelId() == GrayAColorModelID) {
                    info.channels.push_back(info.name + remap(current2original, "G"));
                    info.channels.push_back(info.name + remap(current2original, "A"));
                }
                else if (paintLayer->colorSpace()->colorModelId() == GrayColorModelID) {
                    info.channels.push_back(info.name + remap(current2original, "G"));
                }
                else if (paintLayer->colorSpace()->colorModelId() == XYZAColorModelID) {
                    info.channels.push_back(info.name + remap(current2original, "X"));
                    info.channels.push_back(info.name + remap(current2original, "Y"));
                    info.channels.push_back(info.name + remap(current2original, "Z"));
                    info.channels.push_back(info.name + remap(current2original, "A"));
                }

            }            if (paintLayer->colorSpace()->colorDepthId() == Float16BitsColorDepthID) {
                info.pixelType = Imf::HALF;
            }
            else if (paintLayer->colorSpace()->colorDepthId() == Float32BitsColorDepthID) {
                info.pixelType = Imf::FLOAT;
            }
            else {
                info.pixelType = Imf::NUM_PIXELTYPES;
            }

            if (info.pixelType < Imf::NUM_PIXELTYPES) {
                dbgFile << "Going to save layer" << info.name;
                informationObjects.push_back(info);
            }
            else {
                warnFile << "Will not save layer" << info.name;
                layersNotSaved << node;
            }

        }
        else if (KisGroupLayerSP groupLayer = dynamic_cast<KisGroupLayer*>(node.data())) {
            recBuildPaintLayerSaveInfo(informationObjects, name + groupLayer->name() + '.', groupLayer);
        }
        else {
            /**
             * The EXR can store paint and group layers only. The rest will
             * go to /dev/null :(
             */
            layersNotSaved.insert(node);
        }
    }

    if (!layersNotSaved.isEmpty()) {
        reportLayersNotSaved(layersNotSaved);
    }
}

void exrConverter::Private::reportLayersNotSaved(const QSet<KisNodeSP> &layersNotSaved)
{
    QString layersList;
    QTextStream textStream(&layersList);
    textStream.setCodec("UTF-8");

    Q_FOREACH (KisNodeSP node, layersNotSaved) {
        textStream << "<li>" << i18nc("@item:unsupported-node-message", "%1 (type: \"%2\")", node->name(), node->metaObject()->className()) << "</li>";
    }

    QString msg =
            i18nc("@info",
                  "<p>The following layers have a type that is not supported by EXR format:</p>"
                  "<p><ul>%1</ul></p>"
                  "<p><warning>these layers will NOT be saved to the final EXR file</warning></p>", layersList);

    if (this->showNotifications) {
        QMessageBox::information(0, i18nc("@title:window", "Layers will be lost"), msg);
    } else {
        warnKrita << "WARNING:" << msg;
    }

}

QString exrConverter::Private::fetchExtraLayersInfo(QList<ExrPaintLayerSaveInfo>& informationObjects)
{
    KIS_ASSERT_RECOVER_NOOP(!informationObjects.isEmpty());

    if (informationObjects.size() == 1 && informationObjects[0].name == QString(HDR_LAYER) + ".") {
        return QString();
    }

    QDomDocument doc("krita-extra-layers-info");
    doc.appendChild(doc.createElement("root"));
    QDomElement rootElement = doc.documentElement();

    for (int i = 0; i < informationObjects.size(); i++) {
        ExrPaintLayerSaveInfo &info = informationObjects[i];
        quint32 unused;
        KisSaveXmlVisitor visitor(doc, rootElement, unused, QString(), false);
        QDomElement el = visitor.savePaintLayerAttributes(info.layer.data(), doc);

        // cut the ending '.'
        QString strippedName = info.name.left(info.name.size() - 1);

        el.setAttribute(EXR_NAME, strippedName);

        rootElement.appendChild(el);
    }

    return doc.toString();
}

KisImageBuilder_Result exrConverter::buildFile(const QString &filename, KisGroupLayerSP layer)
{
    if (!layer)
        return KisImageBuilder_RESULT_INVALID_ARG;

    KisImageSP image = layer->image();
    if (!image)
        return KisImageBuilder_RESULT_EMPTY;


    qint32 height = image->height();
    qint32 width = image->width();
    Imf::Header header(width, height);

    QList<ExrPaintLayerSaveInfo> informationObjects;
    m_d->recBuildPaintLayerSaveInfo(informationObjects, "", layer);

    if(informationObjects.isEmpty()) {
        return KisImageBuilder_RESULT_UNSUPPORTED_COLORSPACE;
    }

    m_d->makeLayerNamesUnique(informationObjects);

    QByteArray extraLayersInfo = m_d->fetchExtraLayersInfo(informationObjects).toUtf8();
    if (!extraLayersInfo.isNull()) {
        header.insert(EXR_KRITA_LAYERS, Imf::StringAttribute(extraLayersInfo.constData()));
    }
    dbgFile << informationObjects.size() << " layers to save";

    Q_FOREACH (const ExrPaintLayerSaveInfo& info, informationObjects) {
        if (info.pixelType < Imf::NUM_PIXELTYPES) {
            Q_FOREACH (const QString& channel, info.channels) {
                dbgFile << channel << " " << info.pixelType;
                header.channels().insert(channel.toUtf8().data(), Imf::Channel(info.pixelType));
            }
        }
    }

    // Open file for writing
    Imf::OutputFile file(QFile::encodeName(filename), header);

    encodeData(file, informationObjects, width, height);
    return KisImageBuilder_RESULT_OK;
}

void exrConverter::cancel()
{
    warnKrita << "WARNING: Cancelling of an EXR loading is not supported!";
}


