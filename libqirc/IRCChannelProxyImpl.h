/* qirc - Qt based IRC client
 * Copyright (C) 2012 Jacob Dawid (jacob.dawid@googlemail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef IRCCHANNELPROXYIMPL_H
#define IRCCHANNELPROXYIMPL_H

#include "IRCChannelProxyInterface.h"

/**
  * \class IRCChannelProxyImpl
  * Implements a handle to an IRC channel. This is usually provided by the
  * the IRC client class.
  */
class IRCChannelProxyImpl : public IRCChannelProxyInterface
{
    Q_OBJECT
public:
    IRCChannelProxyImpl (IRCClientInterface *clientInterface, const QString& channelName, QObject *parent = 0);
    QTextDocument *conversationModel ();
    QStringListModel *userListModel ();
    QString channelName ();

    void setNickList (const QStringList &nickList);
    void sendMessage (const QString& message);
    void sendJoinRequest ();
    void leave (const QString &reason);
public slots:
    void handleNickChange (const QString& oldNick, const QString& newNick);
    void handleJoin (const QString& nick);
private:
    QString             m_channelName;
    QStringList         m_userList;
    QStringListModel    m_userListModel;
    QTextDocument       m_conversationModel;
    IRCClientInterface *m_clientInterface;
};

#endif // IRCCHANNELPROXYIMPL_H
