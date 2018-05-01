/*
 *  tool_transform_args.h - part of Krita
 *
 *  Copyright (c) 2010 Marc Pegon <pe.marc@free.fr>
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

#ifndef TOOL_TRANSFORM_ARGS_H_
#define TOOL_TRANSFORM_ARGS_H_

#include <QPointF>
#include <QVector3D>
#include <kis_warptransform_worker.h>
#include <kis_filter_strategy.h>
#include "kis_liquify_properties.h"
#include "kritatooltransform_export.h"
#include "kis_global.h"


#include <QScopedPointer>
class KisLiquifyTransformWorker;
class QDomElement;

/**
 * Class used to store the parameters of a transformation.
 * Some parameters are specific to free transform mode, and
 * others to warp mode : maybe add a union to save a little more
 * memory.
 */

class KRITATOOLTRANSFORM_EXPORT ToolTransformArgs
{
public:
    enum TransformMode {FREE_TRANSFORM = 0,
                        WARP,
                        CAGE,
                        LIQUIFY,
                        PERSPECTIVE_4POINT,
                        N_MODES};

    /**
     * Initializes the parameters for an identity transformation,
     * with mode set to free transform.
     */
    ToolTransformArgs();

    /**
     * The object return will be a copy of args.
     */
    ToolTransformArgs(const ToolTransformArgs& args);

    /**
     * If mode is warp, original and transformed vector points will be of size 0.
     * Use setPoints method to set those vectors.
     */
    ToolTransformArgs(TransformMode mode,
                      QPointF transformedCenter,
                      QPointF originalCenter,
                      QPointF rotationCenterOffset, bool transformAroundRotationCenter,
                      double aX, double aY, double aZ,
                      double scaleX, double scaleY,
                      double shearX, double shearY,
                      KisWarpTransformWorker::WarpType warpType,
                      double alpha,
                      bool defaultPoints,
                      const QString &filterId);
    ~ToolTransformArgs();
    ToolTransformArgs& operator=(const ToolTransformArgs& args);

    bool operator==(const ToolTransformArgs& other) const;
    bool isSameMode(const ToolTransformArgs& other) const;

    inline TransformMode mode() const {
        return m_mode;
    }
    inline void setMode(TransformMode mode) {
        m_mode = mode;
    }

    //warp-related
    inline int numPoints() const {
        KIS_ASSERT_RECOVER_NOOP(m_origPoints.size() == m_transfPoints.size());
        return m_origPoints.size();
    }
    inline QPointF &origPoint(int i) {
        return m_origPoints[i];
    }
    inline QPointF &transfPoint(int i) {
        return m_transfPoints[i];
    }
    inline const QVector<QPointF> &origPoints() const {
        return m_origPoints;
    }
    inline const QVector<QPointF> &transfPoints() const {
        return m_transfPoints;
    }

    inline QVector<QPointF> &refOriginalPoints() {
        return m_origPoints;
    }
    inline QVector<QPointF> &refTransformedPoints() {
        return m_transfPoints;
    }

    inline KisWarpTransformWorker::WarpType warpType() const {
        return m_warpType;
    }
    inline double alpha() const {
        return m_alpha;
    }
    inline bool defaultPoints() const {
        return m_defaultPoints;
    }
    inline void setPoints(QVector<QPointF> origPoints, QVector<QPointF> transfPoints) {
        m_origPoints = QVector<QPointF>(origPoints);
        m_transfPoints = QVector<QPointF>(transfPoints);
    }
    inline void setWarpType(KisWarpTransformWorker::WarpType warpType) {
        m_warpType = warpType;
    }
    inline void setAlpha(double alpha) {
        m_alpha = alpha;
    }
    inline void setDefaultPoints(bool defaultPoints) {
        m_defaultPoints = defaultPoints;
    }

    //"free transform"-related
    inline QPointF transformedCenter() const {
        return m_transformedCenter;
    }
    inline QPointF originalCenter() const {
        return m_originalCenter;
    }
    inline QPointF rotationCenterOffset() const {
        return m_rotationCenterOffset;
    }
    inline bool transformAroundRotationCenter() const {
        return m_transformAroundRotationCenter;
    }
    inline double aX() const {
        return m_aX;
    }
    inline double aY() const {
        return m_aY;
    }
    inline double aZ() const {
        return m_aZ;
    }
    inline QVector3D cameraPos() const {
        return m_cameraPos;
    }
    inline double scaleX() const {
        return m_scaleX;
    }
    inline double scaleY() const {
        return m_scaleY;
    }
    inline bool keepAspectRatio() const {
        return m_keepAspectRatio;
    }
    inline double shearX() const {
        return m_shearX;
    }
    inline double shearY() const {
        return m_shearY;
    }

