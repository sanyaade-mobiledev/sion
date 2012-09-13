/*
 * SION! Client/Server communication library.
 *
 * Copyright (C) Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Gilles Fabre <gilles.fabre@intel.com>
 */

#include <QCoreApplication>

#include "clientserverinterface.h"

ClientServerInterfaceThread::ClientServerInterfaceThread(ClientServerInterface *interfaceP, QSemaphore *socketSemP) : QThread(), m_msgPending(0), m_msgProcessing(1) {
    m_socketP = NULL;
    m_stop = false;
    m_interfaceP = interfaceP;
    m_socketSemP = socketSemP;
}

void ClientServerInterfaceThread::setSocket(QTcpSocket *socketP) {
    m_socketP = socketP;
}

void ClientServerInterfaceThread::messagePending() {
#ifdef _VERBOSE_TCP_INTERFACE
    qDebug() << "Tcp Interface Thread has a message to read/write, will release reader/writer";
#endif

    m_msgPending.release();
}

/**
  * Unblocks the thread and makes it exit
  */
void ClientServerInterfaceThread::stop() {
    m_stop = true;
    m_msgPending.release();
}

void ClientServerInterfaceReader::setSocket(QTcpSocket *socketP) {
    m_socketP = socketP;

    if (socketP)
        connect(m_socketP, SIGNAL(readyRead()), this, SLOT(messagePending()));
}

ClientServerInterface::ClientServerInterface(QObject *parentP) : QObject(parentP), m_rcvQueueSem(1), m_sndQueueSem(1), m_socketSem(1) {
    m_socketP = NULL;

    qRegisterMetaType<QAbstractSocket::SocketError>();

    if (parentP) {
        // connect the parent to the receive/send signal/slot
        connect(this, SIGNAL(messageReceived(QString)), parentP, SLOT(messageReceived(QString)));
        connect(this, SIGNAL(connectionStateChanged(bool)), parentP, SLOT(connectionStateChanged(bool)));
        connect(parentP, SIGNAL(sendMessage(QString, bool)), this, SLOT(sendMessage(QString, bool)));
    }

    // create the reader and the writer threads
    m_readerP = new ClientServerInterfaceReader(this, &m_socketSem);
    m_writerP = new ClientServerInterfaceWriter(this, &m_socketSem);

    // when the reader has received a message, it enqueues it in the receive
    // queue and signals. In turn, the interface signals whatever (parentP) object is connected.
    connect(m_readerP, SIGNAL(messageReceived(QString)), this, SIGNAL(messageReceived(QString)));

    // when the writer has a message to send, signal.
    connect(this, SIGNAL(messagePending()), m_writerP, SLOT(messagePending()));

    // connect to the reader and writer to pass them the socket when connected
    connect(this, SIGNAL(setSocket(QTcpSocket*)), m_readerP, SLOT(setSocket(QTcpSocket*)));
    connect(this, SIGNAL(setSocket(QTcpSocket*)), m_writerP, SLOT(setSocket(QTcpSocket*)));

    // connect to the reader and writer to stop them when the interface is deleted
    connect(this, SIGNAL(stop()), m_readerP, SLOT(stop()));
    connect(this, SIGNAL(stop()), m_writerP, SLOT(stop()));
    connect(m_writerP, SIGNAL(writeToSocket(QTcpSocket*,QByteArray*)), this, SLOT(writeToSocket(QTcpSocket*,QByteArray*)));

    // start the reader and the writer
    m_readerP->start(QThread::HighPriority);
    m_writerP->start(QThread::HighPriority);
}

ClientServerInterface::~ClientServerInterface() {
    if (m_readerP) {
        if (m_readerP->isRunning()) {
            stop();
            m_readerP->wait();
        }
        m_readerP->deleteLater();
    }

    if (m_writerP) {
        if (m_writerP->isRunning()) {
            stop();
            m_writerP->wait();
        }
        m_writerP->deleteLater();
    }

    if (m_socketP) {
        disconnectFromPeer();
        m_socketP->deleteLater();
    }

    m_rcvQueue.clear();
    m_sndQueue.clear();
}

bool ClientServerInterface::isConnected() {
    return m_socketP && m_socketP->state() == QTcpSocket::ConnectedState;
}

void ClientServerInterface::connected() {
    connectionStateChanged(true);

#ifdef _VERBOSE_TCP_INTERFACE
    qDebug() << "Tcp Interface connection state changed: now connected";
#endif
}

void ClientServerInterface::disconnected() {
    connectionStateChanged(false);

#ifdef _VERBOSE_TCP_INTERFACE
    qDebug() << "Tcp Interface connection state changed: now disconnected";
#endif
}

