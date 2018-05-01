/*
 *  Copyright (c) 1999 Matthias Elter <elter@kde.org>
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
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

#ifndef KIS_TOOL_COLOR_PICKER_H_
#define KIS_TOOL_COLOR_PICKER_H_

#include <QList>
#include <QTimer>

#include "KoToolFactoryBase.h"
#include "ui_wdgcolorpicker.h"
#include "kis_tool.h"
#include <flake/kis_node_shape.h>
#include <KoIcon.h>
#include <kis_icon.h>
#include <QKeySequence>

class KoResource;
class KoColorSet;

namespace KisToolUtils {
struct ColorPickerConfig;
}

class ColorPickerOptionsWidget : public QWidget, public Ui::ColorPickerOptionsWidget
{
    Q_OBJECT

public:
    ColorPickerOptionsWidget(QWidget *parent) : QWidget(parent) {
        setupUi(this);
    }
};

class KisToolColorPicker : public KisTool
{

    Q_OBJECT
    Q_PROPERTY(bool toForeground READ toForeground WRITE setToForeground NOTIFY toForegroundChanged)

public:
    KisToolColorPicker(KoCanvasBase* canvas);
    virtual ~KisToolColorPicker();

public:
    struct Configuration {
        Configuration();

        bool toForegroundColor;
        bool updateColor;
        bool addPalette;
        bool normaliseValues;
        bool sampleMerged;
        int radius;

        void save(ToolActivation activation) const;
        void load(ToolActivation activation);
    };

public:
    virtual QWidget* createOptionWidget();

    void beginPrimaryAction(KoPointerEvent *event);
    void continuePrimaryAction(KoPointerEvent *event);
    void endPrimaryAction(KoPointerEvent *event);

    virtual void paint(QPainter& gc, const KoViewConverter &converter);

    bool toForeground() const;

Q_SIGNALS:
    void toForegroundChanged();

protected:
    void activate(ToolActivation activation, const QSet<KoShape*> &);
    void deactivate();

public Q_SLOTS:
    void setToForeground(bool newValue);
    void slotSetUpdateColor(bool);
    void slotSetNormaliseValues(bool);
    void slotSetAddPalette(bool);
    void slotChangeRadius(int);
    void slotAddPalette(KoResource* resource);
    void slotSetColorSource(int value);

private:
    void displayPickedColor();
    void pickColor(const QPointF& pos);
    void updateOptionWidget();

    //Configuration m_config;
    QScopedPointer<KisToolUtils::ColorPickerConfig> m_config;
    ToolActivation m_toolActivationSource;
    bool m_isActivated;

    KoColor m_pickedColor;

    // used to skip some of the tablet events and don't update the colour that often
    QTimer m_colorPickerDelayTimer;

    ColorPickerOptionsWidget *m_optionsWidget;

    QList<KoColorSet*> m_palettes;
};

class KisToolColorPickerFactory : public KoToolFactoryBase
{

public:
    KisToolColorPickerFactory()
            : KoToolFactoryBase("KritaSelected/KisToolColorPicker") {
        setToolTip(i18n("Color Selector Tool"));
        setSection(TOOL_TYPE_FILL);
        setPriority(2);
        setIconName(koIconNameCStr("krita_tool_color_picker"));
        setShortcut(QKeySequence(Qt::Key_P));
        setActivationShapeId(KRITA_TOOL_ACTIVATION_ID);
    }

    virtual ~KisToolColorPickerFactory() {}

    virtual KoToolBase * createTool(KoCanvasBase *canvas) {
        return new KisToolColorPicker(canvas);
    }
};


#endif // KIS_TOOL_COLOR_PICKER_H_

