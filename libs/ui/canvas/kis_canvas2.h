/* This file is part of the KDE project
 * Copyright (C) 2006, 2010 Boudewijn Rempt <boud@valdyas.org>
 * Copyright (C) 2011       Silvio Heinrich <plassy@web.de>
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

#ifndef KIS_CANVAS_H
#define KIS_CANVAS_H

#include <QObject>
#include <QWidget>
#include <QSize>
#include <QString>

#include <KoConfig.h>
#include <KoColorConversionTransformation.h>
#include <KoCanvasBase.h>
#include <kritaui_export.h>
#include <kis_types.h>
#include <KoPointerEvent.h>

#include "opengl/kis_opengl.h"

#include "kis_ui_types.h"
#include "kis_coordinates_converter.h"
#include "kis_canvas_decoration.h"
#include "kis_painting_assistants_decoration.h"

class KoToolProxy;
class KoColorProfile;


class KisViewManager;
class KisFavoriteResourceManager;
class KisDisplayFilter;
class KisDisplayColorConverter;
struct KisExposureGammaCorrectionInterface;
class KisView;
class KisInputManager;
class KisAnimationPlayer;
class KisShapeController;
class KisCoordinatesConverter;
class KoViewConverter;

/**
 * KisCanvas2 is not an actual widget class, but rather an adapter for
 * the widget it contains, which may be either a QPainter based
 * canvas, or an OpenGL based canvas: that are the real widgets.
 */
class KRITAUI_EXPORT KisCanvas2 : public QObject, public KoCanvasBase
{

    Q_OBJECT

public:

    /**
     * Create a new canvas. The canvas manages a widget that will do
     * the actual painting: the canvas itself is not a widget.
     *
     * @param viewConverter the viewconverter for converting between
     *                       window and document coordinates.
     */
    KisCanvas2(KisCoordinatesConverter *coordConverter, KoCanvasResourceManager *resourceManager, KisView *view, KoShapeBasedDocumentBase *sc);

    virtual ~KisCanvas2();

    void notifyZoomChanged();

    virtual void disconnectCanvasObserver(QObject *object);

public: // KoCanvasBase implementation

    bool canvasIsOpenGL() const;

    KisOpenGL::FilterMode openGLFilterMode() const;

    void gridSize(QPointF *offset, QSizeF *spacing) const;

    bool snapToGrid() const;

    // XXX: Why?
    void addCommand(KUndo2Command *command);

    virtual QPoint documentOrigin() const;
    QPoint documentOffset() const;

    /**
     * Return the right shape manager for the current layer. That is
     * to say, if the current layer is a vector layer, return the shape
     * layer's canvas' shapemanager, else the shapemanager associated
     * with the global krita canvas.
     */
    KoShapeManager * shapeManager() const;

    /**
     * Return the shape manager associated with this canvas
     */
    KoShapeManager * globalShapeManager() const;

    void updateCanvas(const QRectF& rc);

    virtual void updateInputMethodInfo();

    const KisCoordinatesConverter* coordinatesConverter() const;
    virtual KoViewConverter *viewConverter() const;

    virtual QWidget* canvasWidget();

    virtual const QWidget* canvasWidget() const;

    virtual KoUnit unit() const;

    virtual KoToolProxy* toolProxy() const;

    const KoColorProfile* monitorProfile();

    /**
     * Prescale the canvas represention of the image (if necessary, it
     * is for QPainter, not for OpenGL).
     */
    void preScale();

    // FIXME:
    // Temporary! Either get the current layer and image from the
    // resource provider, or use this, which gets them from the
    // current shape selection.
    KisImageWSP currentImage() const;

    /**
     * Filters events and sends them to canvas actions. Shared
     * among all the views/canvases
     *
     * NOTE: May be null while initialization!
     */
    KisInputManager* globalInputManager() const;

    KisPaintingAssistantsDecorationSP paintingAssistantsDecoration() const;


public: // KisCanvas2 methods

    KisImageWSP image() const;
    KisViewManager* viewManager() const;
    QPointer<KisView> imageView() const;

    /// @return true if the canvas image should be displayed in vertically mirrored mode
    void addDecoration(KisCanvasDecorationSP deco);
    KisCanvasDecorationSP decoration(const QString& id) const;

