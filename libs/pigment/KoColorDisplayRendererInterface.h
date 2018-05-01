/*
 *  Copyright (c) 2014 Dmitry Kazakov <dimula73@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __KO_COLOR_DISPLAY_RENDERER_INTERFACE_H
#define __KO_COLOR_DISPLAY_RENDERER_INTERFACE_H

#include <QObject>
#include <QColor>
#include <QSharedPointer>

#include "KoColor.h"

class KoChannelInfo;
class KoColorSpace;

/**
 * A special interface class provided by pigment to let widgets render
 * a KoColor on screen using custom profiling provided by the user.
 *
 * If you want to provide your own rendering of the KoColor on screen,
 * reimplement this class and provide its instance to a supporting
 * widget.
 */
class KRITAPIGMENT_EXPORT KoColorDisplayRendererInterface : public QObject
{
    Q_OBJECT

public:
    KoColorDisplayRendererInterface();
    virtual ~KoColorDisplayRendererInterface();

    /**
     * @brief KoColorSpace::convertToQImage converts a whole row of colors in one go
     * @param srcColorSpace the colorspace the pixel data is in
     * @param data a pointer to a byte array with colors
     * @param width of the resulting image
     * @param height of the resulting image
     * @return a QImage that can be displayed
     */
    virtual QImage convertToQImage(const KoColorSpace *srcColorSpace, const quint8 *data, qint32 width, qint32 height) const = 0;

    /**
     * Convert the color \p c to a custom QColor that will be
     * displayed by the widget on screen. Please note, that the
     * reverse conversion may simply not exist.
     */
    virtual QColor toQColor(const KoColor &c) const = 0;

    /**
     * This tries to approximate a rendered QColor into the KoColor
     * of the painting color space. Please note, that in most of the
     * cases the exact reverse transformation does not exist, so the
     * resulting color will be only a rough approximation. Never try
     * to do a round trip like that:
     *
     * // r will never be equal to c!
     * r = approximateFromRenderedQColor(toQColor(c));
     */
    virtual KoColor approximateFromRenderedQColor(const QColor &c) const = 0;

    virtual KoColor fromHsv(int h, int s, int v, int a = 255) const = 0;
    virtual void getHsv(const KoColor &srcColor, int *h, int *s, int *v, int *a = 0) const = 0;


    /**
     * \return the minimum value of a floating point channel that can
     *         be seen on screen
     */
    virtual qreal minVisibleFloatValue(const KoChannelInfo *chaninfo) const = 0;

    /**
     * \return the maximum value of a floating point channel that can
     *         be seen on screen. In normal situation it is 1.0. When
     *         the user changes exposure the value varies.
     */
    virtual qreal maxVisibleFloatValue(const KoChannelInfo *chaninfo) const = 0;

    /**
     * @brief getColorSpace
     * @return the painting color space, this is useful for determining the transform.
     */
    virtual const KoColorSpace* getPaintingColorSpace() const = 0;

Q_SIGNALS:
    void displayConfigurationChanged();

private:
    Q_DISABLE_COPY(KoColorDisplayRendererInterface)
};

/**
 * The default conversion class that just calls KoColor::toQColor()
 * conversion implementation which efectively renders the color into
 * sRGB color space.
 */
class KRITAPIGMENT_EXPORT KoDumbColorDisplayRenderer : public KoColorDisplayRendererInterface
{
public:
    QImage convertToQImage(const KoColorSpace *srcColorSpace, const quint8 *data, qint32 width, qint32 height) const;
    QColor toQColor(const KoColor &c) const;
    KoColor approximateFromRenderedQColor(const QColor &c) const;
    KoColor fromHsv(int h, int s, int v, int a = 255) const;
    void getHsv(const KoColor &srcColor, int *h, int *s, int *v, int *a = 0) const;

    virtual qreal minVisibleFloatValue(const KoChannelInfo *chaninfo) const;
    virtual qreal maxVisibleFloatValue(const KoChannelInfo *chaninfo) const;

    virtual const KoColorSpace* getPaintingColorSpace() const;

    static KoColorDisplayRendererInterface* instance();
};

#endif /* __KO_COLOR_DISPLAY_RENDERER_INTERFACE_H */
