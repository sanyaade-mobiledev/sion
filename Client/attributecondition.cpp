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

#include <QObject>
#include <QVBoxLayout>
#include <QStringList>
#include <QMessageBox>

#include "ui_attributecondition.h"
#include "attributecondition.h"

/**
  * Constructs a graphical condition editor line. The editor prompts the user with the attributes list, a combo to chose the
  * condition operator and an edit line for the right value to compare with the attribute's value.
  */
AttributeCondition::AttributeCondition(QList<AttributeCondition *> *conditionsP, FilterTreeNode *filterP, QString plugin, QString attribute, QString op, QString value, QWidget *parentP) :
    QWidget(parentP),
    ui(new Ui::AttributeCondition) {
    ui->setupUi(this);

    m_conditionsP = conditionsP;
    m_filterP = filterP;
    m_plugin = plugin;
    m_operatorIndex = EQUAL;
    int attributeIndex = 0;
    m_value = value;

    if (!m_value.isEmpty())
        ui->rightValueLineEdit->setText(m_value);

    // populate the operators combo
    // and select the appropriate operator
    QStringList operators;
    operators << tr("EQUAL") << tr("LIKE") << tr("LESS") << tr("LESS OR EQUAL") << tr("GREATER") << tr("GREATER OR EQUAL") << tr("CONTAINS");
    for (int i = 0; i < operators.count(); i++)  {
        ui->operatorCombo->addItem(operators[i]);
        if (operators[i] == op)
            m_operatorIndex = i;
    }
    ui->operatorCombo->setCurrentIndex(m_operatorIndex);
    on_operatorCombo_activated(m_operatorIndex);

    // populate the attributes combo
    // and select the appropriate attribute
    QStringList attributes = m_filterP->getPluginAttributes(m_plugin);
    attributes += tr("Body");
    for (int i = 0; i < attributes.count(); i++)  {
        ui->leftValueCombo->addItem(attributes[i]);
        if (attributes[i] == attribute)
            attributeIndex = i;
    }
    ui->leftValueCombo->setCurrentIndex(attributeIndex); // select the first attribute of none set yet
    m_attribute = attributes[attributeIndex];
}

AttributeCondition::~AttributeCondition() {
    delete ui;
}

/**
  * Removes a condition from the list of condition. A simple DeleteLater does the trick, since
  * the deletion will remove the condition from it's parent.
  */
void AttributeCondition::on_deleteConditionButton_clicked() {
    // remove the condition from the script page condition list
    int i = m_conditionsP->indexOf(this);
    if (i != -1)
        m_conditionsP->removeAt(i);

    // delete this during the next event loop
    deleteLater();
}

/**
  * Selects an attribute.
  */
void AttributeCondition::on_leftValueCombo_activated(const QString &arg1) {
    m_attribute = arg1;
}

/**
  * Selects an operator. The "Contains" operator doesn't need to be handled here
  * because it is implicitely selected when the selected attribute is 'Body'.
  */
void AttributeCondition::on_operatorCombo_activated(int index) {
    m_operatorIndex = index;

    switch (m_operatorIndex) {
        case EQUAL:
            m_operator = "==";
            break;

        case LESS:
            m_operator = "<";
            break;

        case LESS_EQUAL:
            m_operator = "<=";
            break;

        case GREATER:
            m_operator = ">";
            break;

        case GREATER_EQUAL:
            m_operator = ">=";
            break;
    }
}

/**
  * The user has just modified the right value. This is the right place to assist
  * the user in the right value edition.
  */
