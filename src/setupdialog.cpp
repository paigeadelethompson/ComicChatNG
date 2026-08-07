#include "setupdialog.h"
#include "artmanager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

SetupDialog::SetupDialog(AppSettings *settings, ArtManager *art, QWidget *parent)
    : QDialog(parent)
    , m_settings(settings)
    , m_art(art)
{
    setWindowTitle(tr("Settings"));
    resize(480, 300);

    m_nick = new QLineEdit(settings->nick);
    m_realName = new QLineEdit(settings->realName);
    m_artDir = new QLineEdit(settings->artDirectory);
    auto *browse = new QPushButton(tr("Browse…"));
    connect(browse, &QPushButton::clicked, this, &SetupDialog::browseArtDir);

    auto *artRow = new QHBoxLayout;
    artRow->addWidget(m_artDir);
    artRow->addWidget(browse);

    m_avatar = new QComboBox;
    m_avatar->addItems(art->avatarNames());
    int a = m_avatar->findText(settings->avatarName, Qt::MatchFixedString);
    if (a >= 0)
        m_avatar->setCurrentIndex(a);

    m_backdrop = new QComboBox;
    m_backdrop->addItems(art->backdropNames());
    int b = m_backdrop->findText(settings->backdropName, Qt::MatchFixedString);
    if (b >= 0)
        m_backdrop->setCurrentIndex(b);

    m_comicView = new QCheckBox(tr("Show comic view"));
    m_comicView->setChecked(settings->comicView);

    auto *form = new QFormLayout;
    form->addRow(tr("Nickname"), m_nick);
    form->addRow(tr("Real name"), m_realName);
    form->addRow(tr("Art directory"), artRow);
    form->addRow(tr("Character"), m_avatar);
    form->addRow(tr("Backdrop"), m_backdrop);
    form->addRow(QString(), m_comicView);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &SetupDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
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
    m_settings->nick = m_nick->text().trimmed();
    m_settings->realName = m_realName->text().trimmed();
    m_settings->artDirectory = m_artDir->text().trimmed();
    m_settings->avatarName = m_avatar->currentText();
    m_settings->backdropName = m_backdrop->currentText();
    m_settings->comicView = m_comicView->isChecked();
    m_settings->save();

    m_art->setArtDirectory(m_settings->artDirectory);
    m_art->scan();
    QDialog::accept();
}
