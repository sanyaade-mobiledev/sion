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

#ifndef TCPINTERFACE_H
#define TCPINTERFACE_H

#include "ClientServerInterface_global.h"

#include <QTcpSocket>
#include <QTcpServer>
#include <QObject>
#include <QThread>
#include <QQueue>
#include <QSemaphore>

Q_DECLARE_METATYPE(QAbstractSocket::SocketError)

//#define _VERBOSE_TCP_INTERFACE 1

#define DEFAULT_HOST                    "127.0.0.1"
#define DEFAULT_PORT                    50000

#define TCP_INTERFACE_STREAM_VERSION   QDataStream::Qt_4_6

/**
  * Serves as a simplistic RPC over TCP layer between the browser and the
  * server. The tcp interface instance which both the server and client rely
  * upon has a reader and a writer thread. The threads are there to make the
  * sending/receiving of messages independant from the UI event loop.
  */
class ClientServerInterface;
class ClientServerInterfaceThread : public QThread {
    Q_OBJECT

public:
    explicit ClientServerInterfaceThread(ClientServerInterface *interfaceP, QSemaphore *socketSemP);

signals:
    void messageReceived(QString message);
    void messageSent(QString message);
    void writeToSocket(QTcpSocket *socketP, QByteArray *outP);

public slots:
    virtual void setSocket(QTcpSocket *socketP);

    void stop();

    void messagePending();

protected:
    volatile bool           m_stop;
    ClientServerInterface   *m_interfaceP;
    QTcpSocket              *m_socketP;
    QSemaphore              m_msgPending;       // when available, there's a message to read or send
    QSemaphore              m_msgProcessing;    // when available, a new message can be processed (just in case we reenter for some reason..)
    QSemaphore              *m_socketSemP;

    virtual void    writeString(QString string) = 0;
    virtual QString readString() = 0;
};

class ClientServerInterfaceWriter : public ClientServerInterfaceThread {
    Q_OBJECT

public:
    explicit ClientServerInterfaceWriter(ClientServerInterface *interfaceP, QSemaphore *socketSemP);

protected:
    void run();

    void    writeString(QString string);
    QString readString();
};

class ClientServerInterfaceReader : public ClientServerInterfaceThread {
    Q_OBJECT

public:
    explicit ClientServerInterfaceReader(ClientServerInterface *interfaceP, QSemaphore *socketSemP);

public slots:
    void setSocket(QTcpSocket *socketP);

protected:
    void run();

    void    writeString(QString string);
    QString readString();
};

class ClientServerInterface : public QObject {
    Q_OBJECT

    friend class ClientServerInterfaceReader;
    friend class ClientServerInterfaceWriter;

public:
    explicit ClientServerInterface(QObject *parentP = 0);
    virtual ~ClientServerInterface();

    void enqueueSndMessage(QString msg, bool urgent = false) {
        m_sndQueueSem.acquire();
        if (urgent)
            m_sndQueue.insert(0, msg);
        else
            m_sndQueue.enqueue(msg);
        m_sndQueueSem.release();
    }

    void dequeueSndMessage(QString &msg) {
        m_sndQueueSem.acquire();
        if (!m_sndQueue.isEmpty())
            msg = m_sndQueue.dequeue();
        m_sndQueueSem.release();
    }

    void enqueueRcvMessage(QString msg) {
        m_rcvQueueSem.acquire();
        m_rcvQueue.enqueue(msg);
        m_rcvQueueSem.release();
    }

    void dequeueRcvMessage(QString &msg) {
        m_rcvQueueSem.acquire();
        if (!m_rcvQueue.isEmpty())
            msg = m_rcvQueue.dequeue();
        m_rcvQueueSem.release();
    }

    bool isConnected();
    bool isError();

    /**
      * Converts the given byte array into a string containing its compressed base 64
      * encoded values and returns the string.
      */
    inline static QString binToCompressedBase64(QByteArray &bytes) {
        if (bytes.isEmpty())
            return QString();

        QByteArray compressedBytes = qCompress(bytes);
        return QString(compressedBytes.toBase64());
    }

    /**
      * Converts a compressed Base 64 representation of a byte array contained in a
      * string into the original array and returns it.
      */
    inline static QByteArray compressedBase64ToBin(QString &string) {
        if (string.isEmpty())
            return QByteArray();

        return qUncompress(QByteArray().fromBase64(string.toAscii()));
    }

signals:
    void connectionStateChanged(bool connected);
    void messageReceived(QString message);
    void messagePending();
    void setSocket(QTcpSocket *socketP);
    void stop();

public slots:
    void sendMessage(QString message, bool urgent); // if set, the urgent flag posts the msg at the head of the fifo,
                                                    // so that it is processed before any other pending message

    // this call creates a child object, which can't be done
    // in the secondary writer thread
    void writeToSocket(QTcpSocket *socketP, QByteArray *outP) {
        socketP->write(*outP);
        delete outP;
    }


    // connection state signals
    void connected();
    void disconnected();
    void error(QAbstractSocket::SocketError error);

    virtual void    connectToPeer();
    void            disconnectFromPeer();

protected:
    ClientServerInterfaceReader  *m_readerP;
    ClientServerInterfaceWriter  *m_writerP;
    QQueue<QString>     m_rcvQueue;
    QSemaphore          m_rcvQueueSem;
    QQueue<QString>     m_sndQueue;
    QSemaphore          m_sndQueueSem;
    QTcpSocket          *m_socketP;
    QSemaphore          m_socketSem;

private:
    virtual void pureVirtual() = 0;
};

class ServerInterface : public ClientServerInterface {
public:
    ServerInterface(QTcpServer *serverP, QObject *parentP);

    void connectToPeer();

private:
    QTcpServer    *m_serverP;

    void pureVirtual();
};

class ClientInterface : public ClientServerInterface {
public:
    ClientInterface(QObject *parentP = NULL, QString hostName = DEFAULT_HOST);

    void connectToPeer();

private:
    QString m_hostName;

    void pureVirtual();
};

#endif // TCPINTERFACE_H
