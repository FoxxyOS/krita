/* This file is part of the Krita project
 *
 * Copyright (C) 2014 Boudewijn Rempt <boud@kogmbh.com>
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

#include "KisMainWindow.h" // XXX: remove
#include <QMessageBox> // XXX: remove

#include <KisMimeDatabase.h>

#include <KoCanvasBase.h>
#include <KoColor.h>
#include <KoColorProfile.h>
#include <KoColorSpaceEngine.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoDocumentInfoDlg.h>
#include <KoDocumentInfo.h>
#include <KoDpi.h>
#include <KoUnit.h>
#include <KoEmbeddedDocumentSaver.h>
#include <KoFileDialog.h>
#include <KoID.h>
#include <KoOdfReadStore.h>
#include <KoProgressProxy.h>
#include <KoProgressUpdater.h>
#include <KoSelection.h>
#include <KoShape.h>
#include <KoShapeController.h>
#include <KoStore.h>
#include <KoUpdater.h>
#include <KoXmlWriter.h>
#include <KoXmlReader.h>

#include <klocalizedstring.h>
#include <kis_debug.h>
#include <kdesktopfile.h>
#include <kconfiggroup.h>
#include <QTemporaryFile>
#include <kbackup.h>

#include <QApplication>
#include <QBuffer>
#include <QDesktopServices>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFileInfo>
#include <QImage>
#include <QList>
#include <QPainter>
#include <QRect>
#include <QScopedPointer>
#include <QSize>
#include <QStringList>
#include <QtGlobal>
#include <QTimer>
#include <QWidget>

// Krita Image
#include <kis_config.h>
#include <flake/kis_shape_layer.h>
#include <kis_debug.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_layer.h>
#include <kis_name_server.h>
#include <kis_paint_layer.h>
#include <kis_painter.h>
#include <kis_selection.h>
#include <kis_fill_painter.h>
#include <kis_document_undo_store.h>
#include <kis_painting_assistants_decoration.h>
#include <kis_idle_watcher.h>
#include <kis_signal_auto_connection.h>
#include <kis_debug.h>

// Local
#include "KisViewManager.h"
#include "kis_clipboard.h"
#include "widgets/kis_custom_image_widget.h"
#include "canvas/kis_canvas2.h"
#include "flake/kis_shape_controller.h"
#include "kra/kis_kra_loader.h"
#include "kra/kis_kra_saver.h"
#include "kis_statusbar.h"
#include "widgets/kis_progress_widget.h"
#include "kis_canvas_resource_provider.h"
#include "kis_resource_server_provider.h"
#include "kis_node_manager.h"
#include "KisPart.h"
#include "KisApplication.h"
#include "KisDocument.h"
#include "KisImportExportManager.h"
#include "KisPart.h"
#include "KisView.h"
#include "kis_async_action_feedback.h"
#include "kis_grid_config.h"
#include "kis_guides_config.h"
#include "kis_image_barrier_lock_adapter.h"
#include <mutex>


static const char CURRENT_DTD_VERSION[] = "2.0";

// Define the protocol used here for embedded documents' URL
// This used to "store" but QUrl didn't like it,
// so let's simply make it "tar" !
#define STORE_PROTOCOL "tar"
// The internal path is a hack to make QUrl happy and for document children
#define INTERNAL_PROTOCOL "intern"
#define INTERNAL_PREFIX "intern:/"
// Warning, keep it sync in koStore.cc

#include <unistd.h>

using namespace std;

/**********************************************************
 *
 * KisDocument
 *
 **********************************************************/

namespace {

class DocumentProgressProxy : public KoProgressProxy {
public:
    KisMainWindow *m_mainWindow;
    DocumentProgressProxy(KisMainWindow *mainWindow)
        : m_mainWindow(mainWindow)
    {
    }

    ~DocumentProgressProxy() override {
        // signal that the job is done
        setValue(-1);
    }

    int maximum() const override {
        return 100;
    }

    void setValue(int value) override {
        if (m_mainWindow) {
            m_mainWindow->slotProgress(value);
        }
    }

    void setRange(int /*minimum*/, int /*maximum*/) override {

    }

    void setFormat(const QString &/*format*/) override {

    }
};
}

//static
QString KisDocument::newObjectName()
{
    static int s_docIFNumber = 0;
    QString name; name.setNum(s_docIFNumber++); name.prepend("document_");
    return name;
}


class UndoStack : public KUndo2Stack
{
public:
    UndoStack(KisDocument *doc)
        : m_doc(doc)
    {
    }

    void setIndex(int idx) override {
        KisImageWSP image = this->image();
        image->requestStrokeCancellation();
        if(image->tryBarrierLock()) {
            KUndo2Stack::setIndex(idx);
            image->unlock();
        }
    }

    void notifySetIndexChangedOneCommand() override {
        KisImageWSP image = this->image();
        image->unlock();

        /**
         * Some very weird commands may emit blocking signals to
         * the GUI (e.g. KisGuiContextCommand). Here is the best thing
         * we can do to avoid the deadlock
         */
        while(!image->tryBarrierLock()) {
            QApplication::processEvents();
        }
    }

    void undo() override {
        KisImageWSP image = this->image();
        image->requestUndoDuringStroke();

        if (image->tryUndoUnfinishedLod0Stroke() == UNDO_OK) {
            return;
        }

        if(image->tryBarrierLock()) {
            KUndo2Stack::undo();
            image->unlock();
        }
    }

    void redo() override {
        KisImageWSP image = this->image();
        if(image->tryBarrierLock()) {
            KUndo2Stack::redo();
            image->unlock();
        }
    }

private:
    KisImageWSP image() {
        KisImageWSP currentImage = m_doc->image();
        Q_ASSERT(currentImage);
        return currentImage;
    }

private:
    KisDocument *m_doc;
};

class Q_DECL_HIDDEN KisDocument::Private
{
public:
    Private(KisDocument *document) :
        document(document),
        // XXX: the part should _not_ be modified from the document
        docInfo(0),
        progressUpdater(0),
        progressProxy(0),
        importExportManager(0),
        specialOutputFlag(0),   // default is native format
        isImporting(false),
        isExporting(false),
        password(QString()),
        modifiedAfterAutosave(false),
        isAutosaving(false),
        autoErrorHandlingEnabled(true),
        backupFile(true),
        backupPath(QString()),
        doNotSaveExtDoc(false),
        storeInternal(false),
        isLoading(false),
        undoStack(0),
        m_saveOk(false),
        m_waitForSave(false),
        m_duringSaveAs(false),
        m_bTemp(false),
        m_bAutoDetectedMime(false),
        modified(false),
        readwrite(true),
        disregardAutosaveFailure(false),
        nserver(0),
        macroNestDepth(0),
        imageIdleWatcher(2000 /*ms*/),
        kraLoader(0),
        suppressProgress(false),
        fileProgressProxy(0),
        savingLock(&savingMutex)
    {
        if (QLocale().measurementSystem() == QLocale::ImperialSystem) {
            unit = KoUnit::Inch;
        } else {
            unit = KoUnit::Centimeter;
        }
    }

    ~Private() {
        // Don't delete m_d->shapeController because it's in a QObject hierarchy.
        delete nserver;
    }

    KisDocument *document;

    KoDocumentInfo *docInfo;

    KoProgressUpdater *progressUpdater;
    KoProgressProxy *progressProxy;

    KoUnit unit;

    KisImportExportManager *importExportManager; // The filter-manager to use when loading/saving [for the options]

    QByteArray mimeType; // The actual mimetype of the document
    QByteArray outputMimeType; // The mimetype to use when saving
    bool confirmNonNativeSave [2] = {true, true}; // used to pop up a dialog when saving for the
    // first time if the file is in a foreign format
    // (Save/Save As, Export)
    int specialOutputFlag; // See KoFileDialog in koMainWindow.cc
    bool isImporting;
    bool isExporting; // File --> Import/Export vs File --> Open/Save
    QString password; // The password used to encrypt an encrypted document

    QTimer autoSaveTimer;
    QString lastErrorMessage; // see openFile()
    int autoSaveDelay; // in seconds, 0 to disable.
    bool modifiedAfterAutosave;
    bool isAutosaving;
    bool autoErrorHandlingEnabled; // usually true
    bool backupFile;
    QString backupPath;
    bool doNotSaveExtDoc; // makes it possible to save only internally stored child documents
    bool storeInternal; // Store this doc internally even if url is external
    bool isLoading; // True while loading (openUrl is async)

    KUndo2Stack *undoStack;

    KisGuidesConfig guidesConfig;

    bool isEmpty;

    KoPageLayout pageLayout;

    QUrl m_originalURL; // for saveAs
    QString m_originalFilePath; // for saveAs
    bool m_saveOk : 1;
    bool m_waitForSave : 1;
    bool m_duringSaveAs : 1;
    bool m_bTemp: 1;      // If @p true, @p m_file is a temporary file that needs to be deleted later.
    bool m_bAutoDetectedMime : 1; // whether the mimetype in the arguments was detected by the part itself
    QUrl m_url; // Remote (or local) url - the one displayed to the user.
    QString m_file; // Local file - the only one the part implementation should deal with.
    QEventLoop m_eventLoop;
    QMutex savingMutex;

    bool modified;
    bool readwrite;

    QDateTime firstMod;
    QDateTime lastMod;

    bool disregardAutosaveFailure;

    KisNameServer *nserver;
    qint32 macroNestDepth;

    KisImageSP image;
    KisImageSP savingImage;

