/*
 *  Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2014 Sven Langkamp <sven.langkamp@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#ifndef OVERVIEWWIDGET_H
#define OVERVIEWWIDGET_H
#include <QObject>
#include <QWidget>
#include <QPixmap>
#include <QMutex>
#include "kis_idle_watcher.h"
#include "kis_simple_stroke_strategy.h"

class KisCanvas2;
class KisSignalCompressor;
class KoCanvasBase;

class OverviewThumbnailStrokeStrategy : public QObject, public KisSimpleStrokeStrategy
{
    Q_OBJECT
public:
    OverviewThumbnailStrokeStrategy(KisImageWSP image);
    ~OverviewThumbnailStrokeStrategy();

    static QList<KisStrokeJobData*> createJobsData(KisPaintDeviceSP dev, const QRect& imageRect, KisPaintDeviceSP thumbDev, const QSize &thumbnailSize);

private:
    void initStrokeCallback();
    void doStrokeCallback(KisStrokeJobData *data);
    void finishStrokeCallback();
    void cancelStrokeCallback();

Q_SIGNALS:
    //Emitted when thumbnail is updated and overviewImage is fully generated.
    void thumbnailUpdated(QImage pixmap);


private:
    struct Private;
    const QScopedPointer<Private> m_d;
    QMutex m_thumbnailMergeMutex;
    KisImageSP m_image;
};

class OverviewWidget : public QWidget
{
    Q_OBJECT

public:
    OverviewWidget(QWidget * parent = 0);

    virtual ~OverviewWidget();

    virtual void setCanvas(KoCanvasBase *canvas);
    virtual void unsetCanvas()
    {
        m_canvas = 0;
    }

public Q_SLOTS:
    void startUpdateCanvasProjection();
    void generateThumbnail();
    void updateThumbnail(QImage pixmap);

protected:
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);
    void paintEvent(QPaintEvent *event);

    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

private:
    QSize calculatePreviewSize();
    QPointF previewOrigin();
    QTransform imageToPreviewTransform();
    QPolygonF previewPolygon();

    QPixmap m_pixmap;
    KisCanvas2 *m_canvas;

    bool m_dragging;
    QPointF m_lastPos;

    QColor m_outlineColor;
    KisIdleWatcher m_imageIdleWatcher;
    KisStrokeId strokeId;
    QMutex mutex;
};



#endif /* OVERVIEWWIDGET_H */
