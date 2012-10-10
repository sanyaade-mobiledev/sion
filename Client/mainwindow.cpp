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

#include <QSplitter>
#include <QCloseEvent>
#include <QTreeWidget>
#include <QUrl>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QFileDialog>
#include <QProcess>
#include <QFileIconProvider>

#include "ui_mainwindow.h"
#include "mainwindow.h"
#include "treenode.h"
#include "newfilterdialog.h"
#include "scriptpage.h"
#include "listbrowser.h"
#include "editfilterdialog.h"
#include "pastefilterdialog.h"
#include "qfileinfoext.h"

/**
  * The updater thread sends an update signal every TREE_REFRESH_DELAY seconds to the Browser
  * main window, so the latter can refresh its content based on the server's information.
  */
void Updater::run() {
    m_stop = false;
    while (!m_stop) {
        update();
        sleep(TREE_REFRESH_DELAY_SEC);
        QApplication::postEvent(m_windowP, new QEvent(QEvent::User));
    }
}

/**
  * Builds the ui, the main window and the settings. Restore the settings if available.
  */
MainWindow::MainWindow(QString hostName, QWidget *parentP) :
    QMainWindow(parentP),
    ui(new Ui::MainWindow),
    m_settings("Intel Corporation", "SION! Browser"),
    m_serverProxy(this, hostName),
    m_serverIdleImg(":/icon/ServerIdle"),
    m_serverBusyImg(":/icon/ServerBusy"),
    m_serverDisconnectedImg(":/icon/ServerDisconnected"),
    m_updateSem(1),
    m_attributesModel(this),
    m_iconsViewModel(this),
    m_detailsViewModel(this) {
    m_cut = false;
    m_historyIndex = -1;
    m_lastSelectedNodeP = NULL;
    m_updateMenu = true;
    m_updateTree = true;
    m_updateAttributesTable = true;
    m_updateContentTable = true;
    m_updateScriptsBook = true;
    m_pendingFiles = false;
    m_serverIdleImg = m_serverIdleImg.scaled(SERVER_STATE_ICON_SIZE, SERVER_STATE_ICON_SIZE);
    m_serverBusyImg = m_serverBusyImg.scaled(SERVER_STATE_ICON_SIZE, SERVER_STATE_ICON_SIZE);
    m_serverDisconnectedImg = m_serverDisconnectedImg.scaled(SERVER_STATE_ICON_SIZE, SERVER_STATE_ICON_SIZE);

    // initial state for server updated values
    m_accessGranted = false;
    m_filterSetDirty = false;

    // server is considered local if host is unset, equal to "localhost" or "127.0.0.1"
    qDebug() << "server hostname: " << hostName;
    m_isServerLocal = (hostName.isEmpty() || hostName == "localhost" || hostName == "127.0.0.1");
    qDebug() << "server is local: " << m_isServerLocal;

    // builds the ui
    ui->setupUi(this);

    // keep a couple refs on widgets and actions
    m_backButtonP = findChild<QPushButton *>("backButton");
    m_forwardButtonP = findChild<QPushButton *>("forwardButton");
    m_upButtonP = findChild<QPushButton *>("upButton");

    m_newFilterSetP = findChild<QAction *>("actionNewFilterSet");
    m_loadFilterSetP = findChild<QAction *>("actionLoadFilterSet");
    m_saveFilterSetP = findChild<QAction *>("actionSaveFilterSet");
    m_saveFilterSetAsP = findChild<QAction *>("actionSaveFilterSetAs");
    m_deleteFilterSetP = findChild<QAction *>("actionDeleteFilterSet");

    m_openP = findChild<QAction *>("actionOpen");
    m_openFolderP = findChild<QAction *>("actionOpenFolder");

    m_startFilterP = findChild<QAction *>("actionStart");
    m_stopFilterP = findChild<QAction *>("actionStop");

    m_newFilterP = findChild<QAction *>("actionNewFilter");
    m_deleteFilterP = findChild<QAction *>("actionDeleteFilter");

    m_showFilesP = findChild<QAction *>("actionShowFiles");

    m_cutFilterP = findChild<QAction *>("actionCut");
    m_copyFilterP = findChild<QAction *>("actionCopy");
    m_pasteFilterP = findChild<QAction *>("actionPaste");

    m_rescanP = findChild<QAction *>("actionRescan");
    m_cleanupP = findChild<QAction *>("actionCleanup");

    m_rescanFilterP = findChild<QAction *>("actionRescanFilter");
    m_cleanupFilterP = findChild<QAction *>("actionCleanupFilter");

    m_refreshFilesP = findChild<QAction *>("actionRefreshFiles");

    m_setServerPathP = findChild<QAction *>("actionSetServerPath");
    m_startServerP = findChild<QAction *>("actionStartServer");
    m_stopServerP = findChild<QAction *>("actionStopServer");

    m_treeP = findChild<QTreeWidget *>("treeWidget");
    m_statusBarP = findChild<QStatusBar *>("statusBar");
    m_attributesTableP = findChild<QTableView *>("attributesTable");
    m_contentTableP = findChild<QTableView *>("contentTable");
    m_clientServerLogP = findChild<QPlainTextEdit *>("clientServerLogEdit");
    m_clientServerLogP->setMaximumBlockCount(HISTORY_SIZE);

    m_scriptsBookP = findChild<QTabWidget *>("scriptsBook");
    m_toolbarP = findChild<QToolBar *>("mainToolBar");
    m_menubarP = menuBar();

    m_nodeNameP = new QLabel();
    m_statusBarP->addWidget(m_nodeNameP);

    m_serverStatusP = new QLabel();
    m_statusBarP->addPermanentWidget(m_serverStatusP);
    m_statusBarP->addPermanentWidget(&m_serverStatusIcon);
    m_serverStatusIcon.setToolTip(tr("Green: Communicating\nOrange: Thinking\nRed: Disconnected"));

    // creates a dummy root filter
    m_rootP = new FilterTreeNode(TREE_ROOT_NAME, "");
    m_treeP->invisibleRootItem()->addChild(m_rootP);
    m_treeP->header()->setResizeMode(NAME_HEADER_COLUMN, QHeaderView::Stretch);
    m_treeP->header()->setResizeMode(TYPE_HEADER_COLUMN, QHeaderView::Fixed);
    m_treeP->header()->setResizeMode(ENTRIES_HEADER_COLUMN, QHeaderView::Fixed);

    // create the content table view combo
    m_displayComboP = findChild<QComboBox *>("displayCombo");
    QIcon detailsViewIcon(":/icon/DetailsView");
    m_displayComboP->insertItem(DETAILS_VIEW_INDEX, detailsViewIcon, tr("Details View"));
    QIcon iconViewIcon(":/icon/IconView");
    m_displayComboP->insertItem(ICON_VIEW_INDEX, iconViewIcon, tr("Icon View"));
    m_displayComboP->setCurrentIndex(ICON_VIEW_INDEX);

    // sets the tree pop up menu
    QMenu *menuP = new QMenu(m_treeP);
    menuP->addAction(m_openP);
    menuP->addAction(m_openFolderP);
    m_treeP->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_treeP->addAction(m_openP);
    m_treeP->addAction(m_openFolderP);

    // set the tree header
    // see treenode.h for the order of the columns
    QStringList treeHeader;
    treeHeader << tr("Name") << tr("Type") << tr("Entries");
    m_treeP->setHeaderLabels(treeHeader);

    // set the table models
    m_sortedDetailsViewModel.setSourceModel(&m_detailsViewModel);
    m_attributesTableP->setModel(&m_attributesModel);
    m_contentTableP->setModel(&m_iconsViewModel);

    // reload state
    readSettings();

    // sets up the window updater
    m_updaterP = new Updater(this);

    // connect the window updater
    connect(m_updaterP, SIGNAL(update()), this, SLOT(updateSessionData()));

    // connect to the client server interface to show log
    connect(m_serverProxy.getTcpInterface(), SIGNAL(messageReceived(QString)), this, SLOT(messageReceived(QString)));
    connect(&m_serverProxy, SIGNAL(sendMessage(QString,bool)), this, SLOT(sendMessage(QString, bool)));
}

