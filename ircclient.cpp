/* QtIRC - Qt based IRC client
 * Copyright (C) 2012-2015 Jacob Dawid (jacob@omg-it.works)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or(at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// Own includes
#include "ircclient.h"

IRCClient::IRCClient(QObject *parent) :
    QObject(parent) {
    m_loggedIn = false;
    connect(&m_tcpSocket, SIGNAL(connected()), this, SLOT(handleConnected()));
    connect(&m_tcpSocket, SIGNAL(disconnected()), this, SLOT(handleDisconnected()));
    connect(&m_tcpSocket, SIGNAL(readyRead()), this, SLOT(handleReadyRead()));
}

IRCClient::~IRCClient()
{
    foreach(IRCChannel *ircChannelProxy, m_channels)
    {
        delete ircChannelProxy;
    }
}

void
IRCClient::connectToHost(const QHostAddress& host, quint16 port, const QString& initialNick)
{
    m_host = host;
    m_nickname = initialNick;
    m_tcpSocket.connectToHost(host, port);
}

void
IRCClient::disconnect()
{
    m_tcpSocket.disconnect();
}

void
IRCClient::reconnect()
{
    disconnect();
    connectToHost(m_host, m_port, m_nickname);
}

bool
IRCClient::isConnected()
{
    return m_connected;
}

bool
IRCClient::isLoggedIn()
{
    return m_loggedIn;
}

const QHostAddress&
IRCClient::host()
{
    return m_host;
}

int
IRCClient::port()
{
    return m_port;
}

IRCChannel *IRCClient::ircChannel(const QString &channel)
{
    if(!m_channels.contains(channel))
        m_channels[channel] = new IRCChannel(this, channel);
    return m_channels[channel];
}

void
IRCClient::sendNicknameChangeRequest(const QString &nickname)
{
    sendIRCCommand(IRCCommand::Nick, QStringList(nickname));
}

void
IRCClient::sendPrivateMessage(const QString &recipient, const QString &message)
{
    QStringList arguments;
    arguments << recipient;
    arguments << message;
    sendIRCCommand(IRCCommand::PrivateMessage, arguments);
}

const QString&
IRCClient::nickname()
{
    return m_nickname;
}

void
IRCClient::handleConnected()
{
    m_connected = true;
    QStringList arguments;
    arguments << "na" << "0" << "0" << "na";
    sendIRCCommand(IRCCommand::User, arguments);
    sendNicknameChangeRequest(m_nickname);
    emit connected(m_host.toString());
}

void
IRCClient::handleDisconnected()
{
    m_connected = false;
    emit disconnected();
}

void
IRCClient::handleReadyRead()
{
    QByteArray line;
    do
    {
        line = m_tcpSocket.readLine();
        if(line.size())
            handleIncomingLine(QString::fromUtf8(line.data()));
        else
            break;
    }
    while(true);
}

void
IRCClient::handleNicknameChanged(const QString &oldNick, const QString &newNick)
{
    // Check if our nickname changed.
    if(oldNick == m_nickname)
    {
        m_nickname = newNick;
        emit userNicknameChanged(m_nickname);
    }
    emit nicknameChanged(oldNick, newNick);
}

void
IRCClient::handleUserJoined(const QString &nick, const QString &channel)
{
    ircChannel(channel)->handleJoin(nick);
    emit userJoined(nick, channel);
}

void
IRCClient::handleUserQuit(const QString &nick, const QString &reason)
{
    emit userQuit(nick, reason);
}

void
IRCClient::handleIncomingLine(const QString &line)
{
    if(m_connected && !line.isEmpty())
    {
        IRCServerMessage ircServerMessage(line);
        if(ircServerMessage.isNumeric() == true)
        {
            switch(ircServerMessage.numericValue())
            {
            case IRCReply::Welcome:
                m_loggedIn = true;
                emit userNicknameChanged(nickname());
                emit loggedIn(nickname());
                break;
            case IRCError::NicknameInUse:
            case IRCError::NickCollision:
                // If we are already logged in, the user attempted to
                // switch to a username that is already existing.
                // In that case warn him.
                if(isLoggedIn())
                {
                    emit error("The nickname is already in use.");
                }
                // Otherwise we are attempting to log in to the server.
                // Change the nick so that we can at least log in.
                else
                {
                    m_nickname += "_";
                    sendNicknameChangeRequest(m_nickname);
                }
                break;
            case IRCError::PasswordMismatch:
                emit error("The password you provided is not correct.");
                break;
            case IRCReply::MessageOfTheDayStart:
            case IRCReply::MessageOfTheDay:
            case IRCReply::MessageOfTheDayEnd:
            case IRCError::NoMessageOfTheDay:
                break;
            case IRCReply::NoTopic:
            case IRCReply::Topic:
                break;
            case IRCReply::NameReply:
                QString channel = ircServerMessage.parameter(2);
                QString nickList = ircServerMessage.parameter(3);
                ircChannel(channel)
                    ->nameReply(nickList.split(
                        QRegExp("\\s+"), QString::SkipEmptyParts));
                break;
            }
        }
        else
        {
            QString command = ircServerMessage.command();
            if(command == IRCCommand::Nick)
            {
                handleNicknameChanged(ircServerMessage.nick(), ircServerMessage.parameter(0));
            }
            else if(command == IRCCommand::Quit)
            {
                handleUserQuit(ircServerMessage.nick(), ircServerMessage.parameter(0));
            }
            else if(command == IRCCommand::Join)
            {
                handleUserJoined(ircServerMessage.nick(), ircServerMessage.parameter(0));
            }
            else if(command == IRCCommand::Part)
            {
                emit debugMessage("WRITEME: Received part.");
                //emit part(ircEvent.getNick().toStdString().c_str(),
                //           ircEvent.getParam(0).toStdString().c_str(),
                //           ircEvent.getParam(1).toStdString().c_str());
            }
            else if(command == IRCCommand::Mode)
            {
                emit debugMessage("WRITEME: Received mode.");
                //emit mode(&ircEvent);
            }
            else if(command == IRCCommand::Topic)
            {
                emit debugMessage
                       (QString("WRITEME: Received topic: %1")
                         .arg(ircServerMessage.parameter(0)));
            }
            else if(command == IRCCommand::Kick)
            {
                emit debugMessage("WRITEME: Received kick command.");
            }
            else if(command == IRCCommand::Invite)
            {
                emit debugMessage("WRITEME: Received invite command.");

            }
            else if(command == IRCCommand::PrivateMessage)
            {
                IRCChannel *channel = ircChannel(ircServerMessage.parameter(0));
                if(channel) {
                    channel->handleMessage(ircServerMessage.nick(), ircServerMessage.parameter(1));
                }
            }
            else if(command == IRCCommand::Notice)
            {
                emit notification(ircServerMessage.nick(), ircServerMessage.parameter(1));
            }
            else if(command == IRCCommand::Ping)
            {
                sendIRCCommand(IRCCommand::Pong, QStringList(m_nickname));
            }
            else if(command == IRCCommand::Error)
            {
                emit error(ircServerMessage.parameter(0));
            }
            else
            {
                emit debugMessage(QString("FIXME: Received unknown reply: %1").arg(command));
            }
        }
    }
}

void
IRCClient::sendLine(const QString &line)
{
    if(m_connected)
        m_tcpSocket.write((line +  + "\r\n").toUtf8());
}

void
IRCClient::sendIRCCommand(const QString &command, const QStringList &arguments)
{
    QString line = command;
    for(int i = 0; i < arguments.size(); i++)
    {
        bool applyColon = false;
        // Usually all parameters are separated by spaces.
        // The last parameter of the message may contain spaces, it is usually used
        // to transmit messages. In order to parse it correctly, if needs to be prefixed
        // with a colon, so the server knows to ignore all forthcoming spaces and has to treat
        // all remaining characters as a single parameter. If we detect any whitespace in the
        // last argument, prefix it with a colon:
        if((i == arguments.size() - 1) && arguments.at(i).contains(QRegExp("\\s")))
            applyColon = true;
        line += QString(" %1%2").arg(applyColon ? ":" : "").arg(arguments.at(i));
    }
    sendLine(line);
}
