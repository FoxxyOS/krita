/*
 *  preferencesdlg.cc - part of KImageShop
 *
 *  Copyright (c) 1999 Michael Koch <koch@kde.org>
 *  Copyright (c) 2003-2011 Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_dlg_preferences.h"

#include <opengl/kis_opengl.h>

#include <QBitmap>
#include <QCheckBox>
#include <QCursor>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QToolButton>
#include <QThread>
#include <QDesktopServices>
#include <QGridLayout>
#include <QRadioButton>
#include <QGroupBox>
#include <QMdiArea>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QSettings>

#include <KisDocument.h>
#include <KoColorProfile.h>
#include <KisApplication.h>
#include <KoFileDialog.h>
#include <KisPart.h>
#include <KoColorSpaceEngine.h>
#include <kis_icon.h>
#include <KoConfig.h>
#include "KoID.h"
#include <KoConfigAuthorPage.h>
#include <KoVBox.h>

#include <klocalizedstring.h>
#include <kundo2stack.h>
#include <KoResourcePaths.h>
#include "kis_action_registry.h"

#include "widgets/squeezedcombobox.h"
#include "kis_clipboard.h"
#include "widgets/kis_cmb_idlist.h"
#include "KoColorSpace.h"
#include "KoColorSpaceRegistry.h"
#include "KoColorConversionTransformation.h"
#include "kis_cursor.h"
#include "kis_config.h"
#include "kis_canvas_resource_provider.h"
#include "kis_preference_set_registry.h"
#include "kis_color_manager.h"
#include "KisProofingConfiguration.h"
#include "kis_image_config.h"

#include "slider_and_spin_box_sync.h"

// for the performance update
#include <kis_cubic_curve.h>

#include "input/config/kis_input_configuration_page.h"


GeneralTab::GeneralTab(QWidget *_parent, const char *_name)
    : WdgGeneralSettings(_parent, _name)
{
    KisConfig cfg;

    m_cmbCursorShape->addItem(i18n("No Cursor"));
    m_cmbCursorShape->addItem(i18n("Tool Icon"));
    m_cmbCursorShape->addItem(i18n("Arrow"));
    m_cmbCursorShape->addItem(i18n("Small Circle"));
    m_cmbCursorShape->addItem(i18n("Crosshair"));
    m_cmbCursorShape->addItem(i18n("Triangle Righthanded"));
    m_cmbCursorShape->addItem(i18n("Triangle Lefthanded"));
    m_cmbCursorShape->addItem(i18n("Black Pixel"));
    m_cmbCursorShape->addItem(i18n("White Pixel"));

    m_cmbOutlineShape->addItem(i18n("No Outline"));
    m_cmbOutlineShape->addItem(i18n("Circle Outline"));
    m_cmbOutlineShape->addItem(i18n("Preview Outline"));
    m_cmbOutlineShape->addItem(i18n("Tilt Outline"));

    m_cmbCursorShape->setCurrentIndex(cfg.newCursorStyle());
    m_cmbOutlineShape->setCurrentIndex(cfg.newOutlineStyle());

    chkShowRootLayer->setChecked(cfg.showRootLayer());

    int autosaveInterval = cfg.autoSaveInterval();
    //convert to minutes
    m_autosaveSpinBox->setValue(autosaveInterval / 60);
    m_autosaveCheckBox->setChecked(autosaveInterval > 0);
    m_undoStackSize->setValue(cfg.undoStackLimit());
    m_backupFileCheckBox->setChecked(cfg.backupFile());
    m_showOutlinePainting->setChecked(cfg.showOutlineWhilePainting());
    m_hideSplashScreen->setChecked(cfg.hideSplashScreen());
    m_cmbMDIType->setCurrentIndex(cfg.readEntry<int>("mdi_viewmode", (int)QMdiArea::TabbedView));
    m_chkRubberBand->setChecked(cfg.readEntry<int>("mdi_rubberband", cfg.useOpenGL()));
    m_favoritePresetsSpinBox->setValue(cfg.favoritePresets());
    KoColor mdiColor;
    mdiColor.fromQColor(cfg.getMDIBackgroundColor());
    m_mdiColor->setColor(mdiColor);
    m_backgroundimage->setText(cfg.getMDIBackgroundImage());
    m_chkCanvasMessages->setChecked(cfg.showCanvasMessages());
    m_chkCompressKra->setChecked(cfg.compressKra());

    const QString configPath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QSettings kritarc(configPath + QStringLiteral("/kritadisplayrc"), QSettings::IniFormat);
    m_chkHiDPI->setChecked(kritarc.value("EnableHiDPI", false).toBool());
    m_chkSingleApplication->setChecked(kritarc.value("EnableSingleApplication", true).toBool());

    m_radioToolOptionsInDocker->setChecked(cfg.toolOptionsInDocker());
    m_chkSwitchSelectionCtrlAlt->setChecked(cfg.switchSelectionCtrlAlt());
    m_chkConvertOnImport->setChecked(cfg.convertToImageColorspaceOnImport());

    connect(m_bnFileName, SIGNAL(clicked()), SLOT(getBackgroundImage()));
    connect(clearBgImageButton, SIGNAL(clicked()), SLOT(clearBackgroundImage()));
}

void GeneralTab::setDefault()
{
    KisConfig cfg;

    m_cmbCursorShape->setCurrentIndex(cfg.newCursorStyle(true));
    m_cmbOutlineShape->setCurrentIndex(cfg.newOutlineStyle(true));
    chkShowRootLayer->setChecked(cfg.showRootLayer(true));
    m_autosaveCheckBox->setChecked(cfg.autoSaveInterval(true) > 0);
    //convert to minutes
    m_autosaveSpinBox->setValue(cfg.autoSaveInterval(true) / 60);
    m_undoStackSize->setValue(cfg.undoStackLimit(true));
    m_backupFileCheckBox->setChecked(cfg.backupFile(true));
    m_showOutlinePainting->setChecked(cfg.showOutlineWhilePainting(true));
    m_hideSplashScreen->setChecked(cfg.hideSplashScreen(true));
    m_cmbMDIType->setCurrentIndex((int)QMdiArea::TabbedView);
    m_chkRubberBand->setChecked(cfg.useOpenGL(true));
    m_favoritePresetsSpinBox->setValue(cfg.favoritePresets(true));
    KoColor mdiColor;
    mdiColor.fromQColor(cfg.getMDIBackgroundColor(true));
    m_mdiColor->setColor(mdiColor);
    m_backgroundimage->setText(cfg.getMDIBackgroundImage(true));
    m_chkCanvasMessages->setChecked(cfg.showCanvasMessages(true));
    m_chkCompressKra->setChecked(cfg.compressKra(true));
    m_chkHiDPI->setChecked(false);
    m_chkSingleApplication->setChecked(true);

    m_chkHiDPI->setChecked(true);
    m_radioToolOptionsInDocker->setChecked(cfg.toolOptionsInDocker(true));
    m_chkSwitchSelectionCtrlAlt->setChecked(cfg.switchSelectionCtrlAlt(true));
    m_chkConvertOnImport->setChecked(cfg.convertToImageColorspaceOnImport(true));

}

CursorStyle GeneralTab::cursorStyle()
{
    return (CursorStyle)m_cmbCursorShape->currentIndex();
}

OutlineStyle GeneralTab::outlineStyle()
{
    return (OutlineStyle)m_cmbOutlineShape->currentIndex();
}

bool GeneralTab::showRootLayer()
{
    return chkShowRootLayer->isChecked();
}

int GeneralTab::autoSaveInterval()
{
    //convert to seconds
    return m_autosaveCheckBox->isChecked() ? m_autosaveSpinBox->value()*60 : 0;
}

int GeneralTab::undoStackSize()
{
    return m_undoStackSize->value();
}

bool GeneralTab::showOutlineWhilePainting()
{
    return m_showOutlinePainting->isChecked();
}

bool GeneralTab::hideSplashScreen()
{
    return m_hideSplashScreen->isChecked();
}

int GeneralTab::mdiMode()
{
    return m_cmbMDIType->currentIndex();
}

int GeneralTab::favoritePresets()
{
    return m_favoritePresetsSpinBox->value();
}

bool GeneralTab::showCanvasMessages()
{
    return m_chkCanvasMessages->isChecked();
}

bool GeneralTab::compressKra()
{
    return m_chkCompressKra->isChecked();
}

bool GeneralTab::toolOptionsInDocker()
{
    return m_radioToolOptionsInDocker->isChecked();
}

bool GeneralTab::switchSelectionCtrlAlt()
{
    return m_chkSwitchSelectionCtrlAlt->isChecked();

}

bool GeneralTab::convertToImageColorspaceOnImport()
{
    return m_chkConvertOnImport->isChecked();
}


void GeneralTab::getBackgroundImage()
{
    KoFileDialog dialog(this, KoFileDialog::OpenFile, "BackgroundImages");
    dialog.setCaption(i18n("Select a Background Image"));
    dialog.setDefaultDir(QDesktopServices::storageLocation(QDesktopServices::PicturesLocation));
    dialog.setImageFilters();

    QString fn = dialog.filename();
    // dialog box was canceled or somehow no file was selected
    if (fn.isEmpty()) {
        return;
    }

    QImage image(fn);
    if (image.isNull()) {
        QMessageBox::warning(this, i18nc("@title:window", "Krita"), i18n("%1 is not a valid image file!", fn));
    }
    else {
        m_backgroundimage->setText(fn);
    }
}

void GeneralTab::clearBackgroundImage()
{
    // clearing the background image text will implicitly make the background color be used
    m_backgroundimage->setText("");
}

#include "kactioncollection.h"
#include "KisActionsSnapshot.h"

ShortcutSettingsTab::ShortcutSettingsTab(QWidget *parent, const char *name)
    : QWidget(parent)
{
    setObjectName(name);

    QGridLayout * l = new QGridLayout(this);
    l->setMargin(0);
    m_page = new WdgShortcutSettings(this);
    l->addWidget(m_page, 0, 0);


    m_snapshot.reset(new KisActionsSnapshot);

    KActionCollection *collection =
        KisPart::instance()->currentMainwindow()->actionCollection();

    Q_FOREACH (QAction *action, collection->actions()) {
        m_snapshot->addAction(action->objectName(), action);
    }

    QMap<QString, KActionCollection*> sortedCollections =
        m_snapshot->actionCollections();

    for (auto it = sortedCollections.constBegin(); it != sortedCollections.constEnd(); ++it) {
        m_page->addCollection(it.value(), it.key());
    }
}

ShortcutSettingsTab::~ShortcutSettingsTab()
{
}

void ShortcutSettingsTab::setDefault()
{
    m_page->allDefault();
}

void ShortcutSettingsTab::saveChanges()
{
    m_page->save();
    KisActionRegistry::instance()->settingsPageSaved();
}

void ShortcutSettingsTab::cancelChanges()
{
    m_page->undo();
}

ColorSettingsTab::ColorSettingsTab(QWidget *parent, const char *name)
    : QWidget(parent)
{
    setObjectName(name);

    // XXX: Make sure only profiles that fit the specified color model
    // are shown in the profile combos

    QGridLayout * l = new QGridLayout(this);
    l->setMargin(0);
    m_page = new WdgColorSettings(this);
    l->addWidget(m_page, 0, 0);

    KisConfig cfg;

    m_page->chkUseSystemMonitorProfile->setChecked(cfg.useSystemMonitorProfile());
    connect(m_page->chkUseSystemMonitorProfile, SIGNAL(toggled(bool)), this, SLOT(toggleAllowMonitorProfileSelection(bool)));

    m_page->cmbWorkingColorSpace->setIDList(KoColorSpaceRegistry::instance()->listKeys());
    m_page->cmbWorkingColorSpace->setCurrent(cfg.workingColorSpace());

    m_page->bnAddColorProfile->setIcon(KisIconUtils::loadIcon("document-open"));
    m_page->bnAddColorProfile->setToolTip( i18n("Open Color Profile") );
    connect(m_page->bnAddColorProfile, SIGNAL(clicked()), SLOT(installProfile()));

    QFormLayout *monitorProfileGrid = new QFormLayout(m_page->monitorprofileholder);
    for(int i = 0; i < QApplication::desktop()->screenCount(); ++i) {
        QLabel *lbl = new QLabel(i18nc("The number of the screen", "Screen %1:", i + 1));
        m_monitorProfileLabels << lbl;
        SqueezedComboBox *cmb = new SqueezedComboBox();
        cmb->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        monitorProfileGrid->addRow(lbl, cmb);
        m_monitorProfileWidgets << cmb;
    }

    refillMonitorProfiles(KoID("RGBA", ""));

    for(int i = 0; i < QApplication::desktop()->screenCount(); ++i) {
        if (m_monitorProfileWidgets[i]->contains(cfg.monitorProfile(i))) {
            m_monitorProfileWidgets[i]->setCurrent(cfg.monitorProfile(i));
        }
    }

    m_page->chkBlackpoint->setChecked(cfg.useBlackPointCompensation());
    m_page->chkAllowLCMSOptimization->setChecked(cfg.allowLCMSOptimization());

    KisImageConfig cfgImage;

    KisProofingConfigurationSP proofingConfig = cfgImage.defaultProofingconfiguration();
    m_page->sldAdaptationState->setMaximum(20);
    m_page->sldAdaptationState->setMinimum(0);
    m_page->sldAdaptationState->setValue((int)proofingConfig->adaptationState*20);

    //probably this should become the screenprofile?
    KoColor ga(KoColorSpaceRegistry::instance()->rgb8());
    ga.fromKoColor(proofingConfig->warningColor);
    m_page->gamutAlarm->setColor(ga);

    const KoColorSpace *proofingSpace =  KoColorSpaceRegistry::instance()->colorSpace(proofingConfig->proofingModel,
                                                                                      proofingConfig->proofingDepth,
                                                                                      proofingConfig->proofingProfile);
    if (proofingSpace) {
        m_page->proofingSpaceSelector->setCurrentColorSpace(proofingSpace);
    }

    m_page->cmbProofingIntent->setCurrentIndex((int)proofingConfig->intent);
    m_page->ckbProofBlackPoint->setChecked(proofingConfig->conversionFlags.testFlag(KoColorConversionTransformation::BlackpointCompensation));

    m_pasteBehaviourGroup.addButton(m_page->radioPasteWeb, PASTE_ASSUME_WEB);
    m_pasteBehaviourGroup.addButton(m_page->radioPasteMonitor, PASTE_ASSUME_MONITOR);
    m_pasteBehaviourGroup.addButton(m_page->radioPasteAsk, PASTE_ASK);

    QAbstractButton *button = m_pasteBehaviourGroup.button(cfg.pasteBehaviour());
    Q_ASSERT(button);

    if (button) {
        button->setChecked(true);
    }

    m_page->cmbMonitorIntent->setCurrentIndex(cfg.monitorRenderIntent());

    toggleAllowMonitorProfileSelection(cfg.useSystemMonitorProfile());

}

void ColorSettingsTab::installProfile()
{
    KoFileDialog dialog(this, KoFileDialog::OpenFiles, "OpenDocumentICC");
    dialog.setCaption(i18n("Install Color Profiles"));
    dialog.setDefaultDir(QDesktopServices::storageLocation(QDesktopServices::HomeLocation));
    dialog.setMimeTypeFilters(QStringList() << "application/vnd.iccprofile", "application/vnd.iccprofile");
    QStringList profileNames = dialog.filenames();

    KoColorSpaceEngine *iccEngine = KoColorSpaceEngineRegistry::instance()->get("icc");
    Q_ASSERT(iccEngine);

    QString saveLocation = KoResourcePaths::saveLocation("icc_profiles");

    Q_FOREACH (const QString &profileName, profileNames) {
        if (!QFile::copy(profileName, saveLocation + QFileInfo(profileName).fileName())) {
            qWarning() << "Could not install profile!" << saveLocation + QFileInfo(profileName).fileName();
            continue;
        }
        iccEngine->addProfile(saveLocation + QFileInfo(profileName).fileName());
    }

    KisConfig cfg;
    refillMonitorProfiles(KoID("RGBA", ""));

    for(int i = 0; i < QApplication::desktop()->screenCount(); ++i) {
        if (m_monitorProfileWidgets[i]->contains(cfg.monitorProfile(i))) {
            m_monitorProfileWidgets[i]->setCurrent(cfg.monitorProfile(i));
        }
    }

}

void ColorSettingsTab::toggleAllowMonitorProfileSelection(bool useSystemProfile)
{
    if (useSystemProfile) {
        KisConfig cfg;
        QStringList devices = KisColorManager::instance()->devices();
        if (devices.size() == QApplication::desktop()->screenCount()) {
            for(int i = 0; i < QApplication::desktop()->screenCount(); ++i) {
                m_monitorProfileWidgets[i]->clear();
                QString monitorForScreen = cfg.monitorForScreen(i, devices[i]);
                Q_FOREACH (const QString &device, devices) {
                    m_monitorProfileLabels[i]->setText(i18nc("The display/screen we got from Qt", "Screen %1:", i + 1));
                    m_monitorProfileWidgets[i]->addSqueezedItem(KisColorManager::instance()->deviceName(device), device);
                    if (devices[i] == monitorForScreen) {
                        m_monitorProfileWidgets[i]->setCurrentIndex(i);
                    }
                }
            }
        }
    }
    else {
        KisConfig cfg;
        refillMonitorProfiles(KoID("RGBA", ""));

        for(int i = 0; i < QApplication::desktop()->screenCount(); ++i) {
            if (m_monitorProfileWidgets[i]->contains(cfg.monitorProfile(i))) {
                m_monitorProfileWidgets[i]->setCurrent(cfg.monitorProfile(i));
            }
        }
    }
}

void ColorSettingsTab::setDefault()
{
    m_page->cmbWorkingColorSpace->setCurrent("RGBA");

    refillMonitorProfiles(KoID("RGBA", ""));

    KisConfig cfg;
    KisImageConfig cfgImage;
    KisProofingConfigurationSP proofingConfig =  cfgImage.defaultProofingconfiguration();
    const KoColorSpace *proofingSpace =  KoColorSpaceRegistry::instance()->colorSpace(proofingConfig->proofingModel,proofingConfig->proofingDepth,proofingConfig->proofingProfile);
    if (proofingSpace) {
        m_page->proofingSpaceSelector->setCurrentColorSpace(proofingSpace);
    }
    m_page->cmbProofingIntent->setCurrentIndex((int)proofingConfig->intent);
    m_page->ckbProofBlackPoint->setChecked(proofingConfig->conversionFlags.testFlag(KoColorConversionTransformation::BlackpointCompensation));
    m_page->sldAdaptationState->setValue(0);

    //probably this should become the screenprofile?
    KoColor ga(KoColorSpaceRegistry::instance()->rgb8());
    ga.fromKoColor(proofingConfig->warningColor);
    m_page->gamutAlarm->setColor(ga);

    m_page->chkBlackpoint->setChecked(cfg.useBlackPointCompensation(true));
    m_page->chkAllowLCMSOptimization->setChecked(cfg.allowLCMSOptimization(true));
    m_page->cmbMonitorIntent->setCurrentIndex(cfg.monitorRenderIntent(true));
    m_page->chkUseSystemMonitorProfile->setChecked(cfg.useSystemMonitorProfile(true));
    QAbstractButton *button = m_pasteBehaviourGroup.button(cfg.pasteBehaviour(true));
    Q_ASSERT(button);
    if (button) {
        button->setChecked(true);
    }
}


void ColorSettingsTab::refillMonitorProfiles(const KoID & s)
{
    const KoColorSpaceFactory * csf = KoColorSpaceRegistry::instance()->colorSpaceFactory(s.id());

    for (int i = 0; i < QApplication::desktop()->screenCount(); ++i) {
        m_monitorProfileWidgets[i]->clear();
    }

    if (!csf)
        return;

    QMap<QString, const KoColorProfile *>  profileList;
    Q_FOREACH(const KoColorProfile *profile, KoColorSpaceRegistry::instance()->profilesFor(csf)) {
        profileList[profile->name()] = profile;
    }

    Q_FOREACH (const KoColorProfile *profile, profileList.values()) {
        //qDebug() << "Profile" << profile->name() << profile->isSuitableForDisplay() << csf->defaultProfile();
        if (profile->isSuitableForDisplay()) {
            for (int i = 0; i < QApplication::desktop()->screenCount(); ++i) {
                m_monitorProfileWidgets[i]->addSqueezedItem(profile->name());
            }
        }
    }

    for (int i = 0; i < QApplication::desktop()->screenCount(); ++i) {
        m_monitorProfileLabels[i]->setText(i18nc("The number of the screen", "Screen %1:", i + 1));
        m_monitorProfileWidgets[i]->setCurrent(csf->defaultProfile());
    }
}


//---------------------------------------------------------------------------------------------------

void TabletSettingsTab::setDefault()
{
    KisCubicCurve curve;
    curve.fromString(DEFAULT_CURVE_STRING);
    m_page->pressureCurve->setCurve(curve);
}

TabletSettingsTab::TabletSettingsTab(QWidget* parent, const char* name): QWidget(parent)
{
    setObjectName(name);

    QGridLayout * l = new QGridLayout(this);
    l->setMargin(0);
    m_page = new WdgTabletSettings(this);
    l->addWidget(m_page, 0, 0);

    KisConfig cfg;
    KisCubicCurve curve;
    curve.fromString( cfg.pressureTabletCurve() );

    m_page->pressureCurve->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    m_page->pressureCurve->setCurve(curve);
}


//---------------------------------------------------------------------------------------------------
#include "kis_acyclic_signal_connector.h"

int getTotalRAM() {
    KisImageConfig cfg;
    return cfg.totalRAM();
}

int PerformanceTab::realTilesRAM()
{
    return intMemoryLimit->value() - intPoolLimit->value();
}

PerformanceTab::PerformanceTab(QWidget *parent, const char *name)
    : WdgPerformanceSettings(parent, name)
{
    KisImageConfig cfg;
    const int totalRAM = cfg.totalRAM();
    lblTotalMemory->setText(i18n("%1 MiB", totalRAM));

    sliderMemoryLimit->setSuffix(i18n(" %"));
    sliderMemoryLimit->setRange(1, 100, 2);
    sliderMemoryLimit->setSingleStep(0.01);

    sliderPoolLimit->setSuffix(i18n(" %"));
    sliderPoolLimit->setRange(0, 20, 2);
    sliderMemoryLimit->setSingleStep(0.01);

    sliderUndoLimit->setSuffix(i18n(" %"));
    sliderUndoLimit->setRange(0, 50, 2);
    sliderMemoryLimit->setSingleStep(0.01);

    intMemoryLimit->setMinimumWidth(80);
    intPoolLimit->setMinimumWidth(80);
    intUndoLimit->setMinimumWidth(80);


    SliderAndSpinBoxSync *sync1 =
        new SliderAndSpinBoxSync(sliderMemoryLimit,
                                 intMemoryLimit,
                                 getTotalRAM);

    sync1->slotParentValueChanged();
    m_syncs << sync1;

    SliderAndSpinBoxSync *sync2 =
        new SliderAndSpinBoxSync(sliderPoolLimit,
                                 intPoolLimit,
                                 std::bind(&KisIntParseSpinBox::value,
                                             intMemoryLimit));


    connect(intMemoryLimit, SIGNAL(valueChanged(int)), sync2, SLOT(slotParentValueChanged()));
    sync2->slotParentValueChanged();
    m_syncs << sync2;

    SliderAndSpinBoxSync *sync3 =
        new SliderAndSpinBoxSync(sliderUndoLimit,
                                 intUndoLimit,
                                 std::bind(&PerformanceTab::realTilesRAM,
                                             this));


    connect(intPoolLimit, SIGNAL(valueChanged(int)), sync3, SLOT(slotParentValueChanged()));
    sync3->slotParentValueChanged();
    m_syncs << sync3;

    sliderSwapSize->setSuffix(i18n(" GiB"));
    sliderSwapSize->setRange(1, 64);
    intSwapSize->setRange(1, 64);


    KisAcyclicSignalConnector *swapSizeConnector = new KisAcyclicSignalConnector(this);

    swapSizeConnector->connectForwardInt(sliderSwapSize, SIGNAL(valueChanged(int)),
                                         intSwapSize, SLOT(setValue(int)));

    swapSizeConnector->connectBackwardInt(intSwapSize, SIGNAL(valueChanged(int)),
                                          sliderSwapSize, SLOT(setValue(int)));

    lblSwapFileLocation->setText(cfg.swapDir());
    connect(bnSwapFile, SIGNAL(clicked()), SLOT(selectSwapDir()));

    load(false);
}

PerformanceTab::~PerformanceTab()
{
    qDeleteAll(m_syncs);
}

void PerformanceTab::load(bool requestDefault)
{
    KisImageConfig cfg;

    sliderMemoryLimit->setValue(cfg.memoryHardLimitPercent(requestDefault));
    sliderPoolLimit->setValue(cfg.memoryPoolLimitPercent(requestDefault));
    sliderUndoLimit->setValue(cfg.memorySoftLimitPercent(requestDefault));

    chkPerformanceLogging->setChecked(cfg.enablePerfLog(requestDefault));
    chkProgressReporting->setChecked(cfg.enableProgressReporting(requestDefault));

    sliderSwapSize->setValue(cfg.maxSwapSize(requestDefault) / 1024);
    lblSwapFileLocation->setText(cfg.swapDir(requestDefault));

    {
        KisConfig cfg2;
        chkOpenGLLogging->setChecked(cfg2.enableOpenGLDebugging(requestDefault));
        chkDisableVectorOptimizations->setChecked(cfg2.enableAmdVectorizationWorkaround(requestDefault));
    }
}

void PerformanceTab::save()
{
    KisImageConfig cfg;

    cfg.setMemoryHardLimitPercent(sliderMemoryLimit->value());
    cfg.setMemorySoftLimitPercent(sliderUndoLimit->value());
    cfg.setMemoryPoolLimitPercent(sliderPoolLimit->value());

    cfg.setEnablePerfLog(chkPerformanceLogging->isChecked());
    cfg.setEnableProgressReporting(chkProgressReporting->isChecked());

    cfg.setMaxSwapSize(sliderSwapSize->value() * 1024);

    cfg.setSwapDir(lblSwapFileLocation->text());

    {
        KisConfig cfg2;
        cfg2.setEnableOpenGLDebugging(chkOpenGLLogging->isChecked());
        cfg2.setEnableAmdVectorizationWorkaround(chkDisableVectorOptimizations->isChecked());
    }
}

void PerformanceTab::selectSwapDir()
{
    KisImageConfig cfg;
    QString swapDir = cfg.swapDir();
    swapDir = QFileDialog::getExistingDirectory(0, i18nc("@title:window", "Select a swap directory"), swapDir);
    lblSwapFileLocation->setText(swapDir);
}

//---------------------------------------------------------------------------------------------------

#include "KoColor.h"

DisplaySettingsTab::DisplaySettingsTab(QWidget *parent, const char *name)
    : WdgDisplaySettings(parent, name)
{
    KisConfig cfg;

    if (!KisOpenGL::hasOpenGL()) {
        grpOpenGL->setEnabled(false);
        grpOpenGL->setChecked(false);
        chkUseTextureBuffer->setEnabled(false);
        chkDisableVsync->setEnabled(false);
        cmbFilterMode->setEnabled(false);
    } else {
        grpOpenGL->setEnabled(true);
        grpOpenGL->setChecked(cfg.useOpenGL());
        chkUseTextureBuffer->setEnabled(cfg.useOpenGL());
        chkUseTextureBuffer->setChecked(cfg.useOpenGLTextureBuffer());
        chkDisableVsync->setVisible(cfg.showAdvancedOpenGLSettings());
        chkDisableVsync->setEnabled(cfg.useOpenGL());
        chkDisableVsync->setChecked(cfg.disableVSync());
        cmbFilterMode->setEnabled(cfg.useOpenGL());
        cmbFilterMode->setCurrentIndex(cfg.openGLFilteringMode());
        // Don't show the high quality filtering mode if it's not available
        if (!KisOpenGL::supportsLoD()) {
            cmbFilterMode->removeItem(3);
        }
    }
    if (qApp->applicationName() == "kritasketch" || qApp->applicationName() == "kritagemini") {
       grpOpenGL->setVisible(false);
       grpOpenGL->setMaximumHeight(0);
    }

    KoColor c;
    c.fromQColor(cfg.selectionOverlayMaskColor());
    c.setOpacity(1.0);
    btnSelectionOverlayColor->setColor(c);
    sldSelectionOverlayOpacity->setRange(0.0, 1.0, 2);
    sldSelectionOverlayOpacity->setSingleStep(0.05);
    sldSelectionOverlayOpacity->setValue(cfg.selectionOverlayMaskColor().alphaF());

    intCheckSize->setValue(cfg.checkSize());
    chkMoving->setChecked(cfg.scrollCheckers());
    KoColor ck1(KoColorSpaceRegistry::instance()->rgb8());
    ck1.fromQColor(cfg.checkersColor1());
    colorChecks1->setColor(ck1);
    KoColor ck2(KoColorSpaceRegistry::instance()->rgb8());
    ck2.fromQColor(cfg.checkersColor2());
    colorChecks2->setColor(ck2);
    KoColor cb(KoColorSpaceRegistry::instance()->rgb8());
    cb.fromQColor(cfg.canvasBorderColor());
    canvasBorder->setColor(cb);
    hideScrollbars->setChecked(cfg.hideScrollbars());
    chkCurveAntialiasing->setChecked(cfg.antialiasCurves());
    chkSelectionOutlineAntialiasing->setChecked(cfg.antialiasSelectionOutline());
    chkChannelsAsColor->setChecked(cfg.showSingleChannelAsColor());
    chkHidePopups->setChecked(cfg.hidePopups());

    connect(grpOpenGL, SIGNAL(toggled(bool)), SLOT(slotUseOpenGLToggled(bool)));
}

void DisplaySettingsTab::setDefault()
{
    KisConfig cfg;
    if (!KisOpenGL::hasOpenGL()) {
        grpOpenGL->setEnabled(false);
        grpOpenGL->setChecked(false);
        chkUseTextureBuffer->setEnabled(false);
        chkDisableVsync->setEnabled(false);
        cmbFilterMode->setEnabled(false);
    }
    else {
        grpOpenGL->setEnabled(true);
        grpOpenGL->setChecked(cfg.useOpenGL(true));
        chkUseTextureBuffer->setChecked(cfg.useOpenGLTextureBuffer(true));
        chkUseTextureBuffer->setEnabled(true);
        chkDisableVsync->setEnabled(true);
        chkDisableVsync->setChecked(cfg.disableVSync(true));
        cmbFilterMode->setEnabled(true);
        cmbFilterMode->setCurrentIndex(cfg.openGLFilteringMode(true));
    }

    chkMoving->setChecked(cfg.scrollCheckers(true));
    intCheckSize->setValue(cfg.checkSize(true));
    KoColor ck1(KoColorSpaceRegistry::instance()->rgb8());
    ck1.fromQColor(cfg.checkersColor1(true));
    colorChecks1->setColor(ck1);
    KoColor ck2(KoColorSpaceRegistry::instance()->rgb8());
    ck2.fromQColor(cfg.checkersColor2(true));
    colorChecks2->setColor(ck2);
    KoColor cvb(KoColorSpaceRegistry::instance()->rgb8());
    cvb.fromQColor(cfg.canvasBorderColor(true));
    canvasBorder->setColor(cvb);
    hideScrollbars->setChecked(cfg.hideScrollbars(true));
    chkCurveAntialiasing->setChecked(cfg.antialiasCurves(true));
    chkSelectionOutlineAntialiasing->setChecked(cfg.antialiasSelectionOutline(true));
    chkChannelsAsColor->setChecked(cfg.showSingleChannelAsColor(true));
    chkHidePopups->setChecked(cfg.hidePopups(true));

}

void DisplaySettingsTab::slotUseOpenGLToggled(bool isChecked)
{
    chkUseTextureBuffer->setEnabled(isChecked);
    chkDisableVsync->setEnabled(isChecked);
    cmbFilterMode->setEnabled(isChecked);
}

//---------------------------------------------------------------------------------------------------
FullscreenSettingsTab::FullscreenSettingsTab(QWidget* parent) : WdgFullscreenSettingsBase(parent)
{
    KisConfig cfg;

    chkDockers->setChecked(cfg.hideDockersFullscreen());
    chkMenu->setChecked(cfg.hideMenuFullscreen());
    chkScrollbars->setChecked(cfg.hideScrollbarsFullscreen());
    chkStatusbar->setChecked(cfg.hideStatusbarFullscreen());
    chkTitlebar->setChecked(cfg.hideTitlebarFullscreen());
    chkToolbar->setChecked(cfg.hideToolbarFullscreen());

}

void FullscreenSettingsTab::setDefault()
{
    KisConfig cfg;
    chkDockers->setChecked(cfg.hideDockersFullscreen(true));
    chkMenu->setChecked(cfg.hideMenuFullscreen(true));
    chkScrollbars->setChecked(cfg.hideScrollbarsFullscreen(true));
    chkStatusbar->setChecked(cfg.hideStatusbarFullscreen(true));
    chkTitlebar->setChecked(cfg.hideTitlebarFullscreen(true));
    chkToolbar->setChecked(cfg.hideToolbarFullscreen(true));
}


//---------------------------------------------------------------------------------------------------

KisDlgPreferences::KisDlgPreferences(QWidget* parent, const char* name)
    : KPageDialog(parent)
{
    Q_UNUSED(name);
    setWindowTitle(i18n("Configure Krita"));
    setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults);
    button(QDialogButtonBox::Ok)->setDefault(true);

    setFaceType(KPageDialog::List);

    // General
    KoVBox *vbox = new KoVBox();
    KPageWidgetItem *page = new KPageWidgetItem(vbox, i18n("General"));
    page->setObjectName("general");
    page->setHeader(i18n("General"));
    page->setIcon(KisIconUtils::loadIcon("go-home"));
    addPage(page);
    m_general = new GeneralTab(vbox);

    // Shortcuts
    vbox = new KoVBox();
    page = new KPageWidgetItem(vbox, i18n("Keyboard Shortcuts"));
    page->setObjectName("shortcuts");
    page->setHeader(i18n("Shortcuts"));
    page->setIcon(KisIconUtils::loadIcon("document-export"));
    addPage(page);
    m_shortcutSettings = new ShortcutSettingsTab(vbox);
    connect(this, SIGNAL(accepted()), m_shortcutSettings, SLOT(saveChanges()));
    connect(this, SIGNAL(rejected()), m_shortcutSettings, SLOT(cancelChanges()));

    // Canvas input settings
    m_inputConfiguration = new KisInputConfigurationPage();
    page = addPage(m_inputConfiguration, i18n("Canvas Input Settings"));
    page->setHeader(i18n("Canvas Input"));
    page->setObjectName("canvasinput");
    page->setIcon(KisIconUtils::loadIcon("configure"));

    // Display
    vbox = new KoVBox();
    page = new KPageWidgetItem(vbox, i18n("Display"));
    page->setObjectName("display");
    page->setHeader(i18n("Display"));
    page->setIcon(KisIconUtils::loadIcon("preferences-desktop-display"));
    addPage(page);
    m_displaySettings = new DisplaySettingsTab(vbox);

    // Color
    vbox = new KoVBox();
    page = new KPageWidgetItem(vbox, i18n("Color Management"));
    page->setObjectName("colormanagement");
    page->setHeader(i18n("Color"));
    page->setIcon(KisIconUtils::loadIcon("preferences-desktop-color"));
    addPage(page);
    m_colorSettings = new ColorSettingsTab(vbox);

    // Performance
    vbox = new KoVBox();
    page = new KPageWidgetItem(vbox, i18n("Performance"));
    page->setObjectName("performance");
    page->setHeader(i18n("Performance"));
    page->setIcon(KisIconUtils::loadIcon("applications-system"));
    addPage(page);
    m_performanceSettings = new PerformanceTab(vbox);

    // Tablet
    vbox = new KoVBox();
    page = new KPageWidgetItem(vbox, i18n("Tablet settings"));
    page->setObjectName("tablet");
    page->setHeader(i18n("Tablet"));
    page->setIcon(KisIconUtils::loadIcon("document-edit"));
    addPage(page);
    m_tabletSettings = new TabletSettingsTab(vbox);

    // full-screen mode
    vbox = new KoVBox();
    page = new KPageWidgetItem(vbox, i18n("Canvas-only settings"));
    page->setObjectName("canvasonly");
    page->setHeader(i18n("Canvas-only"));
    page->setIcon(KisIconUtils::loadIcon("folder-pictures"));
    addPage(page);
    m_fullscreenSettings = new FullscreenSettingsTab(vbox);

    // Author profiles
    m_authorPage = new KoConfigAuthorPage();
    page = addPage(m_authorPage, i18nc("@title:tab Author page", "Author" ));
    page->setObjectName("author");
    page->setHeader(i18n("Author"));
    page->setIcon(KisIconUtils::loadIcon("im-user"));


    QPushButton *restoreDefaultsButton = button(QDialogButtonBox::RestoreDefaults);

    connect(this, SIGNAL(accepted()), m_inputConfiguration, SLOT(saveChanges()));
    connect(this, SIGNAL(rejected()), m_inputConfiguration, SLOT(revertChanges()));

    KisPreferenceSetRegistry *preferenceSetRegistry = KisPreferenceSetRegistry::instance();
    Q_FOREACH (KisAbstractPreferenceSetFactory *preferenceSetFactory, preferenceSetRegistry->values()) {
        KisPreferenceSet* preferenceSet = preferenceSetFactory->createPreferenceSet();
        vbox = new KoVBox();
        page = new KPageWidgetItem(vbox, preferenceSet->name());
        page->setHeader(preferenceSet->header());
        page->setIcon(preferenceSet->icon());
        addPage(page);
        preferenceSet->setParent(vbox);
        preferenceSet->loadPreferences();

        connect(restoreDefaultsButton, SIGNAL(clicked(bool)), preferenceSet, SLOT(loadDefaultPreferences()), Qt::UniqueConnection);
        connect(this, SIGNAL(accepted()), preferenceSet, SLOT(savePreferences()), Qt::UniqueConnection);
    }


    connect(restoreDefaultsButton, SIGNAL(clicked(bool)), this, SLOT(slotDefault()));

}

KisDlgPreferences::~KisDlgPreferences()
{
}

void KisDlgPreferences::slotDefault()
{
    if (currentPage()->objectName() == "general") {
        m_general->setDefault();
    }
    else if (currentPage()->objectName() == "shortcuts") {
        m_shortcutSettings->setDefault();
    }
    else if (currentPage()->objectName() == "display") {
        m_displaySettings->setDefault();
    }
    else if (currentPage()->objectName() == "colormanagement") {
        m_colorSettings->setDefault();
    }
    else if (currentPage()->objectName() == "performance") {
        m_performanceSettings->load(true);
    }
    else if (currentPage()->objectName() == "tablet") {
        m_tabletSettings->setDefault();
    }
    else if (currentPage()->objectName() == "canvasonly") {
        m_fullscreenSettings->setDefault();
    }
    else if (currentPage()->objectName() == "canvasinput") {
        m_inputConfiguration->setDefaults();
    }
}

bool KisDlgPreferences::editPreferences()
{
    KisDlgPreferences* dialog;

    dialog = new KisDlgPreferences();
    bool baccept = (dialog->exec() == Accepted);
    if (baccept) {
        // General settings
        KisConfig cfg;
        cfg.setNewCursorStyle(dialog->m_general->cursorStyle());
        cfg.setNewOutlineStyle(dialog->m_general->outlineStyle());
        cfg.setShowRootLayer(dialog->m_general->showRootLayer());
        cfg.setShowOutlineWhilePainting(dialog->m_general->showOutlineWhilePainting());
        cfg.setHideSplashScreen(dialog->m_general->hideSplashScreen());
        cfg.writeEntry<int>("mdi_viewmode", dialog->m_general->mdiMode());
        cfg.setMDIBackgroundColor(dialog->m_general->m_mdiColor->color().toQColor());
        cfg.setMDIBackgroundImage(dialog->m_general->m_backgroundimage->text());
        cfg.setAutoSaveInterval(dialog->m_general->autoSaveInterval());
        cfg.setBackupFile(dialog->m_general->m_backupFileCheckBox->isChecked());
        cfg.setShowCanvasMessages(dialog->m_general->showCanvasMessages());
        cfg.setCompressKra(dialog->m_general->compressKra());

        const QString configPath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
        QSettings kritarc(configPath + QStringLiteral("/kritadisplayrc"), QSettings::IniFormat);
        kritarc.setValue("EnableHiDPI", dialog->m_general->m_chkHiDPI->isChecked());
        kritarc.setValue("EnableSingleApplication", dialog->m_general->m_chkSingleApplication->isChecked());

        cfg.setToolOptionsInDocker(dialog->m_general->toolOptionsInDocker());
        cfg.setSwitchSelectionCtrlAlt(dialog->m_general->switchSelectionCtrlAlt());
        cfg.setConvertToImageColorspaceOnImport(dialog->m_general->convertToImageColorspaceOnImport());

        KisPart *part = KisPart::instance();
        if (part) {
            Q_FOREACH (QPointer<KisDocument> doc, part->documents()) {
                if (doc) {
                    doc->setAutoSave(dialog->m_general->autoSaveInterval());
                    doc->setBackupFile(dialog->m_general->m_backupFileCheckBox->isChecked());
                    doc->undoStack()->setUndoLimit(dialog->m_general->undoStackSize());
                }
            }
        }
        cfg.setUndoStackLimit(dialog->m_general->undoStackSize());
        cfg.setFavoritePresets(dialog->m_general->favoritePresets());

        // Color settings
        cfg.setUseSystemMonitorProfile(dialog->m_colorSettings->m_page->chkUseSystemMonitorProfile->isChecked());
        for (int i = 0; i < QApplication::desktop()->screenCount(); ++i) {
            if (dialog->m_colorSettings->m_page->chkUseSystemMonitorProfile->isChecked()) {
                int currentIndex = dialog->m_colorSettings->m_monitorProfileWidgets[i]->currentIndex();
                QString monitorid = dialog->m_colorSettings->m_monitorProfileWidgets[i]->itemData(currentIndex).toString();
                cfg.setMonitorForScreen(i, monitorid);
            }
            else {
                cfg.setMonitorProfile(i,
                                      dialog->m_colorSettings->m_monitorProfileWidgets[i]->itemHighlighted(),
                                      dialog->m_colorSettings->m_page->chkUseSystemMonitorProfile->isChecked());
            }
        }
        cfg.setWorkingColorSpace(dialog->m_colorSettings->m_page->cmbWorkingColorSpace->currentItem().id());

        KisImageConfig cfgImage;
        cfgImage.setDefaultProofingConfig(dialog->m_colorSettings->m_page->proofingSpaceSelector->currentColorSpace(),
                                          dialog->m_colorSettings->m_page->cmbProofingIntent->currentIndex(),
                                          dialog->m_colorSettings->m_page->ckbProofBlackPoint->isChecked(),
                                          dialog->m_colorSettings->m_page->gamutAlarm->color(),
                                          (double)dialog->m_colorSettings->m_page->sldAdaptationState->value()/20);
        cfg.setUseBlackPointCompensation(dialog->m_colorSettings->m_page->chkBlackpoint->isChecked());
        cfg.setAllowLCMSOptimization(dialog->m_colorSettings->m_page->chkAllowLCMSOptimization->isChecked());
        cfg.setPasteBehaviour(dialog->m_colorSettings->m_pasteBehaviourGroup.checkedId());
        cfg.setRenderIntent(dialog->m_colorSettings->m_page->cmbMonitorIntent->currentIndex());

        // Tablet settings
        cfg.setPressureTabletCurve( dialog->m_tabletSettings->m_page->pressureCurve->curve().toString() );

        dialog->m_performanceSettings->save();

        if (!cfg.useOpenGL() && dialog->m_displaySettings->grpOpenGL->isChecked())
            cfg.setCanvasState("TRY_OPENGL");
        cfg.setUseOpenGL(dialog->m_displaySettings->grpOpenGL->isChecked());
        cfg.setUseOpenGLTextureBuffer(dialog->m_displaySettings->chkUseTextureBuffer->isChecked());
        cfg.setOpenGLFilteringMode(dialog->m_displaySettings->cmbFilterMode->currentIndex());
        cfg.setDisableVSync(dialog->m_displaySettings->chkDisableVsync->isChecked());

        cfg.setCheckSize(dialog->m_displaySettings->intCheckSize->value());
        cfg.setScrollingCheckers(dialog->m_displaySettings->chkMoving->isChecked());
        cfg.setCheckersColor1(dialog->m_displaySettings->colorChecks1->color().toQColor());
        cfg.setCheckersColor2(dialog->m_displaySettings->colorChecks2->color().toQColor());
        cfg.setCanvasBorderColor(dialog->m_displaySettings->canvasBorder->color().toQColor());
        cfg.setHideScrollbars(dialog->m_displaySettings->hideScrollbars->isChecked());
        KoColor c = dialog->m_displaySettings->btnSelectionOverlayColor->color();
        c.setOpacity(dialog->m_displaySettings->sldSelectionOverlayOpacity->value());
        cfg.setSelectionOverlayMaskColor(c.toQColor());
        cfg.setAntialiasCurves(dialog->m_displaySettings->chkCurveAntialiasing->isChecked());
        cfg.setAntialiasSelectionOutline(dialog->m_displaySettings->chkSelectionOutlineAntialiasing->isChecked());
        cfg.setShowSingleChannelAsColor(dialog->m_displaySettings->chkChannelsAsColor->isChecked());
        cfg.setHidePopups(dialog->m_displaySettings->chkHidePopups->isChecked());

        cfg.setHideDockersFullscreen(dialog->m_fullscreenSettings->chkDockers->checkState());
        cfg.setHideMenuFullscreen(dialog->m_fullscreenSettings->chkMenu->checkState());
        cfg.setHideScrollbarsFullscreen(dialog->m_fullscreenSettings->chkScrollbars->checkState());
        cfg.setHideStatusbarFullscreen(dialog->m_fullscreenSettings->chkStatusbar->checkState());
        cfg.setHideTitlebarFullscreen(dialog->m_fullscreenSettings->chkTitlebar->checkState());
        cfg.setHideToolbarFullscreen(dialog->m_fullscreenSettings->chkToolbar->checkState());

        dialog->m_authorPage->apply();

    }
    delete dialog;
    return baccept;
}

