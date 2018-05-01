/*
 *  Copyright (c) 2014 Boudewijn Rempt <boud@valdyas.org>
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
#include "kis_splash_screen.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QCheckBox>
#include <kis_debug.h>
#include <QFile>

#include <KisPart.h>
#include <KisApplication.h>

#include <kis_icon.h>

#include <klocalizedstring.h>
#include <kconfig.h>
#include <ksharedconfig.h>
#include <kconfiggroup.h>
#include <QIcon>

KisSplashScreen::KisSplashScreen(const QString &version, const QPixmap &pixmap, bool themed, QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, Qt::SplashScreen | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | f),
      m_themed(themed)
{

    setupUi(this);
    setWindowIcon(KisIconUtils::loadIcon("calligrakrita"));

    // Maintain the aspect ratio on high DPI screens when scaling
    lblSplash->setPixmap(pixmap);

    QString color = colorString();
    lblVersion->setText(i18n("Version: %1", version));
    lblVersion->setStyleSheet("color:" + color);

    bnClose->hide();
    connect(bnClose, SIGNAL(clicked()), this, SLOT(close()));
    chkShowAtStartup->hide();
    connect(chkShowAtStartup, SIGNAL(toggled(bool)), this, SLOT(toggleShowAtStartup(bool)));

    KConfigGroup cfg( KSharedConfig::openConfig(), "SplashScreen");
    bool hideSplash = cfg.readEntry("HideSplashAfterStartup", false);
    chkShowAtStartup->setChecked(hideSplash);

    connect(lblRecent, SIGNAL(linkActivated(QString)), SLOT(linkClicked(QString)));
    connect(&m_timer, SIGNAL(timeout()), SLOT(raise()));

    // hide these labels by default
    lblLinks->setVisible(false);
    lblRecent->setVisible(false);
    line->setVisible(false);



    m_timer.setSingleShot(true);
    m_timer.start(10);
}

void KisSplashScreen::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateText();
}

void KisSplashScreen::updateText()
{
    QString color = colorString();

    KConfigGroup cfg2( KSharedConfig::openConfig(), "RecentFiles");
    int i = 1;

    QString recent = i18n("<html>"
                          "<head/>"
                          "<body>"
                          "<p align=\"center\"><b><span style=\" color:%1;\">Recent Files</span></b></p>", color);

    QString path;
    QStringList recentfiles;

    QFontMetrics metrics(lblRecent->font());

    do {
        path = cfg2.readPathEntry(QString("File%1").arg(i), QString());
        if (!path.isEmpty()) {
            QString name = cfg2.readPathEntry(QString("Name%1").arg(i), QString());
            QUrl url(path);
            if (name.isEmpty()) {
                name = url.fileName();
            }

            name = metrics.elidedText(name, Qt::ElideMiddle, lblRecent->width());

            if (!url.isLocalFile() || QFile::exists(url.toLocalFile())) {
                recentfiles.insert(0, QString("<p><a href=\"%1\"><span style=\"color:%3;\">%2</span></a></p>").arg(path).arg(name).arg(color));
            }
        }

        i++;
    } while (!path.isEmpty() || i <= 8);

    recent += recentfiles.join("\n");
    recent += "</body>"
        "</html>";
    lblRecent->setText(recent);
}

void KisSplashScreen::displayLinks() {

    QString color = colorString();
    lblLinks->setTextFormat(Qt::RichText);
    lblLinks->setText(i18n("<html>"
                           "<head/>"
                           "<body>"
                           "<p align=\"center\"><span style=\" color:%1;\"><b>Links</b></span></p>"

                           "<p><a href=\"https://krita.org/support-us/\"><span style=\" text-decoration: underline; color:%1;\">Support Krita</span></a></p>"

                           "<p><a href=\"https://docs.krita.org/Category:Getting_Started\"><span style=\" text-decoration: underline; color:%1;\">Getting Started</span></a></p>"
                           "<p><a href=\"https://docs.krita.org/\"><span style=\" text-decoration: underline; color:%1;\">Manual</span></a></p>"
                           "<p><a href=\"https://krita.org/\"><span style=\" text-decoration: underline; color:%1;\">Krita Website</span></a></p>"
                           "<p><a href=\"https://forum.kde.org/viewforum.php?f=136\"><span style=\" text-decoration: underline; color:%1;\">User Community</span></a></p>"

                           "<p><a href=\"https://quickgit.kde.org/?p=krita.git\"><span style=\" text-decoration: underline; color:%1;\">Source Code</span></a></p>"

                           "<p><a href=\"https://store.steampowered.com/app/280680/\"><span style=\" text-decoration: underline; color:%1;\">Krita on Steam</span></a></p>"
                           "</body>"
                           "</html>", color));

    lblLinks->setVisible(true);

    updateText();
}


void KisSplashScreen::displayRecentFiles() {
    lblRecent->setVisible(true);
    line->setVisible(true);
}



QString KisSplashScreen::colorString() const
{
    QString color = "#FFFFFF";
    if (m_themed && qApp->palette().background().color().value() > 100) {
        color = "#000000";
    }

    return color;
}


void KisSplashScreen::repaint()
{
    QWidget::repaint();
    QApplication::flush();
}

void KisSplashScreen::show()
{
    QRect r(QPoint(), sizeHint());
    resize(r.size());
    move(QApplication::desktop()->availableGeometry().center() - r.center());
    if (isVisible()) {
        repaint();
    }
    m_timer.setSingleShot(true);
    m_timer.start(1);
    QWidget::show();
}

void KisSplashScreen::toggleShowAtStartup(bool toggle)
{
    KConfigGroup cfg( KSharedConfig::openConfig(), "SplashScreen");
    cfg.writeEntry("HideSplashAfterStartup", toggle);
}

void KisSplashScreen::linkClicked(const QString &link)
{
    KisPart::instance()->openExistingFile(QUrl::fromLocalFile(link));
    if (isTopLevel()) {
        close();
    }
}
