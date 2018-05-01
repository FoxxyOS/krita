/*
 *  Copyright (c) 2009-2010 Lukáš Tvrdý <lukast.dev@gmail.com>
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

#ifndef KIS_DYNA_PAINTOP_SETTINGS_H_
#define KIS_DYNA_PAINTOP_SETTINGS_H_

#include <QScopedPointer>
#include <brushengine/kis_paintop_settings.h>

class KisDynaPaintOpSettings : public KisPaintOpSettings
{

public:
    KisDynaPaintOpSettings();
    ~KisDynaPaintOpSettings();

    void setPaintOpSize(qreal value) override;
    qreal paintOpSize() const override;

    bool paintIncremental() override;
    bool isAirbrushing() const override;
    int rate() const override;

    QList<KisUniformPaintOpPropertySP> uniformProperties(KisPaintOpSettingsSP settings) override;

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif
