/*
 *  Copyright (c) 2007 Boudewijn Rempt <boud@valdyas.org>
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

#ifndef KIS_PAINTOP_SETTINGS_H_
#define KIS_PAINTOP_SETTINGS_H_

#include "kis_types.h"
#include "kritaimage_export.h"

#include <QImage>
#include <QScopedPointer>

#include "kis_properties_configuration.h"
#include <brushengine/kis_paint_information.h>
#include <brushengine/kis_uniform_paintop_property.h>



class KisPaintOpConfigWidget;
class KisPaintopSettingsUpdateProxy;

/**
 * This class is used to cache the settings for a paintop
 * between two creations. There is one KisPaintOpSettings per input device (mouse, tablet,
 * etc...).
 *
 * The settings may be stored in a preset or a recorded brush stroke. Note that if your
 * paintop's settings subclass has data that is not stored as a property, that data is not
 * saved and restored.
 *
 * The object also contains a pointer to its parent KisPaintOpPreset object.This is to control the DirtyPreset
 * property of KisPaintOpPreset. Whenever the settings are changed/modified from the original -- the preset is
 * set to dirty.
 */
class KRITAIMAGE_EXPORT KisPaintOpSettings : public KisPropertiesConfiguration
{

public:

    KisPaintOpSettings();
    virtual ~KisPaintOpSettings();
    KisPaintOpSettings(const KisPaintOpSettings &rhs);

    /**
     *
     */
    virtual void setOptionsWidget(KisPaintOpConfigWidget* widget);

    /**
     * This function is called by a tool when the mouse is pressed. It's useful if
     * the paintop needs mouse interaction for instance in the case of the clone op.
     * If the tool is supposed to ignore the event, the paint op should return false
     * and if the tool is supposed to use the event, return true.
     */
    virtual bool mousePressEvent(const KisPaintInformation &paintInformation, Qt::KeyboardModifiers modifiers, KisNodeWSP currentNode);

    /**
     * Clone the current settings object. Override this if your settings instance doesn't
     * store everything as properties.
     */
    virtual KisPaintOpSettingsSP clone() const;

    /**
     * @return the node the paintop is working on.
     */
    KisNodeSP node() const;

    /**
     * Call this function when the paint op is selected or the tool is activated
     */
    virtual void activate();

    /**
     * XXX: Remove this after 2.0, when the paint operation (incremental/non incremental) will
     *      be completely handled in the paintop, not in the tool. This is a filthy hack to move
     *      the option to the right place, at least.
     * @return true if we paint incrementally, false if we paint like Photoshop. By default, paintops
     *      do not support non-incremental.
     */
    virtual bool paintIncremental() {
        return true;
    }

    /**
     * @return the composite op it to which the indirect painting device
     * should be initialized to. This is used by clone op to reset
     * the composite op to COMPOSITE_COPY
     */
    virtual QString indirectPaintingCompositeOp() const;

    /**
     * Whether this paintop wants to deposit paint even when not moving, i.e. the
     * tool needs to activate its timer.
     */
    virtual bool isAirbrushing() const {
        return false;
    }

    /**
    * If this paintop deposit the paint even when not moving, the tool needs to know the rate of it in miliseconds
    */
    virtual int rate() const {
        return 100;
    }

    /**
     * This enum defines the current mode for painting an outline.
     */
    enum OutlineMode {
        CursorIsOutline = 1, ///< When this mode is set, an outline is painted around the cursor
        CursorIsCircleOutline,
        CursorNoOutline,
        CursorTiltOutline,
        CursorColorOutline
    };

    /**
     * Returns the brush outline in pixel coordinates. Tool is responsible for conversion into view coordinates.
     * Outline mode has to be passed to the paintop which builds the outline as some paintops have to paint outline
     * always like clone paintop indicating the duplicate position
     */
    virtual QPainterPath brushOutline(const KisPaintInformation &info, OutlineMode mode);

    /**
    * Helpers for drawing the brush outline
    */
    static QPainterPath ellipseOutline(qreal width, qreal height, qreal scale, qreal rotation);

    /**
     * Helper for drawing a triangle representing the tilt of the stylus.
     *
     * @param start is the offset from the brush's outline's bounding box
     * @param lengthScale is used for deciding the size of the triangle.
     * Brush diameter or width are common choices for this.
     * @param angle is the angle between the two sides of the triangle.
     */
    static QPainterPath makeTiltIndicator(KisPaintInformation const& info,
        QPointF const& start, qreal lengthScale, qreal angle);

    /**
     * Set paintop opacity directly in the properties
     */
    void setPaintOpOpacity(qreal value);

    /**
     * Set paintop flow directly in the properties
     */
    void setPaintOpFlow(qreal value);

    /**
     * Set paintop composite mode directly in the properties
     */
    void setPaintOpCompositeOp(const QString &value);

    /**
     * @return opacity saved in the properties
     */
    qreal paintOpOpacity();

    /**
     * @return flow saved in the properties
     */
    qreal paintOpFlow();

    /**
     * @return composite mode saved in the properties
     */
    QString paintOpCompositeOp();

    /**
     * Set paintop size directly in the properties
     */
    virtual void setPaintOpSize(qreal value) = 0;

    /**
     * @return size saved in the properties
     */
    virtual qreal paintOpSize() const = 0;

    void setEraserMode(bool value);
    bool eraserMode();

    qreal savedEraserSize() const;
    void setSavedEraserSize(qreal value);
    qreal savedBrushSize() const;
    void setSavedBrushSize(qreal value);

    qreal savedEraserOpacity() const;
    void setSavedEraserOpacity(qreal value);
    qreal savedBrushOpacity() const;
    void setSavedBrushOpacity(qreal value);

    QString effectivePaintOpCompositeOp();

    void setPreset(KisPaintOpPresetWSP preset);

    KisPaintOpPresetWSP preset() const;

    /**
     * @return filename of the 3D brush model, empty if no brush is set
     */
    virtual QString modelName() const;

    /**
    * Set filename of 3D brush model. By default no brush is set
    */
    void setModelName(const QString & modelName);

    /// Check if the settings are valid, setting might be invalid through missing brushes etc
    /// Overwrite if the settings of a paintop can be invalid
    /// @return state of the settings, default implementation is true
    virtual bool isValid() const;

    /// Check if the settings are loadable, that might the case if we can fallback to something
    /// Overwrite if the settings can do some kind of fallback
    /// @return loadable state of the settings, by default implementation return the same as isValid()
    virtual bool isLoadable();

    /**
     * These methods are populating properties with runtime
     * information about canvas rotation/mirroring. This information
     * is set directly by KisToolFreehand. Later the data is accessed
     * by the pressure options to make a final decision.
     */
    void setCanvasRotation(qreal angle);
    void setCanvasMirroring(bool xAxisMirrored, bool yAxisMirrored);

    /**
     * Overrides the method in KisPropertiesCofiguration to allow
     * onPropertyChanged() callback
     */
    void setProperty(const QString & name, const QVariant & value);

    virtual QList<KisUniformPaintOpPropertySP> uniformProperties(KisPaintOpSettingsSP settings);

    static bool isLodUserAllowed(const KisPropertiesConfigurationSP config);
    static void setLodUserAllowed(KisPropertiesConfigurationSP config, bool value);

    /**
    * @return the option widget of the paintop (can be 0 is no option widgets is set)
    */
    KisPaintOpConfigWidget* optionsWidget() const;


    /**
     * This function is called to set random offsets to the brush whenever the mouse is clicked. It is
     * specific to when the pattern option is set.
     *
     */
    virtual void setRandomOffset(const KisPaintInformation &paintInformation);

protected:

    /**
     * The callback is called every time when a property changes
     */
    virtual void onPropertyChanged();

private:



    struct Private;
    const QScopedPointer<Private> d;
};

#endif
