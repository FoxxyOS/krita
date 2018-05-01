/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2012
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

#ifndef KIS_TEXTURE_OPTION_H
#define KIS_TEXTURE_OPTION_H

#include <kritapaintop_export.h>

#include <kis_paint_device.h>
#include <kis_types.h>
#include "kis_paintop_option.h"
#include "kis_pressure_texture_strength_option.h"

#include <QRect>

class KisTextureOptionWidget;
class KoPattern;
class KoResource;
class KisPropertiesConfiguration;
class KisPaintopLodLimitations;

class PAINTOP_EXPORT KisTextureOption : public KisPaintOpOption
{
    Q_OBJECT
public:

    explicit KisTextureOption();
    virtual ~KisTextureOption();

public Q_SLOTS:

    virtual void writeOptionSetting(KisPropertiesConfigurationSP setting) const;
    virtual void readOptionSetting(const KisPropertiesConfigurationSP setting);
    virtual void lodLimitations(KisPaintopLodLimitations *l) const;

private Q_SLOTS:

    void resetGUI(KoResource*); /// called when a new pattern is selected

private:
    KisTextureOptionWidget *m_optionWidget;


};

class PAINTOP_EXPORT KisTextureProperties
{
public:
    KisTextureProperties(int levelOfDetail);

    enum TexturingMode {
        MULTIPLY,
        SUBTRACT
    };

    bool m_enabled;

    /**
     * @brief apply combine the texture map with the dab
     * @param dab the colored, final representation of the dab, after mirroring and everything.
     * @param offset the position of the dab on the image. used to calculate the position of the mask pattern
     */
    void apply(KisFixedPaintDeviceSP dab, const QPoint& offset, const KisPaintInformation & info);
    void fillProperties(const KisPropertiesConfigurationSP setting);

private:

    qreal m_scale;
    int m_offsetX;
    int m_offsetY;
    TexturingMode m_texturingMode;
    bool m_invert;
    KoPattern *m_pattern;
    int m_cutoffLeft;
    int m_cutoffRight;
    int m_cutoffPolicy;

    int m_levelOfDetail;

private:
    KisPressureTextureStrengthOption m_strengthOption;
    QRect m_maskBounds; // this can be different from the extent if we mask out too many pixels in a big mask!
    KisPaintDeviceSP m_mask;
    void recalculateMask();
};

#endif // KIS_TEXTURE_OPTION_H
