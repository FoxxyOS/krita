/* This file is part of the KDE project
 * Copyright (c) 2010 Justin Noel <justin@ics.com>
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
#ifndef KISSLIDERSPINBOX_H
#define KISSLIDERSPINBOX_H

#include <QAbstractSpinBox>

#include <QStyleOptionSpinBox>
#include <QStyleOptionProgressBar>
#include <kritaui_export.h>

class KisAbstractSliderSpinBoxPrivate;
class KisSliderSpinBoxPrivate;
class KisDoubleSliderSpinBoxPrivate;

/**
 * XXX: when inactive, also show the progress bar part as inactive!
 */
class KRITAUI_EXPORT KisAbstractSliderSpinBox : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(KisAbstractSliderSpinBox)
    Q_DECLARE_PRIVATE(KisAbstractSliderSpinBox)
protected:
    explicit KisAbstractSliderSpinBox(QWidget* parent, KisAbstractSliderSpinBoxPrivate*);
public:
    virtual ~KisAbstractSliderSpinBox();

    void showEdit();
    void hideEdit();

    void setPrefix(const QString& prefix);
    void setSuffix(const QString& suffix);

    void setExponentRatio(qreal dbl);

    /**
     * If set to block, it informs inheriting classes that they shouldn't emit signals
     * if the update comes from a mouse dragging the slider.
     * Set this to true when dragging the slider and updates during the drag are not needed.
     */
    void setBlockUpdateSignalOnDrag(bool block);

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;
    virtual QSize minimumSize() const;

    bool isDragging() const;

protected:
    virtual void paintEvent(QPaintEvent* e);
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void keyPressEvent(QKeyEvent* e);
    virtual void wheelEvent(QWheelEvent *);

    virtual bool eventFilter(QObject* recv, QEvent* e);

    QStyleOptionSpinBox spinBoxOptions() const;
    QStyleOptionProgressBar progressBarOptions() const;

    QRect progressRect(const QStyleOptionSpinBox& spinBoxOptions) const;
    QRect upButtonRect(const QStyleOptionSpinBox& spinBoxOptions) const;
    QRect downButtonRect(const QStyleOptionSpinBox& spinBoxOptions) const;

    int valueForX(int x, Qt::KeyboardModifiers modifiers = Qt::NoModifier) const;

    void commitEnteredValue();

    virtual QString valueString() const = 0;
    /**
     * Sets the slider internal value. Inheriting classes should respect blockUpdateSignal
     * so that, in specific cases, we have a performance improvement. See setIgnoreMouseMoveEvents.
     */
    virtual void setInternalValue(int value, bool blockUpdateSignal) = 0;

protected Q_SLOTS:
    void contextMenuEvent(QContextMenuEvent * event);
    void editLostFocus();
protected:
    KisAbstractSliderSpinBoxPrivate* const d_ptr;

    // QWidget interface
protected:
    virtual void changeEvent(QEvent *e);
    void paint(QPainter& painter);
    void paintFusion(QPainter& painter);
    void paintPlastique(QPainter& painter);
    void paintBreeze(QPainter& painter);

private:
    void setInternalValue(int value);
};

class KRITAUI_EXPORT KisSliderSpinBox : public KisAbstractSliderSpinBox
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(KisSliderSpinBox)
    Q_PROPERTY( int minimum READ minimum WRITE setMinimum )
    Q_PROPERTY( int maximum READ maximum WRITE setMaximum )
public:
    KisSliderSpinBox(QWidget* parent = 0);
    ~KisSliderSpinBox();

    void setRange(int minimum, int maximum);

    int minimum() const;
    void setMinimum(int minimum);
    int maximum() const;
    void setMaximum(int maximum);
    int fastSliderStep() const;
    void setFastSliderStep(int step);

    ///Get the value, don't use value()
    int value();

    void setSingleStep(int value);
    void setPageStep(int value);

public Q_SLOTS:

    ///Set the value, don't use setValue()
    void setValue(int value);

protected:
    virtual QString valueString() const;
    virtual void setInternalValue(int value, bool blockUpdateSignal);
Q_SIGNALS:
    void valueChanged(int value);
};

class KRITAUI_EXPORT KisDoubleSliderSpinBox : public KisAbstractSliderSpinBox
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(KisDoubleSliderSpinBox)
public:
    KisDoubleSliderSpinBox(QWidget* parent = 0);
    ~KisDoubleSliderSpinBox();

    void setRange(qreal minimum, qreal maximum, int decimals = 0);

    qreal minimum() const;
    void setMinimum(qreal minimum);
    qreal maximum() const;
    void setMaximum(qreal maximum);
    qreal fastSliderStep() const;
    void setFastSliderStep(qreal step);

    qreal value();
    void setSingleStep(qreal value);

public Q_SLOTS:
    void setValue(qreal value);

protected:
    virtual QString valueString() const;
    virtual void setInternalValue(int value, bool blockUpdateSignal);
Q_SIGNALS:
    void valueChanged(qreal value);
};

#endif //kISSLIDERSPINBOX_H
