/*
 *  Copyright (c) 2016 Laszlo Fazekas <mneko@freemail.hu>
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

#include "csv_saver.h"

#include <QDebug>
#include <QApplication>

#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QVector>
#include <QIODevice>
#include <QRect>
#include <KisMimeDatabase.h>

#include <KisPart.h>
#include <KisDocument.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorModelStandardIds.h>

#include <kis_annotation.h>
#include <kis_types.h>

#include <kis_debug.h>
#include <kis_image.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_raster_keyframe_channel.h>
#include <kis_image_animation_interface.h>
#include <kis_time_range.h>
#include <kis_iterator_ng.h>

#include "csv_layer_record.h"

CSVSaver::CSVSaver(KisDocument *doc, bool batchMode)
    : m_image(doc->image())
    , m_doc(doc)
    , m_batchMode(batchMode)
    , m_stop(false)
{
}

CSVSaver::~CSVSaver()
{
}

KisImageSP CSVSaver::image()
{
    return m_image;
}

KisImageBuilder_Result CSVSaver::encode(const QString &filename)
{
    int idx;
    int start, end;
    KisNodeSP node;
    QByteArray ba;
    KisKeyframeSP keyframe;
    QVector<CSVLayerRecord*> layers;

    KisImageAnimationInterface *animation = m_image->animationInterface();

    //open the csv file for writing
    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly)) {
        return KisImageBuilder_RESULT_NOT_LOCAL;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    //DataStream instead of TextStream for correct line endings
    QDataStream stream(&f);

    //Using the original local path
    QString path = m_doc->localFilePath();

    if (path.right(4).toUpper() == ".CSV")
        path = path.left(path.size() - 4);
    else {
        //something is wrong: the local file name is not .csv!
        //trying the given (probably temporary) filename as well

        path= filename;

        if (path.right(4).toUpper() == ".CSV")
            path = path.left(path.size() - 4);
    }
    path.append(".frames");

    //create directory

    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    //according to the QT docs, the slash is a universal directory separator
    path.append("/");

    node = m_image->rootLayer()->firstChild();

    //TODO: correct handling of the layer tree.
    //for now, only top level paint layers are saved

    idx = 0;

    while (node) {
        if (node->inherits("KisPaintLayer")) {
            KisPaintLayer* paintLayer = dynamic_cast<KisPaintLayer*>(node.data());
            CSVLayerRecord* layerRecord = new CSVLayerRecord();
            layers.prepend(layerRecord); //reverse order!

            layerRecord->name = paintLayer->name();
            layerRecord->name.replace(QRegExp("[\"\\r\\n]"), "_");

            if (layerRecord->name.isEmpty())
                layerRecord->name= QString("Unnamed-%1").arg(idx);

            layerRecord->visible = (paintLayer->visible()) ? 1 : 0;
            layerRecord->density = (float)(paintLayer->opacity()) / OPACITY_OPAQUE_U8;
            layerRecord->blending = convertToBlending(paintLayer->compositeOpId());
            layerRecord->layer = paintLayer;
            layerRecord->channel = paintLayer->projection()->keyframeChannel();
            layerRecord->last = "";
            layerRecord->frame = 0;
            idx++;
        }
        node = node->nextSibling();
    }

    KisTimeRange range = animation->fullClipRange();

    start = (range.isValid()) ? range.start() : 0;

    if (!range.isInfinite()) {
        end = range.end();

        if (end < start) end = start;
    } else {
        //undefined length, searching for the last keyframe
        end = start;

        for (idx = 0; idx < layers.size(); idx++) {
            KisRasterKeyframeChannel *channel = layers.at(idx)->channel;

            if (channel) {
                keyframe = channel->lastKeyframe();

                if ( (!keyframe.isNull()) && (keyframe->time() > end) )
                    end = keyframe->time();
            }
        }
    }

    //create temporary doc for exporting
    QScopedPointer<KisDocument> exportDoc(KisPart::instance()->createDocument());
    createTempImage(exportDoc.data());

    KisImageBuilder_Result retval= KisImageBuilder_RESULT_OK;

    if (!m_batchMode) {
        emit m_doc->statusBarMessage(i18n("Saving CSV file..."));
        emit m_doc->sigProgress(0);
        connect(m_doc, SIGNAL(sigProgressCanceled()), this, SLOT(cancel()));
    }
    int frame = start;
    int step = 0;

    do {
        qApp->processEvents();

        if (m_stop) {
            retval = KisImageBuilder_RESULT_CANCEL;
            break;
        }

        switch(step) {

        case 0 :    //first row
            if (f.write("UTF-8, TVPaint, \"CSV 1.0\"\r\n") < 0) {
                retval = KisImageBuilder_RESULT_FAILURE;
            }
            break;

        case 1 :    //scene header names
            if (f.write("Project Name, Width, Height, Frame Count, Layer Count, Frame Rate, Pixel Aspect Ratio, Field Mode\r\n") < 0) {
                retval = KisImageBuilder_RESULT_FAILURE;
            }
            break;

        case 2 :    //scene header values
            ba = QString("\"%1\", ").arg(m_image->objectName()).toUtf8();
            if (f.write(ba.data()) < 0) {
                retval = KisImageBuilder_RESULT_FAILURE;
                break;
            }
            ba = QString("%1, %2, ").arg(m_image->width()).arg(m_image->height()).toUtf8();
            if (f.write(ba.data()) < 0) {
                retval = KisImageBuilder_RESULT_FAILURE;
                break;
            }

            ba = QString("%1, %2, ").arg(end - start + 1).arg(layers.size()).toUtf8();
            if (f.write(ba.data()) < 0) {
                retval = KisImageBuilder_RESULT_FAILURE;
                break;
            }
            //the framerate is an integer here
            ba = QString("%1, ").arg((double)(animation->framerate()),0,'f',6).toUtf8();
            if (f.write(ba.data()) < 0) {
                retval = KisImageBuilder_RESULT_FAILURE;
                break;
            }
            ba = QString("%1, Progressive\r\n").arg((double)(m_image->xRes() / m_image->yRes()),0,'f',6).toUtf8();
            if (f.write(ba.data()) < 0) {
                retval = KisImageBuilder_RESULT_FAILURE;
                break;
            }
            break;

        case 3 :    //layer header values
            if (f.write("#Layers") < 0) {          //Layers
                retval = KisImageBuilder_RESULT_FAILURE;
                break;
            }

            for (idx = 0; idx < layers.size(); idx++) {
                ba = QString(", \"%1\"").arg(layers.at(idx)->name).toUtf8();
                if (f.write(ba.data()) < 0)
                    break;
            }
            break;

        case 4 :
            if (f.write("\r\n#Density") < 0) {     //Density
                retval = KisImageBuilder_RESULT_FAILURE;
                break;
            }
            for (idx = 0; idx < layers.size(); idx++) {
                ba = QString(", %1").arg((double)(layers.at(idx)->density), 0, 'f', 6).toUtf8();
                if (f.write(ba.data()) < 0)
                    break;
            }
            break;

        case 5 :
            if (f.write("\r\n#Blending") < 0) {     //Blending
                retval = KisImageBuilder_RESULT_FAILURE;
                break;
            }
            for (idx = 0; idx < layers.size(); idx++) {
                ba = QString(", \"%1\"").arg(layers.at(idx)->blending).toUtf8();
                if (f.write(ba.data()) < 0)
                    break;
            }
            break;

        case 6 :
            if (f.write("\r\n#Visible") < 0) {     //Visible
                retval = KisImageBuilder_RESULT_FAILURE;
                break;
            }
            for (idx = 0; idx < layers.size(); idx++) {
                ba = QString(", %1").arg(layers.at(idx)->visible).toUtf8();
                if (f.write(ba.data()) < 0)
                    break;
            }
            if (idx < layers.size()) {
                retval = KisImageBuilder_RESULT_FAILURE;
            }
            break;

        default :    //frames

            if (frame > end) {
                if (f.write("\r\n") < 0)
                    retval = KisImageBuilder_RESULT_FAILURE;

                step = 8;
                break;
            }

            ba = QString("\r\n#%1").arg(frame, 5, 10, QChar('0')).toUtf8();
            if (f.write(ba.data()) < 0) {
                retval = KisImageBuilder_RESULT_FAILURE;
                break;
            }

            for (idx = 0; idx < layers.size(); idx++) {
                CSVLayerRecord *layer = layers.at(idx);
                KisRasterKeyframeChannel *channel = layer->channel;

                if (channel) {
                    if (frame == start) {
                        keyframe = channel->activeKeyframeAt(frame);
                    } else {
                        keyframe = channel->keyframeAt(frame);
                    }
                } else {
                    keyframe.clear(); // without animation
                }

                if ( !keyframe.isNull() || (frame == start) ) {

                    if (!m_batchMode) {
                        emit m_doc->sigProgress(((frame - start) * layers.size() + idx) * 100 /
                                                ((end - start) * layers.size()));
                    }
                    retval = getLayer(layer, exportDoc.data(), keyframe, path, frame, idx);

                    if (retval != KisImageBuilder_RESULT_OK)
                        break;
                }
                ba = QString(", \"%1\"").arg(layer->last).toUtf8();

                if (f.write(ba.data()) < 0)
                    break;
            }
            if (idx < layers.size())
                retval = KisImageBuilder_RESULT_FAILURE;

            frame++;
            step = 6; //keep step here
            break;
        }
        step++;
    } while((retval == KisImageBuilder_RESULT_OK) && (step < 8));

    qDeleteAll(layers);
    f.close();

    if (!m_batchMode) {
        disconnect(m_doc, SIGNAL(sigProgressCanceled()), this, SLOT(cancel()));
        emit m_doc->sigProgress(100);
        emit m_doc->clearStatusBarMessage();
    }
    QApplication::restoreOverrideCursor();
    return retval;
}

QString CSVSaver::convertToBlending(const QString &opid)
{
    if (opid == COMPOSITE_OVER) return "Color";
    if (opid == COMPOSITE_BEHIND) return "Behind";
    if (opid == COMPOSITE_ERASE) return "Erase";
    // "Shade"
    if (opid == COMPOSITE_LINEAR_LIGHT) return "Light";
    if (opid == COMPOSITE_COLORIZE) return "Colorize";
    if (opid == COMPOSITE_HUE) return "Hue";
    if ((opid == COMPOSITE_ADD) ||
        (opid == COMPOSITE_LINEAR_DODGE)) return "Add";
    if (opid == COMPOSITE_INVERSE_SUBTRACT) return "Sub";
    if (opid == COMPOSITE_MULT) return "Multiply";
    if (opid == COMPOSITE_SCREEN) return "Screen";
    // "Replace"
    // "Subtitute"
    if (opid == COMPOSITE_DIFF) return "Difference";
    if (opid == COMPOSITE_DIVIDE) return "Divide";
    if (opid == COMPOSITE_OVERLAY) return "Overlay";
    if (opid == COMPOSITE_DODGE) return "Light2";
    if (opid == COMPOSITE_BURN) return "Shade2";
    if (opid == COMPOSITE_HARD_LIGHT) return "HardLight";
    if ((opid == COMPOSITE_SOFT_LIGHT_PHOTOSHOP) ||
        (opid == COMPOSITE_SOFT_LIGHT_SVG)) return "SoftLight";
    if (opid == COMPOSITE_GRAIN_EXTRACT) return "GrainExtract";
    if (opid == COMPOSITE_GRAIN_MERGE) return "GrainMerge";
    if (opid == COMPOSITE_SUBTRACT) return "Sub2";
    if (opid == COMPOSITE_DARKEN) return "Darken";
    if (opid == COMPOSITE_LIGHTEN) return "Lighten";
    if (opid == COMPOSITE_SATURATION) return "Saturation";

    return "Color";
}

KisImageBuilder_Result CSVSaver::getLayer(CSVLayerRecord* layer, KisDocument* exportDoc, KisKeyframeSP keyframe, const QString &path, int frame, int idx)
{
    //render to the temp layer
    KisImageSP image = exportDoc->image();
    KisPaintDeviceSP device = image->rootLayer()->firstChild()->projection();

    if (!keyframe.isNull()) {
        layer->channel->fetchFrame(keyframe, device);
    } else {
        device->makeCloneFrom(layer->layer->projection(),image->bounds()); // without animation
    }
    QRect bounds = device->exactBounds();

    if (bounds.isEmpty()) {
        layer->last = "";                   //empty frame
        return KisImageBuilder_RESULT_OK;
    }
    layer->last = QString("frame%1-%2.png").arg(idx + 1,5,10,QChar('0')).arg(frame,5,10,QChar('0'));

    QString filename = path;
    filename.append(layer->last);

    //save to PNG

    KisSequentialConstIterator it(device, image->bounds());
    const KoColorSpace* cs = device->colorSpace();

    bool isThereAlpha = false;
    do {
        if (cs->opacityU8(it.oldRawData()) != OPACITY_OPAQUE_U8) {
            isThereAlpha = true;
            break;
        }
    } while (it.nextPixel());

    if (!KisPNGConverter::isColorSpaceSupported(cs)) {
        device = new KisPaintDevice(*device.data());
        KUndo2Command *cmd= device->convertTo(KoColorSpaceRegistry::instance()->rgb8());
        delete cmd;
    }
    KisPNGOptions options;

    options.alpha = isThereAlpha;
    options.interlace = false;
    options.compression = 8;
    options.tryToSaveAsIndexed = false;
    options.transparencyFillColor = QColor(0,0,0);
    options.saveSRGBProfile = true;                 //TVPaint can use only sRGB
    options.forceSRGB = false;

    KisPNGConverter kpc(exportDoc);

    KisImageBuilder_Result result = kpc.buildFile(filename, image->bounds(),
                                                  image->xRes(), image->yRes(), device,
                                                  image->beginAnnotations(), image->endAnnotations(),
                                                  options, (KisMetaData::Store* )0 );

    return result;
}

void CSVSaver::createTempImage(KisDocument* exportDoc)
{
    exportDoc->setAutoSave(0);
    exportDoc->setOutputMimeType("image/png");
    exportDoc->setFileBatchMode(true);

    KisImageSP exportImage = new KisImage(exportDoc->createUndoStore(),
                                           m_image->width(), m_image->height(), m_image->colorSpace(),
                                           QString());

    exportImage->setResolution(m_image->xRes(), m_image->yRes());
    exportDoc->setCurrentImage(exportImage);

    KisPaintLayer* paintLayer = new KisPaintLayer(exportImage, "paint device", OPACITY_OPAQUE_U8);
    exportImage->addNode(paintLayer, exportImage->rootLayer(), KisLayerSP(0));
}


KisImageBuilder_Result CSVSaver::buildAnimation(const QString &filename)
{
    if (!m_image)
        return KisImageBuilder_RESULT_EMPTY;
    return encode(filename);
}

void CSVSaver::cancel()
{
    m_stop = true;
}