void ClientServerInterface::error(QAbstractSocket::SocketError error) {
#ifdef _VERBOSE_TCP_INTERFACE
    qDebug() << "ERROR: Tcp Interface communication error: " << error;
#endif

    switch (error) {
        case QTcpSocket::SocketAccessError:
        case QTcpSocket::DatagramTooLargeError:
        case QTcpSocket::UnsupportedSocketOperationError:
        case QTcpSocket::ProxyAuthenticationRequiredError:
        case QTcpSocket::ProxyProtocolError:
            // fatal error, but the connection is still alive,
            // shut it down
            if (isConnected())
               disconnectFromPeer();
            break;

        case QTcpSocket::SocketAddressNotAvailableError:
        case QTcpSocket::AddressInUseError:
        case QTcpSocket::NetworkError:
        case QTcpSocket::SocketResourceError:
        case QTcpSocket::HostNotFoundError:
        case QTcpSocket::SslHandshakeFailedError:
        case QTcpSocket::RemoteHostClosedError:
        case QTcpSocket::ConnectionRefusedError:
        case QTcpSocket::ProxyNotFoundError:
        case QTcpSocket::ProxyConnectionRefusedError:
        case QTcpSocket::ProxyConnectionClosedError:
        case QTcpSocket::UnknownSocketError:
            // fatal error, the connection is supposedly down
#ifdef _VERBOSE_TCP_INTERFACE
            qDebug() << "ERROR: Fatal Tcp Interface communication error: " << error;
#endif
            break;

        case QTcpSocket::UnfinishedSocketOperationError:
        case QTcpSocket::ProxyConnectionTimeoutError:
        case QTcpSocket::SocketTimeoutError:
            // never mind, the connection is still alive and
            // may very well be functional, keep going
            break;
    }
}

void ClientServerInterface::connectToPeer() {
}

void ClientServerInterface::disconnectFromPeer() {
#ifdef _VERBOSE_TCP_INTERFACE
    qDebug() << "ClientServerInterface will disconnect from Peer";
#endif

    if (m_socketP && m_socketP->isOpen())
        m_socketP->close();
}

void ClientServerInterface::sendMessage(QString message, bool urgent) {
    if (!message.isEmpty() && isConnected()) {
        // place the message in the fifo
        enqueueSndMessage(message, urgent);

        // tell the writer to handle it
        messagePending();
    }
}

ClientServerInterfaceReader::ClientServerInterfaceReader(ClientServerInterface *interfaceP, QSemaphore *socketSemP) : ClientServerInterfaceThread(interfaceP, socketSemP) {
}

void ClientServerInterfaceReader::run() {
    forever {
        // wait for a message to read.
        m_msgPending.acquire();

        if (m_stop)
            break;

#ifdef _VERBOSE_TCP_INTERFACE
        qDebug() << "ClientServerInterfaceReader was unblocked to handle a message";
#endif
        m_msgProcessing.acquire();

#ifdef _VERBOSE_TCP_INTERFACE
        qDebug() << "ClientServerInterfaceReader was unblocked to process the message";
#endif
        while (!m_stop && m_interfaceP->isConnected() && m_socketP->bytesAvailable()) {
            // read...
            QString msg = readString();

#ifdef _VERBOSE_TCP_INTERFACE
            qDebug() << "ClientServerInterfaceReader has read and will enqueue: " << msg;
#endif
            // put the msg in the receive queue
            m_interfaceP->enqueueRcvMessage(msg);

            // signal
            messageReceived(msg);
        };

        m_msgProcessing.release();
    }
}

ClientServerInterfaceWriter::ClientServerInterfaceWriter(ClientServerInterface *interfaceP, QSemaphore *socketSemP) : ClientServerInterfaceThread(interfaceP, socketSemP) {
}

void ClientServerInterfaceWriter::run() {
    forever {
        // wait for a message to send.
        m_msgPending.acquire();

        if (m_stop)
            break;

#ifdef _VERBOSE_TCP_INTERFACE
        qDebug() << "ClientServerInterfaceWriter was unblocked to handle a message";
#endif
        m_msgProcessing.acquire();

#ifdef _VERBOSE_TCP_INTERFACE
        qDebug() << "ClientServerInterfaceWriter was unblocked to process the message";
#endif

        // get the msg from the send queue
        QString msg;
        m_interfaceP->dequeueSndMessage(msg);

        if (!msg.isEmpty() && m_socketP->state() == QAbstractSocket::ConnectedState) {
#ifdef _VERBOSE_TCP_INTERFACE
            qDebug() << "ClientServerInterfaceWriter has dequeued and will write: " << msg;
#endif
            // write...
            writeString(msg);

        }

        m_msgProcessing.release();
    }
}

QString ClientServerInterfaceWriter::readString() {
    // this one doesn't read :)
    return "";
}

/**
  * Sends a string to the peer. Writes QChar instead of the whole string because a sequence of QChars is
  * expected by the reader.
  */
