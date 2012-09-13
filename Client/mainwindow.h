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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QTreeWidget>
#include <QStatusBar>
#include <QThread>
#include <QLabel>
#include <QAction>
#include <QTableWidget>
#include <QProgressBar>
#include <QMenuBar>
#include <QToolBar>
#include <QTreeWidgetItem>
#include <QPixmap>
#include <QMap>
#include <QPushButton>
#include <QComboBox>
#include <QSortFilterProxyModel>
#include <QDesktopServices>
#include <QPlainTextEdit>

#include "common.h"
#include "serverproxy.h"
#include "treenode.h"
#include "attributesmodel.h"
#include "iconsviewmodel.h"
#include "detailsviewmodel.h"

#define TREE_ROOT_NAME                "/"      // simulates a root since top level items can't be deleted...
#define TREE_REFRESH_DELAY_SEC        2        // the UI is updated at this rate
#define NEW_FILES_PER_PASS            100      // don't insert more files than this in the tree per pass

#define STATUS_MSG_DELAY_MILLISEC     4000     // 'volatile' (mostly updates info) messages remain displayed during this delay

#define ICON_VIEW_INDEX               0
#define DETAILS_VIEW_INDEX            1

#define WINDOW_TITLE                  "SION! Browser"

#define HISTORY_SIZE                  100

#define SERVER_STATE_ICON_SIZE        24

namespace Ui {
    class MainWindow;
}

class Updater;
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QString hostName = "", QWidget *parentP = 0);
    ~MainWindow();

    inline TreeNode *getSelectedNode() {
        QList<QTreeWidgetItem *> nodes = m_treeP->selectedItems();
        return nodes.isEmpty() ? NULL : (TreeNode *)nodes[0];
    }

    inline TreeNode *getClipboardNode() {
        return (TreeNode *)m_rootP->findNode(m_clipboardNodePath);
    }

    void setUserAndPassword(QString user, QString pwd) {
        m_user = user;
        m_pwd = pwd;
    }

protected:
    void closeEvent(QCloseEvent *eventP);
    void showEvent(QShowEvent *eventP);

    // update the UI on our custom paint event only...
    inline void customEvent(QEvent *eventP) {
        Q_UNUSED(eventP);
        uiUpdate();
    }

