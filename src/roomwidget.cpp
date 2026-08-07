#include "roomwidget.h"
#include "artmanager.h"
#include "avatar.h"
#include "backdrop.h"
#include "pageview.h"
#include "panel.h"
#include "rules.h"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>

RoomWidget::RoomWidget(const QString &channel, ArtManager *art, AppSettings *settings,
                       IrcClient *irc, QWidget *parent)
    : QWidget(parent)
    , m_channel(channel)
    , m_art(art)
    , m_settings(settings)
    , m_irc(irc)
    , m_roomBackdrop(settings->backdropName)
{
    m_pageView = new PageView;
    auto *comicScroll = new QScrollArea;
    comicScroll->setWidget(m_pageView);
    comicScroll->setWidgetResizable(true);
    m_comicPane = comicScroll;

    m_textView = new QPlainTextEdit;
    m_textView->setReadOnly(true);
    m_textView->setMaximumBlockCount(5000);

    m_members = new QListWidget;
    m_members->setMinimumWidth(140);

    m_preview = new QLabel;
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumHeight(120);
    m_preview->setStyleSheet(QStringLiteral("QLabel { background: #ddd; border: 1px solid #888; }"));

    if (Avatar *selfAv = m_art->avatar(m_settings->avatarName)) {
        const QImage icon = selfAv->iconImage();
        if (!icon.isNull())
            m_preview->setPixmap(QPixmap::fromImage(icon.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    }

    auto *right = new QWidget;
    auto *rightLay = new QVBoxLayout(right);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->addWidget(new QLabel(tr("Members")));
    rightLay->addWidget(m_members, 1);
    rightLay->addWidget(new QLabel(tr("You")));
    rightLay->addWidget(m_preview);

    auto *centerSplit = new QSplitter(Qt::Vertical);
    centerSplit->addWidget(m_comicPane);
    centerSplit->addWidget(m_textView);
    centerSplit->setStretchFactor(0, 3);
    centerSplit->setStretchFactor(1, 1);

    m_mainSplit = new QSplitter(Qt::Horizontal);
    m_mainSplit->addWidget(centerSplit);
    m_mainSplit->addWidget(right);
    m_mainSplit->setStretchFactor(0, 4);
    m_mainSplit->setStretchFactor(1, 1);

    m_sayEdit = new QLineEdit;
    m_sayEdit->setPlaceholderText(tr("Say something…"));
    auto *sendBtn = new QPushButton(tr("Send"));
    connect(m_sayEdit, &QLineEdit::returnPressed, this, &RoomWidget::sendSay);
    connect(sendBtn, &QPushButton::clicked, this, &RoomWidget::sendSay);

    auto *sayRow = new QHBoxLayout;
    sayRow->addWidget(m_sayEdit, 1);
    sayRow->addWidget(sendBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_mainSplit, 1);
    layout->addLayout(sayRow);

    setComicMode(m_settings->comicView);

    m_userAvatars.insert(m_settings->nick.toLower(), m_settings->avatarName);
}

void RoomWidget::setComicMode(bool on)
{
    m_comicPane->setVisible(on);
}

void RoomWidget::appendText(const QString &line)
{
    m_textView->appendPlainText(line);
}

void RoomWidget::ensureMember(const QString &nick)
{
    for (int i = 0; i < m_members->count(); ++i) {
        if (m_members->item(i)->text().compare(nick, Qt::CaseInsensitive) == 0)
            return;
    }
    m_members->addItem(nick);
    if (!m_userAvatars.contains(nick.toLower()))
        m_userAvatars.insert(nick.toLower(), m_art->nextAvatarName());
}

void RoomWidget::removeMember(const QString &nick)
{
    for (int i = 0; i < m_members->count(); ++i) {
        if (m_members->item(i)->text().compare(nick, Qt::CaseInsensitive) == 0) {
            delete m_members->takeItem(i);
            break;
        }
    }
}

QString RoomWidget::avatarFor(const QString &nick) const
{
    return m_userAvatars.value(nick.toLower(), m_settings->avatarName);
}

void RoomWidget::sendSay()
{
    const QString text = m_sayEdit->text().trimmed();
    if (text.isEmpty())
        return;

    const bool localOnly = m_channel.compare(QStringLiteral("#local"), Qt::CaseInsensitive) == 0
        || !m_irc || !m_irc->isConnected();

    if (text.startsWith(QLatin1Char('/'))) {
        const QString cmd = text.mid(1);
        if (cmd.startsWith(QLatin1String("me "), Qt::CaseInsensitive)) {
            const QString act = cmd.mid(3);
            if (!localOnly)
                m_irc->sendAction(m_channel, act);
            onAction(m_channel, m_settings->nick, act);
        } else if (!localOnly) {
            m_irc->sendRaw(cmd);
            appendText(QStringLiteral(">>> %1").arg(cmd));
        } else {
            appendText(tr("(offline) /%1").arg(cmd));
        }
    } else {
        if (!localOnly)
            m_irc->sendPrivmsg(m_channel, text);
        onPrivmsg(m_channel, m_settings->nick, text);
    }
    m_sayEdit->clear();
}

void RoomWidget::addComicLine(const QString &nick, const QString &text, bool isAction)
{
    ComicPanel panel;
    panel.size = QSize(340, 260);

    if (Backdrop *bd = m_art->backdrop(m_roomBackdrop))
        panel.backdrop = bd->image();
    else if (Backdrop *bd = m_art->defaultBackdrop())
        panel.backdrop = bd->image();

    PanelCharacter ch;
    ch.nick = nick;
    ch.avatarName = avatarFor(nick);
    ch.emotion = EmotionRules::analyze(text);
    if (Avatar *av = m_art->avatarOrRandom(ch.avatarName)) {
        RenderedBody body = av->renderForEmotion(ch.emotion);
        ch.body = body.image;
        ch.faceTip = body.faceTip;
        m_userAvatars.insert(nick.toLower(), av->fileName());
    }
    panel.characters.append(ch);

    Balloon balloon;
    balloon.kind = isAction ? BalloonKind::Action : BalloonKind::Say;
    balloon.speaker = nick;
    balloon.text = isAction ? QStringLiteral("* %1 %2").arg(nick, text) : text;
    panel.balloons.append(balloon);

    m_pageView->addPanel(panel);
}

void RoomWidget::onPrivmsg(const QString &channel, const QString &nick, const QString &text)
{
    const bool local = m_channel.compare(QStringLiteral("#local"), Qt::CaseInsensitive) == 0;
    if (!local) {
        if (channel.compare(m_channel, Qt::CaseInsensitive) != 0
            && channel.compare(m_settings->nick, Qt::CaseInsensitive) != 0)
            return;
    }
    ensureMember(nick);
    appendText(QStringLiteral("<%1> %2").arg(nick, text));
    if (m_settings->comicView)
        addComicLine(nick, text, false);
}

void RoomWidget::onAction(const QString &channel, const QString &nick, const QString &text)
{
    const bool local = m_channel.compare(QStringLiteral("#local"), Qt::CaseInsensitive) == 0;
    if (!local && channel.compare(m_channel, Qt::CaseInsensitive) != 0)
        return;
    ensureMember(nick);
    appendText(QStringLiteral("* %1 %2").arg(nick, text));
    if (m_settings->comicView)
        addComicLine(nick, text, true);
}

void RoomWidget::onUserJoined(const QString &channel, const QString &nick)
{
    if (channel.compare(m_channel, Qt::CaseInsensitive) != 0)
        return;
    ensureMember(nick);
    appendText(QStringLiteral("* %1 has joined %2").arg(nick, channel));
}

void RoomWidget::onUserParted(const QString &channel, const QString &nick, const QString &reason)
{
    if (channel.compare(m_channel, Qt::CaseInsensitive) != 0)
        return;
    removeMember(nick);
    appendText(QStringLiteral("* %1 has left (%2)").arg(nick, reason));
}

void RoomWidget::onUserQuit(const QString &nick, const QString &reason)
{
    removeMember(nick);
    appendText(QStringLiteral("* %1 has quit (%2)").arg(nick, reason));
}

void RoomWidget::onNickChanged(const QString &oldNick, const QString &newNick)
{
    for (int i = 0; i < m_members->count(); ++i) {
        if (m_members->item(i)->text().compare(oldNick, Qt::CaseInsensitive) == 0) {
            m_members->item(i)->setText(newNick);
            break;
        }
    }
    if (m_userAvatars.contains(oldNick.toLower())) {
        m_userAvatars.insert(newNick.toLower(), m_userAvatars.take(oldNick.toLower()));
    }
    appendText(QStringLiteral("* %1 is now known as %2").arg(oldNick, newNick));
}

void RoomWidget::onNamesList(const QString &channel, const QStringList &nicks)
{
    if (channel.compare(m_channel, Qt::CaseInsensitive) != 0)
        return;
    m_members->clear();
    for (const QString &n : nicks)
        ensureMember(n);
}

void RoomWidget::onAppearsAs(const QString &nick, const QString &avatarName)
{
    m_userAvatars.insert(nick.toLower(), avatarName);
    ensureMember(nick);
    appendText(QStringLiteral("* %1 appears as %2").arg(nick, avatarName));
}

void RoomWidget::onBackdropAnnounce(const QString &nick, const QString &backdropName)
{
    Q_UNUSED(nick);
    m_roomBackdrop = backdropName;
    appendText(QStringLiteral("* Backdrop set to %1").arg(backdropName));
}

void RoomWidget::onServerMessage(const QString &text)
{
    appendText(text);
}
