/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C) 2009 Thomas Zander <zander@kde.org>
   Copyright (C) 2012 Boudewijn Rempt <boud@valdyas.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "KisApplication.h"

#include <stdlib.h>
#ifdef Q_OS_WIN
#include <windows.h>
#include <tchar.h>
#endif

#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QMessageBox>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QStyle>
#include <QStyleFactory>
#include <QSysInfo>
#include <QTimer>
#include <QWidget>

#include <klocalizedstring.h>
#include <kdesktopfile.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <KoColorSpaceRegistry.h>
#include <KoPluginLoader.h>
#include <KoShapeRegistry.h>
#include <KoDpi.h>
#include "KoGlobal.h"
#include "KoConfig.h"
#include <resources/KoHashGeneratorProvider.h>
#include <KoResourcePaths.h>
#include <KisMimeDatabase.h>
#include "thememanager.h"
#include "KisPrintJob.h"
#include "KisDocument.h"
#include "KisMainWindow.h"
#include "KisAutoSaveRecoveryDialog.h"
#include "KisPart.h"
#include <kis_icon.h>
#include "kis_md5_generator.h"
#include "kis_splash_screen.h"
#include "kis_config.h"
#include "flake/kis_shape_selection.h"
#include <filter/kis_filter.h>
#include <filter/kis_filter_registry.h>
#include <filter/kis_filter_configuration.h>
#include <generator/kis_generator_registry.h>
#include <generator/kis_generator.h>
#include <brushengine/kis_paintop_registry.h>
#include <metadata/kis_meta_data_io_backend.h>
#include "kisexiv2/kis_exiv2.h"
#include "KisApplicationArguments.h"
#include <kis_debug.h>
#include "kis_action_registry.h"
#include <kis_brush_server.h>
#include <kis_resource_server_provider.h>
#include <KoResourceServerProvider.h>
#include "kis_image_barrier_locker.h"
#include "opengl/kis_opengl.h"

#include <KritaVersionWrapper.h>
namespace {
const QTime appStartTime(QTime::currentTime());
}

class KisApplicationPrivate
{
public:
    KisApplicationPrivate()
        : splashScreen(0)
    {}
    QPointer<KisSplashScreen> splashScreen;
};

class KisApplication::ResetStarting
{
public:
    ResetStarting(KisSplashScreen *splash, int fileCount)
        : m_splash(splash)
        , m_fileCount(fileCount)
    {
    }

    ~ResetStarting()  {
        if (m_splash) {

            KConfigGroup cfg( KSharedConfig::openConfig(), "SplashScreen");
            bool hideSplash = cfg.readEntry("HideSplashAfterStartup", false);
            if (m_fileCount > 0 || hideSplash) {
                m_splash->hide();
            }
            else {
                m_splash->setWindowFlags(Qt::Dialog);
                QRect r(QPoint(), m_splash->size());
                m_splash->move(QApplication::desktop()->availableGeometry().center() - r.center());
                m_splash->setWindowTitle(qAppName());
                m_splash->setParent(0);
                Q_FOREACH (QObject *o, m_splash->children()) {
                    QWidget *w = qobject_cast<QWidget*>(o);
                    if (w && w->isHidden()) {
                        w->setVisible(true);
                    }
                }
                m_splash->show();
                m_splash->activateWindow();
            }
        }
    }

    QPointer<KisSplashScreen> m_splash;
    int m_fileCount;
};



KisApplication::KisApplication(const QString &key, int &argc, char **argv)
    : QtSingleApplication(key, argc, argv)
    , d(new KisApplicationPrivate)
    , m_autosaveDialog(0)
    , m_mainWindow(0)
    , m_batchRun(false)
{
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());

    setApplicationDisplayName("Krita");
    setApplicationName("krita");
    // Note: Qt docs suggest we set this, but if we do, we get resource paths of the form of krita/krita, which is weird.
    //    setOrganizationName("krita");
    setOrganizationDomain("krita.org");

    QString version = KritaVersionWrapper::versionString(true);
    setApplicationVersion(version);
    setWindowIcon(KisIconUtils::loadIcon("calligrakrita"));

    if (qgetenv("KRITA_NO_STYLE_OVERRIDE").isEmpty()) {
        QStringList styles = QStringList() << "breeze" << "fusion" << "plastique";
        if (!styles.contains(style()->objectName().toLower())) {
            Q_FOREACH (const QString & style, styles) {
                if (!setStyle(style)) {
                    qDebug() << "No" << style << "available.";
                }
                else {
                    qDebug() << "Set style" << style;
                    break;
                }
            }
        }
    }
    else {
        qDebug() << "Style override disabled, using" << style()->objectName();
    }

    KisOpenGL::initialize();
    qDebug() << "krita has opengl" << KisOpenGL::hasOpenGL();
}