private:
    bool                        m_isServerLocal;
    bool                        m_accessGranted;
    QString                     m_user, m_pwd;   // connection data

    // the last values returned by the server
    // and used to update the UI.
    // they're all related to the current filter set
    QString                     m_filterSet;        // ask on update (not costly)
    bool                        m_filterSetDirty;   // idem
    QStringList                 m_filterSets;       // ask once, then get 'deltas' from the server
    QStringList                 m_availablePlugins; // ask once, ie when receiving a first update on the set.
    QMap<QString, QStringList>  m_filters;          // the filters and their files

    Ui::MainWindow              *ui;                // UI references, to speed up things
    QAction                     *m_newFilterSetP;
    QAction                     *m_loadFilterSetP;
    QAction                     *m_saveFilterSetP;
    QAction                     *m_saveFilterSetAsP;
    QAction                     *m_deleteFilterSetP;
    QAction                     *m_newFilterP;
    QAction                     *m_deleteFilterP;
    QAction                     *m_openP;
    QAction                     *m_openFolderP;
    QAction                     *m_startFilterP;
    QAction                     *m_stopFilterP;
    QAction                     *m_showFilesP;
    QAction                     *m_cutFilterP;
    QAction                     *m_copyFilterP;
    QAction                     *m_pasteFilterP;
    QAction                     *m_rescanP;
    QAction                     *m_cleanupP;
    QAction                     *m_rescanFilterP;
    QAction                     *m_cleanupFilterP;
    QAction                     *m_refreshFilesP;
    QAction                     *m_setServerPathP;
    QAction                     *m_startServerP;
    QAction                     *m_stopServerP;
    QTreeWidget                 *m_treeP;
    FilterTreeNode              *m_rootP;
    QLabel                      *m_nodeNameP;
    QLabel                      *m_serverStatusP;
    QStatusBar                  *m_statusBarP;
    QMenuBar                    *m_menubarP;
    QToolBar                    *m_toolbarP;
    QTableView                  *m_attributesTableP;
    QTableView                  *m_contentTableP;
    QPlainTextEdit              *m_clientServerLogP;
    QTabWidget                  *m_scriptsBookP;
    QPushButton                 *m_backButtonP;
    QPushButton                 *m_forwardButtonP;
    QPushButton                 *m_upButtonP;
    QComboBox                   *m_displayComboP;

    QSettings                   m_settings;                 // the client last saved settings

    ServerProxy                 m_serverProxy;              // used to make the comm to the server easier

    QString                     m_clipboardNodePath;        // clipboard handling

    bool                        m_cut;

    QString                     m_serverPath;               // server executable file path

    QLabel                      m_serverStatusIcon;         // server status
    QPixmap                     m_serverIdleImg;
    QPixmap                     m_serverBusyImg;
    QPixmap                     m_serverDisconnectedImg;

    Updater                     *m_updaterP;                // sends an update signal 'every tick'
    QSemaphore                  m_updateSem;                // prevents reentrancy on lengthy updates

    bool                        m_updateMenu;               // force an update of some part of the UI
    bool                        m_updateTree;
    bool                        m_updateAttributesTable;
    bool                        m_updateContentTable;
    bool                        m_updateScriptsBook;
    bool                        m_pendingFiles;             // all files have not been inserted/deleted from the tree

    QStringList                 m_history;                  // history handling
    int                         m_historyIndex;
    bool                        m_navButtonPressed;

    TreeNode                    *m_lastSelectedNodeP;
    FilterTreeNode              *m_lastSelectedFilterP;     // optimize the models and UI updates by keeping the last selected filter

    AttributesModel             m_attributesModel;          // models (the tree doesn't use a custom one)
    IconsViewModel              m_iconsViewModel;
    DetailsViewModel            m_detailsViewModel;
    QSortFilterProxyModel       m_sortedDetailsViewModel;

    // empty server cached data (do this when the connection drops, the filter set changes, etc)
    inline void     clearSessionData(bool totalWipeOut = false) {
        m_updateSem.acquire();

        // if the server dropped, we need to completely reset the session data
        if (totalWipeOut) {
            FilterTreeNode::removeAll(m_rootP);
            m_filterSets.clear();
            m_availablePlugins.clear();
            m_filters.clear();
        }

        m_filterSet.clear();
        m_filterSetDirty = false;
        m_historyIndex = -1;
        m_history.clear();
        m_lastSelectedNodeP = NULL;
        m_lastSelectedFilterP = NULL;
        m_clipboardNodePath.clear();
        m_cut = false;

        m_updateMenu = true;
        m_updateTree = true;
        m_updateAttributesTable = true;
        m_updateContentTable = true;
        m_updateScriptsBook = true;

        m_updateSem.release();
    }

    // update the UI based on the server shadow data
    inline void     uiUpdate() {
        if (m_updateSem.tryAcquire()) {

            updateTitleBar();
            updateMenu();
            updateTree();
            updateAttributesTable();
            updateContentTable();
            updateScriptsBook();
            updateScriptError();
            updateStatusBar();

            m_lastSelectedNodeP = getSelectedNode();
            m_lastSelectedFilterP = (FilterTreeNode *)m_lastSelectedNodeP;
            if (m_lastSelectedNodeP && !m_lastSelectedNodeP->isFilter())
                m_lastSelectedFilterP = (FilterTreeNode *)m_lastSelectedNodeP;

            m_updateMenu = false;
            m_updateTree = m_pendingFiles;
            m_updateAttributesTable = false;
            m_updateContentTable = false;
            m_updateScriptsBook = false;

            m_updateSem.release();
        }
    }

    /**
      * Get the filter plugins and directory
      */
    inline void     requestFilterData(QString filter) {
        // request plugins and directory
        m_serverProxy.requestFilterPlugins(filter);
        m_serverProxy.requestFilterDirectory(filter);
    }

    /**
      * Get the file attribute values
      */
    void             requestFileData(FilterTreeNode *filterP, QString filepath) {
        QStringList plugins = filterP->getPlugins();
        for (int i = 0; i < plugins.count(); i++) {
            QStringList attributes = filterP->getPluginAttributes(plugins[i]);
            for (int j = 0; j < attributes.count(); j++)
                m_serverProxy.requestAttributeValue(filterP->getPath(), filepath, attributes[j]);
        }
    }

    void            readSettings();
    void            writeSettings();

    void            selectNode(QTreeWidgetItem *nodeP);

    QTreeWidgetItem *addFilterNode(QTreeWidgetItem *parentP, QString fullPath, QString nodePath);
    QTreeWidgetItem *findNode(QTreeWidgetItem *parentP, QString nodePath);

    void            showFiles(TreeNode *rootP, bool show, bool recursive);


    void            updateTitleBar();
    void            updateMenu();
    void            updateTree();
    void            updateAttributesTable();
    void            updateContentTable();
    void            updateScriptsBook();
    void            updateScriptError();
    void            updateStatusBar();

    void            emptyScriptsBook() {
        for (int i = m_scriptsBookP->count(); i > 0; i--) {
           QWidget *pageP = m_scriptsBookP->widget(i - 1);
           if (pageP) {
               m_scriptsBookP->removeTab(i - 1);
               delete pageP;
           }
       }
    }

    bool            pasteNodeCopy(TreeNode *parentP, TreeNode *childToCopyP, QString directory);

    inline void     playErrorSound() {
        QApplication::beep();
    }

