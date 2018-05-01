/*
  Copyright (c) 2006 Gábor Lehel <illissius@gmail.com>
  Copyright (c) 2008 Cyrille Berger <cberger@cberger.net>
  Copyright (c) 2011 José Luis Vergara <pentalis@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/
#include "kis_config.h"
#include "KisNodeDelegate.h"
#include "kis_node_model.h"
#include "KisNodeToolTip.h"
#include "KisNodeView.h"
#include "KisPart.h"
#include "input/kis_input_manager.h"

#include <QtDebug>
#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QStyle>
#include <QStyleOptionViewItem>

#include <klocalizedstring.h>
#include "kis_node_view_color_scheme.h"
#include "kis_icon_utils.h"
#include "kis_layer_properties_icons.h"
#include "krita_utils.h"

typedef KisBaseNode::Property* OptionalProperty;

#include <kis_base_node.h>

class KisNodeDelegate::Private
{
public:
    Private() : view(0), edit(0) { }

    KisNodeView *view;
    QPointer<QWidget> edit;
    KisNodeToolTip tip;

    QList<OptionalProperty> rightmostProperties(const KisBaseNode::PropertyList &props) const;
    int numProperties(const QModelIndex &index) const;
    OptionalProperty findProperty(KisBaseNode::PropertyList &props, const OptionalProperty &refProp) const;
    OptionalProperty findVisibilityProperty(KisBaseNode::PropertyList &props) const;

    void toggleProperty(KisBaseNode::PropertyList &props, OptionalProperty prop, bool controlPressed, const QModelIndex &index);
};

void KisNodeDelegate::slotOnCloseEditor()
{
    KisPart::currentInputManager()->slotFocusOnEnter(true);
}

KisNodeDelegate::KisNodeDelegate(KisNodeView *view, QObject *parent)
    : QAbstractItemDelegate(parent)
    , d(new Private)
{
    d->view = view;
    QApplication::instance()->installEventFilter(this);

    connect(this, SIGNAL(closeEditor(QWidget*)), this, SLOT(slotOnCloseEditor()));
}

KisNodeDelegate::~KisNodeDelegate()
{
    delete d;
}

QSize KisNodeDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    KisNodeViewColorScheme scm;
    return QSize(option.rect.width(), scm.rowHeight());
}

void KisNodeDelegate::paint(QPainter *p, const QStyleOptionViewItem &o, const QModelIndex &index) const
{
    p->save();

    {
        QStyleOptionViewItem option = getOptions(o, index);
        QStyle *style = option.widget ? option.widget->style() : QApplication::style();
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, p, option.widget);

        bool shouldGrayOut = index.data(KisNodeModel::ShouldGrayOutRole).toBool();
        if (shouldGrayOut) {
            option.state &= ~QStyle::State_Enabled;
        }

        p->setFont(option.font);

        drawColorLabel(p, option, index);
        drawFrame(p, option, index);
        drawThumbnail(p, option, index);
        drawText(p, option, index);
        drawIcons(p, option, index);
        drawVisibilityIconHijack(p, option, index);
        drawDecoration(p, option, index);
        drawExpandButton(p, option, index);
        drawBranch(p, option, index);

        drawProgressBar(p, option, index);
    }
    p->restore();
}

void KisNodeDelegate::drawBranch(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    Q_UNUSED(index);

    KisNodeViewColorScheme scm;
    const QPoint base = scm.relThumbnailRect().translated(option.rect.topLeft()).topLeft() - QPoint( scm.indentation(), 0);

    // there is no indention if we are starting negative, so don't draw a branch
    if (base.x() < 0) {
        return;
    }


    QPen oldPen = p->pen();
    const qreal oldOpacity = p->opacity(); // remember previous opacity
    p->setOpacity(1.0);

    QColor color = scm.gridColor(option, d->view);
    QColor bgColor = option.state & QStyle::State_Selected ?
        qApp->palette().color(QPalette::Base) :
        qApp->palette().color(QPalette::Text);
    color = KritaUtils::blendColors(color, bgColor, 0.9);


    // TODO: if we are a mask type, use dotted lines for the branch style
    // p->setPen(QPen(p->pen().color(), 2, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
    p->setPen(QPen(color, 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    QPoint p2 = base + QPoint(scm.iconSize() - scm.decorationMargin()*2, scm.iconSize()*0.45);
    QPoint p3 =  base + QPoint(scm.iconSize() - scm.decorationMargin()*2, scm.iconSize());
    QPoint p4 = base + QPoint(scm.iconSize()*1.4, scm.iconSize());
    p->drawLine(p2, p3);
    p->drawLine(p3, p4);



     // draw parent lines (keep drawing until x position is less than 0
    QPoint p5 = p2 - QPoint(scm.indentation(), 0);
    QPoint p6 = p3 - QPoint(scm.indentation(), 0);

     QPoint parentBase1 = p5;
     QPoint parentBase2 = p6;

     // indent lines needs to be very subtle to avoid making the docker busy looking
     color = KritaUtils::blendColors(color, bgColor, 0.9); // makes it a little lighter than L lines
     p->setPen(QPen(color, 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

     while (parentBase1.x() > scm.visibilityColumnWidth()) {
         p->drawLine(parentBase1, parentBase2);

         parentBase1 = parentBase1 - QPoint(scm.indentation(), 0);
         parentBase2 = parentBase2 - QPoint(scm.indentation(), 0);
     }




     p->setPen(oldPen);
     p->setOpacity(oldOpacity);
}

void KisNodeDelegate::drawColorLabel(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    KisNodeViewColorScheme scm;
    const int label = index.data(KisNodeModel::ColorLabelIndexRole).toInt();
    QColor color = scm.colorLabel(label);
    if (color.alpha() <= 0) return;

    QColor bgColor = qApp->palette().color(QPalette::Base);
    color = KritaUtils::blendColors(color, bgColor, 0.3);

    const QRect rect = option.state & QStyle::State_Selected ?
        iconsRect(option, index) :
        option.rect.adjusted(-scm.indentation(), 0, 0, 0);

    p->fillRect(rect, color);
}

void KisNodeDelegate::drawFrame(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    KisNodeViewColorScheme scm;

    QPen oldPen = p->pen();
    p->setPen(scm.gridColor(option, d->view));

    const QPoint base = option.rect.topLeft();

    QPoint p2 = base + QPoint(-scm.indentation() - 1, 0);
    QPoint p3 = base + QPoint(2 * scm.decorationMargin() + scm.decorationSize(), 0);
    QPoint p4 = base + QPoint(-1, 0);
    QPoint p5(iconsRect(option,
                           index).left() - 1, base.y());
    QPoint p6(option.rect.right(), base.y());

    QPoint v(0, option.rect.height());


    // draw a line that goes the length of the entire frame. one for the
    // top, and one for the bottom
    QPoint pTopLeft(0, option.rect.topLeft().y());
    QPoint pTopRight(option.rect.bottomRight().x(),option.rect.topLeft().y() );
    p->drawLine(pTopLeft, pTopRight);

    QPoint pBottomLeft(0, option.rect.topLeft().y() + scm.rowHeight());
    QPoint pBottomRight(option.rect.bottomRight().x(),option.rect.topLeft().y() + scm.rowHeight() );
    p->drawLine(pBottomLeft, pBottomRight);


    const bool paintForParent =
        index.parent().isValid() &&
        !index.row();

    if (paintForParent) {
        QPoint p1(-2 * scm.indentation() - 1, 0);
        p1 += base;
        p->drawLine(p1, p2);
    }


    QPoint k0(0, base.y());
    QPoint k1(1 * scm.border() + 2 * scm.visibilityMargin() + scm.visibilitySize(), base.y());
    p->drawLine(k0, k1);
    p->drawLine(k0 + v, k1 + v);
    p->drawLine(k0, k0 + v);
    p->drawLine(k1, k1 + v);

    p->drawLine(p2, p6);
    p->drawLine(p2 + v, p6 + v);
    p->drawLine(p2, p2 + v);
    p->drawLine(p3, p3 + v);
    p->drawLine(p4, p4 + v);
    p->drawLine(p5, p5 + v);
    p->drawLine(p6, p6 + v);

    //// For debugging purposes only
    //p->setPen(Qt::blue);
    //KritaUtils::renderExactRect(p, iconsRect(option, index));
    //KritaUtils::renderExactRect(p, textRect(option, index));
    //KritaUtils::renderExactRect(p, scm.relThumbnailRect().translated(option.rect.topLeft()));

    p->setPen(oldPen);
}

void KisNodeDelegate::drawThumbnail(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    KisNodeViewColorScheme scm;

    const int thumbSize = scm.thumbnailSize();
    const qreal oldOpacity = p->opacity(); // remember previous opacity

    QImage img = index.data(int(KisNodeModel::BeginThumbnailRole) + thumbSize).value<QImage>();
    if (!(option.state & QStyle::State_Enabled)) {
        p->setOpacity(0.35);
    }

    QRect fitRect = scm.relThumbnailRect().translated(option.rect.topLeft());

    QPoint offset;
    offset.setX((fitRect.width() - img.width()) / 2);
    offset.setY((fitRect.height() - img.height()) / 2);
    offset += fitRect.topLeft();

    KisConfig cfg;

    // paint in a checkerboard pattern behind the layer contents to represent transparent
    const int step = scm.thumbnailSize() / 6;
    QImage checkers(2 * step, 2 * step, QImage::Format_ARGB32);
    QPainter gc(&checkers);
    gc.fillRect(QRect(0, 0, step, step), cfg.checkersColor1());
    gc.fillRect(QRect(step, 0, step, step), cfg.checkersColor2());
    gc.fillRect(QRect(step, step, step, step), cfg.checkersColor1());
    gc.fillRect(QRect(0, step, step, step), cfg.checkersColor2());

    QBrush brush(checkers);
    p->setBrushOrigin(offset);
    p->fillRect(img.rect().translated(offset), brush);

    p->drawImage(offset, img);
    p->setOpacity(oldOpacity); // restore old opacity

    QRect borderRect = kisGrowRect(img.rect(), 1).translated(offset);
    KritaUtils::renderExactRect(p, borderRect, scm.gridColor(option, d->view));
}

QRect KisNodeDelegate::iconsRect(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    KisNodeViewColorScheme scm;

    int propCount = d->numProperties(index);

    const int iconsWidth =
        propCount * (scm.iconSize() + 2 * scm.iconMargin()) +
        (propCount - 1) * scm.border();

    const int x = option.rect.x() + option.rect.width()
        - (iconsWidth + scm.border());
    const int y = option.rect.y() + scm.border();

    return QRect(x, y,
                 iconsWidth,
                 scm.rowHeight() - scm.border());
}

QRect KisNodeDelegate::textRect(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    KisNodeViewColorScheme scm;

    static QFont f;
    static int minbearing = 1337 + 666; //can be 0 or negative, 2003 is less likely
    if (minbearing == 2003 || f != option.font) {
        f = option.font; //getting your bearings can be expensive, so we cache them
        minbearing = option.fontMetrics.minLeftBearing() + option.fontMetrics.minRightBearing();
    }

    const int decorationOffset =
        2 * scm.border() +
        2 * scm.decorationMargin() +
        scm.decorationSize();

    const int width =
        iconsRect(option, index).left() - option.rect.x() -
        scm.border() + minbearing - decorationOffset;

    return QRect(option.rect.x() - minbearing + decorationOffset,
                 option.rect.y() + scm.border(),
                 width,
                 scm.rowHeight() - scm.border());
}

void KisNodeDelegate::drawText(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    KisNodeViewColorScheme scm;
    const QRect rc = textRect(option, index)
        .adjusted(scm.textMargin(), 0, -scm.textMargin(), 0);

    QPen oldPen = p->pen();
    const qreal oldOpacity = p->opacity(); // remember previous opacity

    p->setPen(option.palette.color(QPalette::Active,QPalette::Text ));



    if (!(option.state & QStyle::State_Enabled)) {
        p->setOpacity(0.55);
    }


    const QString text = index.data(Qt::DisplayRole).toString();
    const QString elided = elidedText(p->fontMetrics(), rc.width(), Qt::ElideRight, text);
    p->drawText(rc, Qt::AlignLeft | Qt::AlignVCenter, elided);

    p->setPen(oldPen); // restore pen settings
    p->setOpacity(oldOpacity);
}

QList<OptionalProperty> KisNodeDelegate::Private::rightmostProperties(const KisBaseNode::PropertyList &props) const
{
    QList<OptionalProperty> list;
    QList<OptionalProperty> prependList;
    list << OptionalProperty(0);
    list << OptionalProperty(0);
    list << OptionalProperty(0);

    KisBaseNode::PropertyList::const_iterator it = props.constBegin();
    KisBaseNode::PropertyList::const_iterator end = props.constEnd();
    for (; it != end; ++it) {
        if (!it->isMutable) continue;

        if (it->id == KisLayerPropertiesIcons::visible.id()) {
            // noop...
        } else if (it->id == KisLayerPropertiesIcons::locked.id()) {
            list[0] = OptionalProperty(&(*it));
        } else if (it->id == KisLayerPropertiesIcons::inheritAlpha.id()) {
            list[1] = OptionalProperty(&(*it));
        } else if (it->id == KisLayerPropertiesIcons::alphaLocked.id()) {
            list[2] = OptionalProperty(&(*it));
        } else {
            prependList.prepend(OptionalProperty(&(*it)));
        }
    }

    {
        QMutableListIterator<OptionalProperty> i(prependList);
        i.toBack();
        while (i.hasPrevious()) {
            OptionalProperty val = i.previous();

            int emptyIndex = list.lastIndexOf(0);
            if (emptyIndex < 0) break;

            list[emptyIndex] = val;
            i.remove();
        }
    }

    return prependList + list;
}

int KisNodeDelegate::Private::numProperties(const QModelIndex &index) const
{
    KisBaseNode::PropertyList props = index.data(KisNodeModel::PropertiesRole).value<KisBaseNode::PropertyList>();
    QList<OptionalProperty> realProps = rightmostProperties(props);
    return realProps.size();
}

OptionalProperty KisNodeDelegate::Private::findProperty(KisBaseNode::PropertyList &props, const OptionalProperty &refProp) const
{
    KisBaseNode::PropertyList::iterator it = props.begin();
    KisBaseNode::PropertyList::iterator end = props.end();
    for (; it != end; ++it) {
        if (it->id == refProp->id) {
            return &(*it);
        }
    }

    return 0;
}

OptionalProperty KisNodeDelegate::Private::findVisibilityProperty(KisBaseNode::PropertyList &props) const
{
    KisBaseNode::PropertyList::iterator it = props.begin();
    KisBaseNode::PropertyList::iterator end = props.end();
    for (; it != end; ++it) {
        if (it->id == KisLayerPropertiesIcons::visible.id()) {
            return &(*it);
        }
    }

    return 0;
}

void KisNodeDelegate::drawIcons(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    KisNodeViewColorScheme scm;
    const QRect r = iconsRect(option, index);

    QTransform oldTransform = p->transform();
    QPen oldPen = p->pen();
    p->setTransform(QTransform::fromTranslate(r.x(), r.y()));
    p->setPen(scm.gridColor(option, d->view));

    int x = 0;
    const int y = (scm.rowHeight() - scm.border() - scm.iconSize()) / 2;
    KisBaseNode::PropertyList props = index.data(KisNodeModel::PropertiesRole).value<KisBaseNode::PropertyList>();
    QList<OptionalProperty> realProps = d->rightmostProperties(props);

    Q_FOREACH (OptionalProperty prop, realProps) {
        x += scm.iconMargin();
        if (prop) {
            QIcon icon = prop->state.toBool() ? prop->onIcon : prop->offIcon;
            bool fullColor = prop->state.toBool() && option.state & QStyle::State_Enabled;
            const qreal oldOpacity = p->opacity(); // remember previous opacity


            if (fullColor) {              
                 p->setOpacity(1.0);
            }
            else {
                p->setOpacity(0.35);
            }

            p->drawPixmap(x, y, icon.pixmap(scm.iconSize(), QIcon::Normal));
            p->setOpacity(oldOpacity); // restore old opacity


        }
        x += scm.iconSize() + scm.iconMargin();
        p->drawLine(x, 0, x, scm.rowHeight() - scm.border());
        x += scm.border();
    }

    p->setTransform(oldTransform);
    p->setPen(oldPen);
}

QRect KisNodeDelegate::visibilityClickRect(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    KisNodeViewColorScheme scm;
    return QRect(scm.border(), scm.border() + option.rect.top(),
                 2 * scm.visibilityMargin() + scm.visibilitySize(),
                 scm.rowHeight() - scm.border());
}

QRect KisNodeDelegate::decorationClickRect(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    KisNodeViewColorScheme scm;

    QRect realVisualRect = d->view->originalVisualRect(index);

    return QRect(realVisualRect.left(), scm.border() + realVisualRect.top(),
                 2 * scm.decorationMargin() + scm.decorationSize(),
                 scm.rowHeight() - scm.border());
}

void KisNodeDelegate::drawVisibilityIconHijack(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    /**
     * Small hack Alert:
     *
     * Here wepaint over the area that sits basically outside our layer's
     * row. Anyway, just update it later...
     */

    KisNodeViewColorScheme scm;

    KisBaseNode::PropertyList props = index.data(KisNodeModel::PropertiesRole).value<KisBaseNode::PropertyList>();
    OptionalProperty prop = d->findVisibilityProperty(props);
    if (!prop) return;

    const int x = scm.border() + scm.visibilityMargin();
    const int y = option.rect.top() + (scm.rowHeight() - scm.border() - scm.visibilitySize()) / 2;

    QIcon icon = prop->state.toBool() ? prop->onIcon : prop->offIcon;
    p->setOpacity(1.0);
    p->drawPixmap(x, y, icon.pixmap(scm.visibilitySize(),  QIcon::Normal));

    //// For debugging purposes only
    // p->save();
    // p->setPen(Qt::blue);
    // KritaUtils::renderExactRect(p, visibilityClickRect(option, index));
    // p->restore();
}

