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

#include <QByteArray>
#include <QDataStream>

#include "serverproxy.h"

/**
  * Creates a server proxy instance. The proxy is created non connected, with no
  * data ready to be read and no socket associated. The socket is created when the
  * first request to the server will be issued.
  */
ServerProxy::ServerProxy(QMainWindow *windowP, QString hostName) : QObject(windowP) {
    if (hostName.isEmpty())
        m_interfaceP = new ClientInterface(this);
    else
        m_interfaceP = new ClientInterface(this, hostName);

    // connect this' update signals to the *windowP's update slots
    connect(m_interfaceP, SIGNAL(connectionStateChanged(bool)), windowP, SLOT(connectionStateChanged(bool)));
    connect(this, SIGNAL(attributeValue(QString,QString,QString,QString)), windowP, SLOT(attributeValue(QString,QString,QString,QString)));
    connect(this, SIGNAL(filters(QStringList)), windowP, SLOT(filters(QStringList)));
    connect(this, SIGNAL(directories(QString, QStringList)), windowP, SLOT(directories(QString, QStringList)));
    connect(this, SIGNAL(filterSet(QString)), windowP, SLOT(filterSet(QString)));
    connect(this, SIGNAL(filterSetAdd(QString)), windowP, SLOT(filterSetAdd(QString)));
    connect(this, SIGNAL(filterSetDel(QString)), windowP, SLOT(filterSetDel(QString)));
    connect(this, SIGNAL(filterSets(QStringList)), windowP, SLOT(filterSets(QStringList)));
    connect(this, SIGNAL(filterSetDirty(bool)), windowP, SLOT(filterSetDirty(bool)));
    connect(this, SIGNAL(filterIsRunning(QString, bool)), windowP, SLOT(filterIsRunning(QString, bool)));
    connect(this, SIGNAL(availablePlugins(QStringList)), windowP, SLOT(availablePlugins(QStringList)));
    connect(this, SIGNAL(filterDirectory(QString,QString)), windowP, SLOT(filterDirectory(QString,QString)));
    connect(this, SIGNAL(plugins(QString,QStringList)), windowP, SLOT(plugins(QString,QStringList)));
    connect(this, SIGNAL(pluginScript(QString,QString,QString)), windowP, SLOT(pluginScript(QString,QString,QString)));
    connect(this, SIGNAL(pluginTip(QString,QString,QString)), windowP, SLOT(pluginTip(QString,QString,QString)));
    connect(this, SIGNAL(pluginScriptError(QString,QString,QString)), windowP, SLOT(pluginScriptError(QString,QString,QString)));
    connect(this, SIGNAL(attributes(QString,QString,QStringList)), windowP, SLOT(attributes(QString,QString,QStringList)));
    connect(this, SIGNAL(attributeClass(QString,QString,QString,QString)), windowP, SLOT(attributeClass(QString,QString,QString,QString)));
    connect(this, SIGNAL(attributeTip(QString,QString,QString,QString)), windowP, SLOT(attributeTip(QString,QString,QString,QString)));
    connect(this, SIGNAL(filterFileAdd(QString,QString)), windowP, SLOT(filterFileAdd(QString,QString)));
    connect(this, SIGNAL(filterFileDel(QString,QString)), windowP, SLOT(filterFileDel(QString,QString)));
    connect(this, SIGNAL(filterAdd(QString)), windowP, SLOT(filterAdd(QString)));
    connect(this, SIGNAL(filterDel(QString)), windowP, SLOT(filterDel(QString)));
    connect(this, SIGNAL(requestAccessGrant()), windowP, SLOT(requestAccessGrant()));
    connect(this, SIGNAL(accessGranted(QString, QString, bool)), windowP, SLOT(accessGranted(QString, QString, bool)));
    connect(this, SIGNAL(fileContent(QString,QString,QString)), windowP, SLOT(fileContent(QString,QString,QString)));

    // try to disconnect a bit earlier than the window's destruction to prevent crashes in the tcp interfaces
    // which slots haven't been invoked and will play with a disconnected socket.
    connect(windowP, SIGNAL(destroyed()), m_interfaceP, SLOT(disconnectFromPeer()));
}

/**
  * Deletes the server proxy.
  */
ServerProxy::~ServerProxy() {
    if (isConnected())
        m_interfaceP->disconnectFromPeer();

    delete m_interfaceP;
}

/**
  * The server commands go through these proxy methods.
  */

