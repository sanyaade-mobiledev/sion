/*
 * SION! Server meta-data / javascript indexing server.
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

#include <QDataStream>
#include <QDebug>
#include <QCoreApplication>

#include "qdirext.h"
#include "server.h"

/**
  * A SINGLE connection meta indexing server. Dialogs with a single connected client (other clients are blocked until
  * a connection can be established with the server (when the current client disconnects)). The communication is of the
  * form: client sends a requests, server obeys and optionnaly replies. Server sends unexpected messages when necessary
  * (new file filtered in for example). Messages are text messages. It's slower than byte oriented comm. but
  * has the big advantage of being human readable and hence easy to test (see ServerTester application).
  */
Server::Server(QObject *parentP) : QTcpServer(parentP),
    m_settings(SION_SERVER_ORGANIZATION, SION_SERVER_EXECUTABLE_NAME) {
    m_accessGranted = false;
    m_exitServer = false;
    m_dirty = false;

    setMaxPendingConnections(1);

    // the classifier informs the server of filter changes
    connect(&m_classifier, SIGNAL(newFilter(QString)), this, SLOT(newFilter(QString)));
    connect(&m_classifier, SIGNAL(delFilter(QString)), this, SLOT(delFilter(QString)));

    // the classifier displays information when loading/saving filter sets
    connect(&m_classifier, SIGNAL(displayActivity(QString)), this, SIGNAL(displayActivity(QString)));
    connect(&m_classifier, SIGNAL(displayProgress(int,int,int)), this, SIGNAL(displayProgress(int,int,int)));
}

/**
  * Iterates on the following:
  *
  *     listen for an incoming connection
  *     ProcessCommands
  *     Exit if EXIT_COMMAND was received
  *
  * Returns true if the server could be started, else returns false.
  */
bool Server::start() {
    // listen for clients on all interfaces
    if (!listen(QHostAddress::Any, DEFAULT_PORT)) {
        qDebug() << QObject::tr("Unable to start the server: %1.").arg(errorString());
        return false;
    }

    return true;
}

void Server::stop() {
    // stops listening
    close();

    // save the filters
    if (!m_filtersFilename.isEmpty()) {
        m_classifier.saveDatabase(m_filtersFilename + DB_EXTENSION);
        m_classifier.saveFilters(m_filtersFilename);
    }

    // stops the filters
    m_classifier.stop();
}

/**
  * Sends a reply to a command. The reply is composed
  * of the command that was sent, followed by the repl(y/ies).
  */
void Server::sendReply(QString reply, bool urgent) {
    QString fullReply = m_command;
    for (QStringList::iterator i = m_arguments.begin(); i != m_arguments.end(); i++) {
        fullReply += CMD_SEPARATOR;
        fullReply += *i;
    }
    fullReply += CMD_SEPARATOR;
    fullReply += reply;

    sendMessage(fullReply, urgent);
}

/**
  * Processes a command sent by the client.
  */
