/*
 *  Copyright (c) 2005 Bart Coppens <kde@bartcoppens.be>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation; either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _Ko_BASIC_HISTOGRAM_PRODUCERS_
#define _Ko_BASIC_HISTOGRAM_PRODUCERS_

#include "KoHistogramProducer.h"

#include <QVector>

#include <KoConfig.h>

#include "KoColorSpace.h"
#include "KoID.h"
#include "kritapigment_export.h"
#include "KoColorSpaceRegistry.h"

class KRITAPIGMENT_EXPORT KoBasicHistogramProducer : public KoHistogramProducer
{
public:
    explicit KoBasicHistogramProducer(const KoID& id, int channelCount, int nrOfBins);
    explicit KoBasicHistogramProducer(const KoID& id, int nrOfBins, const KoColorSpace *colorSpace);
    virtual ~KoBasicHistogramProducer() {}

    virtual void clear();

    virtual void setView(qreal from, qreal size) {
        m_from = from; m_width = size;
    }

    virtual const KoID& id() const {
        return m_id;
    }
    virtual QList<KoChannelInfo *> channels() {
        return m_colorSpace->channels();
    }
    virtual qint32 numberOfBins() {
        return m_nrOfBins;
    }
    virtual qreal viewFrom() const {
        return m_from;
    }
    virtual qreal viewWidth() const {
        return m_width;
    }

    virtual qint32 count() {
        return m_count;
    }

    virtual qint32 getBinAt(int channel, int position) {
        return m_bins.at(externalToInternal(channel)).at(position);
    }

    virtual qint32 outOfViewLeft(int channel) {
        return m_outLeft.at(externalToInternal(channel));
    }

    virtual qint32 outOfViewRight(int channel) {
        return m_outRight.at(externalToInternal(channel));
    }

protected:
    /**
     * The order in which channels() returns is not the same as the internal representation,
     * that of the pixel internally. This method converts external usage to internal usage.
     * This method uses some basic assumtpions about the layout of the pixel, so _extremely_
     * exotic spaces might want to override this (see makeExternalToInternal source for
     * those assumptions)
     **/
    virtual int externalToInternal(int ext) {
        if (channels().count() > 0 && m_external.count() == 0) // Set up the translation table
            makeExternalToInternal();
        return m_external.at(ext);
    }
    // not virtual since that is useless: we call it from constructor
    void makeExternalToInternal();
    typedef QVector<quint32> vBins;
    QVector<vBins> m_bins;
    vBins m_outLeft, m_outRight;
    qreal m_from, m_width;
    qint32 m_count;
    int m_channels, m_nrOfBins;
    const KoColorSpace *m_colorSpace;
    KoID m_id;
    QVector<qint32> m_external;
};

class KRITAPIGMENT_EXPORT KoBasicU8HistogramProducer : public KoBasicHistogramProducer
{
public:
    KoBasicU8HistogramProducer(const KoID& id, const KoColorSpace *colorSpace);
    virtual void addRegionToBin(const quint8 * pixels, const quint8 * selectionMask, quint32 nPixels, const KoColorSpace *colorSpace);
    virtual QString positionToString(qreal pos) const;
    virtual qreal maximalZoom() const {
        return 1.0;
    }
};

class KRITAPIGMENT_EXPORT KoBasicU16HistogramProducer : public KoBasicHistogramProducer
{
public:
    KoBasicU16HistogramProducer(const KoID& id, const KoColorSpace *colorSpace);
    virtual void addRegionToBin(const quint8 * pixels, const quint8 * selectionMask, quint32 nPixels, const KoColorSpace *colorSpace);
    virtual QString positionToString(qreal pos) const;
    virtual qreal maximalZoom() const;
};

class KRITAPIGMENT_EXPORT KoBasicF32HistogramProducer : public KoBasicHistogramProducer
{
public:
    KoBasicF32HistogramProducer(const KoID& id, const KoColorSpace *colorSpace);
    virtual void addRegionToBin(const quint8 * pixels, const quint8 * selectionMask, quint32 nPixels, const KoColorSpace *colorSpace);
    virtual QString positionToString(qreal pos) const;
    virtual qreal maximalZoom() const;
};


#ifdef HAVE_OPENEXR
class KRITAPIGMENT_EXPORT KoBasicF16HalfHistogramProducer : public KoBasicHistogramProducer
{
public:
    KoBasicF16HalfHistogramProducer(const KoID& id, const KoColorSpace *colorSpace);
    virtual void addRegionToBin(const quint8 * pixels, const quint8 * selectionMask, quint32 nPixels, const KoColorSpace *colorSpace);
    virtual QString positionToString(qreal pos) const;
    virtual qreal maximalZoom() const;
};
#endif

