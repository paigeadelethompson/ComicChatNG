#include "roomwidget.h"
#include "artmanager.h"
#include "avatar.h"
#include "backdrop.h"
#include "emotionpicker.h"
#include "pageview.h"
#include "panel.h"
#include "rules.h"

#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QTimer>
#include <QToolButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "icons.h"

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
    m_comicScroll->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_comicScroll, &QScrollArea::customContextMenuRequested,
            this, &RoomWidget::showRoomMenu);

    m_textView = new QPlainTextEdit;
    m_textView->setReadOnly(true);
    m_textView->setMaximumBlockCount(5000);

    m_members = new QListWidget;
    m_members->setMinimumWidth(150);
    m_members->setIconSize(QSize(44, 44));
    m_members->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_members, &QListWidget::customContextMenuRequested,
            this, &RoomWidget::showMemberMenu);

    m_preview = new QLabel;
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumHeight(140);
    m_preview->setStyleSheet(QStringLiteral("QLabel { background: #ddd; border: 1px solid #888; }"));

    m_emotionPicker = new EmotionPicker;
    connect(m_emotionPicker, &EmotionPicker::emotionChanged, this, &RoomWidget::onEmotionChanged);
    connect(this, &RoomWidget::clearHistoryRequested, this, [this] {
        m_textView->clear();
    });

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

    // The classic Chat "action" buttons: Say / Think / Whisper / Action,
    // with the original balloon pictures from res\balloons.bmp.
    struct ModeButton { int mode; const char *tip; int icon; };
    const ModeButton modes[] = {
        { 0, QT_TR_NOOP("Say"), 0 },
        { 1, QT_TR_NOOP("Think"), 1 },
        { 2, QT_TR_NOOP("Whisper"), 2 },
        { 3, QT_TR_NOOP("Action"), 3 },
    };
    QButtonGroup *modeGroup = new QButtonGroup(this);
    modeGroup->setExclusive(true);
    QToolButton *sayBtn = nullptr;
    for (const ModeButton &mb : modes) {
        QToolButton *b = new QToolButton;
        b->setIcon(icons::saybar(mb.icon));
        b->setCheckable(true);
        b->setToolTip(tr(mb.tip));
        b->setAutoRaise(true);
        modeGroup->addButton(b, mb.mode);
        if (mb.mode == 0)
            sayBtn = b;
    }
    connect(modeGroup, &QButtonGroup::idClicked, this, [this](int id) {
        m_sayMode = id;
    });

    auto *sayRow = new QHBoxLayout;
    sayRow->setSpacing(2);
    sayRow->addWidget(modeGroup->button(0));
    sayRow->addWidget(modeGroup->button(1));
    sayRow->addWidget(modeGroup->button(2));
    sayRow->addWidget(modeGroup->button(3));
    sayRow->addWidget(m_sayEdit, 1);
    sayRow->addWidget(sendBtn);
    if (sayBtn)
        sayBtn->setChecked(true);

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
        if (m_sayMode == 3) {
            if (!localOnly)
                m_irc->sendAction(m_channel, text);
            onAction(m_channel, m_settings->nick, text);
        } else if (m_sayMode == 1) {
            if (!localOnly)
                m_irc->sendPrivmsg(m_channel, text);
            onThink(m_channel, m_settings->nick, text);
        } else if (m_sayMode == 2) {
            if (!m_whisperTarget.isEmpty()) {
                if (!localOnly)
                    m_irc->sendPrivmsg(m_whisperTarget, text);
                onWhisper(m_settings->nick, m_whisperTarget, text);
                appendText(QStringLiteral("[whisper to %1] %2").arg(m_whisperTarget, text));
            } else {
                appendText(tr("[whisper] Right-click a member and choose “Whisper…” to pick a target."));
            }
        } else {
            if (!localOnly)
                m_irc->sendPrivmsg(m_channel, text);
            onPrivmsg(m_channel, m_settings->nick, text);
        }
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
    constexpr int kMaxBalloonsPerPanel = 5;

    ComicPanel panel;
    if (m_pageView->panelCount() > 0) {
        const ComicPanel last = m_pageView->lastPanel();
        if (last.balloons.size() < kMaxBalloonsPerPanel
            && !last.containsSpeaker(nick)
            && kind != BalloonKind::Action) {
            panel = last;
        }
    }

    if (Backdrop *bd = m_art->backdrop(m_roomBackdrop))
        panel.backdrop = bd->image();
    else if (Backdrop *bd = m_art->defaultBackdrop())
        panel.backdrop = bd->image();

    // The speaker, plus the previous speaker so the frame reads like a
    // conversation (two characters at most, no duplicate avatars).
    const QStringList actors = panelForActors(nick);

    for (const QString &actor : actors) {
        if (panel.containsSpeaker(actor))
            continue;
        PanelCharacter ch;
        ch.nick = actor;
        ch.avatarName = avatarFor(actor);
        ch.emotion = (actor.compare(m_settings->nick, Qt::CaseInsensitive) == 0
                          && m_selfEmotion.intensity > 0.01f)
            ? m_selfEmotion
            : EmotionRules::analyze(text);
        if (Avatar *av = m_art->avatarOrRandom(ch.avatarName)) {
            RenderedBody body = av->renderForEmotion(ch.emotion);
            ch.body = body.image;
            ch.faceTip = body.faceTip;
            m_userAvatars.insert(actor.toLower(), av->fileName());
        }
        panel.characters.append(ch);
    }

    Balloon balloon;
    balloon.kind = kind;
    balloon.speaker = nick;
    balloon.text = text;
    panel.balloons.append(balloon);

    if (panel.isEmpty())
        m_pageView->addPanel(panel);
    else
        m_pageView->replaceLastPanel(panel);
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

