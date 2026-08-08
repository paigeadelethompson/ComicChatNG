#include "connectdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

ConnectDialog::ConnectDialog(const AppSettings &settings,
                             const QStringList &avatars, QWidget *parent)
    : QDialog(parent) {
  setWindowTitle(tr("Connect to Chat"));
  resize(420, 240);

  m_server = new QLineEdit(settings.server);
  m_port = new QSpinBox;
  m_port->setRange(1, 65535);
  m_port->setValue(settings.port);
  m_channel = new QLineEdit(settings.channel);
  m_nick = new QLineEdit(settings.nick);
  m_avatar = new QComboBox;
  m_avatar->addItems(avatars);
  const int idx = m_avatar->findText(settings.avatarName, Qt::MatchFixedString);
  if (idx >= 0)
    m_avatar->setCurrentIndex(idx);

  auto *form = new QFormLayout;
  form->addRow(tr("Server"), m_server);
  form->addRow(tr("Port"), m_port);
  form->addRow(tr("Chat room"), m_channel);
  form->addRow(tr("Nickname"), m_nick);
  form->addRow(tr("Character"), m_avatar);

  auto *buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  auto *layout = new QVBoxLayout(this);
  layout->addLayout(form);
  layout->addWidget(buttons);
}

AppSettings ConnectDialog::resultSettings() const {
  AppSettings s;
  s.server = m_server->text().trimmed();
  s.port = static_cast<quint16>(m_port->value());
  s.channel = m_channel->text().trimmed();
  s.nick = m_nick->text().trimmed();
  s.avatarName = m_avatar->currentText();
  return s;
}