void KisNodeDelegate::drawDecoration(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    KisNodeViewColorScheme scm;
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();

    if (!icon.isNull()) {
        QPixmap pixmap =
            icon.pixmap(scm.decorationSize(),
                        (option.state & QStyle::State_Enabled) ?
                        QIcon::Normal : QIcon::Disabled);

        const QRect rc = scm.relDecorationRect().translated(option.rect.topLeft());
        const qreal oldOpacity = p->opacity(); // remember previous opacity

        if (!(option.state & QStyle::State_Enabled)) {
            p->setOpacity(0.35);
        }


        p->drawPixmap(rc.topLeft(), pixmap);
        p->setOpacity(oldOpacity); // restore old opacity
    }
}

void KisNodeDelegate::drawExpandButton(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    KisNodeViewColorScheme scm;
    QRect rc = scm.relExpandButtonRect().translated(option.rect.topLeft());
    rc = kisGrowRect(rc, 0);

    if (!(option.state & QStyle::State_Children)) {
        return;
    }

    QString iconName = option.state & QStyle::State_Open ? "arrow-down" : "arrow-right";
    QIcon icon = KisIconUtils::loadIcon(iconName);
    QPixmap pixmap = icon.pixmap(rc.width(),
                                 (option.state & QStyle::State_Enabled) ?
                                 QIcon::Normal : QIcon::Disabled);
    p->drawPixmap(rc.topLeft(), pixmap);
}

