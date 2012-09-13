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

#ifndef SERVER_H
#define SERVER_H

#include <QSettings>
#include <QTcpServer>

#include "servercommands.h"
#include "classifier.h"
#include "clientserverinterface.h"

//#define _VERBOSE_SERVER 1

#define DEFAULT_FILTERS_FILENAME "DefaultSet.SION!"

/**
  * This is the server front end. It just redirects most of the requests to the classifier
  * and analyzed commands, send replies (or unexpected messages based on the signals raised
  * by the underlying components: classifier, filters, watchers).
  */

#define HELP_TEXT "SION!Server Help\n\n\
SION! Server is a rule based meta-indexer. It accepts the following commands:\n\
        \t'access:user:pwd' : grants access to the server as <user> with password <pwd>\n\
        \t'exit' : forces the server to exit\n\
        \t'help' : displays this help messages\n\
        \t'ping' : pings the server to get its response time\n\
        \t'is_set_dirty' : returns 'TRUE' if the current filter set has been modified since last saved\n\
        \t'save_set:set_name' : saves the current filter set\n\
        \t'load_set:set_name' : loads a filter set\n\
        \t'delete_set:set_name' : deletes a filter set\n\
        \t'sets' : lists the available filter sets\n\
        \t'set' : returns the current filter set name\n\
        \t'new_set' : resets the current filter set (deletes filters, clears dirty flag, resets name)\n\
        \t'filters' : lists all the installed filters\n\
        \t'directories' : lists all the directories under the given directory\n\
        \t'add_filter:parent_filter:filter:directory:is_recursive:pluginlib_1:..:pluginlib_n' : adds/installs a new filter\n\
        \t'modify_filter:filter:directory:recursive:plugin 1:..:plugin n' : modifies a filter\n\
        \t'get_filter_dir:filter' : returns the physical directory associated with the filter\n\
        \t'remove_filter:filter' : removes/uninstalls an existing filter\n\
        \t'available_plugins' : lists the available plugins\n\
        \t'plugins:filter' : lists the plugins ('filename'/'title' pairs) used by a filter\n\
        \t'get_plugin_tip:filter:plugin' : gets the tip associated with a plugin\n\
        \t'get_plugin_script:filter:plugin' : gets the javascript associated with a plugin\n\
        \t'set_plugin_script:filter:plugin:script' : sets the javascript associated with a plugin\n\
        \t'get_plugin_script_last_error:filter:plugin': gets the last error of the javascript associated with a plugin\n\
        \t'attributes:filter:plugin': gets the attributes associated with a plugin\n\
        \t'get_plugin_attribute_class:filter:plugin:attribute': gets the class of an attribute associated with a plugin\n\
        \t'get_plugin_attribute_tip:filter:plugin:attribute': gets the tip of an attribute associated with a plugin\n\
        \t'files:filter' : lists (from db) the files retained by a filter\n\
        \t'get_file_attribute_value:filter:file:attribute' : gets the value of a (filtered in) file attribute\n\
        \t'get_file:file' : gets the given file content hexadecimal representation\n\
        \t'filter_rescan:filter' : forces a full scan (cleanup +) of a filter\n\
        \t'filter_cleanup:filter' : removes (from db) the files retained by a filter\n\
        \t'filter_start:filter' : starts a filter\n\
        \t'filter_stop:filter' : stops a filter\n\
        \t'filter_is_running:filter' : returns true if the filter is running\n\
        \t'cleanup' : removes (from db) the files retained by all filters\n\
        \t'scan' : forces a full scan of the system (all filters)\n\
        \t'rescan' : forces a full (cleanup +) rescan of the system (all filters)\n\n"

class Server : public QTcpServer {
    Q_OBJECT

public:
    explicit Server(QObject *parentP = 0);

    bool start();
    void stop();

    void setUserAndPassword(QString user, QString pwd) {
        m_user = user;
        m_pwd = pwd;
    }

    bool isAccessGranted() {
        return m_accessGranted;
    }
    void setAccessGrant(bool grant) {
        m_accessGranted = grant;
    }

    void requestCredentials() {
        sendMessage(REQUEST_ACCESS_GRANT_MSG, true);
    }

