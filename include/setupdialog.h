#pragma once

#include "appsettings.h"

#include <QDialog>

class QLineEdit;
class QListWidget;
class QCheckBox;
class QWidget;
class QTabWidget;
class ArtManager;
class QLabel;

// The old client's "Options" dialog: a tab sheet with
// Settings, Personal Info, Character and Background pages.
class SetupDialog : public QDialog {
  Q_OBJECT
public:
  SetupDialog(AppSettings *settings, ArtManager *art,
              QWidget *parent = nullptr);

public slots:
  void accept() override;

private slots:
  void browseArtDir();
  void onAvatarRow(int row);
  void onBackdropRow(int row);

private:
  QWidget *buildSettingsPage();
  QWidget *buildPersonalPage();
  QWidget *buildCharacterPage();
  QWidget *buildBackgroundPage();
  void refreshAvatarPreview();
  void refreshBackdropPreview();

  AppSettings *m_settings = nullptr;
  ArtManager *m_art = nullptr;

  QLineEdit *m_realName = nullptr;
  QLineEdit *m_nickEdit = nullptr;
  QLineEdit *m_email = nullptr;
  QLineEdit *m_homepage = nullptr;
  QLineEdit *m_artDir = nullptr;
  QListWidget *m_avatarList = nullptr;
  QListWidget *m_backdropList = nullptr;
  QLabel *m_avatarPreview = nullptr;
  QLabel *m_backdropPreview = nullptr;
  QCheckBox *m_noComics = nullptr;
  QCheckBox *m_allowWhispers = nullptr;
  QCheckBox *m_playSounds = nullptr;
  QCheckBox *m_showArrivals = nullptr;
  QCheckBox *m_comicView = nullptr;
};