    KisNodeSP preActivatedNode;
    KisShapeController* shapeController;
    KoShapeController* koShapeController;
    KisIdleWatcher imageIdleWatcher;
    QScopedPointer<KisSignalAutoConnection> imageIdleConnection;


    KisKraLoader* kraLoader;
    KisKraSaver* kraSaver;

    bool suppressProgress;
    KoProgressProxy* fileProgressProxy;

    QList<KisPaintingAssistantSP> assistants;
    KisGridConfig gridConfig;

    StdLockableWrapper<QMutex> savingLock;

    bool openFile() {
        document->setFileProgressProxy();
        document->setUrl(m_url);

        bool ok = document->openFile();

        document->clearFileProgressProxy();
        return ok;
    }

    bool openLocalFile()
    {
        m_bTemp = false;
        // set the mimetype only if it was not already set (for example, by the host application)
        if (mimeType.isEmpty()) {
            // get the mimetype of the file
            // using findByUrl() to avoid another string -> url conversion
            QString mime = KisMimeDatabase::mimeTypeForFile(m_url.toLocalFile());
            mimeType = mime.toLocal8Bit();
            m_bAutoDetectedMime = true;
        }
        const bool ret = openFile();
        if (ret) {
            emit document->completed();
        } else {
            emit document->canceled(QString());
        }
        return ret;
    }

    // Set m_file correctly for m_url
    void prepareSaving()
    {
        // Local file
        if ( m_url.isLocalFile() )
        {
            if ( m_bTemp ) // get rid of a possible temp file first
            {              // (happens if previous url was remote)
                QFile::remove( m_file );
                m_bTemp = false;
            }
            m_file = m_url.toLocalFile();
        }
    }

    void setImageAndInitIdleWatcher(KisImageSP _image) {
        image = _image;

        imageIdleWatcher.setTrackedImage(image);

        if (image) {
            imageIdleConnection.reset(
                        new KisSignalAutoConnection(
                            &imageIdleWatcher, SIGNAL(startedIdleMode()),
                            image.data(), SLOT(explicitRegenerateLevelOfDetail())));
        }
    }

    class SafeSavingLocker;
    class SavingImageSetter;
};

class KisDocument::Private::SafeSavingLocker {
public:
    SafeSavingLocker(KisDocument::Private *_d)
        : d(_d),
          m_locked(false),
          m_imageLock(d->image, true)
    {
        const int realAutoSaveInterval = KisConfig().autoSaveInterval();
        const int emergencyAutoSaveInterval = 10; // sec

        /**
         * Initial try to lock both objects. Locking the image guards
         * us from any image composition threads running in the
         * background, while savingMutex guards us from entering the
         * saving code twice by autosave and main threads.
         *
         * Since we are trying to lock multiple objects, so we should
         * do it in a safe manner.
         */
        m_locked = std::try_lock(m_imageLock, d->savingLock) < 0;

        if (!m_locked) {
            if (d->isAutosaving) {
                d->disregardAutosaveFailure = true;
                if (realAutoSaveInterval) {
                    d->document->setAutoSave(emergencyAutoSaveInterval);
                }
            } else {
                d->image->requestStrokeEnd();
                QApplication::processEvents();

                // one more try...
                m_locked = std::try_lock(m_imageLock, d->savingLock) < 0;
            }
        }

        if (m_locked) {
            d->disregardAutosaveFailure = false;
        }
    }

    ~SafeSavingLocker() {
        if (m_locked) {
            m_imageLock.unlock();
            d->savingLock.unlock();

            const int realAutoSaveInterval = KisConfig().autoSaveInterval();
            d->document->setAutoSave(realAutoSaveInterval);
        }
    }

    void doEmergencyLock() {
        KIS_SAFE_ASSERT_RECOVER_RETURN(!m_locked);

        if (d->savingLock.try_lock()) {
            d->image->lock();
            m_locked = true;
        }
    }

    bool successfullyLocked() const {
        return m_locked;
    }

private:
    KisDocument::Private *d;
    bool m_locked;

    KisImageBarrierLockAdapter m_imageLock;
};

class KisDocument::Private::SavingImageSetter {
public:
    SavingImageSetter(KisDocument::Private *_d, KisImageSP image)
        : d(_d)
    {
        d->savingImage = image;
    }

    ~SavingImageSetter() {
        d->savingImage.clear();
    }

private:
    KisDocument::Private *d;
};


KisDocument::KisDocument()
    : d(new Private(this))
{
    d->undoStack = new UndoStack(this);
    d->undoStack->setParent(this);

    d->isEmpty = true;
    d->importExportManager = new KisImportExportManager(this);
    d->importExportManager->setProgresUpdater(d->progressUpdater);

    connect(&d->autoSaveTimer, SIGNAL(timeout()), this, SLOT(slotAutoSave()));
    setAutoSave(defaultAutoSave());

    setObjectName(newObjectName());

    d->docInfo = new KoDocumentInfo(this);

    d->pageLayout.width = 0;
    d->pageLayout.height = 0;
    d->pageLayout.topMargin = 0;
    d->pageLayout.bottomMargin = 0;
    d->pageLayout.leftMargin = 0;
    d->pageLayout.rightMargin = 0;

    d->firstMod = QDateTime::currentDateTime();
    d->lastMod = QDateTime::currentDateTime();

    // preload the krita resources
    KisResourceServerProvider::instance();

    init();
    undoStack()->setUndoLimit(KisConfig().undoStackLimit());
    connect(d->undoStack, SIGNAL(indexChanged(int)), this, SLOT(slotUndoStackIndexChanged(int)));
    setBackupFile(KisConfig().backupFile());
}

KisDocument::~KisDocument()
{
    /**
     * Push a timebomb, which will try to release the memory after
     * the document has been deleted
     */
    KisPaintDevice::createMemoryReleaseObject()->deleteLater();

    d->autoSaveTimer.disconnect(this);
    d->autoSaveTimer.stop();

    delete d->importExportManager;

    // Despite being QObject they needs to be deleted before the image
    delete d->shapeController;

    delete d->koShapeController;

    if (d->image) {
        d->image->notifyAboutToBeDeleted();

        /**
         * WARNING: We should wait for all the internal image jobs to
         * finish before entering KisImage's destructor. The problem is,
         * while execution of KisImage::~KisImage, all the weak shared
         * pointers pointing to the image enter an inconsistent
         * state(!). The shared counter is already zero and destruction
         * has started, but the weak reference doesn't know about it,
         * because KisShared::~KisShared hasn't been executed yet. So all
         * the threads running in background and having weak pointers will
         * enter the KisImage's destructor as well.
         */

        d->image->requestStrokeCancellation();
        d->image->waitForDone();

        // clear undo commands that can still point to the image
        d->undoStack->clear();
        d->image->waitForDone();

        KisImageWSP sanityCheckPointer = d->image;
        Q_UNUSED(sanityCheckPointer);
        // The following line trigger the deletion of the image
        d->image.clear();

        // check if the image has actually been deleted
        KIS_SAFE_ASSERT_RECOVER_NOOP(!sanityCheckPointer.isValid());
    }


    delete d;
}

void KisDocument::init()
{
    delete d->nserver;
    d->nserver = 0;

    d->nserver = new KisNameServer(1);
    Q_CHECK_PTR(d->nserver);

    d->shapeController = new KisShapeController(this, d->nserver);
    d->koShapeController = new KoShapeController(0, d->shapeController);

    d->kraSaver = 0;
    d->kraLoader = 0;
}

bool KisDocument::reload()
{
    // XXX: reimplement!
    return false;
}

bool KisDocument::exportDocument(const QUrl &_url, KisPropertiesConfigurationSP exportConfiguration)
{
    bool ret;

    d->isExporting = true;

    //
    // Preserve a lot of state here because we need to restore it in order to
    // be able to fake a File --> Export.  Can't do this in saveFile() because,
    // for a start, KParts has already set url and m_file and because we need
    // to restore the modified flag etc. and don't want to put a load on anyone
    // reimplementing saveFile() (Note: importDocument() and exportDocument()
    // will remain non-virtual).
    //
    QUrl oldURL = url();
    QString oldFile = localFilePath();

    bool wasModified = isModified();
    QByteArray oldMimeType = mimeType();

    // save...
    ret = saveAs(_url, exportConfiguration);

    //
    // This is sooooo hacky :(
    // Hopefully we will restore enough state.
    //
    dbgUI << "Restoring KisDocument state to before export";

    // always restore url & m_file because KParts has changed them
    // (regardless of failure or success)
    setUrl(oldURL);
    setLocalFilePath(oldFile);

    // on successful export we need to restore modified etc. too
    // on failed export, mimetype/modified hasn't changed anyway
    if (ret) {
        setModified(wasModified);
        d->mimeType = oldMimeType;
    }

    d->isExporting = false;

    return ret;
}

