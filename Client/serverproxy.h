/*
 * SION! Client interacts with SION! Server to let you virtually
 * organize content as you wish on your computer.
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

#ifndef SERVERPROXY_H
#define SERVERPROXY_H

#include "../Server/servercommands.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QApplication>
#include <QSemaphore>
#include <QMainWindow>
#include <QTime>

#include "common.h"
#include "clientserverinterface.h"

//#define _VERBOSE_PROXY 1

#define NORMAL_REPLY_TIME_SAMPLING_ITERATIONS  10

/**
  * This class serves as a proxy with the SION! Server. The Client calls the proxy methods which in turn
  * communicate with the server. Replies (or unexpected messages) from the server are raised to the application's
  * attention via signals.
  */
class ServerProxy : public QObject {
    Q_OBJECT

signals:
    // this is where the proxy raise the server's answers to the UI..
    void requestAccessGrant();
    void accessGranted(QString user, QString pwd, bool granted);

    void filterSet(QString set);
    void filterSetDirty(bool dirty);
    void filterSetAdd(QString set);
    void filterSetDel(QString set);
    void filterSets(QStringList sets);

    void filterFileAdd(QString filter, QString path);
    void filterFileDel(QString filter, QString path);

    void filters(QStringList filters);
    void directories(QString directory, QStringList directories);
    void filterAdd(QString filter);
    void filterDel(QString filter);
    void filterIsRunning(QString filter, bool running);

    void filterDirectory(QString filter, QString directory);

    void plugins(QString filter, QStringList plugins);
    void pluginNames(QString filter, QStringList pluginNames);

    void pluginTip(QString filter, QString plugin, QString tip);
    void pluginScript(QString filter, QString plugin, QString script);
    void pluginScriptError(QString filter, QString plugin, QString error);

    void attributes(QString filter, QString plugin, QStringList attributes);

    void attributeTip(QString filter, QString plugin, QString attribute, QString tip);
    void attributeClass(QString filter, QString plugin, QString attribute, QString className);
    void attributeValue(QString filter, QString path, QString attribute, QString value);

    void fileContent(QString filename, QString offset, QString hexContent);

    void availablePlugins(QStringList plugins);

public:
    ServerProxy(QMainWindow *windowP, QString hostName = "");
    ~ServerProxy();

    inline bool isServerBusy() {
        return m_busy;
    }

    ClientInterface *getTcpInterface() {
        return m_interfaceP;
    }

    // these one don't generate answers
    void saveFilterSet(QString set);
    void deleteFilterSet(QString set);
    void loadFilterSet(QString set);
    void newFilterSet();

    void addFilter(QString parent, QString filter, QString directory, bool recursive, QStringList plugins);
    void startFilter(QString filter);
    void stopFilter(QString filter);
    void requestIsFilterRunning(QString filter);

    void modifyFilter(QString filter, QString directory, bool recursive, QStringList plugins);
    void removeFilter(QString filter);

    void setPluginScript(QString filter, QString plugin, QString script);

    void rescan();
    void rescanFilter(QString filter);

    void cleanup();
    void cleanupFilter(QString filter);

    void stopServer();

    // these ones should generate answers
    void requestAccess(QString user, QString pwd);

    void pingServer();

    void requestFilterSets();

    void requestFilterSet();
    void requestIsFilterSetDirty();

    void requestFilters();
    void requestDirectories(QString directory);

    void requestFilterDirectory(QString filter);
    void requestFilterPlugins(QString filter);
    void requestFilterFiles(QString filter);

    void requestPluginAttributes(QString filter, QString plugin);
    void requestPluginTip(QString filter, QString plugin);
    void requestPluginScript(QString filter, QString plugin);
    void requestPluginScriptError(QString filter, QString plugin);

    void requestAttributeTip(QString filter, QString plugin, QString attribute);
    void requestAttributeClass(QString filter, QString plugin, QString attribute);
    void requestAttributeValue(QString filter, QString path, QString attribute);

    void requestFileContent(QString filename);

    void requestAvailablePlugins();

    inline bool isConnected() {
        return m_interfaceP && m_interfaceP->isConnected();
    }

signals:
    void sendMessage(QString string, bool urgent);

public slots:
    void messageReceived(QString string);
    void connectionStateChanged(bool) {} // so that there's no warning when the tcp interface is instantiated

private:
    ClientInterface    *m_interfaceP;
    QTime              m_replyTime;
    int                m_normalReplyTime;
    int                m_totalReplyTime;
    int                m_numPingReplies;
    bool               m_busy;

    void sendRequest(QStringList command, bool urgent = false);

    inline bool connectToServer() {
        if (!m_interfaceP)
            return false;

        if (!m_interfaceP->isConnected()) {
            // we must resample the server reply time
            m_totalReplyTime = 0;
            m_numPingReplies = 0;
            m_busy = false;

            m_interfaceP->connectToPeer();

            return m_interfaceP->isConnected();
        }

        return true;
    }
};

#endif // SERVERPROXY_H