void Server::processCommand(QString &string) {
    // split the command
    m_arguments = string.split(CMD_SEPARATOR);

    // read command
    m_command = m_arguments.takeAt(0);

#ifdef _VERBOSE_SERVER
    qDebug() << "Server received command: " << m_command;
    qDebug() << "With arguments: " << m_arguments;
#endif

    // null string (error)?
    if (m_command.isNull())
        return;

    m_command = m_command.toUpper();

    // client requests access grant
    if (m_command == ACCESS_COMMAND) {
        accessCommand();
        return;
    }

    // request user and pwd if not done yet
    if (!m_accessGranted) {
        requestCredentials();
        return;
    }

    // ping server to get its response time
    if (m_command == PING_COMMAND) {
        pingCommand();
        return;
    }

    // get a filtered in file attribute value (from db)
    if (m_command == GET_FILE_ATTRIBUTE_VALUE_COMMAND) {
        getFileAttributeValueCommand();
        return;
    }

    // get a file content
    if (m_command == GET_FILE_COMMAND) {
        getFileCommand();
        return;
    }

    // list filters
    if (m_command == FILTERS_COMMAND){
        filtersCommand();
        return;
    }

    // list directories
    if (m_command == DIRECTORIES_COMMAND){
        directoriesCommand();
        return;
    }

    // get current set
    if (m_command == SET_COMMAND){
        setCommand();
        return;
    }

    // clears/resets current set
    if (m_command == NEW_SET_COMMAND){
        newSetCommand();
        return;
    }

    // list sets
    if (m_command == SETS_COMMAND){
        setsCommand();
        return;
    }

    // is set dirty?
    if (m_command == IS_SET_DIRTY_COMMAND){
        isSetDirtyCommand();
        return;
    }

    // save set
    if (m_command == SAVE_SET_COMMAND){
        saveSetCommand();
        return;
    }

    // load set
    if (m_command == LOAD_SET_COMMAND){
        loadSetCommand();
        return;
    }

    // delete set
    if (m_command == DELETE_SET_COMMAND){
        deleteSetCommand();
        return;
    }

    // list available plugins
    if (m_command == AVAILABLE_PLUGINS_COMMAND){
        availablePluginsCommand();
        return;
    }

    // add filter
    if (m_command == ADD_FILTER_COMMAND){
        addFilterCommand();
        return;
    }

    // start filter
    if (m_command == START_FILTER_COMMAND){
        startFilterCommand();
        return;
    }

    // stop filter
    if (m_command == STOP_FILTER_COMMAND){
        stopFilterCommand();
        return;
    }

    // is filter running
    if (m_command == IS_FILTER_RUNNING_COMMAND){
        isFilterRunningCommand();
        return;
    }

    // modify filter
    if (m_command == MODIFY_FILTER_COMMAND){
        modifyFilterCommand();
        return;
    }

    // get filter directory
    if (m_command == GET_FILTER_DIR_COMMAND){
        getFilterDirCommand();
        return;
    }

    // cleanup filter
    if (m_command == CLEANUP_FILTER_COMMAND){
        cleanupFilterCommand();
        return;
    }

    // remove filter
    if (m_command == REMOVE_FILTER_COMMAND){
        removeFilterCommand();
        return;
    }

    // list plugins for a filter
    if (m_command == PLUGINS_COMMAND){
        pluginsCommand();
        return;
    }

    // get the script from a filter/plugin
    if (m_command == GET_PLUGIN_SCRIPT_COMMAND){
        getPluginScriptCommand();
        return;
    }

    // get the tip from a plugin
    if (m_command == GET_PLUGIN_TIP_COMMAND){
        getPluginTipCommand();
        return;
    }

    // get the last error from script
    if (m_command == GET_PLUGIN_SCRIPT_LAST_ERROR_COMMAND){
        getPluginScriptLastErrorCommand();
        return;
    }

    // set a filter/plugin's script
    if (m_command == SET_PLUGIN_SCRIPT_COMMAND){
        setPluginScriptCommand();
        return;
    }

    // list attributes for a filter/plugin
    if (m_command == ATTRIBUTES_COMMAND){
        attributesCommand();
        return;
    }

    // get the class from a filter/plugin's attribute
    if (m_command == GET_PLUGIN_ATTRIBUTE_CLASS_COMMAND){
        getPluginAttributeClassCommand();
        return;
    }

    // get the tip from a filter/plugin's attribute
    if (m_command == GET_PLUGIN_ATTRIBUTE_TIP_COMMAND){
        getPluginAttributeTipCommand();
        return;
    }

    // rescan filter's physical dir
    if (m_command == RESCAN_FILTER_COMMAND){
        rescanFilterCommand();
        return;
    }

    // rescan all watched physical dirs
    if (m_command == RESCAN_COMMAND){
        rescanCommand();
        return;
    }

    // scan all watched physical dirs
    if (m_command == SCAN_COMMAND){
        scanCommand();
        return;
    }

    // cleanup db
    if (m_command == CLEANUP_COMMAND){
        cleanupCommand();
        return;
    }

    // list files retained by a filter
    if (m_command == FILES_COMMAND){
        filesCommand();
        return;
    }

    // help
    if (m_command == HELP_COMMAND){
        helpCommand();
        return;
    }
}

void Server::filtersCommand() {
    QString reply;

    // get filters list
    QVector<Filter *> *filtersP = m_classifier.getFilters();
    for (QVector<Filter *>::iterator i = filtersP->begin(); i != filtersP->end(); i++) {
        reply += (*i)->getVirtualDirectoryPath();
        reply += CMD_SEPARATOR;
    }

    // remove the trailing separator if any
    if (!reply.isEmpty())
        reply.chop(1);
    sendReply(reply);
}

void Server::directoriesCommand() {
    QString reply;

    if (m_arguments.count() < 1)
        return;

    // if no directory specified, start from the working dir
    if (m_arguments[0].isEmpty())
        m_arguments[0] = QCoreApplication::applicationDirPath();

    // get directories list
    QDirExt dir(m_arguments[0]);
    if (!dir.exists())
        return;

    QStringList dirs = dir.entryList(QDir::AllDirs | QDir::NoDot, QDir::Name);
    for (QList<QString>::iterator i = dirs.begin(); i != dirs.end(); i++) {
        reply += *i;
        reply += CMD_SEPARATOR;
    }

    // remove the trailing separator if any
    if (!reply.isEmpty())
        reply.chop(1);
    sendReply(reply);
}

