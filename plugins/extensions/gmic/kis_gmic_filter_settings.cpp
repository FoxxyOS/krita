/*
 * Copyright (c) 2013 Lukáš Tvrdý <lukast.dev@gmail.com
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

#include <QString>
#include <kis_gmic_filter_settings.h>


KisGmicFilterSetting::KisGmicFilterSetting():m_gmicCommand(),m_previewGmicCommand(),m_inputLayerMode(ACTIVE_LAYER),m_outputMode(IN_PLACE),m_isBlacklisted(false)
{

}

KisGmicFilterSetting::~KisGmicFilterSetting()
{

}

const QString& KisGmicFilterSetting::gmicCommand() const
{
    return m_gmicCommand;
}

void KisGmicFilterSetting::setGmicCommand(QString cmd)
{
    m_gmicCommand = cmd;
}

const QString& KisGmicFilterSetting::previewGmicCommand() const
{
    return m_previewGmicCommand;
}

void KisGmicFilterSetting::setPreviewGmicCommand(QString cmd)
{
    m_previewGmicCommand = cmd;
}

InputLayerMode KisGmicFilterSetting::inputLayerMode() const
{
     return m_inputLayerMode;
}

void KisGmicFilterSetting::setInputLayerMode(InputLayerMode mode)
{
    m_inputLayerMode = mode;
}

OutputMode KisGmicFilterSetting::outputMode() const
{
     return m_outputMode;
}

void KisGmicFilterSetting::setOutputMode(OutputMode mode)
{
    m_outputMode = mode;
}

OutputPreviewMode KisGmicFilterSetting::previewMode() const
{
    return m_previewMode;
}

void KisGmicFilterSetting::setPreviewMode(OutputPreviewMode mode)
{
    m_previewMode = mode;
}

PreviewSize KisGmicFilterSetting::previewSize() const
{
    return m_previewSize;
}

void KisGmicFilterSetting::setPreviewSize(PreviewSize size)
{
    m_previewSize = size;
}
