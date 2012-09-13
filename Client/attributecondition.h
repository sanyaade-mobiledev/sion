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

#ifndef ATTRIBUTECONDITION_H
#define ATTRIBUTECONDITION_H

#include <QWidget>

#include "treenode.h"

namespace Ui {
class AttributeCondition;
}

enum ConditionOperator{EQUAL,
                       LIKE,
                       LESS,
                       LESS_EQUAL,
                       GREATER,
                       GREATER_EQUAL,
                       CONTAINS
                      };

/**
  * The AttributeCondition class holds a graphically edited condition on an attribute. A
  * condition instance returns a script and a script textual definition to the script page
  * to both generate the script (union of all the attribute conditions) and easily reload
  * the attribute conditions from the script, based on the attribute conditions' definitions.
  */
class AttributeCondition : public QWidget {
    Q_OBJECT
    
public:
    explicit AttributeCondition(QList<AttributeCondition *> *conditionsP, FilterTreeNode *filterP, QString plugin, QString attribute = "", QString op = "", QString value = "", QWidget *parentP = NULL);
    ~AttributeCondition();

    QString getScript();
    QString getDefinition();

private slots:
    void on_deleteConditionButton_clicked();
    void on_leftValueCombo_activated(const QString &arg1);
    void on_operatorCombo_activated(int index);
    void on_rightValueLineEdit_textChanged(const QString &arg1);

private:
    Ui::AttributeCondition  *ui;
    FilterTreeNode          *m_filterP;
    QString                 m_plugin;
    QString                 m_attribute;
    QString                 m_prefix;
    QString                 m_value;
    int                     m_operatorIndex;
    QString                 m_operator;
    QList<AttributeCondition *> *m_conditionsP;

    void buildScript();
    bool checkConditionValidity();
};

#endif // ATTRIBUTECONDITION_H