    void processCommand(QString &string);

signals:
    void sendMessage(QString string, bool urgent = false);
    void displayActivity(QString string);
    void displayProgress(int min, int max, int value);

public slots:
    // these slots are invoked by the filters directly to inform the client of 'unexpected' changes
    void newFilter(QString virtualDirectoryPath) {
        // the filter informs the server of file changes
        Filter *filterP = m_classifier.findFilter(virtualDirectoryPath);
        if (!filterP)
            return;

        connect(filterP, SIGNAL(newFile(QString, QString)), this, SLOT(newFile(QString, QString)));
        connect(filterP, SIGNAL(delFile(QString, QString)), this, SLOT(delFile(QString, QString)));
        connect(filterP, SIGNAL(displayActivity(QString)), this, SIGNAL(displayActivity(QString)));
        connect(filterP, SIGNAL(displayProgress(int, int, int)), this, SIGNAL(displayProgress(int, int, int)));

        // send 'unexpected' reply
        QString msg = ADD_FILTER_MSG;
        msg += CMD_SEPARATOR;
        msg += virtualDirectoryPath;
        displayActivity(tr("new filter installed: %1").arg(virtualDirectoryPath));
        sendMessage(msg);
    }

    void delFilter(QString virtualDirectoryPath) {
        // send 'unexpected' reply
        QString msg = DEL_FILTER_MSG;
        msg += CMD_SEPARATOR;
        msg += virtualDirectoryPath;
        displayActivity(tr("filter removed: %1").arg(virtualDirectoryPath));
        sendMessage(msg);
    }

    void newFile(QString virtualDirectoryPath, QString path) {
        // send 'unexpected' reply
        QString msg = ADD_FILE_MSG;
        msg += CMD_SEPARATOR;
        msg += virtualDirectoryPath;
        msg += CMD_SEPARATOR;
        msg += path;
        displayActivity(tr("file 'filtered in': %1").arg(path));
        sendMessage(msg);
    }

    void delFile(QString virtualDirectoryPath, QString path) {
        // send 'unexpected' reply
        QString msg = DEL_FILE_MSG;
        msg += CMD_SEPARATOR;
        msg += virtualDirectoryPath;
        msg += CMD_SEPARATOR;
        msg += path;
        displayActivity(tr("file 'filtered out': %1").arg(path));
        sendMessage(msg);
    }

 private:
    ServerDatabase  m_db;

    QString         m_filtersFilename;    // this is the file were we're storing the filters
    bool            m_dirty;              // the filters have been modified since last written to the filters file

    QSettings       m_settings;

    Classifier      m_classifier;

    bool            m_exitServer;          // the server was asked to exit

    QString         m_command;
    QStringList     m_arguments;

    bool            m_accessGranted;       // user and pwd were accepted
    QString         m_user;                // user and password to access the server,
    QString         m_pwd;                 // passed as command line arguments when starting
                                           // the server.

    void    sendReply(QString reply, bool urgent = false);

    void    accessCommand();
    void    helpCommand();
    void    pingCommand();
    void    addFilterCommand();
    void    modifyFilterCommand();
    void    getFilterDirCommand();
    void    removeFilterCommand();
    void    filtersCommand();
    void    directoriesCommand();
    void    availablePluginsCommand();
    void    pluginsCommand();
    void    attributesCommand();
    void    getPluginAttributeTipCommand();
    void    getPluginAttributeClassCommand();
    void    getPluginTipCommand();
    void    getPluginScriptCommand();
    void    getPluginScriptLastErrorCommand();
    void    setPluginScriptCommand();
    void    cleanupCommand();
    void    cleanupFilterCommand();
    void    rescanCommand();
    void    rescanFilterCommand();
    void    scanCommand();
    void    startFilterCommand();
    void    stopFilterCommand();
    void    isFilterRunningCommand();
    void    filesCommand();
    void    getFileAttributeValueCommand();
    void    getFileCommand();
    void    saveSetCommand();
    void    loadSetCommand();
    void    deleteSetCommand();
    void    setsCommand();
    void    isSetDirtyCommand();
    void    setCommand();
    void    newSetCommand();
};

#endif // SERVER_H