/**
  * Deletes the ui.
  */
MainWindow::~MainWindow() {
    delete m_updaterP;
    delete ui;
}

/**
  * This method is called when the window is first displayed. This is where the initial UI update
  * takes place and where the updater thread is started.
  */
void MainWindow::showEvent(QShowEvent *eventP) {
    Q_UNUSED(eventP);
    if (!m_updaterP->isRunning()) {
        // initial update
        clearSessionData();
        uiUpdate();

        // start the updater thread
        m_updaterP->start();
    }
}

/**
  * This method is called when the window is closed. This is where the application's context
  * shall be saved or the close event rejected.
  */
void MainWindow::closeEvent(QCloseEvent *eventP) {
    // do we have a dirty filter set?
    if (m_filterSetDirty) {
        if (QMessageBox::question(this,
                                  tr("Unsaved Filter Set!"),
                                  tr("The filter set hasn't been saved. Continue?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
            eventP->ignore();
            return;
        }
    }

    // stop updates
    m_updaterP->stop();
    m_updaterP->wait();

    // save settings and exit.
    writeSettings();
    eventP->accept();
}

/**
  * Saves the application's context/settings
  */
void MainWindow::writeSettings() {
    m_statusBarP->showMessage(tr("Writing settings..."), STATUS_MSG_DELAY_MILLISEC);

    // window state
    m_settings.beginGroup("MainWindow");

    m_settings.setValue("size", size());
    m_settings.setValue("pos", pos());

    QSplitter *splitterP = findChild<QSplitter *>("horizontalSplitter");
    if (splitterP)
        m_settings.setValue("horizontalSplitterState", splitterP->saveState());

    splitterP = findChild<QSplitter *>("verticalSplitter");
    if (splitterP)
        m_settings.setValue("verticalSplitterState", splitterP->saveState());

    m_settings.endGroup();

    // server state
    m_settings.beginGroup("Server");

    m_settings.setValue("serverPath", m_serverPath);

    m_settings.endGroup();
}

/**
  * Restores the application's context/settings
  */
void MainWindow::readSettings() {
    m_statusBarP->showMessage(tr("Reading settings..."), STATUS_MSG_DELAY_MILLISEC);

    // server state
    m_settings.beginGroup("Server");

    m_serverPath = m_settings.value("serverPath").toString();

    m_settings.endGroup();

    // window state
    m_settings.beginGroup("MainWindow");

    resize(m_settings.value("size", QSize(400, 400)).toSize());
    move(m_settings.value("pos", QPoint(200, 200)).toPoint());

    QSplitter *splitterP = findChild<QSplitter *>("horizontalSplitter");
    if (splitterP)
        splitterP->restoreState(m_settings.value("horizontalSplitterState").toByteArray());

    splitterP = findChild<QSplitter *>("verticalSplitter");
    if (splitterP)
        splitterP->restoreState(m_settings.value("verticalSplitterState").toByteArray());

    m_settings.endGroup();
}

/**
  * Updates the browser title bar. Displays the name of the filter set and its dirty
  * state. Doesn't require a 'force update' flag to be set since this update is not
  * expensive.
  */
void MainWindow::updateTitleBar() {
    QString title = WINDOW_TITLE;

    if (!m_filterSet.isEmpty())
        title += QString(": ") + m_filterSet + (m_filterSetDirty ? "*" : "");

    this->setWindowTitle(title);
}

/**
  * Updates the browser menu bar.
  *
  */
void MainWindow::updateMenu() {
    if (!m_updateMenu)
        return;

    TreeNode *selNodeP = getSelectedNode();

    bool isSelection = (selNodeP != NULL);
    bool isRoot = isSelection && selNodeP == m_rootP;
    bool isPath = isSelection && !selNodeP->getPath().isEmpty();
    bool isFilter = isSelection && selNodeP->isFilter();
    bool isShowFiles = isSelection && selNodeP->isShowFiles();
    bool isFile = isSelection && !selNodeP->isFilter();
    bool isParent = isSelection && selNodeP->parent() != NULL;
    bool isConnected = m_serverProxy.isConnected();
    bool isFilters = !m_filters.isEmpty();
    bool isFilterSet = !m_filterSet.isEmpty();
    bool isFilterSets = !m_filterSets.isEmpty();
    bool isClipboard = getClipboardNode() != NULL;
    bool isServerPath = !m_serverPath.isEmpty();
    bool isHistory = !m_history.isEmpty();
    bool isBackOk = isHistory && m_historyIndex > 0;
    bool isForwardOk = isHistory && m_historyIndex < m_history.count() - 1;
    bool isUpOk = isSelection && !isRoot;
    bool isFilterRunning = isFilter && ((FilterTreeNode *)selNodeP)->isRunning();
    bool isPlugins = !m_availablePlugins.isEmpty();

    // update the navigation buttons
    m_backButtonP->setEnabled(isBackOk);
    m_forwardButtonP->setEnabled(isForwardOk);
    m_upButtonP->setEnabled(isUpOk);

    // update the actions based on the node type/state and the server state (not cut/copy/paste without
    // server support
    m_newFilterSetP->setEnabled(isConnected);
    m_loadFilterSetP->setEnabled(isConnected && isFilterSets);
    m_saveFilterSetP->setEnabled(isConnected && isFilterSet);
    m_saveFilterSetAsP->setEnabled(isConnected && isFilters);
    m_deleteFilterSetP->setEnabled(isConnected && isFilterSets);

    m_openFolderP->setEnabled(m_isServerLocal && (isSelection && !isRoot && isPath && ((isFile && isParent) || isFilter)));
    m_openP->setEnabled(isSelection && !isRoot && isPath);

    m_newFilterP->setEnabled(isConnected && isPlugins && isFilter);
    m_deleteFilterP->setEnabled(isConnected && isFilter && !isRoot);

    m_startFilterP->setEnabled(isFilter && !isRoot && !isFilterRunning);
    m_stopFilterP->setEnabled(isFilter && !isRoot && isFilterRunning);

    m_rescanP->setEnabled(isConnected);
    m_cleanupP->setEnabled(isConnected);

    m_rescanFilterP->setEnabled(isConnected && isFilter && !isRoot);
    m_cleanupFilterP->setEnabled(isConnected && isFilter && !isRoot);

    m_refreshFilesP->setEnabled(isConnected);

    m_showFilesP->setEnabled(isConnected && isFilter && !isRoot);
    m_showFilesP->setChecked(isFilter && !isRoot && isShowFiles);
    m_copyFilterP->setEnabled(isConnected && isFilter && !isRoot);
    m_cutFilterP->setEnabled(isConnected && isFilter && !isRoot);
    m_pasteFilterP->setEnabled(isConnected && isFilter && isClipboard);

    // update the server actions based on the server path, location (remote vs local)
    // and connection state
    m_setServerPathP->setEnabled(m_isServerLocal);
    m_startServerP->setEnabled(m_isServerLocal && (isServerPath && !isConnected));
    m_stopServerP->setEnabled(m_isServerLocal && isConnected);
}

/**
  * Updates the name and path of the selected node in the status bar. The status bar is updated
  * every UI update pass because it's not expensive.
  */
void MainWindow::updateStatusBar() {
    TreeNode *selNodeP = getSelectedNode();

    // update the server status
    bool connected = m_serverProxy.isConnected();

    QString serverState = !connected ? tr("disconnected") : (m_accessGranted ? tr("connected") : tr("waiting for credentials"));

    m_serverStatusP->setText(tr("Server: ") + serverState);

    // update the selected node info
    if (selNodeP && selNodeP != m_rootP) {
        if (selNodeP->isFilter()) {
            QString path = selNodeP->getPath();
            QString directory = ((FilterTreeNode *)selNodeP)->getDirectory();
            if (!path.isEmpty()) {
                QString entries;
                entries.sprintf("%d", selNodeP->childCount());
                QString filterInfo = path + tr(", watching: \"") + directory + tr("\", entries: ") + entries;
                m_nodeNameP->setText(filterInfo);
            }
        } else
            m_nodeNameP->setText(selNodeP->getPath());
    } else
        m_nodeNameP->setText("");

    // update the server status icon
    m_serverStatusIcon.setPixmap(m_serverProxy.isConnected() ?
                                 (m_serverProxy.isServerBusy() ? m_serverBusyImg : m_serverIdleImg) :
                                 m_serverDisconnectedImg);

    m_statusBarP->update();
}

/**
  * Updates the browser tree. Since this update is based on the delta between the tree content (filters and
  * files).
  */
void MainWindow::updateTree() {
    if (!m_updateTree)
        return;

    m_statusBarP->showMessage(tr("Updating Tree..."), STATUS_MSG_DELAY_MILLISEC);

    QCursor currentCursor = cursor();
    setCursor(QCursor(Qt::WaitCursor));

    m_statusBarP->showMessage(tr("Updating Tree..."));

#ifdef _VERBOSE_BROWSER
    qDebug() << "Updating Filters";
#endif

    // deactivate sorting to speed up tree manipulation
    m_treeP->setSortingEnabled(false);

    // add the nodes if they are not present
    QStringList filters = m_filters.keys();
    m_pendingFiles = false;
    for (QStringList::iterator i = filters.begin(); i != filters.end(); i++) {
        QString filter = *i;
        if (filter.isEmpty())
            continue;

#ifdef _VERBOSE_BROWSER
        qDebug() << "Checking filter: " << filter;
#endif
        FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(filter);

        // add the node if not present.
        if (!filterP) {
            filterP = (FilterTreeNode *)m_rootP->addFilterNode(filter, filter);
            if (!filterP)
                continue; // #### most likely mispelled filter path, fixme in new filter dialog validation
            requestFilterData(filter);
        }

        // if the filter doesn't show its files, nothing to do
        if (!filterP->isShowFiles())
            continue;

        // update the files for this filter
        QStringList files = m_filters[filter];
        int         numFiles = 0;
        for (QStringList::iterator i = files.begin(); numFiles < NEW_FILES_PER_PASS && i != files.end(); i++) {
            QString path = *i;

            // remove the file from the list
            m_filters[filter].removeAt(m_filters[filter].indexOf(path));

            // get the '-' or '+' tag
            QChar   mark = path.at(0);
            path.remove(0, 1);
            if (mark == '+') {
                if (!filterP->findNode(path)) {
                    filterP->addNode(path);
                    if (!filterP)
                        continue; // #### most likely mispelled filter path, fixme in new filter dialog validation
                    requestFileData(filterP, path);
                }
            }
            else
                filterP->deleteNode(path);

            ++numFiles;
        }

        // if there are files pending, keep updating the tree on the next update pass
        m_pendingFiles |= !m_filters[filter].isEmpty();
    }

    // if done with the files, remove the filters that are not present anymore...
    if (!m_pendingFiles)
        FilterTreeNode::removeFilters(m_rootP, m_filters.keys());

    // reactivate sorting
    m_treeP->setSortingEnabled(true);

    m_statusBarP->clearMessage();

    setCursor(currentCursor);
}

/**
  * Updates the scripts book with the selected filter's plugin's scripts. This update method
  * shall not be called outside of the tree selection change scope, because it'll empty the
  * scripts book and disrupt any ongoing script editing session.
  */
void MainWindow::updateScriptsBook() {
    if (!m_updateScriptsBook)
        return;

    TreeNode *selNodeP = getSelectedNode();
    QString filter;

    m_statusBarP->showMessage(tr("Updating scripts book"));

    QCursor currentCursor = cursor();
    setCursor(QCursor(Qt::WaitCursor));

    emptyScriptsBook();

    // create a tab per plugin
    if (selNodeP && selNodeP != m_rootP && selNodeP->isFilter()) {
        FilterTreeNode *filterP = (FilterTreeNode *)selNodeP;
        filter = filterP->getPath();
        QStringList plugins = filterP->getPlugins();
        for (QStringList::iterator i = plugins.begin(); i != plugins.end(); i++) {
            QString plugin = *i;
            ScriptPage *pageP = new ScriptPage(filterP, plugin, filterP->getPluginScript(plugin), m_scriptsBookP);
            m_scriptsBookP->addTab(pageP, plugin);
            pageP->setToolTip(filterP->getPluginTip(plugin));
            connect(pageP, SIGNAL(cancelScript()), this, SLOT(cancelScript()));
            connect(pageP, SIGNAL(applyScript()), this, SLOT(applyScript()));
        }
    }

    m_statusBarP->clearMessage();

    setCursor(currentCursor);
}

/**
  * Sets the error text of the current script book page with the associated filter/plugin's
  * last script error.
  */
void MainWindow::updateScriptError() {
    // retrieve the selected tab
    ScriptPage *pageP = (ScriptPage *)m_scriptsBookP->currentWidget();
    if (!pageP)
        return;

    QString filter = pageP->getFilter();
    QString plugin = pageP->getPlugin();

    // display last received error (most likely not the one requested above)
    FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(filter);
    pageP->setScriptError(filterP ? filterP->getPluginScriptError(plugin) : "");

    // request the error for the next pass
    m_serverProxy.requestPluginScriptError(filter, plugin);
}

/**
  * Updates the attributes table with the selected filter or file attributes. If
  * no changes occured (selection, incoming server data) since the last call, the
  * model won't be updated.
  */
void MainWindow::updateAttributesTable() {
    m_statusBarP->showMessage(tr("Updating attributes view"));

    QCursor currentCursor = cursor();
    setCursor(QCursor(Qt::WaitCursor));

    m_attributesModel.update(m_updateAttributesTable);

    // don't show header if the table is empty
    if (m_attributesModel.rowCount() == 0)
        m_attributesTableP->horizontalHeader()->hide();
    else
        m_attributesTableP->horizontalHeader()->show();

    m_statusBarP->clearMessage();

    setCursor(currentCursor);
}

/**
  * Updates the table content with the selected filter or file content. If
  * no changes occured (selection, incoming server data) since the last call, the
  * model won't be updated.
  */
void MainWindow::updateContentTable() {
    m_statusBarP->showMessage(tr("Updating content view"));

    QCursor currentCursor = cursor();
    setCursor(QCursor(Qt::WaitCursor));

    switch (m_displayComboP->currentIndex()) {
        case ICON_VIEW_INDEX:
            // display a simple icon view
            m_contentTableP->horizontalHeader()->setVisible(false);
            m_contentTableP->horizontalHeader()->setSortIndicatorShown(false);
            m_contentTableP->horizontalHeader()->setAlternatingRowColors(false);
            m_contentTableP->horizontalHeader()->setMovable(false);
            m_contentTableP->setSortingEnabled(false);
            m_contentTableP->setSelectionBehavior(QAbstractItemView::SelectItems);
            m_iconsViewModel.update(m_contentTableP, m_updateContentTable);
            break;

        case DETAILS_VIEW_INDEX:
            // display a detailled, sortable detailled list view
            m_contentTableP->horizontalHeader()->setVisible(true);
            m_contentTableP->horizontalHeader()->setSortIndicatorShown(true);
            m_contentTableP->horizontalHeader()->setAlternatingRowColors(true);
            m_contentTableP->horizontalHeader()->setMovable(true);
            m_contentTableP->setSortingEnabled(true);
            m_contentTableP->setSelectionBehavior(QAbstractItemView::SelectRows);
            m_detailsViewModel.update(m_updateContentTable);
            break;
    }

    // don't show header if the table is empty
    if (m_contentTableP->model()->rowCount() == 0)
        m_contentTableP->horizontalHeader()->hide();

    m_statusBarP->clearMessage();

    setCursor(currentCursor);
}

/**
  * Shows/Hides the files in the selected filter node. This will trigger the
  * removal or proxy request of the filter's children files.
  */
void MainWindow::on_actionShowFiles_toggled(bool show) {
    TreeNode *selNodeP = getSelectedNode();

    if (!selNodeP || !selNodeP->isFilter()) // this shouldn't happen...
        return;

    // CTRL/SHIFT will show/hide all children files
    showFiles(selNodeP, show, QApplication::keyboardModifiers() == (Qt::SHIFT | Qt::CTRL));
}

/**
  * Deletes the selected filter node. The tree is not changed directly because
  * the update is deferred to the associated incoming server data receipt.
  */
void MainWindow::on_actionDeleteFilter_triggered() {
    QString  path;
    TreeNode *selNodeP = getSelectedNode();

    if (!selNodeP || !selNodeP->isFilter() || selNodeP == m_rootP)
        return;

    path = selNodeP->getPath();

    // prompt the user for confirmation
    if (QMessageBox::question(this,
                              tr("Delete Filter!"),
                              tr("Do you really want to delete filter '%1'?").arg(path),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
        return;

    // pause before removing a filter
    m_serverProxy.stopFilter(selNodeP->getPath());

    // then remove
    m_serverProxy.removeFilter(path);
}

/**
  * Creates a new filter. The tree is not changed directly because
  * the update is deferred to the associated incoming server data receipt.
  */
void MainWindow::on_actionNewFilter_triggered() {
    TreeNode *selNodeP = getSelectedNode();

    // if a filter node is selected, pass the parent path to the new filter dialog.
    if (selNodeP && !selNodeP->isFilter())
        selNodeP = (TreeNode *)selNodeP->parent();

    if (!selNodeP || !selNodeP->isFilter()) // ok, this can't happen, but I hate asserts
        return;

    // pause before adding a new filter
    m_serverProxy.stopFilter(selNodeP->getPath());

    // get the parent (if any)'s path
    QString parentPath = selNodeP->getPath();
    QString parentDir = ((FilterTreeNode *)selNodeP)->getDirectory();

    // get the available plugins
    QStringList plugins = m_availablePlugins;

    // show the dialog
    NewFilterDialog dialog(&m_serverProxy, m_rootP, parentPath, parentDir, &plugins, this);

    // create the filter if the user hit 'accept' and the data was validated
    if (dialog.exec() == QDialog::Accepted)
        m_serverProxy.addFilter(dialog.parentPath(),
                                dialog.filterPath(),
                                dialog.directory(),
                                dialog.recursive(),
                                dialog.plugins());
}

/**
  * Rescans the filters. The files must be recursively removed from
  * the filter's subtree, since the server won't send file removal
  * messages.
  */
void MainWindow::on_actionRescan_triggered() {
    m_rootP->removeFiles(true);
    m_serverProxy.rescan();
}

/**
  * Expands a filter node. Request filter's information from the server if not
  * there yet. A semaphore is used to prevent reentrancy due to the cascading events
  * when doing a recursive expansion.
  */
void MainWindow::on_treeWidget_itemExpanded(QTreeWidgetItem* nodeP) {
    static QSemaphore expandingTree(1);

    if (!nodeP)
        return;

    FilterTreeNode *filterP = (FilterTreeNode *)nodeP;
    if (filterP->getDirectory().isEmpty()) {
        // request plugins and directory
        QString filter = filterP->getPath();
        requestFilterData(filter); // should have been done already when adding the node to the tree
    }

    // request the filter's files' attributes if not loaded yet (should've been done when updating the tree)
    for (int i = 0; i < filterP->childCount(); i++) {
        TreeNode *nodeP = (TreeNode *)filterP->child(i);
        if (!nodeP->isFilter()) {
            QString path = nodeP->getPath();
            if (!filterP->fileDataLoaded((FileTreeNode *)nodeP))
                requestFileData(filterP, path);
        }
    }

    if (!expandingTree.tryAcquire()) // the expandAllChildren will generate a bunch of events we need to ignore
        return;

    // recursively open the children filters if CTRL+SHIFT held down
    if (QApplication::keyboardModifiers() == (Qt::SHIFT | Qt::CTRL))
        filterP->expandAllChildren();

    expandingTree.release();
}


/**
  * Collapses a filter node. A semaphore is used to prevent reentrancy due to the cascading events
  * when doing a recursive collapse.
  */
void MainWindow::on_treeWidget_itemCollapsed(QTreeWidgetItem* nodeP) {
    static QSemaphore collapsingTree(1);

    if (!nodeP)
        return;

    if (!collapsingTree.tryAcquire()) // the collapseAllChildren will generate a bunch of events we need to ignore
        return;

    // recursively close the children filters if CTRL+SHIFT held down
    if (QApplication::keyboardModifiers() == (Qt::SHIFT | Qt::CTRL))
        ((TreeNode *)nodeP)->collapseAllChildren();

    collapsingTree.release();
}

/**
 * Updates the actions based on the newly selected tree node. The scripts book is updated
 * and the history enriched if we're not navigating based on it.
 */
void MainWindow::on_treeWidget_itemSelectionChanged() {
    TreeNode *selNodeP = getSelectedNode();

    if (selNodeP) {
        // update filter running state
        if (selNodeP->isFilter())
            m_serverProxy.requestIsFilterRunning(selNodeP->getPath());

        // enrich the navigation history only if the node was selected by the user
        if (!m_navButtonPressed) {
            m_history += selNodeP->getNodePath();
            m_historyIndex = m_history.count() - 1;
        }

        // will be set again in back/forward when pressed
        m_navButtonPressed = false;

        // make the item visible
        m_treeP->scrollToItem(selNodeP);
    }

    // update the UI
    m_updateMenu = true;
    m_updateAttributesTable = true;
    m_updateContentTable = true;
    m_updateScriptsBook = true;
    uiUpdate();
}

/**
  * Sets the content of the script book page with the current filter/plugin's script value.
  * Called when the user hits the 'cancel' button.
  */
void MainWindow::cancelScript() {
    // retrieve the selected tab
    ScriptPage *pageP = (ScriptPage *)m_scriptsBookP->currentWidget();
    if (!pageP)
        return;

    // restores the plugin's script back into the page
    FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(pageP->getFilter());
    if (filterP)
        pageP->setScript(filterP->getPluginScript(pageP->getPlugin()));
}

/**
  * Sets the current filter/plugin's script with the content of the script book page.
  * Called when the user hits the 'apply' button.
  */
void MainWindow::applyScript() {
    // retrieve the selected tab
    ScriptPage *pageP = (ScriptPage *)m_scriptsBookP->currentWidget();
    if (!pageP)
        return;

    QString filter = pageP->getFilter();
    QString plugin = pageP->getPlugin();
    QString script = pageP->getScript();
    pageP->setScript(script); // forces an update of the scripts edit pages
    m_serverProxy.setPluginScript(filter, plugin, script);

    // updates the filter script
    FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(filter);
    if (filterP) {
        filterP->setPluginScript(plugin, script);

        // force a rescan of the directory if the plugin is running
        if (filterP->isRunning())
            m_serverProxy.rescanFilter(filter);
    }
}

/**
  * This slot is called when the user hits the 'exit' menu item. It generates a call to the
  * closeEvent method by invoking the close method.
  */
void MainWindow::on_actionExit_triggered() {
    close();
}

/**
  * If a file is selected, opens it using the associated native application on the desktop. If a
  * filter is selected, opens the modifyFilterDialog to let the user view/modify the filter.
  */
void MainWindow::on_actionOpen_triggered() {
    TreeNode *selNodeP = getSelectedNode();

    // get the selected file
    if (!selNodeP || selNodeP == m_rootP)
        return;

    // if a filter, open the modify filter dialog
    if (selNodeP->isFilter()) {
        FilterTreeNode *filterP = (FilterTreeNode *)selNodeP;

        // if a filter node is selected, pass the parent path to the EDIT filter dialog.

        // get the parent (if any)'s path
        QString filterName = selNodeP->text(NAME_HEADER_COLUMN);
        QString parentPath;
        QString parentDir;
        FilterTreeNode *parentP = (FilterTreeNode *)selNodeP->parent();
        if (parentP) {
            parentPath = parentP->getPath();
            parentDir = parentP->getDirectory();
        }

        // show the dialog
        EditFilterDialog dialog(&m_serverProxy,
                                m_rootP,
                                parentPath,
                                parentDir,
                                filterName,
                                filterP->getDirectory(),
                                &m_availablePlugins,
                                this);

        // modify the filter if the user hit 'accept' and the data was validated
        if (dialog.exec() == QDialog::Accepted) {
            // pause before modifying the node
            m_serverProxy.stopFilter(filterP->getPath());

            QString directory = dialog.directory();
            QStringList plugins = dialog.plugins();
            QString filter = dialog.filterPath();
            m_serverProxy.modifyFilter(filter, directory, dialog.recursive(), plugins);

            // update the filter
            filterP->setText(NAME_HEADER_COLUMN, dialog.filterName());
            filterP->setPath(filter);
            requestFilterData(filter);
        }
    } else {
        // if a file, asks the server for its content
        m_serverProxy.requestFileContent(selNodeP->getPath());
    }
}

/**
  * Pops up the about dialog
  */
void MainWindow::on_actionAbout_triggered() {
    QMessageBox::about(this, tr("About SION! Browser"), tr("SION! (Sort It Out Now!) Browser is a javascript rules based browser. It interacts with the SION! (meta indexer) Server to let you virtually organize content as you wish on your computer."));
}

/**
  * Cut/Copy/Paste nodes. Manipulates filters only.
  */
void MainWindow::on_actionCut_triggered() {
    TreeNode *selNodeP = getSelectedNode();
    if (!selNodeP || !selNodeP->isFilter() || selNodeP == m_rootP)
        return;

    // keep the ref top the selected node
    m_clipboardNodePath = selNodeP->getPath();

    m_cut = true;
}

/**
  * Copy the selected filter reference into the clipboard.
  */
void MainWindow::on_actionCopy_triggered() {
    on_actionCut_triggered();

    m_cut = false;
}

/**
  * Pastes the filter contained in the clipboard. The user is prompted for
  * a name and a physical directory to associate with the new filter copy.
  * Returns TRUE if the complete subtree was pasted, else returns FALSE.
  */
bool MainWindow::pasteNodeCopy(TreeNode *parentP, TreeNode *childToCopyP, QString directory) {
    bool pasted = false;

    // get the parent (if any)'s path
    QString parentPath = parentP->getPath();
    QString filterName = childToCopyP->text(NAME_HEADER_COLUMN);
    FilterTreeNode *sourceP = (FilterTreeNode *)childToCopyP;

    // get the plugin from the copied filter
    QStringList pluginNames = sourceP->getPluginFilenames();

    // show the dialog
    PasteFilterDialog *dialogP = new PasteFilterDialog(&m_serverProxy, m_rootP, parentPath, filterName, directory, &pluginNames, this);

    // create the filter if the user hit 'accept' and the data was validated
    if (dialogP->exec() == QDialog::Accepted) {
        QString filterPath = dialogP->filterPath();
        m_serverProxy.addFilter(dialogP->parentPath(),
                                filterPath,
                                dialogP->directory(),
                                dialogP->recursive(),
                                dialogP->plugins());

        // we're in a full async model, we can't use anything we didn't have before
        // creating the new filter, like its plugins...
        QStringList newPluginNames = dialogP->plugins();
        for (int i = 0; i < newPluginNames.count(); i++) {
            QString pluginName = newPluginNames[i];
            int index = pluginNames.indexOf(pluginName);
            if (index != -1) {
                QStringList plugins = sourceP->getPlugins();
                if (index < plugins.count()) {
                    QString plugin = plugins[index];
                    QString script = sourceP->getPluginScript(plugin);
                    m_serverProxy.setPluginScript(filterPath, plugin, script);
                }
            }
        }

        // get the ref to the newly added filter
        TreeNode *childCopyP;

        // wait for the server to complete the node creation and the node to be in the tree
        // #### find a better way to do this, invoking processEvents will cause application slots
        //      to be potentially triggered, possibly causing an invalid tree state (node deletion
        //      while going through the tree here for example).
        do {
            update();
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            childCopyP = (TreeNode *)parentP->findNode(dialogP->filterName());
            if (!childCopyP) {
                // if we have a comm problem, stop pasting
                if (!m_serverProxy.isConnected()) {
                    delete dialogP;
                    return false;
                }
            }
        } while (!childCopyP);

        pasted = true;

        // paste its children's copies under childP copy
        for (int i = 0; i < childToCopyP->childCount(); i++) {
            TreeNode *nextChildToCopyP = (TreeNode *)childToCopyP->child(i);
            if (nextChildToCopyP->isFilter())
                pasted &= pasteNodeCopy(childCopyP, (TreeNode *)childToCopyP->child(i), dialogP->directory());
        }
    }

    delete dialogP;

    return pasted;
}

/**
  * Handles the paste action. Cut or Copy is distinguished here. Calls pasteNodeCopy
  * to actually paste the node referenced by clipboard.
  */
void MainWindow::on_actionPaste_triggered() {
    TreeNode *selNodeP = getSelectedNode();
    TreeNode *clipboardP = getClipboardNode();

    if (!clipboardP || !selNodeP || clipboardP == selNodeP)
        return;

    // get the selected file
    if (!selNodeP->isFilter()) // this shouldn't happen cause the action is supposedly disabled
        return;

    // paste the m_clipboard subtree under the selected node
    // and cut if appropriate (cut was selected and all nodes were
    // pasted)
    if (pasteNodeCopy(selNodeP, clipboardP, ((FilterTreeNode *)selNodeP)->getDirectory()) && m_cut) {
        // remove m_clipboardP from tree.
        m_serverProxy.removeFilter(m_clipboardNodePath);

        // no cut left
        m_clipboardNodePath.clear();
        m_cut = false;
    }
}

/**
  * Opens the folder containing the selected content. This work only if we're running
  * on the same host as the server.
  */
void MainWindow::on_actionOpenFolder_triggered() {
    TreeNode *selNodeP = getSelectedNode();

    // get the selected file
    if (!selNodeP || selNodeP == m_rootP)
        return;

    // open the associated folder
    QUrl    pathUrl;
    QString path;

    if (selNodeP->isFilter()) {
        // get the associated directory
        path = ((FilterTreeNode *)selNodeP)->getDirectory();
        if (path.isEmpty())
            return;
    } else {
        // get the associated directory
        QFileInfoExt info(selNodeP->getPath());
        path = info.absolutePath();
    }

    pathUrl.setPath("file:///" + path);
    QDesktopServices::openUrl(pathUrl);
}

/**
  * Removes all filters and create a New (unnamed) Filter Set. The session data is
  * cleared to force a complete update of the shadowed server data.
  */
void MainWindow::on_actionNewFilterSet_triggered() {
    // check the dirty state
    if (m_filterSetDirty) {
        if (QMessageBox::question(this,
                                  tr("Unsaved Filter Set!"),
                                  tr("The filter set hasn't been saved. Continue?"),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
            return;
    }

    // create a new filter set
    m_serverProxy.newFilterSet();

    // invalidate any pending clipboard operation
    m_clipboardNodePath.clear();
    m_cut = false;

    // force update
    clearSessionData(true);
}

/**
  * Removes all filters and load an existing Filter Set. The session data is
  * cleared to force a complete update of the shadowed server data.
  */
void MainWindow::on_actionLoadFilterSet_triggered() {
    // check the dirty state
    if (m_filterSetDirty) {
        if (QMessageBox::question(this,
                                  tr("Unsaved Filter Set!"),
                                  tr("The filter set hasn't been saved. Continue?"),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
            return;
    }

    // let the user browse them and select
    ListBrowser browser(false, m_filterSets, this);

    // loads the set if the user hit 'accept'
    if (browser.exec() == QDialog::Accepted) {
        // the tree *MUST* be cleared before the new filters comes in, else
        // deleted filters and added filters may collide if they have the same
        // names
        clearSessionData(true);
        uiUpdate();

        // now load
        m_serverProxy.loadFilterSet(browser.getSelection());

        // invalidate any pending clipboard operation
        m_clipboardNodePath.clear();
        m_cut = false;
    }
}

/**
  * Saves all filters from the (current) Filter Set.
  */
void MainWindow::on_actionSaveFilterSet_triggered() {
    m_serverProxy.saveFilterSet(m_filterSet);
}

void MainWindow::on_actionSaveFilterSetAs_triggered() {
    ListBrowser browser(true, m_filterSets, this);

    // saves the set if the user hit 'accept'
    if (browser.exec() == QDialog::Accepted)
        m_serverProxy.saveFilterSet(browser.getSelection());
}

/**
  * Prompts the user with a directory picker to let her select the
  * server path. This works only if the browser is running on the same
  * host as the server.
  */
void MainWindow::on_actionSetServerPath_triggered() {
    QString path = QFileDialog::getExistingDirectory(this, tr("Select Directory"),
                                                     QCoreApplication::applicationDirPath(),
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);

    if (!path.isEmpty()) {
        // check Path
        QFile serverPath(path + QDir::separator() + SION_SERVER_EXECUTABLE_NAME);
        if (serverPath.exists())
            m_serverPath = path;
    }
}

/**
  * Starts the server from the predefined server path. This works only if the browser is
  * running on the same host as the server.
  */
void MainWindow::on_actionStartServer_triggered() {
    QProcess launcher;
    QStringList arguments;

    arguments << "user=" + m_user << "pwd=" + m_pwd;
    launcher.startDetached(m_serverPath + QDir::separator() + SION_SERVER_EXECUTABLE_NAME,
                           arguments,
                           m_serverPath);
}

/**
  * Stops the server by sending it an exit command.
  */
void MainWindow::on_actionStopServer_triggered() {
    m_serverProxy.stopServer();
    clearSessionData(true);
}

/**
  * Cleans up the whole DB.
  */
void MainWindow::on_actionCleanup_triggered() {
    m_rootP->removeFiles(true);
    m_serverProxy.cleanup();
}

/**
  * Rescans the selected filter.
  */
void MainWindow::on_actionRescanFilter_triggered() {
    TreeNode *selNodeP = getSelectedNode();

    if (!selNodeP || !selNodeP->isFilter() || selNodeP == m_rootP)
        return;

    selNodeP->removeFiles(true);
    m_serverProxy.rescanFilter(selNodeP->getPath());
}

/**
  * Cleans up the selected filter. The selected filter's files must be
  * removed from the tree since the server won't send file removal
  * messages.
  */
void MainWindow::on_actionCleanupFilter_triggered() {
    TreeNode *selNodeP = getSelectedNode();

    if (!selNodeP || !selNodeP->isFilter() || selNodeP == m_rootP)
        return;

    selNodeP->removeFiles(true);
    m_serverProxy.cleanupFilter(selNodeP->getPath());
}

/**
  * Opens the selected file. This works only if the browser is
  * running on the same host as the server.
  */
void MainWindow::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *itemP, int column) {
    Q_UNUSED(column);
    // get the selected file
    if (!itemP || itemP == m_rootP)
        return;

    // is it a file
    if (((TreeNode *)itemP)->isFilter())
        return;

    // a file, open it
    m_serverProxy.requestFileContent(((TreeNode *)itemP)->getPath());
}

/**
  * Shows/hides the files in the selected filter. Hides or request the associated
  * files.
  */
void MainWindow::showFiles(TreeNode *rootP, bool show, bool recursive) {
    if (!rootP || !rootP->isFilter())
        return;

    rootP->showFiles(show);

    // if root is expanded and now has to show files, request files
    if (rootP->isShowFiles())
        m_serverProxy.requestFilterFiles(rootP->getPath());
    else
        rootP->removeFiles();

    if (recursive) {
        // down through the kids now
        for (int i = 0; i < rootP->childCount(); i++)
            showFiles((TreeNode *)rootP->child(i), show, recursive);
    }
}

/**
  * Forces a refresh of the files in the whole tree.
  */
void MainWindow::on_actionRefreshFiles_triggered() {
    // iteratee through all the filters and refresh them
    QStringList filters = m_filters.keys();
    for (int i = 0; i < filters.count(); i++) {
        FilterTreeNode *filterP = (FilterTreeNode *)m_rootP->findNode(filters[i]);
        if (filterP)
            m_serverProxy.requestFilterFiles(filterP->getPath());
    }
}

/**
  * Deletes the selected filter set.
  */
void MainWindow::on_actionDeleteFilterSet_triggered() {
    // let the user browse them and select
    ListBrowser browser(false, m_filterSets, this);

    // loads the set if the user hit 'accept'
    if (browser.exec() == QDialog::Accepted) {
        m_serverProxy.deleteFilterSet(browser.getSelection());
        m_serverProxy.requestFilterSets();
    }
}

/**
  * Selects nodeP in the tree. If necessary, expands the parent and
  * scrolls to the newly selected node.
  */
void MainWindow::selectNode(QTreeWidgetItem *nodeP) {
    // change the selection: expands the parent filter if necessary, deselects the previously selected node, selects
    // the new node and scroll to it.
    if (nodeP) {
        QTreeWidgetItem *parentP = nodeP->parent();

        if (parentP && !parentP->isExpanded())
            parentP->setExpanded(true);

        QTreeWidgetItem *selNodeP = getSelectedNode();
        if (selNodeP)
            selNodeP->setSelected(false);

        nodeP->setSelected(true);
    }
}

/**
  * Navigates back.
  */
void MainWindow::on_backButton_clicked() {
    // do we have a valid history index?
    if (m_historyIndex < 1)
        return;

    // get the previous node
    m_navButtonPressed = true;
    QString nodePath = m_history[--m_historyIndex];
    selectNode(nodePath.isEmpty() ? m_rootP : m_rootP->findNode(nodePath));
}

/**
  * Navigates forward.
  */
void MainWindow::on_forwardButton_clicked() {
    // do we have a valid history index?
    if (m_historyIndex < 0 || m_historyIndex >= m_history.count() - 1)
        return;

    // get the next node
    m_navButtonPressed = true;
    QString nodePath = m_history[++m_historyIndex];
    selectNode(nodePath.isEmpty() ? m_rootP : m_rootP->findNode(nodePath));
}

/**
  * Navigates up.
  */
void MainWindow::on_upButton_clicked() {
    TreeNode *selNodeP = getSelectedNode();

    // need the selected filter parent
    if (!selNodeP)
        return;

    // go up
    QTreeWidgetItem *parentP = selNodeP->parent();
    if (parentP)
        selectNode(parentP);
}

/**
  * The content display view has changed, rebuild it.
  */
void MainWindow::on_displayCombo_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    switch (index) {
        case ICON_VIEW_INDEX:
            m_contentTableP->setModel(&m_iconsViewModel);
            break;

        case DETAILS_VIEW_INDEX:
            m_contentTableP->setModel(&m_sortedDetailsViewModel);
            break;
    }
}

/**
  * Selects in the tree the node corresponding to the selected item in the content page.
  */
void MainWindow::on_contentTable_clicked(const QModelIndex &index) {
    TreeNode *selNodeP = getSelectedNode();

    // need the selected filter parent
    if (!selNodeP)
        return;

    if (!selNodeP->isFilter())
        selNodeP = (TreeNode *)selNodeP->parent();

    // get the selected item user data
    QVariant userData = m_contentTableP->model()->data(index, Qt::UserRole);
    QTreeWidgetItem *nodeP = m_rootP->findNode(userData.toString());
    if (nodeP)
        selectNode(nodeP);
}


/**
  * Opens the double clicked item.
  */
void MainWindow::on_contentTable_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    on_actionOpen_triggered();
}

/**
  * Starts/Stops the scan
  */
void MainWindow::on_actionStart_triggered() {
    TreeNode *selNodeP = getSelectedNode();

    // need the selected filter
    if (!selNodeP || !selNodeP->isFilter())
        return;

    // starts the filter
    m_serverProxy.startFilter(selNodeP->getPath());
    m_serverProxy.requestIsFilterRunning(selNodeP->getPath());
}

void MainWindow::on_actionStop_triggered() {
    TreeNode *selNodeP = getSelectedNode();

    // need the selected filter
    if (!selNodeP || !selNodeP->isFilter())
        return;

    // stops the filter
    m_serverProxy.stopFilter(selNodeP->getPath());
    m_serverProxy.requestIsFilterRunning(selNodeP->getPath());
}