#if defined(Q_OS_WIN) && defined(ENV32BIT)
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

LPFN_ISWOW64PROCESS fnIsWow64Process;

BOOL isWow64()
{
    BOOL bIsWow64 = FALSE;

    //IsWow64Process is not available on all supported versions of Windows.
    //Use GetModuleHandle to get a handle to the DLL that contains the function
    //and GetProcAddress to get a pointer to the function if available.

    fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
                GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

    if(0 != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
        {
            //handle error
        }
    }
    return bIsWow64;
}
#endif

void initializeGlobals(const KisApplicationArguments &args)
{
    int dpiX = args.dpiX();
    int dpiY = args.dpiY();
    if (dpiX > 0 && dpiY > 0) {
        KoDpi::setDPI(dpiX, dpiY);
    }
}

void addResourceTypes()
{
    // All Krita's resource types
    KoResourcePaths::addResourceType("kis_pics", "data", "/pics/");
    KoResourcePaths::addResourceType("kis_images", "data", "/images/");
    KoResourcePaths::addResourceType("icc_profiles", "data", "/profiles/");
    KoResourcePaths::addResourceType("metadata_schema", "data", "/metadata/schemas/");
    KoResourcePaths::addResourceType("kis_brushes", "data", "/brushes/");
    KoResourcePaths::addResourceType("kis_taskset", "data", "/taskset/");
    KoResourcePaths::addResourceType("kis_taskset", "data", "/taskset/");
    KoResourcePaths::addResourceType("gmic_definitions", "data", "/gmic/");
    KoResourcePaths::addResourceType("kis_resourcebundles", "data", "/bundles/");
    KoResourcePaths::addResourceType("kis_defaultpresets", "data", "/defaultpresets/");
    KoResourcePaths::addResourceType("kis_paintoppresets", "data", "/paintoppresets/");
    KoResourcePaths::addResourceType("kis_workspaces", "data", "/workspaces/");
    KoResourcePaths::addResourceType("psd_layer_style_collections", "data", "/asl");
    KoResourcePaths::addResourceType("ko_patterns", "data", "/patterns/", true);
    KoResourcePaths::addResourceType("ko_gradients", "data", "/gradients/");
    KoResourcePaths::addResourceType("ko_gradients", "data", "/gradients/", true);
    KoResourcePaths::addResourceType("ko_palettes", "data", "/palettes/", true);
    KoResourcePaths::addResourceType("kis_shortcuts", "data", "/shortcuts/");
    KoResourcePaths::addResourceType("kis_actions", "data", "/actions");
    KoResourcePaths::addResourceType("icc_profiles", "data", "/color/icc");
    KoResourcePaths::addResourceType("ko_effects", "data", "/effects/");
    KoResourcePaths::addResourceType("tags", "data", "/tags/");
    KoResourcePaths::addResourceType("templates", "data", "/templates");

    //    // Extra directories to look for create resources. (Does anyone actually use that anymore?)
    //    KoResourcePaths::addResourceDir("ko_gradients", "/usr/share/create/gradients/gimp");
    //    KoResourcePaths::addResourceDir("ko_gradients", QDir::homePath() + QString("/.create/gradients/gimp"));
    //    KoResourcePaths::addResourceDir("ko_patterns", "/usr/share/create/patterns/gimp");
    //    KoResourcePaths::addResourceDir("ko_patterns", QDir::homePath() + QString("/.create/patterns/gimp"));
    //    KoResourcePaths::addResourceDir("kis_brushes", "/usr/share/create/brushes/gimp");
    //    KoResourcePaths::addResourceDir("kis_brushes", QDir::homePath() + QString("/.create/brushes/gimp"));
    //    KoResourcePaths::addResourceDir("ko_palettes", "/usr/share/create/swatches");
    //    KoResourcePaths::addResourceDir("ko_palettes", QDir::homePath() + QString("/.create/swatches"));

    // Make directories for all resources we can save, and tags
    QDir d;
    d.mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/tags/");
    d.mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/asl/");
    d.mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/bundles/");
    d.mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/gradients/");
    d.mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/paintoppresets/");
    d.mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/palettes/");
    d.mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/patterns/");
    d.mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/taskset/");
    d.mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/workspaces/");
    d.mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/input/");
}

