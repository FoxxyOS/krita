/*
 *  Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
 *                2016 Sven Langkamp <sven.langkamp@gmail.com>
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

#ifndef _KIS_STOPGRADIENT_EDITOR_H_
#define _KIS_STOPGRADIENT_EDITOR_H_ 

#include "ui_wdgstopgradienteditor.h"

class KoStopGradient;

class KisStopGradientEditor : public QWidget, public Ui::KisWdgStopGradientEditor
{
    Q_OBJECT

public:
    KisStopGradientEditor(KoStopGradient* gradient, QWidget *parent, const char* name, const QString& caption);
    void activate();
private:
     KoStopGradient* m_gradient;
private Q_SLOTS:
    void stopChanged(int stop);
    void colorChanged(const KoColor& color);
    void opacityChanged(qreal value);
    void nameChanged();
    void reverse();
    
    void paramChanged();
};

#endif