bool KisDocument::saveFile(KisPropertiesConfigurationSP exportConfiguration)
{
    // Unset the error message
    setErrorMessage("");

    // Save it to be able to restore it after a failed save
    const bool wasModified = isModified();
    bool ret = false;
    bool suppressErrorDialog = fileBatchMode();
    KisImportExportFilter::ConversionStatus status = KisImportExportFilter::OK;

    // The output format is set by koMainWindow, and by openFile
    QByteArray outputMimeType = d->outputMimeType;
    if (outputMimeType.isEmpty()) {
        outputMimeType = d->outputMimeType = nativeFormatMimeType();
    }
    if (QFileInfo(url().toLocalFile()).exists() && !QFileInfo(url().toLocalFile()).isWritable()) {
        setErrorMessage(i18n("%1 cannot be written to. Please save under a different name.", localFilePath()));
    }
    else {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        if (backupFile()) {
            Q_ASSERT(url().isLocalFile());
            KBackup::backupFile(url().toLocalFile(), d->backupPath);
        }

        qApp->processEvents();
        setFileProgressUpdater(i18n("Saving Document"));

        QFileInfo fi(localFilePath());
        QString tempororaryFileName;
        {
            QTemporaryFile tf(QDir::tempPath() + "/XXXXXX" + fi.baseName() + "." + fi.completeSuffix());
            tf.open();
            tempororaryFileName = tf.fileName();
        }
        Q_ASSERT(!tempororaryFileName.isEmpty());

        if (!isNativeFormat(outputMimeType)) {

            {
                Private::SafeSavingLocker locker(d);

                if (!locker.successfullyLocked()) {
                    d->lastErrorMessage = i18n("The image was still busy while saving. Your saved image might be incomplete.");
                    locker.doEmergencyLock();
                }

                if (!locker.successfullyLocked()) {
                    d->lastErrorMessage = i18n("Could not lock the image for saving.");
                    return false;
                }

                KIS_SAFE_ASSERT_RECOVER_NOOP(d->image->locked());
                Private::SavingImageSetter savingImageSetter(d, d->image);

                status = d->importExportManager->exportDocument(tempororaryFileName, outputMimeType, exportConfiguration);
            }

            ret = status == KisImportExportFilter::OK;
            suppressErrorDialog = (status == KisImportExportFilter::UserCancelled || status == KisImportExportFilter::BadConversionGraph || d->importExportManager->getBatchMode());
            dbgFile << "Export status was" << status;
        } else {
            // Native format => normal save
            ret = saveNativeFormat(tempororaryFileName);
        }

        if (ret) {
            if (!d->suppressProgress) {
                QPointer<KoUpdater> updater = d->progressUpdater->startSubtask(1, "clear undo stack");
                updater->setProgress(0);
                d->undoStack->setClean();
                updater->setProgress(100);
            } else {
                d->undoStack->setClean();
            }

            QFile tempFile(tempororaryFileName);
            QString s = localFilePath();
            QFile dstFile(s);
            while (QFileInfo(s).exists()) {
                s.append("_");
            }
            bool r;
            if (s != localFilePath()) {
                r = dstFile.rename(s);
                if (!r) {
                    setErrorMessage(i18n("Could not rename original file to %1: %2", dstFile.fileName(), dstFile. errorString()));
                }
            }

            if (tempFile.exists()) {

                ret = tempFile.copy(localFilePath());
                if (!ret) {
                    setErrorMessage(i18n("Copying the temporary file failed: %1 to %2: %3", tempFile.fileName(), dstFile.fileName(), tempFile.errorString()));
                }
                else {
                    r = tempFile.remove();
                    if (!ret) {
                        setErrorMessage(i18n("Could not remove temporary file %1: %2", tempFile.fileName(), tempFile.errorString()));
                    }
                    else if (s != localFilePath()) {
                        r = dstFile.remove();
                        if (!ret) {
                            setErrorMessage(i18n("Could not remove saved original file: %1", dstFile.errorString()));
                        }
                    }
                }
            }
            else {
                setErrorMessage(i18n("The temporary file %1 is gone before we could copy it!", tempFile.fileName()));
                ret = false;
            }

            if (errorMessage().isEmpty()) {
                removeAutoSaveFiles();
            }
            else {
                qWarning() << "Error while saving:" << errorMessage();
            }
            // Restart the autosave timer
            // (we don't want to autosave again 2 seconds after a real save)
            setAutoSave(d->autoSaveDelay);

            d->mimeType = outputMimeType;
            setConfirmNonNativeSave(isExporting(), false);
        }
    }
    if (!ret) {
        if (!suppressErrorDialog) {

            if (errorMessage().isEmpty()) {
                setErrorMessage(KisImportExportFilter::conversionStatusString(status));
            }

            if (errorMessage().isEmpty()) {
                QMessageBox::critical(0, i18nc("@title:window", "Krita"), i18n("Could not save\n%1", localFilePath()));
            } else {
                QMessageBox::critical(0, i18nc("@title:window", "Krita"), i18n("Could not save %1\nReason: %2", localFilePath(), errorMessage()));
            }

        }

        // couldn't save file so this new URL is invalid
        // FIXME: we should restore the current document's true URL instead of
        // setting it to nothing otherwise anything that depends on the URL
        // being correct will not work (i.e. the document will be called
        // "Untitled" which may not be true)
        //
        // Update: now the URL is restored in KisMainWindow but really, this
        // should still be fixed in KisDocument/KParts (ditto for file).
        // We still resetURL() here since we may or may not have been called
        // by KisMainWindow - Clarence
        resetURL();

        // As we did not save, restore the "was modified" status
        setModified(wasModified);
    }

    clearFileProgressUpdater();

    QApplication::restoreOverrideCursor();

    return ret;
}


QByteArray KisDocument::mimeType() const
{
    return d->mimeType;
}

void KisDocument::setMimeType(const QByteArray & mimeType)
{
    d->mimeType = mimeType;
}

void KisDocument::setOutputMimeType(const QByteArray & mimeType, int specialOutputFlag)
{
    d->outputMimeType = mimeType;
    d->specialOutputFlag = specialOutputFlag;
}

QByteArray KisDocument::outputMimeType() const
{
    return d->outputMimeType;
}

int KisDocument::specialOutputFlag() const
{
    return d->specialOutputFlag;
}

bool KisDocument::confirmNonNativeSave(const bool exporting) const
{
    // "exporting ? 1 : 0" is different from "exporting" because a bool is
    // usually implemented like an "int", not "unsigned : 1"
    return d->confirmNonNativeSave [ exporting ? 1 : 0 ];
}

void KisDocument::setConfirmNonNativeSave(const bool exporting, const bool on)
{
    d->confirmNonNativeSave [ exporting ? 1 : 0] = on;
}

bool KisDocument::fileBatchMode() const
{
    return d->importExportManager->getBatchMode();
}

void KisDocument::setFileBatchMode(const bool batchMode)
{
    d->importExportManager->setBatchMode(batchMode);
}

bool KisDocument::isImporting() const
{
    return d->isImporting;
}

bool KisDocument::isExporting() const
{
    return d->isExporting;
}

void KisDocument::setAutoErrorHandlingEnabled(bool b)
{
    d->autoErrorHandlingEnabled = b;
}

bool KisDocument::isAutoErrorHandlingEnabled() const
{
    return d->autoErrorHandlingEnabled;
}

void KisDocument::slotAutoSave()
{
    if (!d->isAutosaving && d->modified && d->modifiedAfterAutosave && !d->isLoading) {
        // Give a warning when trying to autosave an encrypted file when no password is known (should not happen)
        if (d->specialOutputFlag == SaveEncrypted && d->password.isNull()) {
            // That advice should also fix this error from occurring again
            emit statusBarMessage(i18n("The password of this encrypted document is not known. Autosave aborted! Please save your work manually."));
        } else {
            connect(this, SIGNAL(sigProgress(int)), KisPart::instance()->currentMainwindow(), SLOT(slotProgress(int)));
            emit statusBarMessage(i18n("Autosaving..."));
            d->isAutosaving = true;
            bool ret = saveNativeFormat(autoSaveFile(localFilePath()));
            setModified(true);
            if (ret) {
                d->modifiedAfterAutosave = false;
                d->autoSaveTimer.stop(); // until the next change
            }
            d->isAutosaving = false;
            emit clearStatusBarMessage();
            disconnect(this, SIGNAL(sigProgress(int)), KisPart::instance()->currentMainwindow(), SLOT(slotProgress(int)));
            if (!ret && !d->disregardAutosaveFailure) {
                emit statusBarMessage(i18n("Error during autosave! Partition full?"));
            }
        }
    }
}

void KisDocument::setReadWrite(bool readwrite)
{
    d->readwrite = readwrite;
    setAutoSave(d->autoSaveDelay);

    Q_FOREACH (KisMainWindow *mainWindow, KisPart::instance()->mainWindows()) {
        mainWindow->setReadWrite(readwrite);
    }

}

void KisDocument::setAutoSave(int delay)
{
    d->autoSaveDelay = delay;
    if (isReadWrite() && d->autoSaveDelay > 0)
        d->autoSaveTimer.start(d->autoSaveDelay * 1000);
    else
        d->autoSaveTimer.stop();
}

KoDocumentInfo *KisDocument::documentInfo() const
{
    return d->docInfo;
}

bool KisDocument::isModified() const
{
    return d->modified;
}