void AttributeCondition::on_rightValueLineEdit_textChanged(const QString &arg1) {
    m_value = arg1;

    // help user (auto-complete + select text)
    QString attrClass = m_filterP->getAttributeClass(m_plugin, m_attribute);

    // nothing to do for strings
    if (attrClass == "String") {
        return;
    }

    // for numerical values, make sure the input is ok
    if (attrClass == "Numeric") {
        QRegExp re("\\d*");  // a digit (\d), zero or more times (*)
        if (!re.exactMatch(m_value)) {
            QMessageBox::warning(this,
                                 windowTitle(),
                                 tr("The attribute is a qint64 so the right value must be numeric!"),
                                 QMessageBox::Close);
        }

        ui->rightValueLineEdit->selectAll();
        return;
    }

    // for boolean, the value should must likely be "true" or "false"
    if (attrClass == "Boolean") {
        if (m_value.toLower() == "t" || m_value.toLower() == "tr" || m_value.toLower() == "tru") {
            ui->rightValueLineEdit->setText("true");
            ui->rightValueLineEdit->selectAll();
            return;
        }
        if (m_value.toLower() == "f" || m_value.toLower() == "fa" || m_value.toLower() == "fal" || m_value.toLower() == "fals") {
            ui->rightValueLineEdit->setText("false");
            ui->rightValueLineEdit->selectAll();
            return;
        }

        return;
    }

    // for datetime, propose: LastYear, LastMonth, LastWeek and Today
    if (attrClass == "Date") {
        if (m_value.toLower() == tr("today")) {
            m_value = "new Date(new Date().getTime() - (24 * 60 * 60 * 1000))";
            ui->rightValueLineEdit->setText(m_value);
            ui->rightValueLineEdit->selectAll();
            return;
        }

        if (m_value.toLower() == tr("lastweek")) {
            m_value = "new Date(new Date().getTime() - (7 * 24 * 60 * 60 * 1000))";
            ui->rightValueLineEdit->setText(m_value);
            ui->rightValueLineEdit->selectAll();
            return;
        }

        if (m_value.toLower() == tr("lastmonth")) {
            m_value = "new Date(new Date().getTime() - (30 * 24 * 60 * 60 * 1000))";
            ui->rightValueLineEdit->setText(m_value);
            ui->rightValueLineEdit->selectAll();
            return;
        }

        if (m_value.toLower() == tr("lastyear")) {
            m_value = "new Date(new Date().getTime() - (365 * 24 * 60 * 60 * 1000))";
            ui->rightValueLineEdit->setText(m_value);
            ui->rightValueLineEdit->selectAll();
            return;
        }

        return;
    }
}

/**
  * The attribute condition is described by a very simple definition text. This
  * greatly improves the script analysis when the script is reloaded.
  */
QString AttributeCondition::getDefinition() {
    QString definition;

    definition = "//" + m_attribute;

    switch (m_operatorIndex) {
        case EQUAL:
            definition += QString(",%1,").arg(tr("EQUAL"));
            break;

        case LESS:
            definition += QString(",%1,").arg(tr("LESS"));
            break;

        case LESS_EQUAL:
            definition += QString(",%1,").arg(tr("LESS OR EQUAL"));
            break;

        case GREATER:
            definition += QString(",%1,").arg(tr("GREATER"));
            break;

        case GREATER_EQUAL:
            definition += QString(",%1,").arg(tr("GREATER OR EQUAL"));
            break;

        case LIKE:
            definition += QString(",%1,").arg(tr("LIKE"));
            break;

        case CONTAINS:
            definition += QString(",%1,").arg(tr("CONTAINS"));
            break;
    }

    definition += m_value;
    definition += ";";

    return definition;
}

/**
  * Returns the script associated with the user selections/input. If the type of the attribute is QString,
  * lowercase its value and quote the right value.
  */
QString AttributeCondition::getScript() {
    QString script;

    if (!checkConditionValidity())
        return script;

    // is the attribute a string?
    bool isString = m_filterP->getAttributeClass(m_plugin, m_attribute) == "String";

    switch (m_operatorIndex) {
        case EQUAL:
        case LESS:
        case LESS_EQUAL:
        case GREATER:
        case GREATER_EQUAL:
        if (isString)
            script = "plugin.getAttributeValue(\"" + m_attribute + "\").toLowerCase() " + m_operator + " \"" + m_value + "\"";
        else
            script = "plugin.getAttributeValue(\"" + m_attribute + "\") " + m_operator + " " + m_value;
            break;

        case LIKE:
            script = "plugin.getAttributeValue(\"" + m_attribute + "\").toString().toLowerCase().indexOf(\"" + m_value + "\") != -1";
            break;

        case CONTAINS:
            script = "plugin.contains(\"" + m_value + "\")";
            break;
    }

    return script;
}

/**
  * Checks the attribute condition validity and returns true if ok.
  */
bool AttributeCondition::checkConditionValidity() {
    // check condition validity
    if (m_attribute == tr("Body") && m_operatorIndex != CONTAINS) {
        QMessageBox::warning(this,
                             windowTitle(),
                             tr("The 'Body' attribute can only be used with the 'CONTAINS' operator!"),
                             QMessageBox::Close);

        return false;
    }

    // #### check right value against attribute type here

    return true;
}