void ClientServerInterfaceWriter::writeString(QString string) {
    m_socketSemP->acquire();

    QByteArray *blockP = new QByteArray();
    QDataStream out(blockP, QIODevice::WriteOnly);

    // set data stream serialization version
    out.setVersion(TCP_INTERFACE_STREAM_VERSION);

    // prepare data packet
    out << (quint16)string.length();
    QByteArray data(string.toUtf8());
    out << data;

    // sends packet
    writeToSocket(m_socketP, blockP);


#ifdef _VERBOSE_TCP_INTERFACE
    qDebug() << "Tcp Interface Writer sent string: " << string;
#endif

    m_socketSemP->release();
}

/**
  * Reads a string from the peer. Read QChars instead of the whole string to prevent eating
  * extra bytes.
  */
QString ClientServerInterfaceReader::readString() {
    m_socketSemP->acquire();

    QString string;
    uint    length;
    char    *dataP;
    quint16 blocksize;

    QDataStream in(m_socketP);
    in.setVersion(TCP_INTERFACE_STREAM_VERSION);

    // wait for the block size to be available
    while (m_socketP->state() == QAbstractSocket::ConnectedState &&
           m_socketP->bytesAvailable() < (int)sizeof(quint16)) {
        msleep(100);
#ifdef _VERBOSE_TCP_INTERFACE
        qDebug() << "Tcp Interface Reader waiting for incoming blocksize data";
#endif
    }

    if (!m_interfaceP->isConnected())
        goto endRead;

    // read block size
    in >> blocksize;

    // wait for the complete data to be available
    while (m_socketP->state() == QAbstractSocket::ConnectedState &&
           m_socketP->bytesAvailable() < blocksize)
        msleep(100);

    if (m_socketP->state() != QAbstractSocket::ConnectedState)
        goto endRead;

    length = blocksize;
    in.readBytes(dataP, length);
    string = QString::fromUtf8(dataP);
    delete dataP;

#ifdef _VERBOSE_TCP_INTERFACE
    qDebug() << "Tcp Interface reader received string: " << string;
#endif

endRead:
    m_socketSemP->release();

    return string;
}

void ClientServerInterfaceReader::writeString(QString string) {
    // this one doesn't write :)
    Q_UNUSED(string);
}

ServerInterface::ServerInterface(QTcpServer *serverP, QObject *parentP) : ClientServerInterface(parentP) {
    m_serverP = serverP;
}

void ServerInterface::connectToPeer() {
    // accepts the connection to the client
    QTcpSocket *oldSocketP = m_socketP;
    m_socketP = m_serverP->nextPendingConnection();
    if (oldSocketP)
        oldSocketP->deleteLater();

    // ouch, something's gone real bad, just return
    if (!m_socketP)
        return;

#ifdef _VERBOSE_TCP_INTERFACE
    qDebug() << "nextPendingConnection: is socket open?: " << m_socketP->isOpen();
    qDebug() << "nextPendingConnection: socket state: " << m_socketP->state();
#endif

    // make sure the connection doesn't drop when the server is busy for a little while
    m_socketP->setSocketOption(QTcpSocket::KeepAliveOption, true);

    connect(m_socketP, SIGNAL(connected()), this, SLOT(connected()));
    connect(m_socketP, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(m_socketP, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));

    // signal socket created
    setSocket(m_socketP);

#ifdef _VERBOSE_TCP_INTERFACE
    qDebug() << "Connected Tcp Server Interface: " << this;
    qDebug() << "socket     : " << m_socketP;
    qDebug() << "connected  : " << isConnected();
    qDebug() << "reader     : " << m_readerP;
    qDebug() << "writer     : " << m_writerP;
#endif
}

void ServerInterface::pureVirtual() {
}

ClientInterface::ClientInterface(QObject *parentP, QString hostName) : ClientServerInterface(parentP) {
    m_hostName = hostName;
}

void ClientInterface::connectToPeer() {
    QTcpSocket *oldSocketP = m_socketP;
    m_socketP = new QTcpSocket(this);
    if (oldSocketP)
        oldSocketP->deleteLater();

    // make sure the connection doesn't drop when the server is busy for a little while
    m_socketP->setSocketOption(QTcpSocket::KeepAliveOption, true);

    connect(m_socketP, SIGNAL(connected()), this, SLOT(connected()));
    connect(m_socketP, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(m_socketP, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));

    // signal socket created
    setSocket(m_socketP);

    // connect to server
    m_socketP->connectToHost(m_hostName, DEFAULT_PORT);

#ifdef _VERBOSE_TCP_INTERFACE
    qDebug() << "Connected Tcp Client Interface: " << this;
    qDebug() << "socket: " << m_socketP;
    qDebug() << "connected: " << isConnected();
    qDebug() << "reader: " << m_readerP;
    qDebug() << "writer: " << m_writerP;
#endif
}

void ClientInterface::pureVirtual() {
}



