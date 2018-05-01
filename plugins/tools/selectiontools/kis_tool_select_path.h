/*
 *  Copyright (c) 2007 Sven Langkamp <sven.langkamp@gmail.com>
 *  Copyright (c) 2015 Michael Abrahams <miabraha@gmail.com>
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

#ifndef KIS_TOOL_SELECT_PATH_H_
#define KIS_TOOL_SELECT_PATH_H_

#include <KoCreatePathTool.h>
#include <KoToolFactoryBase.h>
#include "kis_tool_select_base.h"
#include "kis_delegated_tool.h"

#include <kis_icon.h>

class KoCanvasBase;
class KisToolSelectPath;


class __KisToolSelectPathLocalTool : public KoCreatePathTool {
public:
    __KisToolSelectPathLocalTool(KoCanvasBase * canvas, KisToolSelectPath* parentTool);
    virtual void paintPath(KoPathShape &path, QPainter &painter, const KoViewConverter &converter);
    virtual void addPathShape(KoPathShape* pathShape);

    using KoCreatePathTool::createOptionWidgets;
    using KoCreatePathTool::endPathWithoutLastPoint;
    using KoCreatePathTool::endPath;
    using KoCreatePathTool::cancelPath;
    using KoCreatePathTool::removeLastPoint;

private:
    KisToolSelectPath* const m_selectionTool;
};

typedef KisDelegatedTool<KisTool, __KisToolSelectPathLocalTool,
                         DeselectShapesActivationPolicy> DelegatedSelectPathTool;

struct KisDelegatedSelectPathWrapper : public DelegatedSelectPathTool {
    KisDelegatedSelectPathWrapper(KoCanvasBase *canvas,
                                  const QCursor &cursor,
                                  KisTool* delegateTool)
        : DelegatedSelectPathTool(canvas, cursor, (__KisToolSelectPathLocalTool*) delegateTool)
    {
    }
    bool listeningToModifiers();

    // If an event is explicitly forwarded only as an action (e.g. shift-click is captured by "change size")
    // we will receive a primary action but no mousePressEvent.  Thus these events must be explicitly forwarded.
    void beginPrimaryAction(KoPointerEvent *event);
    void continuePrimaryAction(KoPointerEvent *event);
    void endPrimaryAction(KoPointerEvent *event);
};


class KisToolSelectPath : public KisToolSelectBase<KisDelegatedSelectPathWrapper>
{
    Q_OBJECT
public:
    KisToolSelectPath(KoCanvasBase * canvas);
    void mousePressEvent(KoPointerEvent* event);
    bool eventFilter(QObject *obj, QEvent *event);

protected:
    void requestStrokeCancellation();
    void requestStrokeEnd();
    void setAlternateSelectionAction(SelectionAction action);
    friend class __KisToolSelectPathLocalTool;
    QList<QPointer<QWidget> > createOptionWidgets();
};

class KisToolSelectPathFactory : public KoToolFactoryBase
{
public:
    KisToolSelectPathFactory()
        : KoToolFactoryBase("KisToolSelectPath") {
        setToolTip(i18n("Bezier Curve Selection Tool"));
        setSection(TOOL_TYPE_SELECTION);
        setActivationShapeId(KRITA_TOOL_ACTIVATION_ID);
        setIconName(koIconNameCStr("tool_path_selection"));
        setPriority(6);
    }

    virtual ~KisToolSelectPathFactory() {}

    virtual KoToolBase * createTool(KoCanvasBase *canvas) {
        return new KisToolSelectPath(canvas);
    }
};



#endif // KIS_TOOL_SELECT_PATH_H_
