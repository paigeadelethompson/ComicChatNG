#include "setupdialog.h"
#include "artmanager.h"
#include "avatar.h"
#include "backdrop.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPixmap>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

SetupDialog::SetupDialog(AppSettings *settings, ArtManager *art, QWidget *parent)
    : QDialog(parent)
    , m_settings(settings)
    , m_art(art)
{
    setWindowTitle(tr("Chat Options"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/ruleset.png")));
    resize(520, 420);

    auto *tabs = new QTabWidget;
    tabs->addTab(buildSettingsPage(), tr("&Settings"));
    tabs->addTab(buildPersonalPage(), tr("&Personal Info"));
    tabs->addTab(buildCharacterPage(), tr("&Character"));
    tabs->addTab(buildBackgroundPage(), tr("&Background"));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &SetupDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabs, 1);
    layout->addWidget(buttons);
}

QWidget *SetupDialog::buildSettingsPage()
{
    auto *page = new QWidget;

    auto *conn = new QGroupBox(tr("Connection"));
    m_noComics = new QCheckBox(tr("Don't send Microsoft Chat specific information"));
    auto *connLay = new QVBoxLayout(conn);
    connLay->addWidget(m_noComics);

    m_allowWhispers = new QCheckBox(tr("Allow &whispers"));
    m_allowWhispers->setChecked(true);
    m_playSounds = new QCheckBox(tr("Pla&y sounds"));
    m_showArrivals = new QCheckBox(tr("S&how arrivals/departures"));
    m_showArrivals->setChecked(true);
    m_comicView = new QCheckBox(tr("Use &Comic View"));
    m_comicView->setChecked(m_settings->comicView);

    auto *grid = new QGridLayout;
    grid->addWidget(m_allowWhispers, 0, 0);
    grid->addWidget(m_playSounds, 0, 1);
    grid->addWidget(m_showArrivals, 1, 0);
    grid->addWidget(m_comicView, 1, 1);

    m_artDir = new QLineEdit(m_settings->artDirectory);
    auto *browse = new QPushButton(tr("..."));
    connect(browse, &QPushButton::clicked, this, &SetupDialog::browseArtDir);
    auto *artRow = new QHBoxLayout;
    artRow->addWidget(m_artDir, 1);
    artRow->addWidget(browse);
    auto *artLabel = new QLabel(tr("Comic art &path:"));

    auto *lay = new QVBoxLayout(page);
    lay->addWidget(conn);
    lay->addLayout(grid);
    lay->addWidget(artLabel);
    lay->addLayout(artRow);
    lay->addStretch(1);
    return page;
}

QWidget *SetupDialog::buildPersonalPage()
{
    auto *page = new QWidget;

    m_realName = new QLineEdit(m_settings->realName);
    m_nickEdit = new QLineEdit(m_settings->nick);
    m_email = new QLineEdit;
    m_email->setPlaceholderText(tr("you@example.com"));
    m_homepage = new QLineEdit;
    m_homepage->setPlaceholderText(tr("http://"));

    auto *form = new QFormLayout(page);
    form->addRow(tr("&Real name:"), m_realName);
    form->addRow(tr("&Nickname:"), m_nickEdit);
    form->addRow(tr("&E-mail address:"), m_email);
    form->addRow(tr("&WWW Home Page:"), m_homepage);
    form->addRow(tr("Brief description of yourself:"), new QLineEdit);
    return page;
}

QWidget *SetupDialog::buildCharacterPage()
{
    auto *page = new QWidget;

    m_avatarList = new QListWidget;
    m_avatarList->addItems(m_art->avatarNames());
    int row = 0;
    for (int i = 0; i < m_avatarList->count(); ++i) {
        if (m_avatarList->item(i)->text().compare(m_settings->avatarName, Qt::CaseInsensitive) == 0) {
            row = i;
            break;
        }
    }
    m_avatarList->setCurrentRow(row);
    connect(m_avatarList, &QListWidget::currentRowChanged, this, &SetupDialog::onAvatarRow);

    m_avatarPreview = new QLabel;
    m_avatarPreview->setFixedSize(96, 130);
    m_avatarPreview->setAlignment(Qt::AlignCenter);
    m_avatarPreview->setStyleSheet(QStringLiteral("QLabel { background: #fff; border: 1px solid #888; }"));
    refreshAvatarPreview();

    auto *lay = new QHBoxLayout(page);
    lay->addWidget(m_avatarList, 1);
    lay->addWidget(m_avatarPreview);
    return page;
}

QWidget *SetupDialog::buildBackgroundPage()
{
    auto *page = new QWidget;

    m_backdropList = new QListWidget;
    m_backdropList->addItems(m_art->backdropNames());
    int row = 0;
    for (int i = 0; i < m_backdropList->count(); ++i) {
        if (m_backdropList->item(i)->text().compare(m_settings->backdropName, Qt::CaseInsensitive) == 0) {
            row = i;
            break;
        }
    }
    m_backdropList->setCurrentRow(row);
    connect(m_backdropList, &QListWidget::currentRowChanged, this, &SetupDialog::onBackdropRow);

    m_backdropPreview = new QLabel;
    m_backdropPreview->setFixedSize(140, 100);
    m_backdropPreview->setAlignment(Qt::AlignCenter);
    m_backdropPreview->setStyleSheet(QStringLiteral("QLabel { background: #fff; border: 1px solid #888; }"));
    refreshBackdropPreview();

    auto *lay = new QHBoxLayout(page);
    lay->addWidget(m_backdropList, 1);
    lay->addWidget(m_backdropPreview);
    return page;
}

void SetupDialog::onAvatarRow(int row)
{
    if (row >= 0)
        m_settings->avatarName = m_avatarList->item(row)->text();
    refreshAvatarPreview();
}

void SetupDialog::onBackdropRow(int row)
{
    if (row >= 0)
        m_settings->backdropName = m_backdropList->item(row)->text();
    refreshBackdropPreview();
}

void SetupDialog::refreshAvatarPreview()
{
    const QString name = m_avatarList->currentItem() ? m_avatarList->currentItem()->text()
                                                     : m_settings->avatarName;
    if (Avatar *av = m_art->avatar(name)) {
        const QImage img = av->iconImage();
        if (!img.isNull()) {
            m_avatarPreview->setPixmap(QPixmap::fromImage(img.scaled(96, 130,
                                                                     Qt::KeepAspectRatio,
                                                                     Qt::SmoothTransformation)));
            return;
        }
    }
    m_avatarPreview->setPixmap(QPixmap());
}

void SetupDialog::refreshBackdropPreview()
{
    const QString name = m_backdropList->currentItem() ? m_backdropList->currentItem()->text()
                                                       : m_settings->backdropName;
    if (Backdrop *bd = m_art->backdrop(name)) {
        const QImage img = bd->image();
        if (!img.isNull()) {
            m_backdropPreview->setPixmap(QPixmap::fromImage(img.scaled(140, 100,
                                                                       Qt::KeepAspectRatio,
                                                                       Qt::SmoothTransformation)));
            return;
        }
    }
    m_backdropPreview->setPixmap(QPixmap());
}

void SetupDialog::browseArtDir()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Select ComicArt folder"),
                                                          m_artDir->text());
    if (!dir.isEmpty())
        m_artDir->setText(dir);
}

void SetupDialog::accept()
{
    m_settings->nick = m_nickEdit->text().trimmed();
    m_settings->realName = m_realName->text().trimmed();
    m_settings->artDirectory = m_artDir->text().trimmed();
    if (m_avatarList->currentItem())
        m_settings->avatarName = m_avatarList->currentItem()->text();
    if (m_backdropList->currentItem())
        m_settings->backdropName = m_backdropList->currentItem()->text();
    m_settings->comicView = m_comicView ? m_comicView->isChecked() : m_settings->comicView;
    m_settings->save();

    m_art->setArtDirectory(m_settings->artDirectory);
    m_art->scan();
    QDialog::accept();
}