void KisNodeDelegate::Private::toggleProperty(KisBaseNode::PropertyList &props, OptionalProperty clickedProperty, bool controlPressed, const QModelIndex &index)
{
    QAbstractItemModel *model = view->model();

    // Using Ctrl+click to enter stasis
    if (controlPressed && clickedProperty->canHaveStasis) {
        // STEP 0: Prepare to Enter or Leave control key stasis
        quint16 numberOfLeaves = model->rowCount(index.parent());
        QModelIndex eachItem;
        // STEP 1: Go.
        if (clickedProperty->isInStasis == false) { // Enter
            /* Make every leaf of this node go State = False, saving the old property value to stateInStasis */
            for (quint16 i = 0; i < numberOfLeaves; ++i) { // Foreach leaf in the node (index.parent())
                eachItem = model->index(i, 1, index.parent());
                // The entire property list has to be altered because model->setData cannot set individual properties
                KisBaseNode::PropertyList eachPropertyList = eachItem.data(KisNodeModel::PropertiesRole).value<KisBaseNode::PropertyList>();
                OptionalProperty prop = findProperty(eachPropertyList, clickedProperty);
                if (!prop) continue;
                prop->stateInStasis = prop->state.toBool();
                prop->state = eachItem == index;
                prop->isInStasis = true;
                model->setData(eachItem, QVariant::fromValue(eachPropertyList), KisNodeModel::PropertiesRole);

            }

            for (quint16 i = 0; i < numberOfLeaves; ++i) { // Foreach leaf in the node (index.parent())
                eachItem = model->index(i, 1, index.parent());
                KisBaseNode::PropertyList eachPropertyList = eachItem.data(KisNodeModel::PropertiesRole).value<KisBaseNode::PropertyList>();
                OptionalProperty prop = findProperty(eachPropertyList, clickedProperty);
                if (!prop) continue;
            }
        } else { // Leave
            /* Make every leaf of this node go State = stateInStasis */
            for (quint16 i = 0; i < numberOfLeaves; ++i) {
                eachItem = model->index(i, 1, index.parent());
                // The entire property list has to be altered because model->setData cannot set individual properties
                KisBaseNode::PropertyList eachPropertyList = eachItem.data(KisNodeModel::PropertiesRole).value<KisBaseNode::PropertyList>();
                OptionalProperty prop = findProperty(eachPropertyList, clickedProperty);
                if (!prop) continue;

                prop->state = prop->stateInStasis;
                prop->isInStasis = false;
                model->setData(eachItem, QVariant::fromValue(eachPropertyList), KisNodeModel::PropertiesRole);
            }
        }
    } else {
        clickedProperty->state = !clickedProperty->state.toBool();
        model->setData(index, QVariant::fromValue(props), KisNodeModel::PropertiesRole);
    }
}

