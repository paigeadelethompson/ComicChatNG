#pragma once

#include "appsettings.h"

#include <QDialog>

class QLineEdit;
class QSpinBox;
class QComboBox;

class ConnectDialog : public QDialog {
  Q_OBJECT
public:
  explicit ConnectDialog(const AppSettings &settings,
                         const QStringList &avatars, QWidget *parent = nullptr);

  AppSettings resultSettings() const;

private:
  QLineEdit *m_server = nullptr;
  QSpinBox *m_port = nullptr;
  QLineEdit *m_channel = nullptr;
  QLineEdit *m_nick = nullptr;
  QComboBox *m_avatar = nullptr;
};