public slots:
    // show the client server communication in the log tab
    void messageReceived(QString msg) {
        // add to the log
        m_clientServerLogP->appendHtml("<font color=red>" + msg);
    }

    void sendMessage(QString msg, bool urgent) {
        Q_UNUSED(urgent);
        // add to the log
        m_clientServerLogP->appendHtml("<font color=green>" + msg);
    }

    // the proxy let the app know the connection state by
    // forwarding the tcp interface signal
    void connectionStateChanged(bool connected) {
        if (!connected) {
            // the access to the server needs to be granted again
            m_accessGranted = false;

            // clean everything if the connection dropped and the window is still visible
            if (isVisible())
                clearSessionData(true);

            // clear the log
            m_clientServerLogP->clear();
        }

        // update connection state
        m_updateMenu = true;
    }

    // this is where the proxy raise the server's answers to the UI..
    void requestAccessGrant() {
        qDebug() << "Server requested autentication";

        m_accessGranted = false;
        m_serverProxy.requestAccess(m_user, m_pwd);
    }

    void accessGranted(QString user, QString pwd, bool granted) {
        qDebug() << "Server access granted for user= " << user << " and pwd= " << pwd << ": " << granted;

        m_accessGranted = granted;
    }

    void filterSet(QString set) {
        m_updateSem.acquire();

        m_filterSet = set;

        m_updateMenu = true;

        m_updateSem.release();
    }

    void filterSetAdd(QString set) {
        m_updateSem.acquire();

        if (!m_filterSets.contains(set)) {
            m_filterSets += set;

            m_updateMenu = true;
        }

        m_updateSem.release();
    }

    void filterSetDel(QString set) {
        m_updateSem.acquire();

        int index = m_filterSets.indexOf(set);
        if (index != -1) {
            m_filterSets.removeAt(index);

            m_updateMenu = true;
        }

        m_updateSem.release();
    }

    void filterSets(QStringList sets) {
        m_updateSem.acquire();

        m_filterSets = sets;

        m_updateMenu = true;

        m_updateSem.release();
    }

    void filterSetDirty(bool dirty) {
        m_updateSem.acquire();

        m_filterSetDirty = dirty;

        m_updateMenu = true;

        m_updateSem.release();
    }

    void filterFileAdd(QString filter, QString path) {
        m_updateSem.acquire();

        // is the filter known and does it show its files?
        FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(filter);
        if (filterP && filterP->isShowFiles() && m_filters.contains(filter)) {
            // add a '+' in front of the path
            // to mark it as an 'add file' path.
            path.prepend('+');

            // insert path in head
            m_filters[filter].insert(0, path);
            m_updateTree = true;

            if (m_lastSelectedNodeP == filterP)
                m_updateContentTable = true;
        }

        m_updateSem.release();
    }

    void filterFileDel(QString filter, QString path) {
        m_updateSem.acquire();

        // is the filter known and does it show its files?
        FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(filter);
        if (filterP && filterP->isShowFiles() && m_filters.contains(filter)) {
            // add a '-' in front of the path
            // to mark it as a 'remove file' path.
            path.prepend('-');

            // append path in queue
            m_filters[filter].append(path);
            m_updateTree = true;

            if (m_lastSelectedNodeP  == filterP)
                m_updateContentTable = true;
        }

        m_updateSem.release();
    }

    void filters(QStringList filters) {
        m_updateSem.acquire();

        for (QStringList::iterator i = filters.begin(); i != filters.end(); i++) {
            QString filter = *i;
            if (!filter.isEmpty())
                m_filters.insert(filter, QStringList());
        }

         m_updateTree = true;

         m_updateSem.release();
    }

    void directories(QString directory, QStringList directories) {
        Q_UNUSED(directory);
        Q_UNUSED(directories);
    }

    void filterAdd(QString filter) {
        m_updateSem.acquire();

        if (m_filters.isEmpty() || !m_filters.contains(filter)) {
            m_filters.insert(filter, QStringList());

            m_updateMenu = true;
            m_updateTree = true;
            m_updateContentTable = true;
        }

        m_updateSem.release();
    }

    void filterIsRunning(QString filter, bool running) {
        m_updateSem.acquire();

        FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(filter);
        if (filterP) {
            filterP->setRunning(running);

            m_updateMenu = true;
        }

        m_updateSem.release();
    }

    void filterDel(QString filter) {
        m_updateSem.acquire();

        if (!m_filters.isEmpty() && m_filters.contains(filter)) {
            m_filters.remove(filter);

            m_updateMenu = true;
            m_updateTree = true;
            m_updateContentTable = true;
        }

        m_updateSem.release();
    }

    void filterDirectory(QString filter, QString directory) {
        m_updateSem.acquire();

        FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(filter);
        if (filterP)
            filterP->setDirectory(directory);

        m_updateSem.release();
    }

    void plugins(QString filter, QStringList plugins) {
        m_updateSem.acquire();

        FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(filter);
        if (filterP) {
            filterP->setPlugins(plugins);

            // now ask for the rest of the plugin associated info
            plugins = filterP->getPlugins(); // just the plugin names now, not along with their filenames...
            bool update = false;
            for (int i = 0; i < plugins.count(); i++) {
                QString plugin = plugins[i];
                if (!plugin.isEmpty()) {
                    m_serverProxy.requestPluginAttributes(filter, plugin);
                    m_serverProxy.requestPluginTip(filter, plugin);
                    m_serverProxy.requestPluginScript(filter, plugin);
                    m_serverProxy.requestPluginScriptError(filter, plugin);
                    update = true;
                }
            }

            if (update && (m_lastSelectedNodeP  == filterP || m_lastSelectedFilterP == filterP)) {
                m_updateAttributesTable = true;
                m_updateContentTable = true;
                m_updateScriptsBook = true;
            }

        }

        m_updateSem.release();
    }

    void pluginTip(QString filter, QString plugin, QString tip) {
        m_updateSem.acquire();

        FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(filter);
        if (filterP) {
            filterP->setPluginTip(plugin, tip);

            if (m_lastSelectedNodeP  == filterP)
                m_updateScriptsBook = true;

        }

        m_updateSem.release();
    }

    void pluginScript(QString filter, QString plugin, QString script) {
        m_updateSem.acquire();

        FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(filter);
        if (filterP) {
            filterP->setPluginScript(plugin, script);

            if (m_lastSelectedNodeP == filterP)
                m_updateScriptsBook = true;
        }

        m_updateSem.release();
    }

    void pluginScriptError(QString filter, QString plugin, QString error) {
        m_updateSem.acquire();

        FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(filter);
        if (filterP)
            filterP->setPluginScriptError(plugin, error);

        m_updateSem.release();
    }

    void attributes(QString filter, QString plugin, QStringList attributes) {
        m_updateSem.acquire();

        FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(filter);
        if (filterP) {
            filterP->setPluginAttributes(plugin, attributes);

            // now ask for the rest of the attribute associated info
            bool update = false;
            for (int i = 0; i < attributes.count(); i++) {
                QString attribute = attributes[i];
                if (!attribute.isEmpty()) {
                    m_serverProxy.requestAttributeClass(filter, plugin, attribute);
                    m_serverProxy.requestAttributeTip(filter, plugin, attribute);
                    update = true;
                }
            }

            if (update && (m_lastSelectedNodeP  == filterP || m_lastSelectedFilterP == filterP)) {
                m_updateAttributesTable = true;
                m_updateContentTable = true;
            }
        }

        m_updateSem.release();
    }

    void attributeTip(QString filter, QString plugin, QString attribute, QString tip) {
        m_updateSem.acquire();

        FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(filter);
        if (filterP) {
            filterP->setPluginAttributeTip(plugin, attribute, tip);

            if (m_lastSelectedNodeP  == filterP || m_lastSelectedFilterP == filterP) {
                m_updateAttributesTable = true;
                m_updateContentTable = true;
            }
        }

        m_updateSem.release();
    }

    void attributeClass(QString filter, QString plugin, QString attribute, QString className) {
        m_updateSem.acquire();

        FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(filter);
        if (filterP) {
            filterP->setPluginAttributeClass(plugin, attribute, className);

            if (m_lastSelectedNodeP  == filterP || m_lastSelectedFilterP == filterP) {
                m_updateAttributesTable = true;
                m_updateContentTable = true;
            }
        }

        m_updateSem.release();
    }

    void attributeValue(QString filter, QString path, QString attribute, QString value) {
        m_updateSem.acquire();

        FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(filter);
        if (filterP) {
            filterP->setAttributeValue(path, attribute, value);

            if (m_lastSelectedNodeP  == filterP || m_lastSelectedFilterP == filterP) {
                m_updateAttributesTable = true;
                m_updateContentTable = true;
            }
        }

        m_updateSem.release();
    }

    void fileContent(QString filename, QString offStr, QString hexContent) {
        int offset = offStr.toInt();

        // save the file to the cache
        QFileInfo fileInfo(filename);

        QString filepath = QCoreApplication::applicationDirPath() + QDir::separator() + CACHE_DIRECTORY + QDir::separator() + fileInfo.fileName();
        if (!hexContent.isEmpty()) {
            QFile file(filepath);
            file.open(offset ? QIODevice::ReadWrite : QIODevice::ReadWrite | QIODevice::Truncate);
            file.seek(offset);
            file.write(ClientServerInterface::compressedBase64ToBin(hexContent));
            file.close();
        } else {
            // view/edit the file
            QUrl pathUrl;
            pathUrl.setPath("file:///" + filepath);
            QDesktopServices::openUrl(pathUrl);
        }
    }

    void availablePlugins(QStringList plugins) {
        m_updateSem.acquire();

        m_availablePlugins = plugins;

        m_updateMenu = true;

        m_updateSem.release();
    }

    void updateSessionData() {
        if (!m_updateSem.tryAcquire())
            return;

        // if the server is not busy, request data
        if (!m_serverProxy.isServerBusy()) {
            // doesn't update the UI directly but
            // send requests to the server here, ask for
            // the set, its dirty flag, filters, etc
            m_serverProxy.requestFilterSet();
            m_serverProxy.requestIsFilterSetDirty();

            // ask for filters if not retrieved yet (we may get some twice when starting the server though)
            if (m_filters.isEmpty())
                m_serverProxy.requestFilters();

            // ask for the available plugins if not retrieved yet
            if (m_availablePlugins.isEmpty())
                m_serverProxy.requestAvailablePlugins();

            // ask for the filter sets if not there yet
            if (m_filterSets.isEmpty())
                m_serverProxy.requestFilterSets();
        }

        // check the server responsiveness (this sets the busy flag to true...)
        m_serverProxy.pingServer();

        // post paint event to the window
        QMainWindow::update();


        m_updateSem.release();
    }

    void cancelScript();
    void applyScript();