void KisApplication::loadResources()
{
    setSplashScreenLoadingText(i18n("Loading Gradients..."));
    processEvents();
    KoResourceServerProvider::instance()->gradientServer(true);


    // Load base resources
    setSplashScreenLoadingText(i18n("Loading Patterns..."));
    processEvents();
    KoResourceServerProvider::instance()->patternServer(true);

    setSplashScreenLoadingText(i18n("Loading Palettes..."));
    processEvents();
    KoResourceServerProvider::instance()->paletteServer(false);

    setSplashScreenLoadingText(i18n("Loading Brushes..."));
    processEvents();
    KisBrushServer::instance()->brushServer(true);

    // load paintop presets
    setSplashScreenLoadingText(i18n("Loading Paint Operations..."));
    processEvents();
    KisResourceServerProvider::instance()->paintOpPresetServer(true);

    setSplashScreenLoadingText(i18n("Loading Resource Bundles..."));
    processEvents();
    KisResourceServerProvider::instance()->resourceBundleServer();
}

void KisApplication::loadPlugins()
{
    KoShapeRegistry* r = KoShapeRegistry::instance();
    r->add(new KisShapeSelectionFactory());

    KisActionRegistry::instance();
    KisFilterRegistry::instance();
    KisGeneratorRegistry::instance();
    KisPaintOpRegistry::instance();
    KoColorSpaceRegistry::instance();

    // Load the krita-specific tools
    setSplashScreenLoadingText(i18n("Loading Plugins for Krita/Tool..."));
    processEvents();
    KoPluginLoader::instance()->load(QString::fromLatin1("Krita/Tool"),
                                     QString::fromLatin1("[X-Krita-Version] == 28"));


    // Load dockers
    setSplashScreenLoadingText(i18n("Loading Plugins for Krita/Dock..."));
    processEvents();
    KoPluginLoader::instance()->load(QString::fromLatin1("Krita/Dock"),
                                     QString::fromLatin1("[X-Krita-Version] == 28"));

    // XXX_EXIV: make the exiv io backends real plugins
    setSplashScreenLoadingText(i18n("Loading Plugins Exiv/IO..."));
    processEvents();
    KisExiv2::initialize();
}