    inline void setTransformedCenter(QPointF transformedCenter) {
        m_transformedCenter = transformedCenter;
    }
    inline void setOriginalCenter(QPointF originalCenter) {
        m_originalCenter = originalCenter;
    }
    inline void setRotationCenterOffset(QPointF rotationCenterOffset) {
        m_rotationCenterOffset = rotationCenterOffset;
    }
    void setTransformAroundRotationCenter(bool value);
    inline void setAX(double aX) {
        KIS_ASSERT_RECOVER_NOOP(aX == normalizeAngle(aX));
        m_aX = aX;
    }
    inline void setAY(double aY) {
        KIS_ASSERT_RECOVER_NOOP(aY == normalizeAngle(aY));
        m_aY = aY;
    }
    inline void setAZ(double aZ) {
        KIS_ASSERT_RECOVER_NOOP(aZ == normalizeAngle(aZ));
        m_aZ = aZ;
    }
    inline void setCameraPos(const QVector3D &pos) {
        m_cameraPos = pos;
    }
    inline void setScaleX(double scaleX) {
        m_scaleX = scaleX;
    }
    inline void setScaleY(double scaleY) {
        m_scaleY = scaleY;
    }
    inline void setKeepAspectRatio(bool value) {
        m_keepAspectRatio = value;
    }
    inline void setShearX(double shearX) {
        m_shearX = shearX;
    }
    inline void setShearY(double shearY) {
        m_shearY = shearY;
    }

    inline QString filterId() const {
        return m_filter->id();
    }

    void setFilterId(const QString &id);

    inline KisFilterStrategy* filter() const {
        return m_filter;
    }

    bool isIdentity() const;

    inline QTransform flattenedPerspectiveTransform() const {
        return m_flattenedPerspectiveTransform;
    }

    inline void setFlattenedPerspectiveTransform(const QTransform &value) {
        m_flattenedPerspectiveTransform = value;
    }

    bool isEditingTransformPoints() const {
        return m_editTransformPoints;
    }

    void setEditingTransformPoints(bool value) {
        m_editTransformPoints = value;
    }

    const KisLiquifyProperties* liquifyProperties() const {
        return m_liquifyProperties.data();
    }

    KisLiquifyProperties* liquifyProperties() {
        return m_liquifyProperties.data();
    }

    void initLiquifyTransformMode(const QRect &srcRect);
    void saveLiquifyTransformMode() const;

    KisLiquifyTransformWorker* liquifyWorker() const {
        return m_liquifyWorker.data();
    }

    void toXML(QDomElement *e) const;
    static ToolTransformArgs fromXML(const QDomElement &e);

    void translate(const QPointF &offset);

    void saveContinuedState();
    void restoreContinuedState();
    const ToolTransformArgs* continuedTransform() const;

private:
    void clear();
    void init(const ToolTransformArgs& args);
    TransformMode m_mode;

    // warp-related arguments
    // these are basically the arguments taken by the warp transform worker
    bool m_defaultPoints; // true : the original points are set to make a grid
                          // which density is given by numPoints()
    QVector<QPointF> m_origPoints;
    QVector<QPointF> m_transfPoints;
    KisWarpTransformWorker::WarpType m_warpType;
    double m_alpha;

    //'free transform'-related
    // basically the arguments taken by the transform worker
    QPointF m_transformedCenter;
    QPointF m_originalCenter;
    QPointF m_rotationCenterOffset; // the position of the rotation center relative to
                                    // the original top left corner of the selection
                                    // before any transformation
    bool m_transformAroundRotationCenter; // In freehand mode makes the scaling and other transformations
                                          // be anchored to the rotation center point.

    double m_aX;
    double m_aY;
    double m_aZ;
    QVector3D m_cameraPos;
    double m_scaleX;
    double m_scaleY;
    double m_shearX;
    double m_shearY;
    bool m_keepAspectRatio;

    // perspective trasform related
    QTransform m_flattenedPerspectiveTransform;

    KisFilterStrategy *m_filter;
    bool m_editTransformPoints;
    QSharedPointer<KisLiquifyProperties> m_liquifyProperties;
    QScopedPointer<KisLiquifyTransformWorker> m_liquifyWorker;

    /**
     * When we continue a transformation, m_continuedTransformation
     * stores the initial step of our transform. All cancel and revert
     * operations should revert to it.
     */
    QScopedPointer<ToolTransformArgs> m_continuedTransformation;
};

#endif // TOOL_TRANSFORM_ARGS_H_
