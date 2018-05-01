/*
 * This file is part of Krita
 *
 * Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
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

#include "blur.h"
#include <kpluginfactory.h>

#include "kis_blur_filter.h"
#include "kis_gaussian_blur_filter.h"
#include "kis_motion_blur_filter.h"
#include "kis_lens_blur_filter.h"
#include "filter/kis_filter_registry.h"

K_PLUGIN_FACTORY_WITH_JSON(BlurFilterPluginFactory, "kritablurfilter.json", registerPlugin<BlurFilterPlugin>();)

BlurFilterPlugin::BlurFilterPlugin(QObject *parent, const QVariantList &)
        : QObject(parent)
{
    KisFilterRegistry::instance()->add(new KisBlurFilter());
    KisFilterRegistry::instance()->add(new KisGaussianBlurFilter());
    KisFilterRegistry::instance()->add(new KisMotionBlurFilter());
    KisFilterRegistry::instance()->add(new KisLensBlurFilter());

}

BlurFilterPlugin::~BlurFilterPlugin()
{
}

#include "blur.moc"