bool KisNodeDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    KisNodeViewColorScheme scm;

    if ((event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick)
        && (index.flags() & Qt::ItemIsEnabled))
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        /**
         * Small hack Alert:
         *
         * Here we handle clicking even when it happened outside
         * the rectangle of the current index. The point is, we
         * use some virtual scroling offset to move the tree to the
         * right of the visibility icon. So the icon itself is placed
         * in an empty area that doesn't belong to any index. But we still
         * handle it.
         */

        const QRect iconsRect = this->iconsRect(option, index);
        const bool iconsClicked = iconsRect.isValid() &&
            iconsRect.contains(mouseEvent->pos());

        const QRect visibilityRect = visibilityClickRect(option, index);
        const bool visibilityClicked = visibilityRect.isValid() &&
            visibilityRect.contains(mouseEvent->pos());

        const QRect decorationRect = decorationClickRect(option, index);
        const bool decorationClicked = decorationRect.isValid() &&
            decorationRect.contains(mouseEvent->pos());

        const bool leftButton = mouseEvent->buttons() & Qt::LeftButton;

        if (leftButton && iconsClicked) {
            KisBaseNode::PropertyList props = index.data(KisNodeModel::PropertiesRole).value<KisBaseNode::PropertyList>();
            QList<OptionalProperty> realProps = d->rightmostProperties(props);
            const int numProps = realProps.size();

            const int iconWidth = scm.iconSize() + 2 * scm.iconMargin() + scm.border();
            const int xPos = mouseEvent->pos().x() - iconsRect.left();
            const int clickedIcon = xPos / iconWidth;
            const int distToBorder = qMin(xPos % iconWidth, iconWidth - xPos % iconWidth);

            if (iconsClicked &&
                clickedIcon >= 0 &&
                clickedIcon < numProps &&
                distToBorder > scm.iconMargin()) {

                OptionalProperty clickedProperty = realProps[clickedIcon];
                if (!clickedProperty) return false;
                d->toggleProperty(props, clickedProperty, mouseEvent->modifiers() == Qt::ControlModifier, index);
                return true;
            }
        } else if (leftButton && visibilityClicked) {
            KisBaseNode::PropertyList props = index.data(KisNodeModel::PropertiesRole).value<KisBaseNode::PropertyList>();
            OptionalProperty clickedProperty = d->findVisibilityProperty(props);
            if (!clickedProperty) return false;
            d->toggleProperty(props, clickedProperty, mouseEvent->modifiers() == Qt::ControlModifier, index);
            return true;
        } else if (leftButton && decorationClicked) {
            bool isExpandable = model->hasChildren(index);
            if (isExpandable) {
                bool isExpanded = d->view->isExpanded(index);
                d->view->setExpanded(index, !isExpanded);
                return true;
            }
        }

        if (mouseEvent->button() == Qt::LeftButton &&
            mouseEvent->modifiers() == Qt::AltModifier) {

            d->view->setCurrentIndex(index);
            model->setData(index, true, KisNodeModel::AlternateActiveRole);
            return true;
        }
    }
    else if (event->type() == QEvent::ToolTip) {
        if (!KisConfig().hidePopups()) {
            QHelpEvent *helpEvent = static_cast<QHelpEvent*>(event);
            d->tip.showTip(d->view, helpEvent->pos(), option, index);
        }
        return true;
    } else if (event->type() == QEvent::Leave) {
        d->tip.hide();
    }

    return false;
}