bool KisApplication::start(const KisApplicationArguments &args)
{
    KisConfig cfg;

#if defined(Q_OS_WIN)
#ifdef ENV32BIT

    if (isWow64() && !cfg.readEntry("WarnedAbout32Bits", false)) {
        QMessageBox::information(0,
                                 i18nc("@title:window", "Krita: Warning"),
                                 i18n("You are running a 32 bits build on a 64 bits Windows.\n"
                                      "This is not recommended.\n"
                                      "Please download and install the x64 build instead."));
        cfg.writeEntry("WarnedAbout32Bits", true);

    }
#endif
#endif

    QString opengl = cfg.canvasState();
    if (opengl == "OPENGL_NOT_TRIED" ) {
        cfg.setCanvasState("TRY_OPENGL");
    }
    else if (opengl != "OPENGL_SUCCESS") {
        cfg.setCanvasState("OPENGL_FAILED");
    }

    setSplashScreenLoadingText(i18n("Initializing Globals"));
    processEvents();
    initializeGlobals(args);

    const bool doTemplate = args.doTemplate();
    const bool print = args.print();
    const bool exportAs = args.exportAs();
    const bool exportAsPdf = args.exportAsPdf();
    const QString exportFileName = args.exportFileName();

    m_batchRun = (print || exportAs || exportAsPdf || !exportFileName.isEmpty());
    // print & exportAsPdf do user interaction ATM
    const bool needsMainWindow = !exportAs;
    // only show the mainWindow when no command-line mode option is passed
    // TODO: fix print & exportAsPdf to work without mainwindow shown
    const bool showmainWindow = !exportAs; // would be !batchRun;

    const bool showSplashScreen = !m_batchRun && qEnvironmentVariableIsEmpty("NOSPLASH");// &&  qgetenv("XDG_CURRENT_DESKTOP") != "GNOME";
    if (showSplashScreen && d->splashScreen) {
        d->splashScreen->show();
        d->splashScreen->repaint();
        processEvents();
    }

    KoHashGeneratorProvider::instance()->setGenerator("MD5", new KisMD5Generator());

    // Initialize all Krita directories etc.
    KoGlobal::initialize();

    KConfigGroup group(KSharedConfig::openConfig(), "theme");
    Digikam::ThemeManager themeManager;
    themeManager.setCurrentTheme(group.readEntry("Theme", "Krita dark"));


    ResetStarting resetStarting(d->splashScreen, args.filenames().count()); // remove the splash when done
    Q_UNUSED(resetStarting);

    // Make sure we can save resources and tags
    setSplashScreenLoadingText(i18n("Adding resource types"));
    processEvents();
    addResourceTypes();

    // Load all resources and tags before the plugins do that
    loadResources();

    // Load the plugins
    loadPlugins();

    if (needsMainWindow) {
        // show a mainWindow asap, if we want that
        setSplashScreenLoadingText(i18n("Loading Main Window..."));
        processEvents();
        m_mainWindow = KisPart::instance()->createMainWindow();

        if (showmainWindow) {
            m_mainWindow->initializeGeometry();
            m_mainWindow->show();
        }
    }
    short int numberOfOpenDocuments = 0; // number of documents open

    // Check for autosave files that can be restored, if we're not running a batchrun (test, print, export to pdf)
    if (!m_batchRun) {
        checkAutosaveFiles();
    }

    setSplashScreenLoadingText(QString()); // done loading, so clear out label
    processEvents();

    // Get the command line arguments which we have to parse
    int argsCount = args.filenames().count();
    if (argsCount > 0) {
        // Loop through arguments
        short int nPrinted = 0;
        for (int argNumber = 0; argNumber < argsCount; argNumber++) {
            QString fileName = args.filenames().at(argNumber);
            // are we just trying to open a template?
            if (doTemplate) {
                // called in mix with batch options? ignore and silently skip
                if (m_batchRun) {
                    continue;
                }
                if (createNewDocFromTemplate(fileName, m_mainWindow)) {
                    ++numberOfOpenDocuments;
                }
                // now try to load
            }
            else {

                if (exportAs) {
                    QString outputMimetype = KisMimeDatabase::mimeTypeForFile(exportFileName);
                    if (outputMimetype == "application/octetstream") {
                        dbgKrita << i18n("Mimetype not found, try using the -mimetype option") << endl;
                        return 1;
                    }

                    KisDocument *doc = KisPart::instance()->createDocument();
                    doc->setFileBatchMode(m_batchRun);
                    doc->openUrl(QUrl::fromLocalFile(fileName));

                    qApp->processEvents(); // For vector layers to be updated

                    doc->setFileBatchMode(true);
                    doc->setOutputMimeType(outputMimetype.toLatin1());
                    if (!doc->exportDocument(QUrl::fromLocalFile(exportFileName))) {
                        dbgKrita << "Could not export " << fileName << "to" << exportFileName << ":" << doc->errorMessage();
                    }
                    nPrinted++;
                    QTimer::singleShot(0, this, SLOT(quit()));
                }
                else if (m_mainWindow) {
                    KisDocument *doc = KisPart::instance()->createDocument();
                    doc->setFileBatchMode(m_batchRun);
                    if (m_mainWindow->openDocumentInternal(QUrl::fromLocalFile(fileName), doc)) {
                        if (print) {
                            m_mainWindow->slotFilePrint();
                            nPrinted++;
                            // TODO: trigger closing of app once printing is done
                        }
                        else if (exportAsPdf) {
                            KisPrintJob *job = m_mainWindow->exportToPdf(exportFileName);
                            if (job)
                                connect (job, SIGNAL(destroyed(QObject*)), m_mainWindow,
                                         SLOT(slotFileQuit()), Qt::QueuedConnection);
                            nPrinted++;
                        } else {
                            // Normal case, success
                            numberOfOpenDocuments++;
                        }
                    } else {
                        // .... if failed
                        // delete doc; done by openDocument
                    }
                }
            }
        }

        if (m_batchRun) {
            return nPrinted > 0;
        }
    }

    // fixes BUG:369308  - Krita crashing on splash screen when loading.
    // trying to open a file before Krita has loaded can cause it to hang and crash
    if (d->splashScreen) {
        d->splashScreen->displayLinks();
        d->splashScreen->displayRecentFiles();
    }


    // not calling this before since the program will quit there.
    return true;
}