bool KisDocument::saveNativeFormat(const QString & file)
{
    d->lastErrorMessage.clear();

    Private::SafeSavingLocker locker(d);

    if (!locker.successfullyLocked()) {
        d->lastErrorMessage = i18n("The image was still busy while saving. Your saved image might be incomplete.");
        locker.doEmergencyLock();
    }

    if (!locker.successfullyLocked()) {
        d->lastErrorMessage = i18n("Could not lock the image for saving.");
        return false;
    }

    KIS_SAFE_ASSERT_RECOVER_NOOP(d->image->locked());
    Private::SavingImageSetter savingImageSetter(d, d->image);

    //dbgUI <<"Saving to store";

    KoStore::Backend backend = KoStore::Auto;
    if (d->specialOutputFlag == SaveAsDirectoryStore) {
        backend = KoStore::Directory;
        dbgUI << "Saving as uncompressed XML, using directory store.";
    }
    else if (d->specialOutputFlag == SaveAsFlatXML) {
        dbgUI << "Saving as a flat XML file.";
        QFile f(file);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            bool success = saveToStream(&f);
            f.close();
            return success;
        } else {
            return false;
        }
    }

    dbgUI << "KisDocument::saveNativeFormat nativeFormatMimeType=" << nativeFormatMimeType();

    // TODO: use std::auto_ptr or create store on stack [needs API fixing],
    // to remove all the 'delete store' in all the branches
    KoStore *store = KoStore::createStore(file, KoStore::Write, d->outputMimeType, backend);
    if (d->specialOutputFlag == SaveEncrypted && !d->password.isNull()) {
        store->setPassword(d->password);
    }
    if (store->bad()) {
        d->lastErrorMessage = i18n("Could not create the file for saving");   // more details needed?
        delete store;
        return false;
    }

    bool result = false;

    if (!d->isAutosaving) {
        KisAsyncActionFeedback f(i18n("Saving document..."), 0);
        result = f.runAction(std::bind(&KisDocument::saveNativeFormatCalligraImpl, this, store));
    } else {
        result = saveNativeFormatCalligraImpl(store);
    }

    delete store;
    return result;
}

QByteArray KisDocument::serializeToNativeByteArray()
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);

    QScopedPointer<KoStore> store(KoStore::createStore(&buffer, KoStore::Write, nativeFormatMimeType(), KoStore::Auto));
    if (store->bad()) {
        return byteArray;
    }

    Private::SafeSavingLocker locker(d);
    if (!locker.successfullyLocked()) {
        return byteArray;
    }

    Private::SavingImageSetter savingImageSetter(d, d->image);

    KisAsyncActionFeedback f(i18n("Exporting document..."), 0);
    bool result = f.runAction(std::bind(&KisDocument::saveNativeFormatCalligraImpl, this, store.data()));

    if (!result) {
        byteArray.clear();
    }

    return byteArray;
}

bool KisDocument::saveNativeFormatCalligraDirect(KoStore *store)
{
    bool result = false;

    Private::SafeSavingLocker locker(d);
    if (locker.successfullyLocked()) {
        d->savingImage = d->image;
        result = saveNativeFormatCalligraImpl(store);
        d->savingImage = 0;
    }

    return result;
}

bool KisDocument::saveNativeFormatCalligraImpl(KoStore *store)
{
    dbgUI << "Saving root";
    if (store->open("root")) {
        KoStoreDevice dev(store);
        if (!saveToStream(&dev) || !store->close()) {
            dbgUI << "saveToStream failed";
            return false;
        }
    } else {
        d->lastErrorMessage = i18n("Not able to write '%1'. Partition full?", QString("maindoc.xml"));
        return false;
    }
    if (store->open("documentinfo.xml")) {
        QDomDocument doc = KisDocument::createDomDocument("document-info"
                                                          /*DTD name*/, "document-info" /*tag name*/, "1.1");


        doc = d->docInfo->save(doc);
        KoStoreDevice dev(store);

        QByteArray s = doc.toByteArray(); // this is already Utf8!
        (void)dev.write(s.data(), s.size());
        (void)store->close();
    }

    if (store->open("preview.png")) {
        // ### TODO: missing error checking (The partition could be full!)
        savePreview(store);
        (void)store->close();
    }

    if (!completeSaving(store)) {
        return false;
    }
    dbgUI << "Saving done of url:" << url().url();
    if (!store->finalize()) {
        return false;
    }

    return true;
}

bool KisDocument::saveToStream(QIODevice *dev)
{
    QDomDocument doc = saveXML();
    // Save to buffer
    QByteArray s = doc.toByteArray(); // utf8 already
    dev->open(QIODevice::WriteOnly);
    int nwritten = dev->write(s.data(), s.size());
    if (nwritten != (int)s.size())
        warnUI << "wrote " << nwritten << "- expected" <<  s.size();
    return nwritten == (int)s.size();
}

// Called for embedded documents
bool KisDocument::saveToStore(KoStore *_store, const QString & _path)
{
    dbgUI << "Saving document to store" << _path;

    _store->pushDirectory();
    // Use the path as the internal url
    if (_path.startsWith(STORE_PROTOCOL))
        setUrl(QUrl(_path));
    else // ugly hack to pass a relative URI
        setUrl(QUrl(INTERNAL_PREFIX +  _path));

    // In the current directory we're the king :-)
    if (_store->open("root")) {
        KoStoreDevice dev(_store);
        if (!saveToStream(&dev)) {
            _store->close();
            return false;
        }
        if (!_store->close())
            return false;
    }

    if (!completeSaving(_store))
        return false;

    // Now that we're done leave the directory again
    _store->popDirectory();

    dbgUI << "Saved document to store";

    return true;
}

bool KisDocument::savePreview(KoStore *store)
{
    QPixmap pix = generatePreview(QSize(256, 256));
    QImage preview(pix.toImage().convertToFormat(QImage::Format_ARGB32, Qt::ColorOnly));
    if (preview.size() == QSize(0,0)) {
        QSize newSize = d->image->bounds().size();
        newSize.scale(QSize(256, 256), Qt::KeepAspectRatio);
        preview = QImage(newSize, QImage::Format_ARGB32);
        preview.fill(QColor(0, 0, 0, 0));
    }
    KoStoreDevice io(store);
    if (!io.open(QIODevice::WriteOnly)) {
        return false;
    }

    if (!preview.save(&io, "PNG")) {     // ### TODO What is -9 in quality terms?
        return false;
    }

    io.close();
    return true;
}

QPixmap KisDocument::generatePreview(const QSize& size)
{
    if (d->image) {
        QRect bounds = d->image->bounds();
        QSize newSize = bounds.size();
        newSize.scale(size, Qt::KeepAspectRatio);
        return QPixmap::fromImage(d->image->convertToQImage(newSize, 0));
    }
    return QPixmap(size);
}

QString KisDocument::autoSaveFile(const QString & path) const
{
    QString retval;

    // Using the extension allows to avoid relying on the mime magic when opening
    const QString extension (".kra");

    if (path.isEmpty()) {
        // Never saved?
#ifdef Q_OS_WIN
        // On Windows, use the temp location (https://bugs.kde.org/show_bug.cgi?id=314921)
        retval = QString("%1%2.%3-%4-%5-autosave%6").arg(QDir::tempPath()).arg(QDir::separator()).arg("krita").arg(qApp->applicationPid()).arg(objectName()).arg(extension);
#else
        // On Linux, use a temp file in $HOME then. Mark it with the pid so two instances don't overwrite each other's autosave file
        retval = QString("%1%2.%3-%4-%5-autosave%6").arg(QDir::homePath()).arg(QDir::separator()).arg("krita").arg(qApp->applicationPid()).arg(objectName()).arg(extension);
#endif
    } else {
        QFileInfo fi(path);
        QString dir = fi.absolutePath();
        QString filename = fi.fileName();
        retval = QString("%1%2.%3-autosave%4").arg(dir).arg(QDir::separator()).arg(filename).arg(extension);
    }
    return retval;
}

bool KisDocument::importDocument(const QUrl &_url)
{
    bool ret;

    dbgUI << "url=" << _url.url();
    d->isImporting = true;

    // open...
    ret = openUrl(_url);

    // reset url & m_file (kindly? set by KisParts::openUrl()) to simulate a
    // File --> Import
    if (ret) {
        dbgUI << "success, resetting url";
        resetURL();
        setTitleModified();
    }

    d->isImporting = false;

    return ret;
}


