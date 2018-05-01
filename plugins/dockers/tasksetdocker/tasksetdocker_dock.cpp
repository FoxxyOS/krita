/*
 *  Copyright (c) 2011 Sven Langkamp <sven.langkamp@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "tasksetdocker_dock.h"

#include <QGridLayout>
#include <QListView>
#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QInputDialog>
#include <QThread>
#include <QAction>

#include <klocalizedstring.h>
#include <kactioncollection.h>

#include <kis_icon.h>

#include <KoCanvasBase.h>
#include <KoResourceItemChooser.h>
#include <KoResourceServerAdapter.h>
#include <KoResourceServerProvider.h>

#include <kis_resource_server_provider.h>
#include <KisViewManager.h>
#include <kis_canvas2.h>
#include <KisMainWindow.h>
#include "tasksetmodel.h"


class KisTasksetDelegate : public QStyledItemDelegate
{
public:
    KisTasksetDelegate(QObject * parent = 0) : QStyledItemDelegate(parent) {}
    ~KisTasksetDelegate() override {}
    /// reimplemented
    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const override {
        return QSize(QStyledItemDelegate::sizeHint(option, index).width(),
                     qMin(QStyledItemDelegate::sizeHint(option, index).width(), 25));
    }
};

class KisTasksetResourceDelegate : public QStyledItemDelegate
{
public:
    KisTasksetResourceDelegate(QObject * parent = 0) : QStyledItemDelegate(parent) {}
    ~KisTasksetResourceDelegate() override {}
    /// reimplemented
    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const override;
};

void KisTasksetResourceDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (! index.isValid())
        return;

    TasksetResource* taskset = static_cast<TasksetResource*>(index.internalPointer());

    if (option.state & QStyle::State_Selected) {
        painter->setPen(QPen(option.palette.highlight(), 2.0));
        painter->fillRect(option.rect, option.palette.highlight());
        painter->setBrush(option.palette.highlightedText());
    }
    else {
        painter->setBrush(option.palette.text());
    }

    painter->drawText(option.rect.x() + 5, option.rect.y() + painter->fontMetrics().ascent() + 5, taskset->name());

}

TasksetDockerDock::TasksetDockerDock( ) : QDockWidget(i18n("Task Sets")), m_canvas(0), m_blocked(false)
{
    QWidget* widget = new QWidget(this);
    setupUi(widget);
    m_model = new TasksetModel(this);
    tasksetView->setModel(m_model);
    tasksetView->setItemDelegate(new KisTasksetDelegate(this));
    recordButton->setIcon(KisIconUtils::loadIcon("media-record"));
    recordButton->setCheckable(true);
    clearButton->setIcon(KisIconUtils::loadIcon("edit-delete"));
    saveButton->setIcon(KisIconUtils::loadIcon("document-save"));
    saveButton->setEnabled(false);

    chooserButton->setIcon(KisIconUtils::loadIcon("edit-copy"));

    m_rserver = new KoResourceServerSimpleConstruction<TasksetResource>("kis_taskset", "*.kts");

    if (!QFileInfo(m_rserver->saveLocation()).exists()) {
        QDir().mkpath(m_rserver->saveLocation());
    }
    QSharedPointer<KoAbstractResourceServerAdapter> adapter (new KoResourceServerAdapter<TasksetResource>(m_rserver));
    m_taskThread = new KoResourceLoaderThread(m_rserver);
    m_taskThread->start();

    KoResourceItemChooser* itemChooser = new KoResourceItemChooser(adapter, this);
    itemChooser->setItemDelegate(new KisTasksetResourceDelegate(this));
    itemChooser->setFixedSize(500, 250);
    itemChooser->setRowHeight(30);
    itemChooser->setColumnCount(1);
    itemChooser->showTaggingBar(true);
    chooserButton->setPopupWidget(itemChooser);

    connect(itemChooser, SIGNAL(resourceSelected(KoResource*)),
            this, SLOT(resourceSelected(KoResource*)));

    setWidget(widget);

    connect( tasksetView, SIGNAL(clicked( const QModelIndex & ) ),
            this, SLOT(activated ( const QModelIndex & ) ) );

    connect( recordButton, SIGNAL(toggled(bool)), this, SLOT(recordClicked()));
    connect( clearButton, SIGNAL(clicked(bool)), this, SLOT(clearClicked()));
    connect( saveButton, SIGNAL(clicked(bool)), this, SLOT(saveClicked()));
}

TasksetDockerDock::~TasksetDockerDock()
{
    delete m_taskThread;
    delete m_rserver;
}

void TasksetDockerDock::setCanvas(KoCanvasBase * canvas)
{
    if (m_canvas && m_canvas->viewManager()) {
         m_canvas->viewManager()->actionCollection()->disconnect(this);
         Q_FOREACH (KXMLGUIClient* client, m_canvas->viewManager()->mainWindow()->childClients()) {
            client->actionCollection()->disconnect(this);
        }
    }
    m_canvas = dynamic_cast<KisCanvas2*>(canvas);
}

void TasksetDockerDock::unsetCanvas()
{
    m_canvas = 0;
    m_model->clear();
}

void TasksetDockerDock::actionTriggered(QAction* action)
{
    if(action && !action->objectName().isEmpty() &&
       !m_blocked && recordButton->isChecked()) {
        m_model->addAction(action);
        saveButton->setEnabled(true);
    }
}

void TasksetDockerDock::activated(const QModelIndex& index)
{
    QAction* action = m_model->actionFromIndex(index);
    m_blocked = true;
    action->trigger();
    m_blocked = false;
}

void TasksetDockerDock::recordClicked()
{
    if(m_canvas) {
        KisViewManager* view = m_canvas->viewManager();
        connect(view->actionCollection(), SIGNAL(actionTriggered(QAction*)),
                this, SLOT(actionTriggered(QAction*)), Qt::UniqueConnection);
        Q_FOREACH (KXMLGUIClient* client, view->mainWindow()->childClients()) {
            connect(client->actionCollection(), SIGNAL(actionTriggered(QAction*)),
                    this, SLOT(actionTriggered(QAction*)), Qt::UniqueConnection);
        }
    }
}

void TasksetDockerDock::saveClicked()
{
    bool ok;
    QString name = QInputDialog::getText(this, i18n("Taskset Name"),
                                         i18n("Name:"), QLineEdit::Normal,
                                         QString(), &ok);
    if (!ok) {
        return;
    }

    m_taskThread->barrier();

    TasksetResource* taskset = new TasksetResource(QString());

    QStringList actionNames;
    Q_FOREACH (QAction* action, m_model->actions()) {
        actionNames.append(action->objectName());
    }
    taskset->setActionList(actionNames);
    taskset->setValid(true);
    QString saveLocation = m_rserver->saveLocation();

    bool newName = false;
    if(name.isEmpty()) {
        newName = true;
        name = i18n("Taskset");
    }
    QFileInfo fileInfo(saveLocation + name + taskset->defaultFileExtension());

    int i = 1;
    while (fileInfo.exists()) {
        fileInfo.setFile(saveLocation + name + QString("%1").arg(i) + taskset->defaultFileExtension());
        i++;
    }
    taskset->setFilename(fileInfo.filePath());
    if(newName) {
        name = i18n("Taskset %1", i);
    }
    taskset->setName(name);
    m_rserver->addResource(taskset);
}

void TasksetDockerDock::clearClicked()
{
    saveButton->setEnabled(false);
    m_model->clear();
}

void TasksetDockerDock::resourceSelected(KoResource* resource)
{
    if(!m_canvas) {
        return;
    }
    m_model->clear();
    saveButton->setEnabled(true);
    Q_FOREACH (const QString& actionName, static_cast<TasksetResource*>(resource)->actionList()) {
        QAction* action = m_canvas->viewManager()->actionCollection()->action(actionName);
        if(action) {
            m_model->addAction(action);
        }
    }
}

