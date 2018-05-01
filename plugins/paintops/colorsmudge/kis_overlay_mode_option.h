/*
 *  Copyright (c) 2011 Silvio Heinrich <plassy@web.de>
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
#ifndef _KIS_OVERLAYMODE_OPTION_H_
#define _KIS_OVERLAYMODE_OPTION_H_

#include <QLabel>

#include <kis_paintop_option.h>

#include <brushengine/kis_paintop_lod_limitations.h>


class KisOverlayModeOption : public KisPaintOpOption
{
public:
    KisOverlayModeOption():
        KisPaintOpOption(KisPaintOpOption::GENERAL, false)
    {
        setObjectName("KisOverlayModeOption");
    }

    virtual bool isCheckable() const {
        return true;
    }

    virtual void writeOptionSetting(KisPropertiesConfigurationSP setting) const {
        setting->setProperty("MergedPaint", isChecked());
    }

    virtual void readOptionSetting(const KisPropertiesConfigurationSP setting) {
        bool enabled = setting->getBool("MergedPaint");
        setChecked(enabled);
    }

    virtual void lodLimitations(KisPaintopLodLimitations *l) const {
        l->blockers << KoID("colorsmudge-overlay", i18nc("PaintOp instant preview limitation", "Overlay Option"));
    }

};

class KisOverlayModeOptionWidget: public KisOverlayModeOption
{
public:
    KisOverlayModeOptionWidget() {
        QLabel* label = new QLabel(
            i18n("Paints on the current layer\n\
            but uses all layers that are currently visible for smudge input\n\
            NOTE: This mode is only able to work correctly with a fully opaque background")
        );

        label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        setConfigurationPage(label);
    }
};

#endif // _KIS_OVERLAYMODE_OPTION_H_
