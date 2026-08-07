#pragma once

#include "appsettings.h"

#include <QDialog>

class QLineEdit;
class QComboBox;
class QCheckBox;
class ArtManager;

class SetupDialog : public QDialog
{
    Q_OBJECT
public:
    SetupDialog(AppSettings *settings, ArtManager *art, QWidget *parent = nullptr);

public slots:
    void accept() override;

private slots:
    void browseArtDir();

private:
    AppSettings *m_settings = nullptr;
    ArtManager *m_art = nullptr;
    QLineEdit *m_nick = nullptr;
    QLineEdit *m_realName = nullptr;
    QLineEdit *m_artDir = nullptr;
    QComboBox *m_avatar = nullptr;
    QComboBox *m_backdrop = nullptr;
    QCheckBox *m_comicView = nullptr;
};