// these one don't generate answers
void ServerProxy::saveFilterSet(QString set) {
    QStringList command;
    command << SAVE_SET_COMMAND << set;
    sendRequest(command);
}

void ServerProxy::deleteFilterSet(QString set) {
    QStringList command;
    command << DELETE_SET_COMMAND << set;
    sendRequest(command);
}

void ServerProxy::loadFilterSet(QString set) {
    QStringList command;
    command << LOAD_SET_COMMAND << set;
    sendRequest(command);
}

void ServerProxy::newFilterSet() {
    QStringList command;
    command << NEW_SET_COMMAND;
    sendRequest(command);
}

void ServerProxy::addFilter(QString parent, QString filter, QString directory, bool recursive, QStringList plugins) {
    QStringList command;
    command << ADD_FILTER_COMMAND << parent << filter << directory << (recursive ? "TRUE" : "FALSE") << plugins;
    sendRequest(command);
}

void ServerProxy::startFilter(QString filter) {
    QStringList command;
    command << START_FILTER_COMMAND << filter;
    sendRequest(command, true);
}

void ServerProxy::stopFilter(QString filter) {
    QStringList command;
    command << STOP_FILTER_COMMAND << filter;
    sendRequest(command, true);
}

void ServerProxy::requestIsFilterRunning(QString filter) {
    QStringList command;
    command << IS_FILTER_RUNNING_COMMAND << filter;
    sendRequest(command);
}

void ServerProxy::modifyFilter(QString filter, QString directory, bool recursive, QStringList plugins) {
    QStringList command;
    command << MODIFY_FILTER_COMMAND << filter << directory << (recursive ? "TRUE" : "FALSE") << plugins;
    sendRequest(command);
}

void ServerProxy::removeFilter(QString filter) {
    QStringList command;
    command << REMOVE_FILTER_COMMAND << filter;
    sendRequest(command);
}

void ServerProxy::setPluginScript(QString filter, QString plugin, QString script) {
    QStringList command;
    command << SET_PLUGIN_SCRIPT_COMMAND << filter << plugin << script;
    sendRequest(command);
}

void ServerProxy::rescan() {
    QStringList command;
    command << RESCAN_COMMAND;
    sendRequest(command);
}

void ServerProxy::rescanFilter(QString filter) {
    QStringList command;
    command << RESCAN_FILTER_COMMAND << filter;
    sendRequest(command);
}

void ServerProxy::cleanup() {
    QStringList command;
    command << CLEANUP_COMMAND;
    sendRequest(command);
}

void ServerProxy::cleanupFilter(QString filter) {
    QStringList command;
    command << CLEANUP_FILTER_COMMAND << filter;
    sendRequest(command);
}

void ServerProxy::stopServer() {
    QStringList command;
    command << EXIT_COMMAND;
    sendRequest(command, true);
}

// these ones should generate answers
void ServerProxy::requestAccess(QString user, QString pwd) {
    QStringList command;
    command << ACCESS_COMMAND << user << pwd;
    sendRequest(command, true);
}

void ServerProxy::pingServer() {
    QStringList command;
    command << PING_COMMAND;
    sendRequest(command);
    m_busy = true; // just in case the server doesn't reply for a little while...
    m_replyTime.restart();
}

void ServerProxy::requestFilterSets() {
    QStringList command;
    command << SETS_COMMAND;
    sendRequest(command);
}

void ServerProxy::requestFilterSet() {
    QStringList command;
    command << SET_COMMAND;
    sendRequest(command);
}

void ServerProxy::requestIsFilterSetDirty() {
    QStringList command;
    command << IS_SET_DIRTY_COMMAND;
    sendRequest(command);
}

void ServerProxy::requestFilters() {
    QStringList command;
    command << FILTERS_COMMAND;
    sendRequest(command);
}

void ServerProxy::requestDirectories(QString directory) {
    QStringList command;
    command << DIRECTORIES_COMMAND << directory;
    sendRequest(command);
}

void ServerProxy::requestFilterDirectory(QString filter) {
    QStringList command;
    command << GET_FILTER_DIR_COMMAND << filter;
    sendRequest(command);
}

void ServerProxy::requestFilterPlugins(QString filter) {
    QStringList command;
    command << PLUGINS_COMMAND << filter;
    sendRequest(command);
}

void ServerProxy::requestPluginAttributes(QString filter, QString plugin) {
    QStringList command;
    command << ATTRIBUTES_COMMAND << filter << plugin;
    sendRequest(command);
}

