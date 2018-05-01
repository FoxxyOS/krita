/*
 *  Copyright (c) 2014 Victor Lafon metabolic.ewilan@hotmail.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include "dlg_bundle_manager.h"
#include "ui_wdgdlgbundlemanager.h"

#include "resourcemanager.h"
#include "dlg_create_bundle.h"

#include <QListWidget>
#include <QTreeWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QPixmap>
#include <QMessageBox>

#include <kis_icon.h>
#include "kis_action.h"
#include <kis_resource_server_provider.h>

#define ICON_SIZE 48

DlgBundleManager::DlgBundleManager(ResourceManager *resourceManager, KisActionManager* actionMgr, QWidget *parent)
    : KoDialog(parent)
    , m_page(new QWidget())
    , m_ui(new Ui::WdgDlgBundleManager)
    , m_currentBundle(0)
    , m_resourceManager(resourceManager)
{
    setCaption(i18n("Manage Resource Bundles"));
    m_ui->setupUi(m_page);
    setMainWidget(m_page);
    resize(m_page->sizeHint());
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);

    m_ui->listActive->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    m_ui->listActive->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_ui->listActive, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), SLOT(itemSelected(QListWidgetItem*,QListWidgetItem*)));
    connect(m_ui->listActive, SIGNAL(itemClicked(QListWidgetItem*)), SLOT(itemSelected(QListWidgetItem*)));

    m_ui->listInactive->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    m_ui->listInactive->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_ui->listInactive, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), SLOT(itemSelected(QListWidgetItem*,QListWidgetItem*)));
    connect(m_ui->listInactive, SIGNAL(itemClicked(QListWidgetItem*)), SLOT(itemSelected(QListWidgetItem*)));

    m_ui->bnAdd->setIcon(KisIconUtils::loadIcon("arrow-right"));
    connect(m_ui->bnAdd, SIGNAL(clicked()), SLOT(addSelected()));

    m_ui->bnRemove->setIcon(KisIconUtils::loadIcon("arrow-left"));
    connect(m_ui->bnRemove, SIGNAL(clicked()), SLOT(removeSelected()));

    m_ui->listBundleContents->setHeaderLabel(i18n("Resource"));
    m_ui->listBundleContents->setSelectionMode(QAbstractItemView::NoSelection);

    m_actionManager = actionMgr;

    refreshListData();

    connect(m_ui->bnEditBundle, SIGNAL(clicked()), SLOT(editBundle()));

    connect(m_ui->bnImportBrushes, SIGNAL(clicked()), SLOT(slotImportResource()));
    connect(m_ui->bnImportGradients, SIGNAL(clicked()), SLOT(slotImportResource()));
    connect(m_ui->bnImportPalettes, SIGNAL(clicked()), SLOT(slotImportResource()));
    connect(m_ui->bnImportPatterns, SIGNAL(clicked()), SLOT(slotImportResource()));
    connect(m_ui->bnImportPresets, SIGNAL(clicked()), SLOT(slotImportResource()));
    connect(m_ui->bnImportWorkspaces, SIGNAL(clicked()), SLOT(slotImportResource()));
    connect(m_ui->bnImportBundles, SIGNAL(clicked()), SLOT(slotImportResource()));


    connect(m_ui->createBundleButton, SIGNAL(clicked()), SLOT(slotCreateBundle()));
    connect(m_ui->deleteBackupFilesButton, SIGNAL(clicked()), SLOT(slotDeleteBackupFiles()));
    connect(m_ui->openResourceFolderButton, SIGNAL(clicked()), SLOT(slotOpenResourceFolder()));

}


void DlgBundleManager::refreshListData()
{
    KoResourceServer<KisResourceBundle> *bundleServer = KisResourceServerProvider::instance()->resourceBundleServer();

    m_ui->listInactive->clear();
    m_ui->listActive->clear();

    Q_FOREACH (const QString &f, bundleServer->blackListedFiles()) {
        KisResourceBundle *bundle = new KisResourceBundle(f);
        bundle->load();
        if (bundle->valid()) {
            bundle->setInstalled(false);
            m_blacklistedBundles[f] = bundle;
        }
    }
    fillListWidget(m_blacklistedBundles.values(), m_ui->listInactive);

    Q_FOREACH (KisResourceBundle *bundle, bundleServer->resources()) {
        if (bundle->valid()) {
            m_activeBundles[bundle->filename()] = bundle;
        }
    }
    fillListWidget(m_activeBundles.values(), m_ui->listActive);
}

void DlgBundleManager::accept()
{
    KoResourceServer<KisResourceBundle> *bundleServer = KisResourceServerProvider::instance()->resourceBundleServer();

    for (int i = 0; i < m_ui->listActive->count(); ++i) {
        QListWidgetItem *item = m_ui->listActive->item(i);
        QByteArray ba = item->data(Qt::UserRole).toByteArray();
        KisResourceBundle *bundle = bundleServer->resourceByMD5(ba);
        QMessageBox bundleFeedback;
        bundleFeedback.setIcon(QMessageBox::Warning);
        QString feedback = "bundlefeedback";

        if (!bundle) {
            // Get it from the blacklisted bundles
            Q_FOREACH (KisResourceBundle *b2, m_blacklistedBundles.values()) {
                if (b2->md5() == ba) {
                    bundle = b2;
                    break;
                }
            }
        }

        if (bundle) {
            if(!bundle->isInstalled()){
                bundle->install();
                //this removes the bundle from the blacklist and add it to the server without saving or putting it in front//
                if(!bundleServer->addResource(bundle, false, false)){

                    feedback = i18n("Couldn't add bundle to resource server");
                    bundleFeedback.setText(feedback);
                    bundleFeedback.exec();
                }
                if(!bundleServer->removeFromBlacklist(bundle)){
                    feedback = i18n("Couldn't remove bundle from blacklist");
                    bundleFeedback.setText(feedback);
                    bundleFeedback.exec();
                }
            }
            else {
            bundleServer->removeFromBlacklist(bundle);
            //let's asume that bundles who exist and are installed have to be removed from the blacklist, and if they were already this returns false, so that's not a problem.
            }
        }
        else{
        QString feedback = i18n("Bundle doesn't exist!");
        bundleFeedback.setText(feedback);
        bundleFeedback.exec();

        }
    }

    for (int i = 0; i < m_ui->listInactive->count(); ++i) {
        QListWidgetItem *item = m_ui->listInactive->item(i);
        QByteArray ba = item->data(Qt::UserRole).toByteArray();
        KisResourceBundle *bundle = bundleServer->resourceByMD5(ba);


        if (bundle && bundle->isInstalled()) {
            bundle->uninstall();
            bundleServer->removeResourceAndBlacklist(bundle);
        }
    }


    KoDialog::accept();
}

void DlgBundleManager::addSelected()
{

    Q_FOREACH (QListWidgetItem *item, m_ui->listActive->selectedItems()) {
        m_ui->listInactive->addItem(m_ui->listActive->takeItem(m_ui->listActive->row(item)));
    }

}

void DlgBundleManager::removeSelected()
{
    Q_FOREACH (QListWidgetItem *item, m_ui->listInactive->selectedItems()) {
        m_ui->listActive->addItem(m_ui->listInactive->takeItem(m_ui->listInactive->row(item)));
    }
}

void DlgBundleManager::itemSelected(QListWidgetItem *current, QListWidgetItem *)
{
    if (!current) {
        m_ui->lblName->clear();
        m_ui->lblAuthor->clear();
        m_ui->lblEmail->clear();
        m_ui->lblLicense->clear();
        m_ui->lblWebsite->clear();
        m_ui->lblDescription->clear();
        m_ui->lblCreated->clear();
        m_ui->lblUpdated->clear();
        m_ui->lblPreview->setPixmap(QPixmap::fromImage(QImage()));
        m_ui->listBundleContents->clear();
        m_ui->bnEditBundle->setEnabled(false);
        m_currentBundle = 0;
    }
    else {

        QByteArray ba = current->data(Qt::UserRole).toByteArray();
        KoResourceServer<KisResourceBundle> *bundleServer = KisResourceServerProvider::instance()->resourceBundleServer();
        KisResourceBundle *bundle = bundleServer->resourceByMD5(ba);

        if (!bundle) {
            // Get it from the blacklisted bundles
            Q_FOREACH (KisResourceBundle *b2, m_blacklistedBundles.values()) {
                if (b2->md5() == ba) {
                    bundle = b2;
                    break;
                }
            }
        }


        if (bundle) {

            m_currentBundle = bundle;
            m_ui->bnEditBundle->setEnabled(true);

            m_ui->lblName->setText(bundle->name());
            m_ui->lblAuthor->setText(bundle->getMeta("author"));
            m_ui->lblEmail->setText(bundle->getMeta("email"));
            m_ui->lblLicense->setText(bundle->getMeta("license"));
            m_ui->lblWebsite->setText(bundle->getMeta("website"));
            m_ui->lblDescription->setPlainText(bundle->getMeta("description"));
            m_ui->lblCreated->setText(bundle->getMeta("created"));
            m_ui->lblUpdated->setText(bundle->getMeta("updated"));
            m_ui->lblPreview->setPixmap(QPixmap::fromImage(bundle->image().scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            m_ui->listBundleContents->clear();

            Q_FOREACH (const QString & resType, bundle->resourceTypes()) {

                QTreeWidgetItem *toplevel = new QTreeWidgetItem();
                if (resType == "gradients") {
                    toplevel->setText(0, i18n("Gradients"));
                }
                else if (resType  == "patterns") {
                    toplevel->setText(0, i18n("Patterns"));
                }
                else if (resType  == "brushes") {
                    toplevel->setText(0, i18n("Brushes"));
                }
                else if (resType  == "palettes") {
                    toplevel->setText(0, i18n("Palettes"));
                }
                else if (resType  == "workspaces") {
                    toplevel->setText(0, i18n("Workspaces"));
                }
                else if (resType  == "paintoppresets") {
                    toplevel->setText(0, i18n("Brush Presets"));
                }

                m_ui->listBundleContents->addTopLevelItem(toplevel);

                Q_FOREACH (const KoResource *res, bundle->resources(resType)) {
                    if (res) {
                        QTreeWidgetItem *i = new QTreeWidgetItem();
                        i->setIcon(0, QIcon(QPixmap::fromImage(res->image())));
                        i->setText(0, res->name());
                        toplevel->addChild(i);
                    }
                }
            }
        }
        else {
            m_currentBundle = 0;
        }
    }
}

void DlgBundleManager::itemSelected(QListWidgetItem *current)
{
    itemSelected(current, 0);
}

void DlgBundleManager::editBundle()
{
    if (m_currentBundle) {
        DlgCreateBundle dlg(m_currentBundle);
        m_activeBundles.remove(m_currentBundle->filename());
        m_currentBundle = 0;
        if (dlg.exec() != QDialog::Accepted) {
            return;
        }
        m_currentBundle = m_resourceManager->saveBundle(dlg);
        refreshListData();
    }
}

void DlgBundleManager::fillListWidget(QList<KisResourceBundle *> bundles, QListWidget *w)
{
    w->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    w->setSelectionMode(QAbstractItemView::MultiSelection);

    Q_FOREACH (KisResourceBundle *bundle, bundles) {
        QPixmap pixmap(ICON_SIZE, ICON_SIZE);
        pixmap.fill(Qt::gray);
        if (!bundle->image().isNull()) {
            QImage scaled = bundle->image().scaled(ICON_SIZE, ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            int x = (ICON_SIZE - scaled.width()) / 2;
            int y = (ICON_SIZE - scaled.height()) / 2;
            QPainter gc(&pixmap);
            gc.drawImage(x, y, scaled);
            gc.end();
        }

        QListWidgetItem *item = new QListWidgetItem(pixmap, bundle->name());
        item->setData(Qt::UserRole, bundle->md5());
        w->addItem(item);
    }
}


void DlgBundleManager::slotImportResource()
{
    if (m_actionManager) {
        QObject *button = sender();
        QString buttonName = button->objectName();
        KisAction *action = 0;
        if (buttonName == "bnImportBundles") {
            action = m_actionManager->actionByName("import_bundles");
        }
        else if (buttonName == "bnImportBrushes") {
            action = m_actionManager->actionByName("import_brushes");
        }
        else if (buttonName == "bnImportGradients") {
            action = m_actionManager->actionByName("import_gradients");
        }
        else if (buttonName == "bnImportPalettes") {
            action = m_actionManager->actionByName("import_palettes");
        }
        else if (buttonName == "bnImportPatterns") {
            action = m_actionManager->actionByName("import_patterns");
        }
        else if (buttonName == "bnImportPresets") {
            action = m_actionManager->actionByName("import_presets");
        }
        else if (buttonName == "bnImportWorkspaces") {
            action = m_actionManager->actionByName("import_workspaces");
        }
        else {
            warnUI << "Unhandled bundle manager import button " << buttonName;
            return;
        }

        action->trigger();
        refreshListData();
    }
}

void DlgBundleManager::slotCreateBundle() {

    if (m_actionManager) {
        KisAction *action = m_actionManager->actionByName("create_bundle");
        action->trigger();
        refreshListData();
    }
}

void DlgBundleManager::slotDeleteBackupFiles() {

    if (m_actionManager) {
        KisAction *action = m_actionManager->actionByName("edit_blacklist_cleanup");
        action->trigger();
    }
}

void DlgBundleManager::slotOpenResourceFolder() {

    if (m_actionManager) {
        KisAction *action = m_actionManager->actionByName("open_resources_directory");
        action->trigger();
    }
}

