/*
 * Copyright (C) 2020, 2021 Gavin MacGregor
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

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegExpValidator>
#include <QSpinBox>
#include <QVBoxLayout>

#include "pageoptionsdockwidget.h"

PageOptionsDockWidget::PageOptionsDockWidget(TeletextWidget *parent): QDockWidget(parent)
{
	QVBoxLayout *pageOptionsLayout = new QVBoxLayout;
	QWidget *pageOptionsWidget = new QWidget;

	m_parentMainWidget = parent;

	this->setObjectName("PageOptionsDockWidget");
	this->setWindowTitle("Page options");

	// Page number
	m_pageNumberValidator = new QRegExpValidator(QRegExp("[1-8][0-9A-Fa-f][0-9A-Fa-f]"), this);

	QHBoxLayout *pageNumberLayout = new QHBoxLayout;
	pageNumberLayout->addWidget(new QLabel(tr("Page number")));
	m_pageNumberEdit = new QLineEdit("100");
	m_pageNumberEdit->setMaxLength(3);
	m_pageNumberEdit->setInputMask(">DHH");
	m_pageNumberEdit->setValidator(m_pageNumberValidator);
	pageNumberLayout->addWidget(m_pageNumberEdit);
	connect(m_pageNumberEdit, &QLineEdit::textEdited, m_parentMainWidget->document(), &TeletextDocument::setPageNumberFromString);

	pageOptionsLayout->addLayout(pageNumberLayout);

	// Page description
	m_pageDescriptionEdit = new QLineEdit();
	m_pageDescriptionEdit->setPlaceholderText("Page description");
	connect(m_pageDescriptionEdit, &QLineEdit::textEdited, m_parentMainWidget->document(), &TeletextDocument::setDescription);

	pageOptionsLayout->addWidget(m_pageDescriptionEdit);

	// FastText links
	QGridLayout *fastTextLayout = new QGridLayout;
	for (int i=0; i<6; i++) {
		const char *fastTextLabel[] = { "Red", "Green", "Yellow" ,"Blue", "Next", "Index" };

		fastTextLayout->addWidget(new QLabel(fastTextLabel[i]), 0, i, 1, 1, Qt::AlignCenter);
		m_fastTextEdit[i] = new QLineEdit;
		m_fastTextEdit[i]->setMaxLength(3);
		m_fastTextEdit[i]->setInputMask(">DHH");
		m_fastTextEdit[i]->setValidator(m_pageNumberValidator);
		fastTextLayout->addWidget(m_fastTextEdit[i], 1, i, 1, 1);
		connect(m_fastTextEdit[i], &QLineEdit::textEdited, [=](QString value) { setFastTextLinkPageNumber(i, value); } );
	}

	pageOptionsLayout->addLayout(fastTextLayout);

	QGroupBox *subPageGroupBox = new QGroupBox(tr("Subpage options"));
	QVBoxLayout *subPageOptionsLayout = new QVBoxLayout;

	// Cycle
	QHBoxLayout *pageCycleLayout = new QHBoxLayout;
	pageCycleLayout->addWidget(new QLabel(tr("Page cycle")));
	m_cycleValueSpinBox = new QSpinBox;
	m_cycleValueSpinBox->setRange(1, 99);
	m_cycleValueSpinBox->setWrapping(true);
	pageCycleLayout->addWidget(m_cycleValueSpinBox);
	// Since TeletextPage doesn't inherit from QObject (so we can copy construct it) it doesn't have a slot
	// to connect to, so we need to use a lambda
	connect(m_cycleValueSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int index) { m_parentMainWidget->document()->currentSubPage()->setCycleValue(index); } );
	m_cycleTypeCombo = new QComboBox;
	m_cycleTypeCombo->addItem("cycles");
	m_cycleTypeCombo->addItem("seconds");
	pageCycleLayout->addWidget(m_cycleTypeCombo);
	connect(m_cycleTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) { m_parentMainWidget->document()->currentSubPage()->setCycleType(index == 0 ? LevelOnePage::CTcycles : LevelOnePage::CTseconds); } );

	subPageOptionsLayout->addLayout(pageCycleLayout);

	// Page status bits
	for (int i=0; i<=7; i++) {
		const char *controlBitsLabel[] = { "C4 Erase page", "C5 Newsflash", "C6 Subtitle", "C7 Suppress header", "C8 Update page", "C9 Page not in sequence", "C10 Inhibit display", "C11 Serial magazine" };

		m_controlBitsAct[i] = new QCheckBox(controlBitsLabel[i]);
		subPageOptionsLayout->addWidget(m_controlBitsAct[i]);
		connect(m_controlBitsAct[i], &QCheckBox::stateChanged, [=](bool active) { m_parentMainWidget->setControlBit(i, active); });
	}

	// Region and language
	QGridLayout *regionLayout = new QGridLayout;
	regionLayout->addWidget(new QLabel(tr("Region")), 0, 0, 1, 1);
	m_defaultRegionCombo = new QComboBox;
	addRegionList(m_defaultRegionCombo);

	regionLayout->addWidget(m_defaultRegionCombo, 0, 1, 1, 1, Qt::AlignTop);
	connect(m_defaultRegionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]{ setDefaultRegion(); });

	m_defaultNOSCombo = new QComboBox;
	regionLayout->addWidget(m_defaultNOSCombo, 0, 2, 1, 1, Qt::AlignTop);
	connect(m_defaultNOSCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]{ setDefaultNOS(); });

	regionLayout->addWidget(new QLabel(tr("2nd region")), 1, 0, 1, 1);
	m_secondRegionCombo = new QComboBox;
	m_secondRegionCombo->addItem("Not needed", 15);
	addRegionList(m_secondRegionCombo);
	regionLayout->addWidget(m_secondRegionCombo, 1, 1, 1, 1, Qt::AlignTop);
	connect(m_secondRegionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]{ setSecondRegion(); });

	m_secondNOSCombo = new QComboBox;
	regionLayout->addWidget(m_secondNOSCombo, 1, 2, 1, 1, Qt::AlignTop);
	connect(m_secondNOSCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]{ setSecondNOS(); });

	subPageOptionsLayout->addLayout(regionLayout);

	subPageGroupBox->setLayout(subPageOptionsLayout);
	pageOptionsLayout->addWidget(subPageGroupBox);

	pageOptionsLayout->addStretch(1);

	pageOptionsWidget->setLayout(pageOptionsLayout);
	this->setWidget(pageOptionsWidget);

}

void PageOptionsDockWidget::addRegionList(QComboBox *regionCombo)
{
	regionCombo->addItem("0", 0);
	regionCombo->addItem("1", 1);
	regionCombo->addItem("2", 2);
	regionCombo->addItem("3", 3);
	regionCombo->addItem("4", 4);
	regionCombo->addItem("6", 6);
	regionCombo->addItem("8", 8);
	regionCombo->addItem("10", 10);
}

void PageOptionsDockWidget::updateWidgets()
{
	m_pageNumberEdit->blockSignals(true);
	m_pageNumberEdit->setText(QString::number(m_parentMainWidget->document()->pageNumber(), 16).toUpper());
	m_pageNumberEdit->blockSignals(false);
	m_pageDescriptionEdit->blockSignals(true);
	m_pageDescriptionEdit->setText(m_parentMainWidget->document()->description());
	m_pageDescriptionEdit->blockSignals(false);
	for (int i=0; i<6; i++) {
		// Stored as page link with relative magazine number, convert to absolute page number for display
		int absoluteLinkPageNumber = m_parentMainWidget->document()->currentSubPage()->fastTextLinkPageNumber(i) ^ (m_parentMainWidget->document()->pageNumber() & 0x700);
		// Fix magazine 0 to 8
		if ((absoluteLinkPageNumber & 0x700) == 0x000)
			absoluteLinkPageNumber |= 0x800;
		m_fastTextEdit[i]->blockSignals(true);
		m_fastTextEdit[i]->setText(QString::number(absoluteLinkPageNumber, 16).toUpper());
		m_fastTextEdit[i]->blockSignals(false);
	}
	m_cycleValueSpinBox->blockSignals(true);
	m_cycleValueSpinBox->setValue(m_parentMainWidget->document()->currentSubPage()->cycleValue());
	m_cycleValueSpinBox->blockSignals(false);
	m_cycleTypeCombo->blockSignals(true);
	m_cycleTypeCombo->setCurrentIndex(m_parentMainWidget->document()->currentSubPage()->cycleType() == LevelOnePage::CTseconds);
	m_cycleTypeCombo->blockSignals(false);
	for (int i=0; i<=7; i++) {
		m_controlBitsAct[i]->blockSignals(true);
		m_controlBitsAct[i]->setChecked(m_parentMainWidget->document()->currentSubPage()->controlBit(i));
		m_controlBitsAct[i]->blockSignals(false);
	}
	m_defaultRegionCombo->blockSignals(true);
	m_defaultRegionCombo->setCurrentText(QString::number(m_parentMainWidget->document()->currentSubPage()->defaultCharSet()));
	m_defaultRegionCombo->blockSignals(false);
	m_defaultNOSCombo->blockSignals(true);
	updateDefaultNOSOptions();
	m_defaultNOSCombo->setCurrentIndex(m_defaultNOSCombo->findData((m_parentMainWidget->document()->currentSubPage()->defaultCharSet() << 3) | m_parentMainWidget->document()->currentSubPage()->defaultNOS()));
	m_defaultNOSCombo->blockSignals(false);
	m_secondRegionCombo->blockSignals(true);
	m_secondRegionCombo->setCurrentText(QString::number(m_parentMainWidget->document()->currentSubPage()->secondCharSet()));
	m_secondRegionCombo->blockSignals(false);
	m_secondNOSCombo->blockSignals(true);
	updateSecondNOSOptions();
	m_secondNOSCombo->setCurrentIndex(m_secondNOSCombo->findData((m_parentMainWidget->document()->currentSubPage()->secondCharSet() << 3) | m_parentMainWidget->document()->currentSubPage()->secondNOS()));
	m_secondNOSCombo->blockSignals(false);
}

void PageOptionsDockWidget::setFastTextLinkPageNumber(int linkNumber, const QString &pageNumberString)
{
	// The LineEdit should check if a valid hex number was entered, but just in case...
	bool pageNumberOk;
	int pageNumberRead = pageNumberString.toInt(&pageNumberOk, 16);
	if ((!pageNumberOk) || pageNumberRead < 0x100 || pageNumberRead > 0x8ff)
		return;

	// Stored as page link with relative magazine number, convert from absolute page number that was entered
	pageNumberRead ^= (m_parentMainWidget->document()->pageNumber() & 0x700);
	pageNumberRead &= 0x7ff; // Fixes magazine 8 to 0

	// TODO bring in option to allow different FastText links per subpage
//	m_parentMainWidget->document()->currentSubPage()->setFastTextLinkPageNumber(linkNumber, pageNumberRead);
	m_parentMainWidget->document()->setFastTextLinkPageNumberOnAllSubPages(linkNumber, pageNumberRead);
}

void PageOptionsDockWidget::updateDefaultNOSOptions()
{
	while (m_defaultNOSCombo->count() > 0)
		m_defaultNOSCombo->removeItem(0);
	for (int i=0; i<languageComboBoxItemCount; i++)
		if ((languageComboBoxItems[i].bits >> 3) == m_parentMainWidget->document()->currentSubPage()->defaultCharSet())
			m_defaultNOSCombo->addItem(languageComboBoxItems[i].name, languageComboBoxItems[i].bits);
}

void PageOptionsDockWidget::updateSecondNOSOptions()
{
	while (m_secondNOSCombo->count() > 0)
		m_secondNOSCombo->removeItem(0);
	for (int i=0; i<languageComboBoxItemCount; i++)
		if ((languageComboBoxItems[i].bits >> 3) == m_parentMainWidget->document()->currentSubPage()->secondCharSet())
			m_secondNOSCombo->addItem(languageComboBoxItems[i].name, languageComboBoxItems[i].bits);
}

void PageOptionsDockWidget::setDefaultRegion()
{
	m_parentMainWidget->document()->currentSubPage()->setDefaultCharSet(m_defaultRegionCombo->currentData().toInt());
	m_defaultNOSCombo->blockSignals(true);
	updateDefaultNOSOptions();
	setDefaultNOS();
	m_defaultNOSCombo->blockSignals(false);
	m_parentMainWidget->refreshPage();
}

void PageOptionsDockWidget::setDefaultNOS()
{
	m_parentMainWidget->document()->currentSubPage()->setDefaultNOS(m_defaultNOSCombo->currentData().toInt() & 0x07);
	m_parentMainWidget->refreshPage();
}

void PageOptionsDockWidget::setSecondRegion()
{
	m_parentMainWidget->document()->currentSubPage()->setSecondCharSet(m_secondRegionCombo->currentData().toInt());
	m_secondNOSCombo->blockSignals(true);
	updateSecondNOSOptions();
	setSecondNOS();
	m_secondNOSCombo->blockSignals(false);
	m_parentMainWidget->refreshPage();
}

void PageOptionsDockWidget::setSecondNOS()
{
	m_parentMainWidget->document()->currentSubPage()->setSecondNOS(m_secondNOSCombo->currentData().toInt() & 0x07);
	m_parentMainWidget->refreshPage();
}
