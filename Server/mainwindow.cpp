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

#include <QMessageBox>
#include <QDesktopWidget>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "servercommands.h"

/**
  * Builds the (very simple) UI and connect the server interface and the window to the server so
  * the latter can receive commands and reply.
  */
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    m_interfaceP = NULL;
    m_serverRunning = false;

    m_messageP = findChild<QPlainTextEdit *>("messageEdit");
    m_activityP = findChild<QLineEdit *>("activityEdit");
    m_progressP = findChild<QProgressBar *>("progressBar");
    m_messageP->setMaximumBlockCount(HISTORY_SIZE);

    // bring server windows on top of the window stack
    // setWindowFlags(windowFlags() | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint);

    // set the window position to the center of the screen
    QRect rect = frameGeometry();
    rect.moveCenter(QDesktopWidget().availableGeometry().center());
    move(rect.topLeft());
}

/**
  * The window is destroyed, let's do some cleanup with the server and tcp interface.
  */
MainWindow::~MainWindow() {
    delete ui;
}

/**
  * Called when the window shows up. If the server can't be started, pops
  * up a warning dialog and shuts down everything.
  */
void MainWindow::showEvent(QShowEvent *eventP) {
    Q_UNUSED(eventP);

    // this event is also sent when the window is de-iconified
    // so don't restart the server...
    if (m_serverRunning)
        return;

    // starts the server (loads previous sessions settings and listens)
    if (!m_server.start()) {
        QMessageBox::warning(this,
                             windowTitle(),
                             tr("The server couldn't be started. The socket must be in use."),
                             QMessageBox::Close);
        close();
    }

    // connects the server to the tcp interface
    m_interfaceP = new ServerInterface(&m_server, this);
    connect(&m_server, SIGNAL(newConnection()), m_interfaceP, SLOT(connectToPeer()));

    // transfers the messages sent to the server through the window's signal
    connect(&m_server, SIGNAL(sendMessage(QString)), this, SLOT(displayAndSendMessage(QString)));
    connect(&m_server, SIGNAL(displayActivity(QString)), this, SLOT(displayActivity(QString)));
    connect(&m_server, SIGNAL(displayProgress(int,int,int)), this, SLOT(displayProgress(int, int, int)));

    // the server now runs
    m_serverRunning = true;
}

/**
  * This method is called when the window is closed. The server is stopped and the connection closed
  * there.
  */
void MainWindow::closeEvent(QCloseEvent *eventP) {
    Q_UNUSED(eventP);

    // if the server was correctly started, shut everything down
    if (m_interfaceP) {
        // stop the server
        m_server.stop();

        if (m_interfaceP->isConnected())
            m_interfaceP->disconnectFromPeer();

        delete m_interfaceP;
    }
}

/**
  * The tcp interface raised a connection state change signal.
  */
void MainWindow::connectionStateChanged(bool connected) {
    if (!connected)
        // drop credentials before next connection
        m_server.setAccessGrant(false);
}

/**
  * The server has signaled activity to report to the user attention
  */
void MainWindow::displayActivity(QString string) {
    // add to the log
    m_activityP->setText(string);
}

/**
  * The server has signaled activity progress to report to the user attention
  */
void MainWindow::displayProgress(int min, int max, int value) {
    // update the progress bar
    m_progressP->setRange(min, max);
    m_progressP->setValue(value);
}

/**
  * The server has signaled for a new message to send. Display the
  * message, then signal to the tcp interface.
  */
void MainWindow::displayAndSendMessage(QString string) {
    if (string.isEmpty())
        return;

    // add to the log
    m_messageP->appendHtml("<font color=green>" + string);

    // pass command over to the tcp interface
    sendMessage(string);
}

/**
  * The Tcp Server Interface has received a string, display it and pass it over to the server.
  */
void MainWindow::messageReceived(QString string) {
    if (string.isEmpty())
        return;

    // add to the log
    m_messageP->appendHtml("<font color=red>" + string);

    // if we have an exit command,
    // we process it in the closeEvent.
    if (string.toUpper() == EXIT_COMMAND) {
        // non granted clients can't shut the server
        if (m_server.isAccessGranted())
            close();

        return;
    }

    // pass command over to server
    m_server.processCommand(string);
}