QStringList RoomWidget::panelForActors(const QString &speaker)
{
    QStringList actors;
    actors.append(speaker);
    if (!m_previousSpeaker.isEmpty()
        && m_previousSpeaker.compare(speaker, Qt::CaseInsensitive) != 0)
        actors.append(m_previousSpeaker);
    m_previousSpeaker = speaker;
    return actors;
}

void RoomWidget::applySettings()
{
    m_roomBackdrop = m_settings->backdropName;
    m_userAvatars.insert(m_settings->nick.toLower(), m_settings->avatarName);
    refreshSelfPreview();
}

void RoomWidget::setSelfAvatar(const QString &name)
{
    if (!m_art->avatarOrRandom(name))
        return;
    m_settings->avatarName = name;
    m_settings->save();
    m_userAvatars.insert(m_settings->nick.toLower(), name);
    onAppearsAs(m_settings->nick, name);
}

void RoomWidget::showRoomMenu(const QPoint &gpos)
{
    QMenu menu(this);

    QAction *comic = menu.addAction(tr("Comic Stri&p"));
    comic->setCheckable(true);
    comic->setChecked(m_settings->comicView);
    connect(comic, &QAction::toggled, this, &RoomWidget::setComicMode);

    QAction *text = menu.addAction(tr("Plain Te&xt"));
    text->setCheckable(true);
    text->setChecked(!m_settings->comicView);
    connect(text, &QAction::toggled, this, &RoomWidget::setComicMode);

    menu.addSeparator();
    connect(menu.addAction(tr("&Copy\tCtrl+C")), &QAction::triggered, this, [this] {
        if (m_textView)
            m_textView->copy();
    });
    connect(menu.addAction(tr("Clear &History")), &QAction::triggered, this, [this] {
        if (m_textView)
            m_textView->clear();
        m_pageView->clear();
    });

    menu.addSeparator();
    connect(menu.addAction(tr("&Room Properties...")), &QAction::triggered, this,
            &RoomWidget::showRoomProperties);

    menu.addSeparator();
    QMenu *charMenu = menu.addMenu(tr("&Character"));
    for (const QString &name : m_art->avatarNames()) {
        QAction *a = charMenu->addAction(name);
        a->setCheckable(true);
        a->setChecked(name.compare(m_settings->avatarName, Qt::CaseInsensitive) == 0);
        connect(a, &QAction::triggered, this, [this, name] { setSelfAvatar(name); });
    }
    QMenu *bMenu = menu.addMenu(tr("&Backdrop"));
    for (const QString &name : m_art->backdropNames()) {
        QAction *a = bMenu->addAction(name);
        a->setCheckable(true);
        a->setChecked(name.compare(m_roomBackdrop, Qt::CaseInsensitive) == 0);
        connect(a, &QAction::triggered, this, [this, name] {
            m_roomBackdrop = name;
            if (m_irc && m_irc->isConnected())
                m_irc->announceBackdrop(m_channel, name);
        });
    }
    menu.exec(m_comicScroll->mapToGlobal(gpos));
}