void ServerProxy::requestPluginTip(QString filter, QString plugin) {
    QStringList command;
    command << GET_PLUGIN_TIP_COMMAND << filter << plugin;
    sendRequest(command);
}

void ServerProxy::requestPluginScript(QString filter, QString plugin) {
    QStringList command;
    command << GET_PLUGIN_SCRIPT_COMMAND << filter << plugin;
    sendRequest(command);
}

void ServerProxy::requestPluginScriptError(QString filter, QString plugin) {
    QStringList command;
    command << GET_PLUGIN_SCRIPT_LAST_ERROR_COMMAND << filter << plugin;
    sendRequest(command);
}

void ServerProxy::requestAttributeTip(QString filter, QString plugin, QString attribute) {
    QStringList command;
    command << GET_PLUGIN_ATTRIBUTE_TIP_COMMAND << filter << plugin << attribute;
    sendRequest(command);
}

void ServerProxy::requestAttributeClass(QString filter, QString plugin, QString attribute) {
    QStringList command;
    command << GET_PLUGIN_ATTRIBUTE_CLASS_COMMAND << filter << plugin << attribute;
    sendRequest(command);
}

void ServerProxy::requestAttributeValue(QString filter, QString path, QString attribute) {
    QStringList command;
    command << GET_FILE_ATTRIBUTE_VALUE_COMMAND << filter << path << attribute;
    sendRequest(command);
}

void ServerProxy::requestFileContent(QString filename) {
    QStringList command;
    command << GET_FILE_COMMAND << filename;
    sendRequest(command);
}

void ServerProxy::requestAvailablePlugins() {
    QStringList command;
    command << AVAILABLE_PLUGINS_COMMAND;
    sendRequest(command);
}

void ServerProxy::requestFilterFiles(QString filter) {
    QStringList command;
    command << FILES_COMMAND << filter;
    sendRequest(command);
}

/**
  * This is where the command is formatted and sent to the server.
  */
void ServerProxy::sendRequest(QStringList command, bool urgent) {
    // if the proxy can't connect to the server, do nothing
    if (!connectToServer())
        return;

    // if the command is empty, there isn't much the proxy can do
    if (command.isEmpty())
        return;

    // prepare the command line
    QString request;
    int     numArgs = command.count();
    for (int i = 0; i < numArgs; i++) {
        request += command[i];
        if (i < numArgs - 1)
            request += CMD_SEPARATOR;
    }

#ifdef _VERBOSE_PROXY
    qDebug() << "Proxy will send request: " << request;
#endif

    // send the request to the server
    sendMessage(request, urgent);
}

/**
  * The Tcp Client Interface has received a string. Parse it and send the appropriate signals to the UI.
  */