/**
 * Parametrized on a specific KoHistogramProducer. Its generated producers
 * will have the same KoID as the factory's. This is acceptable because we can't mix
 * Factories with Producers in the code because they are incompatible types, and
 * in the GUI we actually only need a producer's name, not a factory's.
 */
template<class T> class KoBasicHistogramProducerFactory : public KoHistogramProducerFactory
{
public:
    KoBasicHistogramProducerFactory(const KoID& id, const QString& modelId, const QString& depthId )
            : KoHistogramProducerFactory(id), m_modelId(modelId), m_depthId(depthId) {
    }
    virtual ~KoBasicHistogramProducerFactory() {}

    virtual KoHistogramProducer *generate() {
        KoHistogramProducer *producer = 0;
        const KoColorSpace *cs = KoColorSpaceRegistry::instance()->colorSpace(m_modelId, m_depthId, 0);
        if (cs) {
            producer = new T(KoID(id(), name()), cs);
        }
        return producer;

    }
    virtual bool isCompatibleWith(const KoColorSpace* colorSpace, bool strict = false) const {
        if( strict ){
            return colorSpace->colorDepthId().id() == m_depthId;
        }
        return colorSpace->colorModelId().id() == m_modelId || colorSpace->colorDepthId().id() == m_depthId;
    }
    virtual float preferrednessLevelWith(const KoColorSpace* colorSpace) const {
        return 0.5 * ( (colorSpace->colorModelId().id() == m_modelId) + (colorSpace->colorDepthId().id() == m_depthId) );
    }
protected:
    QString m_modelId, m_depthId;
};

/**
 * This is a Producer (with associated factory) that converts the pixels of the colorspace
 * to RGB8 with toQColor, and then does its counting on RGB. This is NOT registered with the
 * Registry, because it isCompatibleWith all colorspaces, and should only be used in extreme
 * cases (like no other producer being available
 **/
class KRITAPIGMENT_EXPORT KoGenericRGBHistogramProducer : public KoBasicHistogramProducer
{
public:
    KoGenericRGBHistogramProducer();
    virtual void addRegionToBin(const quint8 * pixels, const quint8 * selectionMask, quint32 nPixels, const KoColorSpace *colorSpace);
    virtual QString positionToString(qreal pos) const;
    virtual qreal maximalZoom() const;
    virtual QList<KoChannelInfo *> channels();
protected:
    QList<KoChannelInfo *> m_channelsList;
};

/** KoGenericRGBHistogramProducer his special Factory that isCompatibleWith everything. */
class  KRITAPIGMENT_EXPORT KoGenericRGBHistogramProducerFactory : public KoHistogramProducerFactory
{
public:
    KoGenericRGBHistogramProducerFactory();

    virtual ~KoGenericRGBHistogramProducerFactory() {}

    virtual KoHistogramProducer *generate() {
        return new KoGenericRGBHistogramProducer();
    }

    virtual bool isCompatibleWith(const KoColorSpace*, bool strict = false) const {
        Q_UNUSED(strict);
        return true;
    }

    virtual float preferrednessLevelWith(const KoColorSpace*) const {
        return 0.0;
    }
};


/**
 * This is a Producer (with associated factory) that converts the pixels of the colorspace
 * to L*a*b*, and then does its counting.
 * It isCompatibleWith all colorspaces
 **/
class  KRITAPIGMENT_EXPORT KoGenericLabHistogramProducer : public KoBasicHistogramProducer
{
public:
    KoGenericLabHistogramProducer();
    virtual ~KoGenericLabHistogramProducer();
    virtual void addRegionToBin(const quint8 * pixels, const quint8 * selectionMask, quint32 nPixels, const KoColorSpace *colorSpace);
    virtual QString positionToString(qreal pos) const;
    virtual qreal maximalZoom() const;
    virtual QList<KoChannelInfo *> channels();
protected:
    QList<KoChannelInfo *> m_channelsList;
};

/** KoGenericLabHistogramProducer his special Factory that isCompatibleWith everything. */
class /*KRITAPIGMENT_EXPORT*/ KoGenericLabHistogramProducerFactory : public KoHistogramProducerFactory
{
public:
    KoGenericLabHistogramProducerFactory();
    virtual ~KoGenericLabHistogramProducerFactory() {}

    virtual KoHistogramProducer *generate() {
        return new KoGenericLabHistogramProducer();
    }

    virtual bool isCompatibleWith(const KoColorSpace*, bool strict = false) const {
        Q_UNUSED(strict);
        return true;
    }

    virtual float preferrednessLevelWith(const KoColorSpace*) const {
        return 0.0;
    }
};


#endif // _Ko_BASIC_HISTOGRAM_PRODUCERS_
