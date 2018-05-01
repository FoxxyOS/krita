/*
 *  Copyright (c) 2016 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_base_option.h"
#include "kis_properties_configuration.h"

KisBaseOption::~KisBaseOption()
{
}


void KisBaseOption::readOptionSetting(KisPropertiesConfigurationSP settings)
{
    readOptionSettingImpl(settings.data());
}

void KisBaseOption::writeOptionSetting(KisPropertiesConfigurationSP settings) const
{
    writeOptionSettingImpl(settings.data());
}

void KisBaseOption::readOptionSetting(const KisPropertiesConfiguration *settings)
{
    readOptionSettingImpl(settings);
}

void KisBaseOption::writeOptionSetting(KisPropertiesConfiguration *settings) const
{
    writeOptionSettingImpl(settings);
}