    void setDisplayFilter(QSharedPointer<KisDisplayFilter> displayFilter);
    QSharedPointer<KisDisplayFilter> displayFilter() const;

    KisDisplayColorConverter *displayColorConverter() const;
    KisExposureGammaCorrectionInterface* exposureGammaCorrectionInterface() const;
    /**
     * @brief setProofingOptions
     * set the options for softproofing, without affecting the proofing options as stored inside the image.
     */
    void setProofingOptions(bool softProof, bool gamutCheck);
    KisProofingConfigurationSP proofingConfiguration() const;
    /**
     * @brief setProofingConfigUpdated This function is to set whether the proofing config is updated,
     * this is needed for determining whether or not to generate a new proofing transform.
     * @param updated whether it's updated. Just set it to false in normal usage.
     */
    void setProofingConfigUpdated(bool updated);
    /**
     * @brief proofingConfigUpdated ask the canvas whether or not it updated the proofing config.
     * @return whether or not the proofing config is updated, if so, a new proofing transform needs to be made
     * in KisOpenGL canvas.
     */bool proofingConfigUpdated();

    void setCursor(const QCursor &cursor);
    KisAnimationFrameCacheSP frameCache() const;
    KisAnimationPlayer *animationPlayer() const;
    void refetchDataFromImage();

Q_SIGNALS:
    void imageChanged(KisImageWSP image);

    void sigCanvasCacheUpdated();
    void sigContinueResizeImage(qint32 w, qint32 h);

    void documentOffsetUpdateFinished();

    // emitted whenever the canvas widget thinks sketch should update
    void updateCanvasRequested(const QRect &rc);

public Q_SLOTS:

    /// Update the entire canvas area
    void updateCanvas();

    void startResizingImage();
    void finishResizingImage(qint32 w, qint32 h);

    /// canvas rotation in degrees
    qreal rotationAngle() const;
    /// Bools indicating canvasmirroring.
    bool xAxisMirrored() const;
    bool yAxisMirrored() const;
    void slotSoftProofing(bool softProofing);
    void slotGamutCheck(bool gamutCheck);
    void slotChangeProofingConfig();

    void channelSelectionChanged();

    /**
     * Called whenever the display monitor profile resource changes
     */
    void slotSetDisplayProfile(const KoColorProfile *profile);
    void startUpdateInPatches(const QRect &imageRect);

private Q_SLOTS:

    /// The image projection has changed, now start an update
    /// of the canvas representation.
    void startUpdateCanvasProjection(const QRect & rc);
    void updateCanvasProjection();


    /**
     * Called whenever the view widget needs to show a different part of
     * the document
     *
     * @param documentOffset the offset in widget pixels
     */
    void documentOffsetMoved(const QPoint &documentOffset);

    /**
     * Called whenever the configuration settings change.
     */
    void slotConfigChanged();

    void slotSelectionChanged();

    void slotDoCanvasUpdate();

    void bootstrapFinished();

public:

    bool isPopupPaletteVisible() const;
    void slotShowPopupPalette(const QPoint& = QPoint(0,0));

    // interface for KisCanvasController only
    void setWrapAroundViewingMode(bool value);
    bool wrapAroundViewingMode() const;

    void setLodAllowedInCanvas(bool value);
    bool lodAllowedInCanvas() const;

    void initializeImage();

    void setFavoriteResourceManager(KisFavoriteResourceManager* favoriteResourceManager);

private:
    Q_DISABLE_COPY(KisCanvas2)

    void connectCurrentCanvas();
    void createCanvas(bool useOpenGL);
    void createQPainterCanvas();
    void createOpenGLCanvas();
    void updateCanvasWidgetImpl(const QRect &rc = QRect());
    void setCanvasWidget(QWidget *widget);
    void resetCanvas(bool useOpenGL);

    void notifyLevelOfDetailChange();

    // Completes construction of canvas.
    // To be called by KisView in its constructor, once it has been setup enough
    // (to be defined what that means) for things KisCanvas2 expects from KisView
    // TODO: see to avoid that
    void setup();

private:
    friend class KisView; // calls setup()
    class KisCanvas2Private;
    KisCanvas2Private * const m_d;
};

#endif
