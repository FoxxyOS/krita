/*
 *  Copyright (c) 2004 Boudewijn Rempt (boud@valdyas.org)
 *  Copyright (c) 2004-2006 Cyrille Berger <cberger@cberger.net>
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
#ifndef _KIS_CONFIG_WIDGET_H_
#define _KIS_CONFIG_WIDGET_H_

#include <QWidget>
#include <kritaimage_export.h>

#include "kis_signal_compressor.h"
#include <kis_properties_configuration.h>

class KisViewManager;

/**
 * Empty base class. Configurable resources like filters, paintops etc.
 * can build their own configuration widgets that inherit this class.
 * The configuration widget should emit sigConfigurationItemChanged
 * when it wants a preview updated; there is a timer that
 * waits a little time to see if there are more changes coming
 * and then emits sigConfigurationUpdated.
 */
class KRITAIMAGE_EXPORT KisConfigWidget : public QWidget
{

    Q_OBJECT

protected:

    KisConfigWidget(QWidget * parent = 0, Qt::WFlags f = 0, int delay = 200);

public:
    virtual ~KisConfigWidget();

    /**
     * @param config the configuration for this configuration widget.
     */
    virtual void setConfiguration(const KisPropertiesConfigurationSP  config) = 0;

    /**
     * @return the configuration
     */
    virtual KisPropertiesConfigurationSP configuration() const = 0;

    /**
     * Sets the view object that can be used by the configuration
     * widget for richer functionality
     */
    virtual void setView(KisViewManager *view);

Q_SIGNALS:

    /**
     * emitted whenever it makes sense to update the preview
     */
    void sigConfigurationUpdated();

    /**
     * Subclasses should emit this signal whenever the preview should be
     * be recalculated. This kicks of a timer, so it's perfectly fine
     * to connect this to the changed signals of the widgets in your configuration
     * widget.
     */
    void sigConfigurationItemChanged();
    void sigSaveLockedConfig(KisPropertiesConfigurationSP p);
    void sigDropLockedConfig(KisPropertiesConfigurationSP p);

private Q_SLOTS:

    void slotConfigChanged();

private:
    KisSignalCompressor m_compressor;
};


#endif
