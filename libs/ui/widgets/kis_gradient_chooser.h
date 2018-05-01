/*
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
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
#ifndef KIS_GRADIENT_CHOOSER_H_
#define KIS_GRADIENT_CHOOSER_H_

#include <KoDialog.h>

#include <QFrame>
#include <kritaui_export.h>

class KoAbstractGradient;
class KoStopGradient;
class KisViewManager;
class QLabel;
class QPushButton;
class KisAutogradient;
class KoResource;
class KoResourceItemChooser;

class KisCustomGradientDialog : public KoDialog
{

    Q_OBJECT

public:

    KisCustomGradientDialog(KoAbstractGradient* gradient, QWidget * parent, const char *name);

private:

    QWidget * m_page;

};

class KRITAUI_EXPORT KisGradientChooser : public QFrame
{

    Q_OBJECT

public:
    KisGradientChooser(QWidget *parent = 0, const char *name = 0);
    virtual ~KisGradientChooser();

    /// Gets the currently selected resource
    /// @returns the selected resource, 0 is no resource is selected
    KoResource *currentResource();
    void setCurrentResource(KoResource *resource);

    void setCurrentItem(int row, int column);

Q_SIGNALS:
    /// Emitted when a resource was selected
    void resourceSelected(KoResource * resource);

private Q_SLOTS:
    virtual void update(KoResource * resource);
    void addStopGradient();
    void addSegmentedGradient();
    void editGradient();

private:
      void addGradient(KoAbstractGradient* gradient);
private:
    QLabel *m_lbName;
    KoResourceItemChooser * m_itemChooser;
    QPushButton* m_editGradient;
};

#endif // KIS_GRADIENT_CHOOSER_H_

