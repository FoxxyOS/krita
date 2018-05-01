/*
 *  Copyright (c) 2012 Boudewijn Rempt <boud@valdyas.org>
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
#ifndef KIS_SMOOTHING_OPTIONS_H
#define KIS_SMOOTHING_OPTIONS_H

#include <qglobal.h>
#include <QObject>
#include <QSharedPointer>
#include <QScopedPointer>
#include <kritaui_export.h>


class KRITAUI_EXPORT KisSmoothingOptions : public QObject
{
    Q_OBJECT
public:
    enum SmoothingType {
        NO_SMOOTHING = 0,
        SIMPLE_SMOOTHING,
        WEIGHTED_SMOOTHING,
        STABILIZER
    };

public:

    KisSmoothingOptions(bool useSavedSmoothing = true);
    ~KisSmoothingOptions();

    SmoothingType smoothingType() const;
    void setSmoothingType(SmoothingType value);

    qreal smoothnessDistance() const;
    void setSmoothnessDistance(qreal value);

    qreal tailAggressiveness() const;
    void setTailAggressiveness(qreal value);

    bool smoothPressure() const;
    void setSmoothPressure(bool value);

    bool useScalableDistance() const;
    void setUseScalableDistance(bool value);

    qreal delayDistance() const;
    void setDelayDistance(qreal value);

    void setUseDelayDistance(bool value);
    bool useDelayDistance() const;

    void setFinishStabilizedCurve(bool value);
    bool finishStabilizedCurve() const;

    void setStabilizeSensors(bool value);
    bool stabilizeSensors() const;

private Q_SLOTS:
    void slotWriteConfig();

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

typedef QSharedPointer<KisSmoothingOptions> KisSmoothingOptionsSP;

#endif // KIS_SMOOTHING_OPTIONS_H