void Server::addFilterCommand() {
    if (m_arguments.count() < 5)
        return;

    // read parent filter virtual path
    QString parentVirDirPath = m_arguments[0];

    // read virtual path
    QString virtualDirectoryPath = m_arguments[1];

    // read dir
    QString dir = m_arguments[2];

    // read recursive
    bool recursive = m_arguments[3].toUpper() == "TRUE";

    // read plugin names
    QStringList plugins;
    for(int i = 4; i < m_arguments.count(); i++) {
        // read plugin name
        QString plugin = m_arguments[i];
        if (plugin.isEmpty())
            break;

        plugins.append(plugin);
    }

    // find parent
    Filter *parentP = m_classifier.findFilter(parentVirDirPath);

    // add the (non started) filter
    m_classifier.addFilter(parentP, virtualDirectoryPath, dir, recursive, plugins, true);

    m_dirty = true;
}

void Server::startFilterCommand() {
    if (m_arguments.count() < 1)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP)
        // start filter
        filterP->start();
}

void Server::stopFilterCommand() {
    if (m_arguments.count() < 1)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP)
        // stop filter
        filterP->stop();
}

void Server::isFilterRunningCommand() {
    if (m_arguments.count() < 1)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP)
        // start filter
        sendReply(filterP->isRunning() ? "TRUE" : "FALSE", true);
}

void Server::modifyFilterCommand() {
    if (m_arguments.count() < 4)
        return;

    // read virtual path
    QString virtualDirectoryPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virtualDirectoryPath);
    if (filterP) {
        // read dir
        QString dir = m_arguments[1];

        // read recursive
        bool recursive = m_arguments[2].toUpper() == "TRUE";

        // read plugin names
        QStringList plugins;
        for(int i = 3; i < m_arguments.count(); i++) {
            // read plugin name
            QString plugin = m_arguments[i];
            if (plugin.isEmpty())
                break;

            plugins.append(plugin);
        }

        // modify the filter
        m_classifier.modifyFilter(filterP, dir, recursive, plugins);

        m_dirty = true;
    }
}

void Server::helpCommand() {
    sendReply(HELP_TEXT);
}

void Server::accessCommand() {
    if (m_arguments.count() < 2)
        return;

    setAccessGrant((m_user == m_arguments[0] && m_pwd == m_arguments[1]));

#ifdef _VERBOSE_SERVER
    qDebug() << "Server access granted: " << m_accessGranted;
#endif

    sendReply(m_accessGranted ? "TRUE" : "FALSE");
}

void Server::pingCommand() {
    sendMessage(PING_COMMAND);
}

void Server::removeFilterCommand() {
    if (m_arguments.count() < 1)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter...
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP) {
        filterP->stop(); // stop it...
        m_classifier.removeFilter(filterP); // and remove it

        m_dirty = true;
    } else
        delFilter(virDirPath); // let's make sure the browser and the server keep in sync...
}

void Server::pluginsCommand() {
    if (m_arguments.count() < 1)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP) {
        QString reply;

        // get the filter plugins...
        QStringList pluginFilenames = filterP->getPluginFilenames();
        QVector<PluginInterface *> *pluginsP = filterP->getPlugins();
        for (int i = 0; i < pluginsP->count() && i < pluginFilenames.count(); i++) {
            reply += pluginFilenames[i];
            reply += CMD_SEPARATOR;
            reply += pluginsP->at(i)->getName();
            reply += CMD_SEPARATOR;
        }

        // remove the trailing separator if any
        if (!reply.isEmpty())
            reply.chop(1);
        sendReply(reply);
    }
}

void Server::getPluginScriptCommand() {
    if (m_arguments.count() < 2)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP) {
        // read plugin name
        QString plugin = m_arguments[1];

        // get script
        sendReply(filterP->getScript(plugin));
    }
}

void Server::getPluginTipCommand() {
    if (m_arguments.count() < 2)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP) {
        // read plugin name
        QString plugin = m_arguments[1];

        // get tip
        sendReply(filterP->getPluginTip(plugin));
    }
}

void Server::getPluginScriptLastErrorCommand() {
    if (m_arguments.count() < 2)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP) {
        // read plugin name
        QString plugin = m_arguments[1];

        // get script's last error
        sendReply(filterP->getScriptLastError(plugin));
    }
}

