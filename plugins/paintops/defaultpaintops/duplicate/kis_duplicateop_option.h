/*
 *  Copyright (c) 2008 Boudewijn Rempt <boud@valdyas.org>
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
#ifndef KIS_DUPLICATEOP_OPTION_H
#define KIS_DUPLICATEOP_OPTION_H

#include <kis_paintop_option.h>

const QString DUPLICATE_HEALING = "Duplicateop/Healing";
const QString DUPLICATE_CORRECT_PERSPECTIVE = "Duplicateop/CorrectPerspective";
const QString DUPLICATE_MOVE_SOURCE_POINT = "Duplicateop/MoveSourcePoint";
const QString DUPLICATE_CLONE_FROM_PROJECTION = "Duplicateop/CloneFromProjection";

class KisDuplicateOpOptionsWidget;

class KisDuplicateOpOption : public KisPaintOpOption
{
public:
    KisDuplicateOpOption();

    ~KisDuplicateOpOption();
private:
    bool healing() const;
    void setHealing(bool healing);

    bool correctPerspective() const;
    void setPerspective(bool perspective);

    bool moveSourcePoint() const;
    void setMoveSourcePoint(bool move);

    bool cloneFromProjection() const;
    void setCloneFromProjection(bool cloneFromProjection);
public:
    void writeOptionSetting(KisPropertiesConfigurationSP setting) const;

    void readOptionSetting(const KisPropertiesConfigurationSP setting);

    void setImage(KisImageWSP image);

private:

    KisDuplicateOpOptionsWidget * m_optionWidget;

};

struct DuplicateOption : public KisBaseOption
{
    bool duplicate_healing;
    bool duplicate_correct_perspective;
    bool duplicate_move_source_point;
    bool duplicate_clone_from_projection;

    void readOptionSettingImpl(const KisPropertiesConfiguration* setting) {
        duplicate_healing = setting->getBool(DUPLICATE_HEALING, false);
        duplicate_correct_perspective = setting->getBool(DUPLICATE_CORRECT_PERSPECTIVE, false);
        duplicate_move_source_point = setting->getBool(DUPLICATE_MOVE_SOURCE_POINT, true);
        duplicate_clone_from_projection = setting->getBool(DUPLICATE_CLONE_FROM_PROJECTION, false);
    }

    void writeOptionSettingImpl(KisPropertiesConfiguration *setting) const {
        setting->setProperty(DUPLICATE_HEALING, duplicate_healing);
        setting->setProperty(DUPLICATE_CORRECT_PERSPECTIVE, duplicate_correct_perspective);
        setting->setProperty(DUPLICATE_MOVE_SOURCE_POINT, duplicate_move_source_point);
        setting->setProperty(DUPLICATE_CLONE_FROM_PROJECTION, duplicate_clone_from_projection);
    }
};

#endif
