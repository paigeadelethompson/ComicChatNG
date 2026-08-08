#include "roomwidget.h"
#include "artmanager.h"
#include "avatar.h"
#include "backdrop.h"
#include "emotionpicker.h"
#include "pageview.h"
#include "panel.h"
#include "rules.h"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QIcon>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QTimer>
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
    m_comicScroll = new QScrollArea;
    m_comicScroll->setWidget(m_pageView);
    m_comicScroll->setWidgetResizable(false);
    m_comicScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_comicScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_textView = new QPlainTextEdit;
    m_textView->setReadOnly(true);
    m_textView->setMaximumBlockCount(5000);

    m_members = new QListWidget;
    m_members->setMinimumWidth(150);
    m_members->setIconSize(QSize(44, 44));

    m_preview = new QLabel;
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumHeight(140);
    m_preview->setStyleSheet(QStringLiteral("QLabel { background: #ddd; border: 1px solid #888; }"));

    m_emotionPicker = new EmotionPicker;
    connect(m_emotionPicker, &EmotionPicker::emotionChanged, this, &RoomWidget::onEmotionChanged);

    auto *right = new QWidget;
    auto *rightLay = new QVBoxLayout(right);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->addWidget(new QLabel(tr("Members")));
    rightLay->addWidget(m_members, 1);
    rightLay->addWidget(new QLabel(tr("You")));
    rightLay->addWidget(m_preview);
    rightLay->addWidget(m_emotionPicker);
    rightLay->setStretchFactor(m_emotionPicker, 0);

    auto *centerSplit = new QSplitter(Qt::Vertical);
    centerSplit->addWidget(m_comicScroll);
    centerSplit->addWidget(m_textView);
    centerSplit->setStretchFactor(0, 4);
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
    refreshSelfPreview();
}

void RoomWidget::setComicMode(bool on)
{
    m_comicScroll->setVisible(on);
    m_textView->setVisible(!on);
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
    applyMemberIcon(m_members->count() - 1);
}

void RoomWidget::applyMemberIcon(int row)
{
    QListWidgetItem *item = m_members->item(row);
    if (!item)
        return;
    const QString name = m_userAvatars.value(item->text().toLower());
    if (name.isEmpty()) {
        item->setIcon(QIcon());
        return;
    }
    if (Avatar *av = m_art->avatar(name)) {
        const QImage icon = av->iconImage();
        if (!icon.isNull())
            item->setIcon(QPixmap::fromImage(icon.scaled(40, 40, Qt::KeepAspectRatio,
                                                          Qt::SmoothTransformation)));
    }
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
        } else if (cmd.startsWith(QLatin1String("think "), Qt::CaseInsensitive)) {
            const QString thought = cmd.mid(6);
            if (!localOnly)
                m_irc->sendPrivmsg(m_channel, thought);
            onThink(m_channel, m_settings->nick, thought);
        } else if (cmd.startsWith(QLatin1String("appear "), Qt::CaseInsensitive)) {
            const QString name = cmd.mid(7).trimmed();
            if (name.isEmpty()) {
                appendText(tr("/appear <avatar>"));
            } else if (!m_art->avatarOrRandom(name)) {
                appendText(tr("no such art: %1").arg(name));
            } else {
                m_settings->avatarName = name;
                m_settings->save();
                m_userAvatars.insert(m_settings->nick.toLower(), name);
                if (!localOnly)
                    m_irc->announceAppearance(m_channel, name);
                onAppearsAs(m_settings->nick, name);
                appendText(tr("* you now appear as %1").arg(name));
            }
        } else if (cmd.startsWith(QLatin1String("whisper "), Qt::CaseInsensitive)) {
            const QString rest = cmd.mid(7);
            const int sp = rest.indexOf(QLatin1Char(' '));
            const QString who = sp >= 0 ? rest.left(sp) : rest;
            const QString wtext = sp >= 0 ? rest.mid(sp + 1) : QString();
            if (who.isEmpty() || wtext.isEmpty()) {
                appendText(tr("/whisper <nick> <message>"));
            } else {
                if (!localOnly)
                    m_irc->sendPrivmsg(who, wtext);
                onWhisper(m_settings->nick, who, wtext);
                appendText(QStringLiteral("[whisper to %1] %2").arg(who, wtext));
            }
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
    addComicBalloon(nick, isAction ? QStringLiteral("* %1 %2").arg(nick, text) : text,
                    isAction ? BalloonKind::Action : BalloonKind::Say);
}