void Server::setPluginScriptCommand() {
    if (m_arguments.count() < 3)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP) {
        // read plugin name
        QString plugin = m_arguments[1];

        // read script
        QString script = m_arguments[2];

        // set script
        filterP->setScript(plugin, script);

        m_dirty = true;
    }
}

void Server::attributesCommand() {
    if (m_arguments.count() < 2)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP) {
        // read plugin name
        QString plugin = m_arguments[1];

        // get attributes
        QString reply;
        QList<QString> attributes(filterP->getAttributes(plugin));
        for (QList<QString>::iterator i = attributes.begin(); i != attributes.end(); i++) {
            reply += *i;
            reply += CMD_SEPARATOR;
        }

        // remove the trailing separator if any
        if (!reply.isEmpty())
            reply.chop(1);
        sendReply(reply);
    }
}


void Server::getPluginAttributeClassCommand() {
    if (m_arguments.count() < 3)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP) {
        // read plugin name
        QString plugin = m_arguments[1];

        // read attribute name
        QString attribute = m_arguments[2];

        // get attribute's class
        sendReply(filterP->getAttributeClass(plugin, attribute));
    }
}

void Server::getPluginAttributeTipCommand() {
    if (m_arguments.count() < 3)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP) {
        // read plugin name
        QString plugin = m_arguments[1];

        // read attribute name
        QString attribute = m_arguments[2];

        // get attribute's tip
        sendReply(filterP->getAttributeTip(plugin, attribute));
    }
}

void Server::rescanCommand() {
    m_classifier.rescan();
}

void Server::scanCommand() {
    m_classifier.scan();
}

void Server::cleanupCommand() {
    // faster implementation
    m_db.cleanup();
}

void Server::filesCommand() {
    if (m_arguments.count() < 1)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP) {
        // retrieve files
        QStringList files = m_db.getFiles(filterP->getFilterId());
        for (QStringList::iterator i = files.begin(); i != files.end(); i++)
            sendReply(*i);
    }
}

void Server::availablePluginsCommand() {
    QDir workingDir(QCoreApplication::applicationDirPath());

    // get the directory entries
    QStringList entries = workingDir.entryList();

    QString reply;
    for (QStringList::iterator i = entries.begin(); i != entries.end(); i++) {
        QString file = *i;

        if (!file.startsWith("lib") && file.endsWith(PLUGIN_SUFFIX)) {
            reply += file;
            reply += CMD_SEPARATOR;
        }
    }

    // remove the trailing separator if any
    if (!reply.isEmpty())
        reply.chop(1);
    sendReply(reply);
}

void Server::getFilterDirCommand() {
    if (m_arguments.count() < 1)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP)
        // get filter's directory
        sendReply(filterP->getDirectory());
}

void Server::getFileAttributeValueCommand() {
    if (m_arguments.count() < 3)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP) {
        // read file full path
        QString file = m_arguments[1];

        // read attribute's name
        QString attribute = m_arguments[2];

        sendReply(filterP->getAttributeValue(file, attribute));
    }
}

void Server::getFileCommand() {
    if (m_arguments.count() < 1)
        return;

    // read file path
    QString filePath = m_arguments[0];

    // find file
    QFile file(filePath);
    if (!file.exists())
        return;

    // open the file, read its content, convert and send it by chunks,
    // then close the file.
    file.open(QIODevice::ReadOnly);
    QByteArray content;
    QString    cmdReply, offString;
    int        offset = 0;
    int        size = file.size();
    cmdReply = GET_FILE_COMMAND;
    cmdReply += CMD_SEPARATOR;
    cmdReply += filePath;
    cmdReply += CMD_SEPARATOR;
    displayActivity(tr("sending file: %1").arg(filePath));
    do  {
        displayProgress(0, size, offset);

        content = file.read(MAX_FILE_CHUNK_SIZE);
        offString.sprintf("%d", offset);
        sendMessage(cmdReply + offString + CMD_SEPARATOR + ClientServerInterface::binToCompressedBase64(content));

        offset += content.length();
    } while (!content.isEmpty());
    file.close();
}

void Server::rescanFilterCommand() {
    if (m_arguments.count() < 1)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP)
        filterP->rescanDirectory();
}

void Server::cleanupFilterCommand() {
    if (m_arguments.count() < 1)
        return;

    // read filter virtual path
    QString virDirPath = m_arguments[0];

    // find filter
    Filter *filterP = m_classifier.findFilter(virDirPath);
    if (filterP)
        filterP->cleanup();
}