QWidget *KisNodeDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem&, const QModelIndex&) const
{
    KisPart::currentInputManager()->slotFocusOnEnter(false);
    d->edit = new QLineEdit(parent);
    d->edit->installEventFilter(const_cast<KisNodeDelegate*>(this)); //hack?
    return d->edit;
}

void KisNodeDelegate::setEditorData(QWidget *widget, const QModelIndex &index) const
{
    QLineEdit *edit = qobject_cast<QLineEdit*>(widget);
    Q_ASSERT(edit);

    edit->setText(index.data(Qt::DisplayRole).toString());
}

void KisNodeDelegate::setModelData(QWidget *widget, QAbstractItemModel *model, const QModelIndex &index) const
{
    QLineEdit *edit = qobject_cast<QLineEdit*>(widget);
    Q_ASSERT(edit);

    model->setData(index, edit->text(), Qt::DisplayRole);
}

void KisNodeDelegate::updateEditorGeometry(QWidget *widget, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    widget->setGeometry(option.rect);
}


// PROTECTED


bool KisNodeDelegate::eventFilter(QObject *object, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        if (d->edit) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (!QRect(d->edit->mapToGlobal(QPoint()), d->edit->size()).contains(me->globalPos())) {
                emit commitData(d->edit);
                emit closeEditor(d->edit);
            }
        }
    } break;
    case QEvent::KeyPress: {
        QLineEdit *edit = qobject_cast<QLineEdit*>(object);
        if (edit && edit == d->edit) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            switch (ke->key()) {
            case Qt::Key_Escape:
                emit closeEditor(edit);
                return true;
            case Qt::Key_Tab:
                emit commitData(edit);
                emit closeEditor(edit,EditNextItem);
                return true;
            case Qt::Key_Backtab:
                emit commitData(edit);
                emit closeEditor(edit, EditPreviousItem);
                return true;
            case Qt::Key_Return:
            case Qt::Key_Enter:
                emit commitData(edit);
                emit closeEditor(edit);
                return true;
            default: break;
            }
        }
    } break;
    case QEvent::FocusOut : {
        QLineEdit *edit = qobject_cast<QLineEdit*>(object);
        if (edit && edit == d->edit) {
            emit commitData(edit);
            emit closeEditor(edit);
        }
    }
    default: break;
    }

    return QAbstractItemDelegate::eventFilter(object, event);
}


