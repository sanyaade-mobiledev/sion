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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QProgressBar>

#include "server.h"
#include "clientserverinterface.h"

#define HISTORY_SIZE  100

namespace Ui {
class MainWindow;
}

/**
  * The server has a window to display indexing progress information.
  */
class MainWindow : public QMainWindow {
    Q_OBJECT
    
signals:
    void sendMessage(QString string, bool urgent = false);

public slots:
    void messageReceived(QString string);
    void displayAndSendMessage(QString string);
    void displayActivity(QString string);
    void displayProgress(int min, int max, int value);
    void connectionStateChanged(bool);

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void setUserAndPassword(QString user, QString pwd) {
        m_server.setUserAndPassword(user, pwd);
    }

protected:
    void showEvent(QShowEvent *eventP);
    void closeEvent(QCloseEvent *eventP);

private:
    Ui::MainWindow        *ui;
    ServerInterface       *m_interfaceP;
    Server                m_server;
    QPlainTextEdit        *m_messageP;
    QLineEdit             *m_activityP;
    QProgressBar          *m_progressP;
    bool                  m_serverRunning;
};

#endif // MAINWINDOW_H