KisApplication::~KisApplication()
{
    delete d;
}

void KisApplication::setSplashScreen(QWidget *splashScreen)
{
    d->splashScreen = qobject_cast<KisSplashScreen*>(splashScreen);
}

void KisApplication::setSplashScreenLoadingText(QString textToLoad)
{
    if (d->splashScreen) {
        d->splashScreen->loadingLabel->setText(textToLoad);
        d->splashScreen->repaint();
    }
}

void KisApplication::hideSplashScreen()
{
    if (d->splashScreen) {
        // hide the splashscreen to see the dialog
        d->splashScreen->hide();
    }
}

bool KisApplication::notify(QObject *receiver, QEvent *event)
{
    try {
        return QApplication::notify(receiver, event);
    } catch (std::exception &e) {
        qWarning("Error %s sending event %i to object %s",
                 e.what(), event->type(), qPrintable(receiver->objectName()));
    } catch (...) {
        qWarning("Error <unknown> sending event %i to object %s",
                 event->type(), qPrintable(receiver->objectName()));
    }
    return false;
}


void KisApplication::remoteArguments(QByteArray message, QObject *socket)
{
    Q_UNUSED(socket);

    // check if we have any mainwindow
    KisMainWindow *mw = qobject_cast<KisMainWindow*>(qApp->activeWindow());
    if (!mw) {
        mw = KisPart::instance()->mainWindows().first();
    }

    if (!mw) {
        return;
    }

    KisApplicationArguments args = KisApplicationArguments::deserialize(message);
    const bool doTemplate = args.doTemplate();
    const int argsCount = args.filenames().count();

    if (argsCount > 0) {
        // Loop through arguments
        for (int argNumber = 0; argNumber < argsCount; ++argNumber) {
            QString filename = args.filenames().at(argNumber);
            // are we just trying to open a template?
            if (doTemplate) {
                createNewDocFromTemplate(filename, mw);
            }
            else if (QFile(filename).exists()) {
                KisDocument *doc = KisPart::instance()->createDocument();
                doc->setFileBatchMode(m_batchRun);
                mw->openDocumentInternal(QUrl::fromLocalFile(filename), doc);
            }
        }
    }
}

void KisApplication::fileOpenRequested(const QString &url)
{
    KisMainWindow *mainWindow = KisPart::instance()->mainWindows().first();
    if (mainWindow) {
        KisDocument *doc = KisPart::instance()->createDocument();
        doc->setFileBatchMode(m_batchRun);
        mainWindow->openDocumentInternal(QUrl::fromLocalFile(url), doc);
    }
}


void KisApplication::checkAutosaveFiles()
{
    if (m_batchRun) return;

    // Check for autosave files from a previous run. There can be several, and
    // we want to offer a restore for every one. Including a nice thumbnail!

    QStringList filters;
    filters << QString(".krita-*-*-autosave.kra");

#ifdef Q_OS_WIN
    QDir dir = QDir::temp();
#else
    QDir dir = QDir::home();
#endif

    // all autosave files for our application
    m_autosaveFiles = dir.entryList(filters, QDir::Files | QDir::Hidden);

    // Allow the user to make their selection
    if (m_autosaveFiles.size() > 0) {
        if (d->splashScreen) {
            // hide the splashscreen to see the dialog
            d->splashScreen->hide();
        }
        m_autosaveDialog = new KisAutoSaveRecoveryDialog(m_autosaveFiles, activeWindow());
        QDialog::DialogCode result = (QDialog::DialogCode) m_autosaveDialog->exec();

        if (result == QDialog::Accepted) {
            QStringList filesToRecover = m_autosaveDialog->recoverableFiles();
            Q_FOREACH (const QString &autosaveFile, m_autosaveFiles) {
                if (!filesToRecover.contains(autosaveFile)) {
                    QFile::remove(dir.absolutePath() + "/" + autosaveFile);
                }
            }
            m_autosaveFiles = filesToRecover;
        } else {
            m_autosaveFiles.clear();
        }

        if (m_autosaveFiles.size() > 0) {
            QList<QUrl> autosaveUrls;
            Q_FOREACH (const QString &autoSaveFile, m_autosaveFiles) {
                const QUrl url = QUrl::fromLocalFile(dir.absolutePath() + QLatin1Char('/') + autoSaveFile);
                autosaveUrls << url;
            }
            if (m_mainWindow) {
                Q_FOREACH (const QUrl &url, autosaveUrls) {
                    KisDocument *doc = KisPart::instance()->createDocument();
                    doc->setFileBatchMode(m_batchRun);
                    m_mainWindow->openDocumentInternal(url, doc);
                }
            }
        }
        // cleanup
        delete m_autosaveDialog;
        m_autosaveDialog = nullptr;
    }
}

