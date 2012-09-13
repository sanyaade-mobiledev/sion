/*
 * SION! Server file plugin interface.
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

#ifndef SCRIPT_H
#define SCRIPT_H

#include <QObject>
#include <QString>
#include <QDebug>

//#define _VERBOSE_SCRIPT 1

/**
 * The script class defines a rules script. It just holds a script text and the
 * last error that was encountered when running this script (if any).
 */

class Script : public QObject
{
    Q_OBJECT

public:
    explicit Script(QString string, QObject *parentP = 0);

    void setScript(QString script) {
#ifdef _VERBOSE_SCRIPT
        qDebug() << "Script::setScript(" << script << ")";
#endif
        m_script = script;
        m_lastError = ""; // no error since this script hasn't been executed yet.
    }

    void setError(QString error) {
#ifdef _VERBOSE_SCRIPT
        qDebug() << "Script::setError(" << error << ")";
#endif

        m_lastError = error;
    }

    QString getScript() {
#ifdef _VERBOSE_SCRIPT
        qDebug() << "Script::getScript() will return " << m_script;
#endif

        return m_script;
    }

    QString getLastError() {
#ifdef _VERBOSE_SCRIPT
        qDebug() << "Script::getLastError() will return " << m_lastError;
#endif

        return m_lastError;
    }

signals:

public slots:

private:
    QString m_script;     // the script text
    QString m_lastError;  // the last error, if any
};

#endif // SCRIPT_H