private slots:
    void on_actionStopServer_triggered();
    void on_actionStartServer_triggered();
    void on_actionSetServerPath_triggered();
    void on_actionSaveFilterSetAs_triggered();
    void on_actionSaveFilterSet_triggered();
    void on_actionLoadFilterSet_triggered();
    void on_actionNewFilterSet_triggered();
    void on_actionOpenFolder_triggered();
    void on_actionPaste_triggered();
    void on_actionCopy_triggered();
    void on_actionCut_triggered();
    void on_actionAbout_triggered();
    void on_actionOpen_triggered();
    void on_actionNewFilter_triggered();
    void on_actionDeleteFilter_triggered();

    void on_actionShowFiles_toggled(bool );

    void on_treeWidget_itemExpanded(QTreeWidgetItem* item);
    void on_treeWidget_itemCollapsed(QTreeWidgetItem* nodeP);
    void on_treeWidget_itemSelectionChanged();

    void on_actionExit_triggered();
    void on_actionRescan_triggered();
    void on_actionCleanup_triggered();
    void on_actionRescanFilter_triggered();
    void on_actionCleanupFilter_triggered();
    void on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_actionRefreshFiles_triggered();
    void on_actionDeleteFilterSet_triggered();
    void on_backButton_clicked();
    void on_forwardButton_clicked();
    void on_upButton_clicked();
    void on_displayCombo_currentIndexChanged(int index);
    void on_contentTable_clicked(const QModelIndex &index);
    void on_contentTable_doubleClicked(const QModelIndex &index);
    void on_actionStart_triggered();
    void on_actionStop_triggered();
};

class Updater : public QThread {
    Q_OBJECT

public:
    explicit Updater(MainWindow *windowP) : QThread((QObject *)windowP) {
        m_windowP = windowP;
    }

    inline void stop() {
        m_stop = true;
    }

protected:
    void run();

signals:
    void update();

private:
    volatile bool   m_stop;
    MainWindow      *m_windowP;
};

#endif // MAINWINDOW_H