void Server::saveSetCommand() {
    if (m_arguments.count() < 1)
        return;

    // read filter set
    QString set = m_arguments[0];
    if (!set.isEmpty()) {
        QString dirName = QCoreApplication::applicationDirPath();

        // add <m_user> directory prefix
        dirName += QDir::separator();
        dirName += m_user;

        QDir workingDir(dirName);

        // if the directory doesn't exist, create it
        if (!workingDir.exists())
            workingDir.mkpath(dirName);

        QFileInfo file(workingDir.absolutePath() + QDir::separator() + set + "." + FILTER_SET_SUFFIX);

        m_filtersFilename = file.absoluteFilePath();

        // save the filters
        m_classifier.saveFilters(m_filtersFilename);

        // save the db
        m_classifier.saveDatabase(m_filtersFilename + DB_EXTENSION);

        // send 'unexpected' reply
        QString msg = ADD_FILTER_SET_MSG;
        msg += CMD_SEPARATOR;
        msg += set;
        sendMessage(msg);

        m_dirty = false;
    }
}

void Server::loadSetCommand() {
    if (m_arguments.count() < 1)
        return;

    // read filter set
    QString set = m_arguments[0];
    if (!set.isEmpty()) {
        QString dirName = QCoreApplication::applicationDirPath();

        // add <m_user> directory prefix
        dirName += QDir::separator();
        dirName += m_user;

        QDir workingDir(dirName);

        QFileInfo file(workingDir.absolutePath() + QDir::separator() + set + "." + FILTER_SET_SUFFIX);
        if (file.exists()) {
            // stop all root filters
            m_classifier.stop();

            // delete all filters
            m_classifier.removeAllFilters();

            m_filtersFilename = file.absoluteFilePath();

            // load the db
            m_classifier.loadDatabase(m_filtersFilename + DB_EXTENSION);

            // load the filters
            m_classifier.loadFilters(m_filtersFilename);

            m_dirty = false;
        }
    }
}

void Server::deleteSetCommand() {
    if (m_arguments.count() < 1)
        return;

    // read filter set
    QString set = m_arguments[0];
    if (!set.isEmpty()) {
        QString dirName = QCoreApplication::applicationDirPath();

        // add <m_user> directory prefix
        dirName += QDir::separator();
        dirName += m_user;

        QDir workingDir(dirName);

        // delete the set
        QString filepath = workingDir.absolutePath() + QDir::separator() + set + "." + FILTER_SET_SUFFIX;
        QFileInfo fileInfo(filepath);
        if (fileInfo.exists()) {
            // delete the filter set
            QFile file(fileInfo.absoluteFilePath());
            file.remove();

            fileInfo = QFileInfo(filepath + DB_EXTENSION);
            if (fileInfo.exists()) {
                // delete the db
                QFile file(fileInfo.absoluteFilePath());
                file.remove();
            }

            // send 'unexpected' reply
            QString msg;
            msg = DELETE_SET_COMMAND;
            msg += CMD_SEPARATOR;
            msg += set;
            sendMessage(msg);
        }
    }
}

void Server::setsCommand() {
    QString dirName = QCoreApplication::applicationDirPath();

    // add <m_user> directory prefix
    dirName += QDir::separator();
    dirName += m_user;

    QDir workingDir(dirName);

    // get the directory entries
    QStringList entries = workingDir.entryList();

    QString reply;
    for (QStringList::iterator i = entries.begin(); i != entries.end(); i++) {
        QFileInfo file(*i);

        if (file.suffix() == FILTER_SET_SUFFIX) {
            reply += file.baseName();
            reply += CMD_SEPARATOR;
        }
    }

    // remove the trailing separator if any
    if (!reply.isEmpty())
        reply.chop(1);
    sendReply(reply);
}

void Server::isSetDirtyCommand() {
    sendReply(m_dirty ? "TRUE" : "FALSE");
}

void Server::setCommand() {
    QFileInfo file(m_filtersFilename);
    sendReply(file.baseName());
}

void Server::newSetCommand() {
    // stop all root filters
    m_classifier.stop();

    // delete all filters
    m_classifier.removeAllFilters();

    // resets everything
    QString dirName = QCoreApplication::applicationDirPath();

    // add <m_user> directory prefix
    dirName += QDir::separator();
    dirName += m_user;

    QDir workingDir(dirName);
    m_filtersFilename = workingDir.absolutePath() + QDir::separator() + DEFAULT_FILTERS_FILENAME;

    m_dirty = false;
}