bool KisApplication::createNewDocFromTemplate(const QString &fileName, KisMainWindow *mainWindow)
{
    QString templatePath;

    const QUrl templateUrl = QUrl::fromLocalFile(fileName);
    if (QFile::exists(fileName)) {
        templatePath = templateUrl.toLocalFile();
        dbgUI << "using full path...";
    }
    else {
        QString desktopName(fileName);
        const QString templatesResourcePath =  QStringLiteral("templates/");

        QStringList paths = KoResourcePaths::findAllResources("data", templatesResourcePath + "*/" + desktopName);
        if (paths.isEmpty()) {
            paths = KoResourcePaths::findAllResources("data", templatesResourcePath + desktopName);
        }

        if (paths.isEmpty()) {
            QMessageBox::critical(0, i18nc("@title:window", "Krita"),
                                  i18n("No template found for: %1", desktopName));
        } else if (paths.count() > 1) {
            QMessageBox::critical(0, i18nc("@title:window", "Krita"),
                                  i18n("Too many templates found for: %1", desktopName));
        } else {
            templatePath = paths.at(0);
        }
    }

    if (!templatePath.isEmpty()) {
        QUrl templateBase;
        templateBase.setPath(templatePath);
        KDesktopFile templateInfo(templatePath);

        QString templateName = templateInfo.readUrl();
        QUrl templateURL;
        templateURL.setPath(templateBase.adjusted(QUrl::RemoveFilename|QUrl::StripTrailingSlash).path() + '/' + templateName);

        KisDocument *doc = KisPart::instance()->createDocument();
        doc->setFileBatchMode(m_batchRun);
        if (mainWindow->openDocumentInternal(templateURL, doc)) {
            doc->resetURL();
            doc->setEmpty();
            doc->setTitleModified();
            dbgUI << "Template loaded...";
            return true;
        }
        else {
            QMessageBox::critical(0, i18nc("@title:window", "Krita"),
                                  i18n("Template %1 failed to load.", templateURL.toDisplayString()));
        }
    }

    return false;
}

void KisApplication::clearConfig()
{
    KIS_ASSERT_RECOVER_RETURN(qApp->thread() == QThread::currentThread());

    KSharedConfigPtr config =  KSharedConfig::openConfig();

    // find user settings file
    bool createDir = false;
    QString kritarcPath = KoResourcePaths::locateLocal("config", "kritarc", createDir);

    QFile configFile(kritarcPath);
    if (configFile.exists()) {
        // clear file
        if (configFile.open(QFile::WriteOnly)) {
            configFile.close();
        }
        else {
            QMessageBox::warning(0,
                                 i18nc("@title:window", "Krita"),
                                 i18n("Failed to clear %1\n\n"
                                      "Please make sure no other program is using the file and try again.",
                                      kritarcPath),
                                 QMessageBox::Ok, QMessageBox::Ok);
        }
    }

    // reload from disk; with the user file settings cleared,
    // this should load any default configuration files shipping with the program
    config->reparseConfiguration();
    config->sync();
}

void KisApplication::askClearConfig()
{
    Qt::KeyboardModifiers mods = QApplication::queryKeyboardModifiers();
    bool askClearConfig = (mods & Qt::ControlModifier) && (mods & Qt::ShiftModifier) && (mods & Qt::AltModifier);

    if (askClearConfig) {
        bool ok = QMessageBox::question(0,
                                        i18nc("@title:window", "Krita"),
                                        i18n("Do you want to clear the settings file?"),
                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes;
        if (ok) {
            clearConfig();
        }
    }
}
