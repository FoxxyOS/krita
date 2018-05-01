/*
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
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
#ifndef KIS_BRUSH_CHOOSER_H_
#define KIS_BRUSH_CHOOSER_H_

#include <QLabel>
#include "kritapaintop_export.h"
#include <kis_brush.h>

class KisDoubleSliderSpinBox;
class QLabel;
class QCheckBox;

class KisDoubleSliderSpinBox;
class KisSpacingSelectionWidget;
class KisCustomBrushWidget;
class KisClipboardBrushWidget;
class KoResourceItemChooser;
class KoResource;

class PAINTOP_EXPORT KisBrushChooser : public QWidget
{

    Q_OBJECT

public:
    KisBrushChooser(QWidget *parent = 0, const char *name = 0);
    virtual ~KisBrushChooser();

    KisBrushSP brush() {
        return m_brush;
    };

    void setBrush(KisBrushSP _brush);
    void setBrushSize(qreal xPixels, qreal yPixels);
    void setImage(KisImageWSP image);

private Q_SLOTS:

    void slotResetBrush();
    void slotSetItemSize(qreal);
    void slotSetItemRotation(qreal);
    void slotSpacingChanged();
    void slotSetItemUseColorAsMask(bool);
    void slotActivatedBrush(KoResource *);
    void slotOpenStampBrush();
    void slotOpenClipboardBrush();
    void slotNewPredefinedBrush(KoResource *);
    void update(KoResource *);

Q_SIGNALS:

    void sigBrushChanged();

private:
    QLabel* m_lbName;
    QLabel* m_lbRotation;
    QLabel* m_lbSize;
    QLabel* m_lbSpacing;
    KisDoubleSliderSpinBox* m_slRotation;
    KisDoubleSliderSpinBox* m_slSize;
    KisSpacingSelectionWidget* m_slSpacing;
    QCheckBox* m_chkColorMask;
    KisBrushSP m_brush;
    KoResourceItemChooser* m_itemChooser;
    KisImageWSP m_image;
    KisCustomBrushWidget* m_stampBrushWidget;
    KisClipboardBrushWidget* m_clipboardBrushWidget;

};

#endif // KIS_BRUSH_CHOOSER_H_