void ServerProxy::messageReceived(QString string) {
    QString     command;
    QStringList arguments;

#ifdef _VERBOSE_PROXY
    qDebug() << "Proxy received reply: " << string;
#endif

    // split the command
    arguments = string.split(CMD_SEPARATOR);

    // read command and remove it from argument list
    command = arguments.takeAt(0);

    // null string (error)?
    if (command.isNull())
        return;

    // just in case we want to implement a manual test app to stress the browser
    // because the server sends caps only.
    command = command.toUpper();

    // unexpected msgs first (they occurs more often -overall the files related ones-)

    // access grant
    if (command == REQUEST_ACCESS_GRANT_MSG) {
        if (arguments.count() == 0)
            requestAccessGrant();
        return;
    }

    // new filter set
    if (command == ADD_FILTER_SET_MSG) {
        if (arguments.count() == 1)
            filterSetAdd(arguments[0]);
        return;
    }

    // new filter
    if (command == ADD_FILTER_MSG) {
        if (arguments.count() == 1)
            filterAdd(arguments[0]);
        return;
    }

    // filter removed
    if (command == DEL_FILTER_MSG) {
        if (arguments.count() == 1)
            filterDel(arguments[0]);
        return;
    }

    // new filter file
    if (command == ADD_FILE_MSG) {
        if (arguments.count() == 2)
            filterFileAdd(arguments[0], arguments[1]);
        return;
    }

    // filter file removed
    if (command == DEL_FILE_MSG) {
        if (arguments.count() == 2)
            filterFileDel(arguments[0], arguments[1]);
        return;
    }

    // ping reply
    if (command == PING_COMMAND) {
        // sampling or checking server response time
        if (m_numPingReplies < NORMAL_REPLY_TIME_SAMPLING_ITERATIONS) {
            m_busy = false;
            ++m_numPingReplies;
            m_totalReplyTime += m_replyTime.elapsed();
            m_normalReplyTime = m_totalReplyTime / m_numPingReplies;
        } else
            m_busy = m_replyTime.elapsed() >= (m_normalReplyTime * 1.5);

        m_replyTime.restart();
        return;
    }

    // is filter running?
    if (command == IS_FILTER_RUNNING_COMMAND){
        if (arguments.count() == 2)
            filterIsRunning(arguments[0], arguments[1] == "TRUE");
        return;
    }

    // get a filtered in file attribute value (from db)
    if (command == GET_FILE_ATTRIBUTE_VALUE_COMMAND) {
        if (arguments.count() == 4)
            attributeValue(arguments[0], arguments[1], arguments[2], arguments[3]);
        return;
    }

    // get file content
    if (command == GET_FILE_COMMAND) {
        if (arguments.count() == 3)
            fileContent(arguments[0], arguments[1], arguments[2]);
        return;
    }

    // list filters
    if (command == FILTERS_COMMAND){
        if (arguments.count() >= 1)
            filters(arguments);
        return;
    }

    // list directories
    if (command == DIRECTORIES_COMMAND) {
        if (arguments.count() >= 2) {
            QString directory = arguments.takeFirst();
            directories(directory, arguments);
        }
        return;
    }

    // get current set
    if (command == SET_COMMAND){
        if (arguments.count() == 1)
            filterSet(arguments[0]);
        return;
    }

    // list sets
    if (command == SETS_COMMAND){
        if (arguments.count() >= 1)
            filterSets(arguments);
        return;
    }

    // is set dirty?
    if (command == IS_SET_DIRTY_COMMAND){
        if (arguments.count() == 1)
            filterSetDirty(arguments[0] == "TRUE");
        return;
    }

    // list available plugins
    if (command == AVAILABLE_PLUGINS_COMMAND){
        if (arguments.count() >= 1)
            availablePlugins(arguments);
        return;
    }

    // get filter directory
    if (command == GET_FILTER_DIR_COMMAND){
        if (arguments.count() == 2)
            filterDirectory(arguments[0], arguments[1]);
        return;
    }

    // list plugins for a filter
    if (command == PLUGINS_COMMAND){
        if (arguments.count() >= 2) {
            QString filter = arguments.takeFirst();
            plugins(filter, arguments);
        }
        return;
    }

    // get the script from a filter/plugin
    if (command == GET_PLUGIN_SCRIPT_COMMAND){
        if (arguments.count() == 3)
            pluginScript(arguments[0], arguments[1], arguments[2]);
        return;
    }

    // get the tip from a plugin
    if (command == GET_PLUGIN_TIP_COMMAND){
        if (arguments.count() == 3)
            pluginTip(arguments[0], arguments[1], arguments[2]);
        return;
    }

    // get the last error from script
    if (command == GET_PLUGIN_SCRIPT_LAST_ERROR_COMMAND){
        if (arguments.count() == 3)
            pluginScriptError(arguments[0], arguments[1], arguments[2]);
        return;
    }

    // list attributes for a filter/plugin
    if (command == ATTRIBUTES_COMMAND){
        if (arguments.count() >= 3) {
            QString filter = arguments.takeFirst();
            QString plugin = arguments.takeFirst();
            attributes(filter, plugin, arguments);
        }
        return;
    }

    // get the class from a filter/plugin's attribute
    if (command == GET_PLUGIN_ATTRIBUTE_CLASS_COMMAND){
        if (arguments.count() == 4)
            attributeClass(arguments[0], arguments[1], arguments[2], arguments[3]);
        return;
    }

    // get the tip from a filter/plugin's attribute
    if (command == GET_PLUGIN_ATTRIBUTE_TIP_COMMAND){
        if (arguments.count() == 4)
            attributeTip(arguments[0], arguments[1], arguments[2], arguments[3]);
        return;
    }

    // list files retained by a filter
    if (command == FILES_COMMAND){
        if (arguments.count() == 2)
            filterFileAdd(arguments[0], arguments[1]);
        return;
    }

    // access grant request result
    if (command == ACCESS_COMMAND) {
        if (arguments.count() == 3)
            accessGranted(arguments[0], arguments[1], arguments[2] == "TRUE");
        return;
    }
}

