/*
 *  Copyright (c) 2013 Sven Langkamp <sven.langkamp@gmail.com>
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


#ifndef PALETTEDOCKER_DOCK_H
#define PALETTEDOCKER_DOCK_H

#include <QDockWidget>
#include <QModelIndex>
#include <KoCanvasObserverBase.h>
#include <KoResourceServerAdapter.h>
#include <KoResourceServerObserver.h>
#include <resources/KoColorSet.h>

#include <kis_mainwindow_observer.h>

class KisViewManager;
class KisCanvasResourceProvider;
class KisCanvas2;
class KisWorkspaceResource;
class KisColorsetChooser;
class KisPaletteModel;
class Ui_WdgPaletteDock;


class PaletteDockerDock : public QDockWidget, public KisMainwindowObserver, public KoResourceServerObserver<KoColorSet> {
    Q_OBJECT
public:
    PaletteDockerDock();
    virtual ~PaletteDockerDock();
    QString observerName() { return "PaletteDockerDock"; }
    virtual void setMainWindow(KisViewManager* kisview);
    virtual void setCanvas(KoCanvasBase *canvas);
    virtual void unsetCanvas();

public: // KoResourceServerObserver

    virtual void unsetResourceServer();
    virtual void resourceAdded(KoColorSet *) {}
    virtual void removingResource(KoColorSet *resource);
    virtual void resourceChanged(KoColorSet *resource);
    virtual void syncTaggedResourceView() {}
    virtual void syncTagAddition(const QString&) {}
    virtual void syncTagRemoval(const QString&) {}

private Q_SLOTS:
    void addColorForeground();
    void addColor();
    void removeColor();
    void entrySelected(QModelIndex index);
    void setColorSet(KoColorSet* colorSet);

    void saveToWorkspace(KisWorkspaceResource* workspace);
    void loadFromWorkspace(KisWorkspaceResource* workspace);

private:    
    Ui_WdgPaletteDock* m_wdgPaletteDock;
    KisPaletteModel *m_model;
    QSharedPointer<KoAbstractResourceServerAdapter> m_serverAdapter;
    KoColorSet *m_currentColorSet;
    KisColorsetChooser *m_colorSetChooser;
    KisCanvasResourceProvider *m_resourceProvider;
    KisCanvas2 *m_canvas;
};


#endif
