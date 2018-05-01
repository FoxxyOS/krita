/*
 * This file is part of the KDE project
 *
 * Copyright (c) 2015 Boudewijn Rempt <boud@valdyas.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef KISANIMATEDBRUSHANNOTATION_H
#define KISANIMATEDBRUSHANNOTATION_H

#include <kis_annotation.h>

class KisPipeBrushParasite;

class KisAnimatedBrushAnnotation : public KisAnnotation
{
public:
    KisAnimatedBrushAnnotation(const KisPipeBrushParasite &parasite);
    virtual KisAnnotation* clone() const Q_DECL_OVERRIDE {
        return new KisAnimatedBrushAnnotation(*this);
    }
};

#endif // KISANIMATEDBRUSHANNOTATION_H
