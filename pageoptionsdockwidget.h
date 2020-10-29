/*
 * Copyright (C) 2020 Gavin MacGregor
 *
 * This file is part of QTeletextMaker.
 *
 * QTeletextMaker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QTeletextMaker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QTeletextMaker.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PAGEOPTIONSDOCKWIDGET_H
#define PAGEOPTIONSDOCKWIDGET_H

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QLineEdit>
#include <QSpinBox>

#include "mainwidget.h"

class PageOptionsDockWidget : public QDockWidget
{
	Q_OBJECT

public:
	PageOptionsDockWidget(TeletextWidget *parent);
	void updateWidgets();

private:
	TeletextWidget *m_parentMainWidget;
	QLineEdit *m_pageNumberEdit, *m_pageDescriptionEdit;
	QSpinBox *m_cycleValueSpinBox;
	QComboBox *m_cycleTypeCombo;
	QCheckBox *m_controlBitsAct[8];
	QComboBox *m_defaultRegionCombo, *m_defaultNOSCombo, *m_secondRegionCombo, *m_secondNOSCombo;
	QLineEdit *m_fastTextEdit[6];

	void addRegionList(QComboBox *);
	void setFastTextLinkPageNumber(int, const QString &);
	void setDefaultRegion();
	void setDefaultNOS();
	void setSecondRegion();
	void setSecondNOS();
	void updateDefaultNOSOptions();
	void updateSecondNOSOptions();
};

struct languageComboBoxItem {
	QString name;
	int bits;
};

static const int languageComboBoxItemCount = 36;

static const languageComboBoxItem languageComboBoxItems[languageComboBoxItemCount] {
	{ "English", 0x00 } ,
	{ "German", 0x01 } ,
	{ "Swedish/Finnish/Hungarian", 0x02 } ,
	{ "Italian", 0x03 } ,
	{ "French", 0x04 } ,
	{ "Portuguese/Spanish", 0x05 } ,
	{ "Czech/Slovak", 0x06 } ,

	{ "Polish", 0x08 } ,
	{ "German", 0x09 } ,
	{ "Swedish/Finnish/Hungarian", 0x0a } ,
	{ "Italian", 0x0b } ,
	{ "French", 0x0c } ,
	{ "Czech/Slovak", 0x0e } ,

	{ "English", 0x10 } ,
	{ "German", 0x11 } ,
	{ "Swedish/Finnish/Hungarian", 0x12 } ,
	{ "Italian", 0x13 } ,
	{ "French", 0x14 } ,
	{ "Portuguese/Spanish", 0x15 } ,
	{ "Turkish", 0x16 } ,

	{ "Serbian/Croatian/Slovenian", 0x1d } ,
	{ "Rumanian", 0x1f } ,

	{ "Serbian/Croatian", 0x20 } ,
	{ "German", 0x21 } ,
	{ "Estonian", 0x22 } ,
	{ "Lettish/Lithuanian", 0x23 } ,
	{ "Russian/Bulgarian", 0x24 } ,
	{ "Ukrainian", 0x25 } ,
	{ "Czech/Slovak", 0x26 } ,

	{ "Turkish", 0x36 } ,
	{ "Greek", 0x37 } ,

	{ "English", 0x40 } ,
	{ "French", 0x44 } ,
	{ "Arabic", 0x47 } ,

	{ "Hebrew", 0x55 } ,
	{ "Arabic", 0x57 } ,
};

#endif
