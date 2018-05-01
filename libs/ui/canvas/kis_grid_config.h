/*
 *  Copyright (c) 2015 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_GRID_CONFIG_H
#define __KIS_GRID_CONFIG_H

#include <QPoint>
#include <QColor>
#include <QPen>

#include <boost/operators.hpp>
#include "kritaui_export.h"

class QDomElement;
class QDomDocument;


class KRITAUI_EXPORT KisGridConfig : boost::equality_comparable<KisGridConfig>
{
public:
    enum LineTypeInternal {
        LINE_SOLID = 0,
        LINE_DASHED,
        LINE_DOTTED
    };
public:
    KisGridConfig()
        : m_showGrid(false),
          m_snapToGrid(false),
          m_spacing(20,20),
          m_offsetAspectLocked(true),
          m_spacingAspectLocked(true),
          m_subdivision(2),
          m_lineTypeMain(LINE_SOLID),
          m_lineTypeSubdivision(LINE_DOTTED),
          m_colorMain(200, 200, 200, 200),
          m_colorSubdivision(200, 200, 200, 150)
    {
        loadStaticData();
    }

    bool operator==(const KisGridConfig &rhs) const {
        return
            m_showGrid == rhs.m_showGrid &&
            m_snapToGrid == rhs.m_snapToGrid &&
            m_spacing == rhs.m_spacing &&
            m_offset == rhs.m_offset &&
            m_offsetAspectLocked == rhs.m_offsetAspectLocked &&
            m_spacingAspectLocked == rhs.m_spacingAspectLocked &&
            m_subdivision == rhs.m_subdivision &&
            m_lineTypeMain == rhs.m_lineTypeMain &&
            m_lineTypeSubdivision == rhs.m_lineTypeSubdivision &&
            m_colorMain == rhs.m_colorMain &&
            m_colorSubdivision == rhs.m_colorSubdivision;
    }

    bool showGrid() const {
        return m_showGrid;
    }
    void setShowGrid(bool value) {
        m_showGrid = value;
    }

    bool snapToGrid() const {
        return m_snapToGrid;
    }
    void setSnapToGrid(bool value) {
        m_snapToGrid = value;
    }

    QPoint offset() const {
        return m_offset;
    }
    void setOffset(const QPoint &value) {
        m_offset = value;
    }

    QPoint spacing() const {
        return m_spacing;
    }
    void setSpacing(const QPoint &value) {
        m_spacing = value;
    }

    int subdivision() const {
        return m_subdivision;
    }
    void setSubdivision(int value) {
        m_subdivision = value;
    }

    bool offsetAspectLocked() const {
        return m_offsetAspectLocked;
    }
    void setOffsetAspectLocked(bool value) {
        m_offsetAspectLocked = value;
    }

    bool spacingAspectLocked() const {
        return m_spacingAspectLocked;
    }
    void setSpacingAspectLocked(bool value) {
        m_spacingAspectLocked = value;
    }


    LineTypeInternal lineTypeMain() const {
        return m_lineTypeMain;
    }
    void setLineTypeMain(LineTypeInternal value) {
        m_lineTypeMain = value;
    }

    LineTypeInternal lineTypeSubdivision() const {
        return m_lineTypeSubdivision;
    }
    void setLineTypeSubdivision(LineTypeInternal value) {
        m_lineTypeSubdivision = value;
    }

    QColor colorMain() const {
        return m_colorMain;
    }
    void setColorMain(const QColor &value) {
        m_colorMain = value;
    }

    QColor colorSubdivision() const {
        return m_colorSubdivision;
    }
    void setColorSubdivision(const QColor &value) {
        m_colorSubdivision = value;
    }

    QPen penMain() const {
        return QPen(m_colorMain, 0, toPenStyle(m_lineTypeMain));
    }

    QPen penSubdivision() const {
        return QPen(m_colorSubdivision, 0, toPenStyle(m_lineTypeSubdivision));
    }

    void loadStaticData();
    void saveStaticData() const;

    QDomElement saveDynamicDataToXml(QDomDocument& doc, const QString &tag) const;
    bool loadDynamicDataFromXml(const QDomElement &parent);

    static const KisGridConfig& defaultGrid();

    bool isDefault() const {
        return *this == defaultGrid();
    }

private:
    static Qt::PenStyle toPenStyle(LineTypeInternal type) {
        return type == LINE_SOLID ? Qt::SolidLine :
            type == LINE_DASHED ? Qt::DashLine :
            type == LINE_DOTTED ? Qt::DotLine :
            Qt::DashDotDotLine;
    }
private:
    // Dynamic data. Stored in KisDocument.

    bool m_showGrid;
    bool m_snapToGrid;

    QPoint m_offset;
    QPoint m_spacing;

    bool m_offsetAspectLocked;
    bool m_spacingAspectLocked;

    int m_subdivision;

    // Static data. Stored in the Krita config.

    LineTypeInternal m_lineTypeMain;
    LineTypeInternal m_lineTypeSubdivision;

    QColor m_colorMain;
    QColor m_colorSubdivision;
};

Q_DECLARE_METATYPE(KisGridConfig)

#endif /* __KIS_GRID_CONFIG_H */