// PRIVATE


QStyleOptionViewItem KisNodeDelegate::getOptions(const QStyleOptionViewItem &o, const QModelIndex &index)
{
    QStyleOptionViewItem option = o;
    QVariant v = index.data(Qt::FontRole);
    if (v.isValid()) {
        option.font = v.value<QFont>();
        option.fontMetrics = QFontMetrics(option.font);
    }
    v = index.data(Qt::TextAlignmentRole);
    if (v.isValid())
        option.displayAlignment = QFlag(v.toInt());
    v = index.data(Qt::TextColorRole);
    if (v.isValid())
        option.palette.setColor(QPalette::Text, v.value<QColor>());
    v = index.data(Qt::BackgroundColorRole);
    if (v.isValid())
        option.palette.setColor(QPalette::Window, v.value<QColor>());

   return option;
}

QRect KisNodeDelegate::progressBarRect(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return iconsRect(option, index);
}

void KisNodeDelegate::drawProgressBar(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant value = index.data(KisNodeModel::ProgressRole);
    if (!value.isNull() && (value.toInt() >= 0 && value.toInt() <= 100)) {
        const QRect r = progressBarRect(option, index);
        p->save();
        {
            p->setClipRect(r);
            QStyle* style = QApplication::style();
            QStyleOptionProgressBar opt;

            opt.minimum = 0;
            opt.maximum = 100;
            opt.progress = value.toInt();
            opt.textVisible = true;
            opt.textAlignment = Qt::AlignHCenter;
            opt.text = i18n("%1 %", opt.progress);
            opt.rect = r;
            opt.orientation = Qt::Horizontal;
            opt.state = option.state;
            style->drawControl(QStyle::CE_ProgressBar, &opt, p, 0);
        }
        p->restore();
    }
}