void RoomWidget::showMemberMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_members->itemAt(pos);
    if (!item)
        return;
    const QString nick = item->text();
    QMenu menu(this);

    connect(menu.addAction(tr("&Get Profile")), &QAction::triggered, this, [this, nick] {
        appendText(tr("[profile of %1 not available on this server]").arg(nick));
    });
    connect(menu.addAction(tr("Get &Identity")), &QAction::triggered, this, [this, nick] {
        appendText(tr("[identity of %1 not available on this server]").arg(nick));
    });
    QAction *whisper = menu.addAction(tr("&Whisper Box..."));
    whisper->setIcon(icons::saybar(2));
    connect(whisper, &QAction::triggered, this, [this, nick] {
        m_whisperTarget = nick;
        m_sayMode = 2;
        m_sayEdit->setFocus();
        appendText(tr("[whispering to %1]").arg(nick));
    });
    connect(menu.addAction(tr("Add to Notifi&cations...")), &QAction::triggered,
            this, [this, nick] {
        appendText(tr("[%1 added to notifications]").arg(nick));
    });
    connect(menu.addAction(tr("&Ignore")), &QAction::triggered, this, [this, nick] {
        appendText(tr("[%1 ignored]").arg(nick));
    });
    menu.addSeparator();
    connect(menu.addAction(tr("&Send E-mail")), &QAction::triggered, this, &RoomWidget::notImplemented);
    connect(menu.addAction(tr("Send &File...")), &QAction::triggered, this, &RoomWidget::notImplemented);
    connect(menu.addAction(tr("Visit H&ome Page")), &QAction::triggered, this, &RoomWidget::notImplemented);
    connect(menu.addAction(tr("Net&Meeting")), &QAction::triggered, this, &RoomWidget::notImplemented);
    menu.addSeparator();
    connect(menu.addAction(tr("&Version")), &QAction::triggered, this, &RoomWidget::notImplemented);
    connect(menu.addAction(tr("Lag &Time")), &QAction::triggered, this, &RoomWidget::notImplemented);
    connect(menu.addAction(tr("&Local Time")), &QAction::triggered, this, &RoomWidget::notImplemented);

    menu.addSeparator();
    if (m_userAvatars.contains(nick.toLower())) {
        QMenu *pick = menu.addMenu(tr("&Character"));
        for (const QString &name : m_art->avatarNames()) {
            QAction *a = pick->addAction(name);
            a->setCheckable(true);
            a->setChecked(name.compare(m_settings->avatarName, Qt::CaseInsensitive) == 0);
            connect(a, &QAction::triggered, this, [this, name] { setSelfAvatar(name); });
        }
    }
    menu.exec(m_members->viewport()->mapToGlobal(pos));
}

void RoomWidget::showRoomProperties()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("%1 Properties").arg(m_channel));
    auto *form = new QFormLayout;
    form->addRow(tr("Room:"), new QLabel(m_channel));
    auto *topic = new QLineEdit(m_roomTopic);
    topic->setPlaceholderText(tr("Set the room topic"));
    form->addRow(tr("Topic:"), topic);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    auto *lay = new QVBoxLayout(&dlg);
    lay->addLayout(form);
    lay->addWidget(buttons);
    if (dlg.exec() != QDialog::Accepted)
        return;
    if (m_irc && m_irc->isConnected())
        m_irc->sendRaw(QStringLiteral("TOPIC %1 :%2").arg(m_channel, topic->text()));
    m_roomTopic = topic->text();
    appendText(tr("* Topic is: %1").arg(m_roomTopic));
}

void RoomWidget::notImplemented()
{
    appendText(tr("[not supported by this server]"));
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