void RoomWidget::addComicBalloon(const QString &nick, const QString &text, BalloonKind kind)
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
    // When the user has actively chosen an expression via the emotion joystick,
    // their own character uses it instead of the keyword heuristic.
    ch.emotion = (nick.compare(m_settings->nick, Qt::CaseInsensitive) == 0
                      && m_selfEmotion.intensity > 0.01f)
        ? m_selfEmotion
        : EmotionRules::analyze(text);
    if (Avatar *av = m_art->avatarOrRandom(ch.avatarName)) {
        RenderedBody body = av->renderForEmotion(ch.emotion);
        ch.body = body.image;
        ch.faceTip = body.faceTip;
        m_userAvatars.insert(nick.toLower(), av->fileName());
    }
    panel.characters.append(ch);

    Balloon balloon;
    balloon.kind = kind;
    balloon.speaker = nick;
    balloon.text = text;
    panel.balloons.append(balloon);

    m_pageView->addPanel(panel);
    // Keep the newest panel in view.
    QTimer::singleShot(0, this, [this] {
        QScrollBar *sb = m_comicScroll->verticalScrollBar();
        sb->setValue(sb->maximum());
    });
}

void RoomWidget::onEmotionChanged(const Emotion &e)
{
    m_selfEmotion = e;
    refreshSelfPreview();
}

void RoomWidget::refreshSelfPreview()
{
    const QString name = avatarFor(m_settings->nick);
    if (Avatar *av = m_art->avatar(name)) {
        const QImage img = av->renderForEmotion(m_selfEmotion).image;
        if (!img.isNull())
            m_preview->setPixmap(QPixmap::fromImage(img.scaled(96, 120, Qt::KeepAspectRatio,
                                                               Qt::SmoothTransformation)));
    }
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

void RoomWidget::onThink(const QString &channel, const QString &nick, const QString &text)
{
    const bool local = m_channel.compare(QStringLiteral("#local"), Qt::CaseInsensitive) == 0;
    if (!local && channel.compare(m_channel, Qt::CaseInsensitive) != 0)
        return;
    ensureMember(nick);
    appendText(QStringLiteral("(%1 %2)").arg(nick, text));
    if (m_settings->comicView)
        addComicBalloon(nick, text, BalloonKind::Think);
}

void RoomWidget::onWhisper(const QString &sender, const QString &who, const QString &text)
{
    // Whisper shows a subtle balloon; the target isn't part of the panel.
    Q_UNUSED(who);
    ensureMember(sender);
    if (m_settings->comicView)
        addComicBalloon(sender, QStringLiteral("%1 %2").arg(sender, text), BalloonKind::Whisper);
}

void RoomWidget::onUserJoined(const QString &channel, const QString &nick)
{
    if (channel.compare(m_channel, Qt::CaseInsensitive) != 0)
        return;
    ensureMember(nick);
    if (nick.compare(m_settings->nick, Qt::CaseInsensitive) == 0 && m_irc && m_irc->isConnected()) {
        const QString av = avatarFor(nick);
        m_irc->announceAppearance(m_channel, av);
        m_irc->announceBackdrop(m_channel, m_roomBackdrop);
        onAppearsAs(nick, av);
    }
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
    for (int i = 0; i < m_members->count(); ++i) {
        if (m_members->item(i)->text().compare(nick, Qt::CaseInsensitive) == 0) {
            applyMemberIcon(i);
            break;
        }
    }
    appendText(QStringLiteral("* %1 appears as %2").arg(nick, avatarName));
    if (nick.compare(m_settings->nick, Qt::CaseInsensitive) == 0)
        refreshSelfPreview();
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