bool KisDocument::openUrl(const QUrl &_url, KisDocument::OpenUrlFlags flags)
{
    if (!_url.isLocalFile()) {
        return false;
    }
    dbgUI << "url=" << _url.url();
    d->lastErrorMessage.clear();

    // Reimplemented, to add a check for autosave files and to improve error reporting
    if (!_url.isValid()) {
        d->lastErrorMessage = i18n("Malformed URL\n%1", _url.url());  // ## used anywhere ?
        return false;
    }

    QUrl url(_url);
    bool autosaveOpened = false;
    d->isLoading = true;
    if (url.isLocalFile() && !fileBatchMode()) {
        QString file = url.toLocalFile();
        QString asf = autoSaveFile(file);
        if (QFile::exists(asf)) {
            KisApplication *kisApp = static_cast<KisApplication*>(qApp);
            kisApp->hideSplashScreen();
            //dbgUI <<"asf=" << asf;
            // ## TODO compare timestamps ?
            int res = QMessageBox::warning(0,
                                           i18nc("@title:window", "Krita"),
                                           i18n("An autosaved file exists for this document.\nDo you want to open it instead?"),
                                           QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
            switch (res) {
            case QMessageBox::Yes :
                url.setPath(asf);
                autosaveOpened = true;
                break;
            case QMessageBox::No :
                QFile::remove(asf);
                break;
            default: // Cancel
                d->isLoading = false;
                return false;
            }
        }
    }

    bool ret = openUrlInternal(url);

    if (autosaveOpened) {
        resetURL(); // Force save to act like 'Save As'
        setReadWrite(true); // enable save button
        setModified(true);
    }
    else {
        if( !(flags & OPEN_URL_FLAG_DO_NOT_ADD_TO_RECENT_FILES) ) {
            KisPart::instance()->addRecentURLToAllMainWindows(_url);
        }

        if (ret) {
            // Detect readonly local-files; remote files are assumed to be writable
            QFileInfo fi(url.toLocalFile());
            setReadWrite(fi.isWritable());
        }
    }
    return ret;
}

bool KisDocument::openFile()
{
    //dbgUI <<"for" << localFilePath();
    if (!QFile::exists(localFilePath())) {
        QApplication::restoreOverrideCursor();
        if (d->autoErrorHandlingEnabled)
            // Maybe offer to create a new document with that name ?
            QMessageBox::critical(0, i18nc("@title:window", "Krita"), i18n("File %1 does not exist.", localFilePath()));
        d->isLoading = false;
        return false;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    d->specialOutputFlag = 0;

    QString filename = localFilePath();
    QString typeName = mimeType();

    if (typeName.isEmpty()) {
        typeName = KisMimeDatabase::mimeTypeForFile(filename);
    }

    //qDebug() << "mimetypes 4:" << typeName;

    // Allow to open backup files, don't keep the mimetype application/x-trash.
    if (typeName == "application/x-trash") {
        QString path = filename;
        while (path.length() > 0) {
            path.chop(1);
            typeName = KisMimeDatabase::mimeTypeForFile(path);
            //qDebug() << "\t" << path << typeName;
            if (!typeName.isEmpty()) {
                break;
            }
        }
        //qDebug() << "chopped" << filename  << "to" << path << "Was trash, is" << typeName;
    }
    dbgUI << localFilePath() << "type:" << typeName;

    QString importedFile = localFilePath();

    setFileProgressUpdater(i18n("Opening Document"));

    if (!isNativeFormat(typeName.toLatin1())) {
        KisImportExportFilter::ConversionStatus status;

        importedFile = d->importExportManager->importDocument(localFilePath(), typeName, status);
        if (status != KisImportExportFilter::OK) {
            QApplication::restoreOverrideCursor();

            QString msg = KisImportExportFilter::conversionStatusString(status);

            if (d->autoErrorHandlingEnabled && !msg.isEmpty()) {
                QString errorMsg(i18n("Could not open %2.\nReason: %1.\n%3", msg, prettyPathOrUrl(), errorMessage()));
                QMessageBox::critical(0, i18nc("@title:window", "Krita"), errorMsg);
            }
            d->isLoading = false;
            clearFileProgressUpdater();
            return false;
        }
        d->isEmpty = false;
        //qDebug() << "importedFile" << importedFile << "status:" << static_cast<int>(status);
    }

    QApplication::restoreOverrideCursor();

    bool ok = true;

    if (!importedFile.isEmpty()) { // Something to load (tmp or native file) ?
        // The filter, if any, has been applied. It's all native format now.
        if (!loadNativeFormat(importedFile)) {
            ok = false;
            if (d->autoErrorHandlingEnabled) {
                showLoadingErrorDialog();
            }
        }
    }

    if (importedFile != localFilePath()) {
        // We opened a temporary file (result of an import filter)
        // Set document URL to empty - we don't want to save in /tmp !
        // But only if in readwrite mode (no saving problem otherwise)
        // --
        // But this isn't true at all.  If this is the result of an
        // import, then importedFile=temporary_file.kwd and
        // file/m_url=foreignformat.ext so m_url is correct!
        // So don't resetURL() or else the caption won't be set when
        // foreign files are opened (an annoying bug).
        // - Clarence
        //
#if 0
        if (isReadWrite())
            resetURL();
#endif

        // remove temp file - uncomment this to debug import filters
        if (!importedFile.isEmpty()) {
#ifndef NDEBUG
            if (!getenv("CALLIGRA_DEBUG_FILTERS"))
#endif
                QFile::remove(importedFile);
        }
    }

    if (ok) {
        setMimeTypeAfterLoading(typeName);
        emit sigLoadingFinished();
    }

    if (!d->suppressProgress && d->progressUpdater) {
        QPointer<KoUpdater> updater = d->progressUpdater->startSubtask(1, "clear undo stack");
        updater->setProgress(0);
        undoStack()->clear();
        updater->setProgress(100);

        clearFileProgressUpdater();
    } else {
        undoStack()->clear();
    }
    d->isLoading = false;

    return ok;
}

KoProgressUpdater *KisDocument::progressUpdater() const
{
    return d->progressUpdater;
}

void KisDocument::setProgressProxy(KoProgressProxy *progressProxy)
{
    d->progressProxy = progressProxy;
}

KoProgressProxy* KisDocument::progressProxy() const
{
    if (!d->progressProxy) {
        KisMainWindow *mainWindow = 0;
        if (KisPart::instance()->mainwindowCount() > 0) {
            mainWindow = KisPart::instance()->mainWindows()[0];
        }
        d->progressProxy = new DocumentProgressProxy(mainWindow);
    }
    return d->progressProxy;
}

// shared between openFile and koMainWindow's "create new empty document" code
void KisDocument::setMimeTypeAfterLoading(const QString& mimeType)
{
    d->mimeType = mimeType.toLatin1();

    d->outputMimeType = d->mimeType;

    const bool needConfirm = !isNativeFormat(d->mimeType);
    setConfirmNonNativeSave(false, needConfirm);
    setConfirmNonNativeSave(true, needConfirm);
}

// The caller must call store->close() if loadAndParse returns true.
bool KisDocument::oldLoadAndParse(KoStore *store, const QString& filename, KoXmlDocument& doc)
{
    //dbgUI <<"Trying to open" << filename;

    if (!store->open(filename)) {
        warnUI << "Entry " << filename << " not found!";
        d->lastErrorMessage = i18n("Could not find %1", filename);
        return false;
    }
    // Error variables for QDomDocument::setContent
    QString errorMsg;
    int errorLine, errorColumn;
    bool ok = doc.setContent(store->device(), &errorMsg, &errorLine, &errorColumn);
    store->close();
    if (!ok) {
        errUI << "Parsing error in " << filename << "! Aborting!" << endl
              << " In line: " << errorLine << ", column: " << errorColumn << endl
              << " Error message: " << errorMsg << endl;
        d->lastErrorMessage = i18n("Parsing error in %1 at line %2, column %3\nError message: %4"
                                   , filename  , errorLine, errorColumn ,
                                   QCoreApplication::translate("QXml", errorMsg.toUtf8(), 0,
                                                               QCoreApplication::UnicodeUTF8));
        return false;
    }
    dbgUI << "File" << filename << " loaded and parsed";
    return true;
}

bool KisDocument::loadNativeFormat(const QString & file_)
{
    QString file = file_;
    QFileInfo fileInfo(file);
    if (!fileInfo.exists()) { // check duplicated from openUrl, but this is useful for templates
        d->lastErrorMessage = i18n("The file %1 does not exist.", file);
        return false;
    }
    if (!fileInfo.isFile()) {
        file += "/content.xml";
        QFileInfo fileInfo2(file);
        if (!fileInfo2.exists() || !fileInfo2.isFile()) {
            d->lastErrorMessage = i18n("%1 is not a file." , file_);
            return false;
        }
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    dbgUI << file;

    QFile in;
    bool isRawXML = false;
    if (d->specialOutputFlag != SaveAsDirectoryStore) { // Don't try to open a directory ;)
        in.setFileName(file);
        if (!in.open(QIODevice::ReadOnly)) {
            QApplication::restoreOverrideCursor();
            d->lastErrorMessage = i18n("Could not open the file for reading (check read permissions).");
            return false;
        }

        char buf[6];
        buf[5] = 0;
        int pos = 0;
        do {
            if (in.read(buf + pos , 1) < 1) {
                QApplication::restoreOverrideCursor();
                in.close();
                d->lastErrorMessage = i18n("Could not read the beginning of the file.");
                return false;
            }

            if (QChar(buf[pos]).isSpace())
                continue;
            pos++;
        } while (pos < 5);
        isRawXML = (qstrnicmp(buf, "<?xml", 5) == 0);
        if (! isRawXML)
            // also check for broken MathML files, which seem to be rather common
            isRawXML = (qstrnicmp(buf, "<math", 5) == 0);   // file begins with <math ?
        //dbgUI <<"PATTERN=" << buf;
    }
    // Is it plain XML?
    if (isRawXML) {
        in.seek(0);
        QString errorMsg;
        int errorLine;
        int errorColumn;
        KoXmlDocument doc = KoXmlDocument(true);
        bool res;
        if (doc.setContent(&in, &errorMsg, &errorLine, &errorColumn)) {
            res = loadXML(doc, 0);
            if (res)
                res = completeLoading(0);
        } else {
            errUI << "Parsing Error! Aborting! (in KisDocument::loadNativeFormat (QFile))" << endl
                  << "  Line: " << errorLine << " Column: " << errorColumn << endl
                  << "  Message: " << errorMsg << endl;
            d->lastErrorMessage = i18n("parsing error in the main document at line %1, column %2\nError message: %3", errorLine, errorColumn, i18n(errorMsg.toUtf8()));
            res = false;
        }

        QApplication::restoreOverrideCursor();
        in.close();
        d->isEmpty = false;
        return res;
    }
    else { // It's a calligra store (tar.gz, zip, directory, etc.)
        in.close();

        KoStore::Backend backend = (d->specialOutputFlag == SaveAsDirectoryStore) ? KoStore::Directory : KoStore::Auto;
        KoStore *store = KoStore::createStore(file, KoStore::Read, "", backend);

        if (store->bad()) {
            d->lastErrorMessage = i18n("Not a valid Krita file: %1", file);
            delete store;
            QApplication::restoreOverrideCursor();
            return false;
        }

        // Remember that the file was encrypted
        if (d->specialOutputFlag == 0 && store->isEncrypted() && !d->isImporting)
            d->specialOutputFlag = SaveEncrypted;

        const bool success = loadNativeFormatFromStoreInternal(store);

        // Retrieve the password after loading the file, only then is it guaranteed to exist
        if (success && store->isEncrypted() && !d->isImporting)
            d->password = store->password();

        delete store;

        return success;

    }
}

bool KisDocument::loadNativeFormatFromByteArray(QByteArray &data)
{
    bool succes;
    KoStore::Backend backend = (d->specialOutputFlag == SaveAsDirectoryStore) ? KoStore::Directory : KoStore::Auto;
    QBuffer buffer(&data);
    KoStore *store = KoStore::createStore(&buffer, KoStore::Read, "", backend);

    if (store->bad()) {
        delete store;
        return false;
    }

    // Remember that the file was encrypted
    if (d->specialOutputFlag == 0 && store->isEncrypted() && !d->isImporting)
        d->specialOutputFlag = SaveEncrypted;

    succes = loadNativeFormatFromStoreInternal(store);

    // Retrieve the password after loading the file, only then is it guaranteed to exist
    if (succes && store->isEncrypted() && !d->isImporting)
        d->password = store->password();

    delete store;

    return succes;
}

bool KisDocument::loadNativeFormatFromStoreInternal(KoStore *store)
{
    if (store->hasFile("root") || store->hasFile("maindoc.xml")) {   // Fallback to "old" file format (maindoc.xml)
        KoXmlDocument doc = KoXmlDocument(true);

        bool ok = oldLoadAndParse(store, "root", doc);
        if (ok)
            ok = loadXML(doc, store);
        if (!ok) {
            QApplication::restoreOverrideCursor();
            return false;
        }

    } else {
        errUI << "ERROR: No maindoc.xml" << endl;
        d->lastErrorMessage = i18n("Invalid document: no file 'maindoc.xml'.");
        QApplication::restoreOverrideCursor();
        return false;
    }

    if (store->hasFile("documentinfo.xml")) {
        KoXmlDocument doc = KoXmlDocument(true);
        if (oldLoadAndParse(store, "documentinfo.xml", doc)) {
            d->docInfo->load(doc);
        }
    } else {
        //dbgUI <<"cannot open document info";
        delete d->docInfo;
        d->docInfo = new KoDocumentInfo(this);
    }

    bool res = completeLoading(store);
    QApplication::restoreOverrideCursor();
    d->isEmpty = false;
    return res;
}

// For embedded documents
bool KisDocument::loadFromStore(KoStore *_store, const QString& url)
{
    if (_store->open(url)) {
        KoXmlDocument doc = KoXmlDocument(true);
        doc.setContent(_store->device());
        if (!loadXML(doc, _store)) {
            _store->close();
            return false;
        }
        _store->close();
    } else {
        dbgKrita << "couldn't open " << url;
    }

    _store->pushDirectory();
    // Store as document URL
    if (url.startsWith(STORE_PROTOCOL)) {
        setUrl(QUrl::fromUserInput(url));
    } else {
        setUrl(QUrl(INTERNAL_PREFIX + url));
        _store->enterDirectory(url);
    }

    bool result = completeLoading(_store);

    // Restore the "old" path
    _store->popDirectory();

    return result;
}

bool KisDocument::loadOdf(KoOdfReadStore & odfStore)
{
    Q_UNUSED(odfStore);
    setErrorMessage(i18n("Krita does not support the OpenDocument file format."));
    return false;
}


bool KisDocument::saveOdf(SavingContext &documentContext)
{
    Q_UNUSED(documentContext);
    setErrorMessage(i18n("Krita does not support the OpenDocument file format."));
    return false;
}



bool KisDocument::isStoredExtern() const
{
    return !storeInternal() && hasExternURL();
}


void KisDocument::setModified()
{
    d->modified = true;
}

void KisDocument::setModified(bool mod)
{
    if (mod) {
        updateEditingTime(false);
    }

    if (d->isAutosaving)   // ignore setModified calls due to autosaving
        return;

    if ( !d->readwrite && d->modified ) {
        errKrita << "Can't set a read-only document to 'modified' !" << endl;
        return;
    }

    //dbgUI<<" url:" << url.path();
    //dbgUI<<" mod="<<mod<<" MParts mod="<<KisParts::ReadWritePart::isModified()<<" isModified="<<isModified();

    if (mod && !d->modifiedAfterAutosave) {
        // First change since last autosave -> start the autosave timer
        setAutoSave(d->autoSaveDelay);
    }
    d->modifiedAfterAutosave = mod;

    if (mod == isModified())
        return;

    d->modified = mod;

    if (mod) {
        d->isEmpty = false;
        documentInfo()->updateParameters();
    }

    // This influences the title
    setTitleModified();
    emit modified(mod);
}

void KisDocument::updateEditingTime(bool forceStoreElapsed)
{
    QDateTime now = QDateTime::currentDateTime();
    int firstModDelta = d->firstMod.secsTo(now);
    int lastModDelta = d->lastMod.secsTo(now);

    if (lastModDelta > 30) {
        d->docInfo->setAboutInfo("editing-time", QString::number(d->docInfo->aboutInfo("editing-time").toInt() + d->firstMod.secsTo(d->lastMod)));
        d->firstMod = now;
    } else if (firstModDelta > 60 || forceStoreElapsed) {
        d->docInfo->setAboutInfo("editing-time", QString::number(d->docInfo->aboutInfo("editing-time").toInt() + firstModDelta));
        d->firstMod = now;
    }

    d->lastMod = now;
}

QString KisDocument::prettyPathOrUrl() const
{
    QString _url(url().toDisplayString());
#ifdef Q_OS_WIN
    if (url().isLocalFile()) {
        _url = QDir::toNativeSeparators(_url);
    }
#endif
    return _url;
}

// Get caption from document info (title(), in about page)
QString KisDocument::caption() const
{
    QString c;
    if (documentInfo()) {
        c = documentInfo()->aboutInfo("title");
    }
    const QString _url(url().fileName());
    if (!c.isEmpty() && !_url.isEmpty()) {
        c = QString("%1 - %2").arg(c).arg(_url);
    }
    else if (c.isEmpty()) {
        c = _url; // Fall back to document URL
    }
    return c;
}

void KisDocument::setTitleModified()
{
    emit titleModified(caption(), isModified());
}

bool KisDocument::completeLoading(KoStore* store)
{
    if (!d->image) {
        if (d->kraLoader->errorMessages().isEmpty()) {
            setErrorMessage(i18n("Unknown error."));
        }
        else {
            setErrorMessage(d->kraLoader->errorMessages().join(".\n"));
        }
        return false;
    }

    d->image->blockUpdates();
    d->kraLoader->loadBinaryData(store, d->image, url().url(), isStoredExtern());
    d->image->unblockUpdates();
    bool retval = true;
    if (!d->kraLoader->errorMessages().isEmpty()) {
        setErrorMessage(d->kraLoader->errorMessages().join(".\n"));
        retval = false;
    }
    if (retval) {
        vKisNodeSP preselectedNodes = d->kraLoader->selectedNodes();
        if (preselectedNodes.size() > 0) {
            d->preActivatedNode = preselectedNodes.first();
        }

        // before deleting the kraloader, get the list with preloaded assistants and save it
        d->assistants = d->kraLoader->assistants();
        d->shapeController->setImage(d->image);

        connect(d->image.data(), SIGNAL(sigImageModified()), this, SLOT(setImageModified()));

        if (d->image) {
            d->image->initialRefreshGraph();
        }
        setAutoSave(KisConfig().autoSaveInterval());

        emit sigLoadingFinished();
    }

    delete d->kraLoader;
    d->kraLoader = 0;

    return retval;
}

bool KisDocument::completeSaving(KoStore* store)
{
    d->kraSaver->saveKeyframes(store, url().url(), isStoredExtern());
    d->kraSaver->saveBinaryData(store, d->savingImage, url().url(), isStoredExtern(), d->isAutosaving);
    bool retval = true;
    if (!d->kraSaver->errorMessages().isEmpty()) {
        setErrorMessage(d->kraSaver->errorMessages().join(".\n"));
        retval = false;
    }

    delete d->kraSaver;
    d->kraSaver = 0;

    emit sigSavingFinished();

    return retval;
}

QDomDocument KisDocument::createDomDocument(const QString& tagName, const QString& version) const
{
    return createDomDocument("krita", tagName, version);
}

//static
QDomDocument KisDocument::createDomDocument(const QString& appName, const QString& tagName, const QString& version)
{
    QDomImplementation impl;
    QString url = QString("http://www.calligra.org/DTD/%1-%2.dtd").arg(appName).arg(version);
    QDomDocumentType dtype = impl.createDocumentType(tagName,
                                                     QString("-//KDE//DTD %1 %2//EN").arg(appName).arg(version),
                                                     url);
    // The namespace URN doesn't need to include the version number.
    QString namespaceURN = QString("http://www.calligra.org/DTD/%1").arg(appName);
    QDomDocument doc = impl.createDocument(namespaceURN, tagName, dtype);
    doc.insertBefore(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""), doc.documentElement());
    return doc;
}

bool KisDocument::loadXML(const KoXmlDocument& doc, KoStore *store)
{
    Q_UNUSED(store);
    if (d->image) {
        d->shapeController->setImage(0);
        d->image = 0;
    }

    KoXmlElement root;
    KoXmlNode node;
    KisImageSP image;

    if (doc.doctype().name() != "DOC") {
        setErrorMessage(i18n("The format is not supported or the file is corrupted"));
        return false;
    }
    root = doc.documentElement();
    int syntaxVersion = root.attribute("syntaxVersion", "3").toInt();
    if (syntaxVersion > 2) {
        setErrorMessage(i18n("The file is too new for this version of Krita (%1).", syntaxVersion));
        return false;
    }

    if (!root.hasChildNodes()) {
        setErrorMessage(i18n("The file has no layers."));
        return false;
    }

    if (d->kraLoader) delete d->kraLoader;
    d->kraLoader = new KisKraLoader(this, syntaxVersion);

    // Legacy from the multi-image .kra file period.
    for (node = root.firstChild(); !node.isNull(); node = node.nextSibling()) {
        if (node.isElement()) {
            if (node.nodeName() == "IMAGE") {
                KoXmlElement elem = node.toElement();
                if (!(image = d->kraLoader->loadXML(elem))) {
                    if (d->kraLoader->errorMessages().isEmpty()) {
                        setErrorMessage(i18n("Unknown error."));
                    }
                    else {
                        setErrorMessage(d->kraLoader->errorMessages().join(".\n"));
                    }
                    return false;
                }

            }
            else {
                if (d->kraLoader->errorMessages().isEmpty()) {
                    setErrorMessage(i18n("The file does not contain an image."));
                }
                return false;
            }
        }
    }

    if (d->image) {
        // Disconnect existing sig/slot connections
        d->image->disconnect(this);
    }
    d->setImageAndInitIdleWatcher(image);

    return true;
}



QDomDocument KisDocument::saveXML()
{
    dbgFile << url();
    QDomDocument doc = createDomDocument("DOC", CURRENT_DTD_VERSION);
    QDomElement root = doc.documentElement();

    root.setAttribute("editor", "Krita");
    root.setAttribute("syntaxVersion", "2");

    if (d->kraSaver) delete d->kraSaver;
    d->kraSaver = new KisKraSaver(this);

    root.appendChild(d->kraSaver->saveXML(doc, d->savingImage));
    if (!d->kraSaver->errorMessages().isEmpty()) {
        setErrorMessage(d->kraSaver->errorMessages().join(".\n"));
    }

    return doc;
}

bool KisDocument::isNativeFormat(const QByteArray& mimetype) const
{
    if (mimetype == nativeFormatMimeType())
        return true;
    return extraNativeMimeTypes().contains(mimetype);
}

int KisDocument::supportedSpecialFormats() const
{
    return 0; // we don't support encryption.
}

void KisDocument::setErrorMessage(const QString& errMsg)
{
    d->lastErrorMessage = errMsg;
}

QString KisDocument::errorMessage() const
{
    return d->lastErrorMessage;
}

void KisDocument::showLoadingErrorDialog()
{
    if (errorMessage().isEmpty()) {
        QMessageBox::critical(0, i18nc("@title:window", "Krita"), i18n("Could not open\n%1", localFilePath()));
    }
    else {
        QMessageBox::critical(0, i18nc("@title:window", "Krita"), i18n("Could not open %1\nReason: %2", localFilePath(), errorMessage()));
    }
}

bool KisDocument::isLoading() const
{
    return d->isLoading;
}

void KisDocument::removeAutoSaveFiles()
{
    // Eliminate any auto-save file
    QString asf = autoSaveFile(localFilePath());   // the one in the current dir
    if (QFile::exists(asf))
        QFile::remove(asf);
    asf = autoSaveFile(QString());   // and the one in $HOME
    if (QFile::exists(asf))
        QFile::remove(asf);
}

void KisDocument::setBackupFile(bool _b)
{
    d->backupFile = _b;
}

bool KisDocument::backupFile()const
{
    return d->backupFile;
}


void KisDocument::setBackupPath(const QString & _path)
{
    d->backupPath = _path;
}

QString KisDocument::backupPath()const
{
    return d->backupPath;
}


bool KisDocument::storeInternal() const
{
    return d->storeInternal;
}

void KisDocument::setStoreInternal(bool i)
{
    d->storeInternal = i;
    //dbgUI<<"="<<d->storeInternal<<" doc:"<<url().url();
}

bool KisDocument::hasExternURL() const
{
    return    !url().scheme().isEmpty()
            && url().scheme() != STORE_PROTOCOL
            && url().scheme() != INTERNAL_PROTOCOL;
}

static const struct {
    const char *localName;
    const char *documentType;
} TN2DTArray[] = {
{ "text", I18N_NOOP("a word processing") },
{ "spreadsheet", I18N_NOOP("a spreadsheet") },
{ "presentation", I18N_NOOP("a presentation") },
{ "chart", I18N_NOOP("a chart") },
{ "drawing", I18N_NOOP("a drawing") }
};
static const unsigned int numTN2DT = sizeof(TN2DTArray) / sizeof(*TN2DTArray);

QString KisDocument::tagNameToDocumentType(const QString& localName)
{
    for (unsigned int i = 0 ; i < numTN2DT ; ++i)
        if (localName == TN2DTArray[i].localName)
            return i18n(TN2DTArray[i].documentType);
    return localName;
}

KoPageLayout KisDocument::pageLayout(int /*pageNumber*/) const
{
    return d->pageLayout;
}

void KisDocument::setPageLayout(const KoPageLayout &pageLayout)
{
    d->pageLayout = pageLayout;
}

KoUnit KisDocument::unit() const
{
    return d->unit;
}

void KisDocument::setUnit(const KoUnit &unit)
{
    if (d->unit != unit) {
        d->unit = unit;
        emit unitChanged(unit);
    }
}

KUndo2Stack *KisDocument::undoStack()
{
    return d->undoStack;
}

KisImportExportManager *KisDocument::importExportManager() const
{
    return d->importExportManager;
}

void KisDocument::addCommand(KUndo2Command *command)
{
    if (command)
        d->undoStack->push(command);
}

void KisDocument::beginMacro(const KUndo2MagicString & text)
{
    d->undoStack->beginMacro(text);
}

void KisDocument::endMacro()
{
    d->undoStack->endMacro();
}

void KisDocument::slotUndoStackIndexChanged(int idx)
{
    // even if the document was already modified, call setModified to re-start autosave timer
    setModified(idx != d->undoStack->cleanIndex());
}

void KisDocument::clearUndoHistory()
{
    d->undoStack->clear();
}

KisGridConfig KisDocument::gridConfig() const
{
    return d->gridConfig;
}

void KisDocument::setGridConfig(const KisGridConfig &config)
{
    d->gridConfig = config;
}

const KisGuidesConfig& KisDocument::guidesConfig() const
{
    return d->guidesConfig;
}

void KisDocument::setGuidesConfig(const KisGuidesConfig &data)
{
    if (d->guidesConfig == data) return;

    d->guidesConfig = data;
    emit sigGuidesConfigChanged(d->guidesConfig);
}

bool KisDocument::isEmpty() const
{
    return d->isEmpty;
}

void KisDocument::setEmpty()
{
    d->isEmpty = true;
}


// static
int KisDocument::defaultAutoSave()
{
    return 300;
}

void KisDocument::resetURL() {
    setUrl(QUrl());
    setLocalFilePath(QString());
}

int KisDocument::pageCount() const {
    return 1;
}

KoDocumentInfoDlg *KisDocument::createDocumentInfoDialog(QWidget *parent, KoDocumentInfo *docInfo) const
{
    return new KoDocumentInfoDlg(parent, docInfo);
}

bool KisDocument::isReadWrite() const
{
    return d->readwrite;
}

QUrl KisDocument::url() const
{
    return d->m_url;
}

bool KisDocument::closeUrl(bool promptToSave)
{
    if (promptToSave) {
        if ( d->document->isReadWrite() && d->document->isModified()) {
            Q_FOREACH (KisView *view, KisPart::instance()->views()) {
                if (view && view->document() == this) {
                    if (!view->queryClose()) {
                        return false;
                    }
                }
            }
        }
    }
    // Not modified => ok and delete temp file.
    d->mimeType = QByteArray();

    if ( d->m_bTemp )
    {
        QFile::remove( d->m_file );
        d->m_bTemp = false;
    }
    // It always succeeds for a read-only part,
    // but the return value exists for reimplementations
    // (e.g. pressing cancel for a modified read-write part)
    return true;
}


bool KisDocument::saveAs(const QUrl &kurl, KisPropertiesConfigurationSP exportConfiguration)
{
    if (!kurl.isValid()) {
        errKrita << "saveAs: Malformed URL " << kurl.url() << endl;
        return false;
    }
    d->m_duringSaveAs = true;
    d->m_originalURL = d->m_url;
    d->m_originalFilePath = d->m_file;
    d->m_url = kurl; // Store where to upload in saveToURL
    d->prepareSaving();
    bool result = save(exportConfiguration); // Save local file and upload local file
    if (!result) {
        d->m_url = d->m_originalURL;
        d->m_file = d->m_originalFilePath;
        d->m_duringSaveAs = false;
        d->m_originalURL = QUrl();
        d->m_originalFilePath.clear();
    }

    return result;
}

bool KisDocument::save(KisPropertiesConfigurationSP exportConfiguration)
{
    d->m_saveOk = false;
    if ( d->m_file.isEmpty() ) { // document was created empty
        d->prepareSaving();
    }

    updateEditingTime(true);

    d->document->setFileProgressProxy();
    d->document->setUrl(url());

    bool ok = d->document->saveFile(exportConfiguration);

    d->document->clearFileProgressProxy();

    if (ok) {
        return saveToUrl();
    }
    else {
        emit canceled(QString());
    }
    return false;
}


bool KisDocument::waitSaveComplete()
{
    return d->m_saveOk;
}


void KisDocument::setUrl(const QUrl &url)
{
    d->m_url = url;
}

QString KisDocument::localFilePath() const
{
    return d->m_file;
}


void KisDocument::setLocalFilePath( const QString &localFilePath )
{
    d->m_file = localFilePath;
}

bool KisDocument::saveToUrl()
{
    if ( d->m_url.isLocalFile() ) {
        d->document->setModified( false );
        emit completed();
        // if m_url is a local file there won't be a temp file -> nothing to remove
        Q_ASSERT( !d->m_bTemp );
        d->m_saveOk = true;
        d->m_duringSaveAs = false;
        d->m_originalURL = QUrl();
        d->m_originalFilePath.clear();
        return true; // Nothing to do
    }
    return false;
}


bool KisDocument::openUrlInternal(const QUrl &url)
{
    if ( !url.isValid() )
        return false;

    if (d->m_bAutoDetectedMime) {
        d->mimeType = QByteArray();
        d->m_bAutoDetectedMime = false;
    }

    QByteArray mimetype = d->mimeType;

    if ( !closeUrl() )
        return false;

    d->mimeType = mimetype;
    setUrl(url);

    d->m_file.clear();

    if (d->m_url.isLocalFile()) {
        d->m_file = d->m_url.toLocalFile();
        return d->openLocalFile();
    }

    return false;
}

KisImageWSP KisDocument::newImage(const QString& name, qint32 width, qint32 height, const KoColorSpace* colorspace)
{
    KoColor backgroundColor(Qt::white, colorspace);

    /**
     * FIXME: check whether this is a good value
     */
    double defaultResolution=1.;

    newImage(name, width, height, colorspace, backgroundColor, "",
             defaultResolution);
    return image();
}

bool KisDocument::newImage(const QString& name, qint32 width, qint32 height, const KoColorSpace * cs, const KoColor &bgColor, const QString &imageDescription, const double imageResolution) {
    return newImage(name, width, height, cs, bgColor, false, 1, imageDescription, imageResolution);
}

bool KisDocument::newImage(const QString& name,
                           qint32 width, qint32 height,
                           const KoColorSpace* cs,
                           const KoColor &bgColor, bool backgroundAsLayer,
                           int numberOfLayers,
                           const QString &description, const double imageResolution)
{
    Q_ASSERT(cs);

    KisConfig cfg;

    KisImageSP image;
    KisPaintLayerSP layer;

    if (!cs) return false;

    QApplication::setOverrideCursor(Qt::BusyCursor);

    image = new KisImage(createUndoStore(), width, height, cs, name);

    Q_CHECK_PTR(image);

    connect(image.data(), SIGNAL(sigImageModified()), this, SLOT(setImageModified()));
    image->setResolution(imageResolution, imageResolution);

    image->assignImageProfile(cs->profile());
    documentInfo()->setAboutInfo("title", name);
    if (name != i18n("Unnamed") && !name.isEmpty()) {
        setUrl(QUrl::fromLocalFile(QDesktopServices::storageLocation(QDesktopServices::PicturesLocation) + '/' + name + ".kra"));
    }
    documentInfo()->setAboutInfo("abstract", description);

    layer = new KisPaintLayer(image.data(), image->nextLayerName(), OPACITY_OPAQUE_U8, cs);
    Q_CHECK_PTR(layer);

    if (backgroundAsLayer) {
        image->setDefaultProjectionColor(KoColor(cs));

        if (bgColor.opacityU8() == OPACITY_OPAQUE_U8) {
            layer->paintDevice()->setDefaultPixel(bgColor);
        } else {
            // Hack: with a semi-transparent background color, the projection isn't composited right if we just set the default pixel
            KisFillPainter painter;
            painter.begin(layer->paintDevice());
            painter.fillRect(0, 0, width, height, bgColor, bgColor.opacityU8());
        }
    } else {
        image->setDefaultProjectionColor(bgColor);
    }
    layer->setDirty(QRect(0, 0, width, height));

    image->addNode(layer.data(), image->rootLayer().data());
    setCurrentImage(image);

    for(int i = 1; i < numberOfLayers; ++i) {
        KisPaintLayerSP layer = new KisPaintLayer(image, image->nextLayerName(), OPACITY_OPAQUE_U8, cs);
        image->addNode(layer, image->root(), i);
        layer->setDirty(QRect(0, 0, width, height));
    }

    cfg.defImageWidth(width);
    cfg.defImageHeight(height);
    cfg.defImageResolution(imageResolution);
    cfg.defColorModel(image->colorSpace()->colorModelId().id());
    cfg.setDefaultColorDepth(image->colorSpace()->colorDepthId().id());
    cfg.defColorProfile(image->colorSpace()->profile()->name());

    QApplication::restoreOverrideCursor();

    return true;
}

KoShapeBasedDocumentBase *KisDocument::shapeController() const
{
    return d->shapeController;
}

KoShapeLayer* KisDocument::shapeForNode(KisNodeSP layer) const
{
    return d->shapeController->shapeForNode(layer);
}

vKisNodeSP KisDocument::activeNodes() const
{
    vKisNodeSP nodes;
    Q_FOREACH (KisView *v, KisPart::instance()->views()) {
        if (v->document() == this && v->viewManager()) {
            KisNodeSP activeNode = v->viewManager()->activeNode();
            if (activeNode && !nodes.contains(activeNode)) {
                if (activeNode->inherits("KisMask")) {
                    activeNode = activeNode->parent();
                }
                nodes.append(activeNode);
            }
        }
    }
    return nodes;
}

QList<KisPaintingAssistantSP> KisDocument::assistants() const
{
    return d->assistants;
}

void KisDocument::setAssistants(const QList<KisPaintingAssistantSP> value)
{
    d->assistants = value;
}

void KisDocument::setPreActivatedNode(KisNodeSP activatedNode)
{
    d->preActivatedNode = activatedNode;
}

KisNodeSP KisDocument::preActivatedNode() const
{
    return d->preActivatedNode;
}

void KisDocument::prepareForImport()
{
    /* TODO: remove this function? I kept it because it might be useful for
     * other kind of preparing, but currently it was checking on d->nserver
     * being null and then calling init() if it was, but the document is always
     * initialized in the constructor (and init() does other things too).
     * Moreover, nserver cannot be nulled by some external call.*/
}

void KisDocument::setFileProgressUpdater(const QString &text)
{
    d->suppressProgress = d->importExportManager->getBatchMode();

    if (!d->suppressProgress) {
        d->progressUpdater = new KoProgressUpdater(d->progressProxy, KoProgressUpdater::Unthreaded);
        d->progressUpdater->start(100, text);
        d->importExportManager->setProgresUpdater(d->progressUpdater);

        connect(this, SIGNAL(sigProgress(int)), KisPart::instance()->currentMainwindow(), SLOT(slotProgress(int)));
        connect(KisPart::instance()->currentMainwindow(), SIGNAL(sigProgressCanceled()), this, SIGNAL(sigProgressCanceled()));
    }
}

void KisDocument::clearFileProgressUpdater()
{
    if (!d->suppressProgress && d->progressUpdater) {
        disconnect(KisPart::instance()->currentMainwindow(), SIGNAL(sigProgressCanceled()), this, SIGNAL(sigProgressCanceled()));
        disconnect(this, SIGNAL(sigProgress(int)), KisPart::instance()->currentMainwindow(), SLOT(slotProgress(int)));
        delete d->progressUpdater;
        d->importExportManager->setProgresUpdater(0);
        d->progressUpdater = 0;
    }
}

void KisDocument::setFileProgressProxy()
{
    if (!d->progressProxy && !d->importExportManager->getBatchMode()) {
        d->fileProgressProxy = progressProxy();
    } else {
        d->fileProgressProxy = 0;
    }
}

void KisDocument::clearFileProgressProxy()
{
    if (d->fileProgressProxy) {
        setProgressProxy(0);
        delete d->fileProgressProxy;
        d->fileProgressProxy = 0;
    }
}

KisImageWSP KisDocument::image() const
{
    return d->image;
}

KisImageSP KisDocument::savingImage() const
{
    return d->savingImage;
}


void KisDocument::setCurrentImage(KisImageSP image)
{
    if (!image) return;

    if (d->image) {
        // Disconnect existing sig/slot connections
        d->image->disconnect(this);
        d->shapeController->setImage(0);
    }
    d->setImageAndInitIdleWatcher(image);
    d->shapeController->setImage(image);
    setModified(false);
    connect(d->image, SIGNAL(sigImageModified()), this, SLOT(setImageModified()));
    d->image->initialRefreshGraph();
    setAutoSave(KisConfig().autoSaveInterval());
}

void KisDocument::initEmpty()
{
    KisConfig cfg;
    const KoColorSpace * rgb = KoColorSpaceRegistry::instance()->rgb8();
    newImage("", cfg.defImageWidth(), cfg.defImageHeight(), rgb);
}

void KisDocument::setImageModified()
{
    setModified(true);
}


KisUndoStore* KisDocument::createUndoStore()
{
    return new KisDocumentUndoStore(this);
}

bool KisDocument::isAutosaving() const
{
    return d->isAutosaving;
}
