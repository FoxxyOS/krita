/*
 *  Copyright (c) 2014 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_LIQUIFY_TRANSFORM_STRATEGY_H
#define __KIS_LIQUIFY_TRANSFORM_STRATEGY_H

#include <QObject>
#include <QScopedPointer>

#include "kis_transform_strategy_base.h"

class QPointF;
class QPainter;
class KisCoordinatesConverter;
class ToolTransformArgs;
class TransformTransactionProperties;
class QCursor;


class KisLiquifyTransformStrategy : public KisTransformStrategyBase
{
    Q_OBJECT
public:
    KisLiquifyTransformStrategy(const KisCoordinatesConverter *converter,
                             ToolTransformArgs &currentArgs,
                             TransformTransactionProperties &transaction, const KoCanvasResourceManager *manager);
    ~KisLiquifyTransformStrategy();

    void setTransformFunction(const QPointF &mousePos, bool perspectiveModifierActive);
    void paint(QPainter &gc);
    QCursor getCurrentCursor() const;
    QPainterPath getCursorOutline() const;

    bool acceptsClicks() const;

    void externalConfigChanged();

    bool beginPrimaryAction(KoPointerEvent *event);
    void continuePrimaryAction(KoPointerEvent *event);
    bool endPrimaryAction(KoPointerEvent *event);
    void hoverActionCommon(KoPointerEvent *event);

    void activateAlternateAction(KisTool::AlternateAction action);
    void deactivateAlternateAction(KisTool::AlternateAction action);

    bool beginAlternateAction(KoPointerEvent *event, KisTool::AlternateAction action);
    void continueAlternateAction(KoPointerEvent *event, KisTool::AlternateAction action);
    bool endAlternateAction(KoPointerEvent *event, KisTool::AlternateAction action);

Q_SIGNALS:
    void requestCanvasUpdate();
    void requestUpdateOptionWidget();
    void requestCursorOutlineUpdate(const QPointF &imagePoint);

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif /* __KIS_LIQUIFY_TRANSFORM_STRATEGY_H */
