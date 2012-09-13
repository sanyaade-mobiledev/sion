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

#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include <QObject>
#include <QString>
#include <QVariant>

/**
 * An attribute name/value pair. Attributes are used by plugins to both publish the meta-data
 * (attributes) they extract from associated content and store the values extracted from the
 * last visited file.
 *
 */
class Attribute : public QObject
{
    Q_OBJECT

public:
    explicit Attribute(QString name, QString tip, QString className, QObject *parent = 0);

    QString  m_className; // attribute's class
    QVariant m_value;   // attribute's value
    QString  m_tip;     // attribute's help tip
    QString  m_name;    // attribute's name
};

#endif // ATTRIBUTE_H
