/*
 * Copyright (C) 2020-2026 Gavin MacGregor
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
#include <QRadioButton>
#include <QSpinBox>
#include <QStackedLayout>

#include "pageenhancementsdockwidget.h"

#include "x28commands.h"

PageEnhancementsDockWidget::PageEnhancementsDockWidget(TeletextWidget *parent): QDockWidget(parent)
{
	QVBoxLayout *pageEnhancementsLayout = new QVBoxLayout;
	QWidget *pageEnhancementsWidget = new QWidget;

	m_parentMainWidget = parent;

	this->setObjectName("PageEnhancementsDockWidget");
	this->setWindowTitle("X/28 page enhancements");

	// Colour group box and layout
	QGroupBox *colourGroupBox = new QGroupBox(tr("Colours"));
	QGridLayout *colourLayout = new QGridLayout;

	// Default screen and default row colours
	colourLayout->addWidget(new QLabel(tr("Default screen colour")), 0, 0, 1, 1);
	colourLayout->addWidget(new QLabel(tr("Default row colour")), 1, 0, 1, 1);
	m_defaultScreenColourCombo = new QComboBox;
	m_defaultScreenColourCombo->setModel(m_parentMainWidget->document()->clutModel());
	m_defaultRowColourCombo = new QComboBox;
	m_defaultRowColourCombo->setModel(m_parentMainWidget->document()->clutModel());
	colourLayout->addWidget(m_defaultScreenColourCombo, 0, 1, 1, 1, Qt::AlignTop);
	connect(m_defaultScreenColourCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index){ m_parentMainWidget->document()->undoStack()->push(new SetFullScreenColourCommand(m_parentMainWidget->document(), index)); });
	colourLayout->addWidget(m_defaultRowColourCombo, 1, 1, 1, 1, Qt::AlignTop);
	connect(m_defaultRowColourCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index){ m_parentMainWidget->document()->undoStack()->push(new SetFullRowColourCommand(m_parentMainWidget->document(), index)); });

	// CLUT remapping
	colourLayout->addWidget(new QLabel(tr("CLUT remapping")), 2, 0, 1, 1);
	m_colourTableCombo = new QComboBox;
	m_colourTableCombo->addItem("Fore 0  Back 0");
	m_colourTableCombo->addItem("Fore 0  Back 1");
	m_colourTableCombo->addItem("Fore 0  Back 2");
	m_colourTableCombo->addItem("Fore 1  Back 1");
	m_colourTableCombo->addItem("Fore 1  Back 2");
	m_colourTableCombo->addItem("Fore 2  Back 1");
	m_colourTableCombo->addItem("Fore 2  Back 2");
	m_colourTableCombo->addItem("Fore 2  Back 3");
	colourLayout->addWidget(m_colourTableCombo, 2, 1, 1, 1, Qt::AlignTop);
	connect(m_colourTableCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index){ m_parentMainWidget->document()->undoStack()->push(new SetCLUTRemapCommand(m_parentMainWidget->document(), index)); });

	// Black background colour substitution
	m_blackBackgroundSubstAct = new QCheckBox("Black background colour substitution");
	colourLayout->addWidget(m_blackBackgroundSubstAct, 3, 0, 1, 2, Qt::AlignTop);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	connect(m_blackBackgroundSubstAct, &QCheckBox::checkStateChanged, [=](int state){ m_parentMainWidget->document()->undoStack()->push(new SetBlackBackgroundSubstCommand(m_parentMainWidget->document(), state)); });
#else
	connect(m_blackBackgroundSubstAct, &QCheckBox::stateChanged, [=](int state){ m_parentMainWidget->document()->undoStack()->push(new SetBlackBackgroundSubstCommand(m_parentMainWidget->document(), state)); });
#endif

	// Add colour group box to the main layout
	colourGroupBox->setLayout(colourLayout);
	pageEnhancementsLayout->addWidget(colourGroupBox);


	// Side panels group box and layout
	QGroupBox *sidePanelsGroupBox = new QGroupBox(tr("Side panels"));
	QGridLayout *sidePanelsLayout = new QGridLayout;

	// Left or right side panel radio buttons
	m_sidePanelRadioButton[0] = new QRadioButton(tr("None"));
	m_sidePanelRadioButton[1] = new QRadioButton(tr("Left"));
	m_sidePanelRadioButton[2] = new QRadioButton(tr("Right"));
	m_sidePanelRadioButton[3] = new QRadioButton(tr("Both"));

	for (int i=0; i<4; i++) {
		sidePanelsLayout->addWidget(m_sidePanelRadioButton[i], 0, i, 1, 1);
		connect(m_sidePanelRadioButton[i], &QAbstractButton::clicked, [=]() { setSidePanels(i); });
	}

	// Stacked widgets that alter the number of side panel columns
	QHBoxLayout *oneSidePanelLayout = new QHBoxLayout;
	// Index 0 is a blank label widget, we allocate it later when we start stacking

	// Index 1 - single spinbox for left or right side panel only
	QLabel *oneSidePanelLabel = new QLabel("Columns");
	oneSidePanelLayout->addWidget(oneSidePanelLabel);
	m_oneSidePanelSpinBox = new QSpinBox;
	m_oneSidePanelSpinBox->setMinimum(1);
	m_oneSidePanelSpinBox->setMaximum(16);
	oneSidePanelLayout->addWidget(m_oneSidePanelSpinBox);
	connect(m_oneSidePanelSpinBox, &QSpinBox::valueChanged, this, &PageEnhancementsDockWidget::setOneSidePanelWidth);

	// Index 2 - two spinboxes for both side panels
	QHBoxLayout *bothSidePanelsLayout = new QHBoxLayout;

	QLabel *bothLeftLabel = new QLabel(tr("Left cols"));
	bothSidePanelsLayout->addWidget(bothLeftLabel);
	m_leftSidePanelSpinBox = new QSpinBox;
	m_leftSidePanelSpinBox->setMaximum(15);
	bothSidePanelsLayout->addWidget(m_leftSidePanelSpinBox);
	connect(m_leftSidePanelSpinBox, &QSpinBox::valueChanged, this, &PageEnhancementsDockWidget::setLeftSidePanelWidth);

	QLabel *bothRightLabel = new QLabel(tr("Right cols"));
	bothSidePanelsLayout->addWidget(bothRightLabel);
	m_rightSidePanelSpinBox = new QSpinBox;
	m_rightSidePanelSpinBox->setMinimum(1);
	m_rightSidePanelSpinBox->setMaximum(16);
	bothSidePanelsLayout->addWidget(m_rightSidePanelSpinBox);
	connect(m_rightSidePanelSpinBox, &QSpinBox::valueChanged, this, &PageEnhancementsDockWidget::setRightSidePanelWidth);

	// Stack all the column layouts together
	m_columnsStackedLayout = new QStackedLayout;

	// Index 0
	// Blank label
	QLabel *blankWidget = new QLabel(" ");
	m_columnsStackedLayout->addWidget(blankWidget);

	// Index 1
	oneSidePanelLayout->setContentsMargins(0, 0, 0, 0);
	QWidget *oneSidePanelWidget = new QWidget;
	oneSidePanelWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	oneSidePanelWidget->setLayout(oneSidePanelLayout);
	m_columnsStackedLayout->addWidget(oneSidePanelWidget);

	// Index 2
	bothSidePanelsLayout->setContentsMargins(0, 0, 0, 0);
	QWidget *bothSidePanelsWidget = new QWidget;
	bothSidePanelsWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	bothSidePanelsWidget->setLayout(bothSidePanelsLayout);
	m_columnsStackedLayout->addWidget(bothSidePanelsWidget);

	// We could simply: sidePanelsLayout->addLayout(m_columnsStackedLayout);
	// but we need to keep it to the smallest size, only possible by putting it inside a QWidget
	QWidget *columnsStackedWidget = new QWidget;
	columnsStackedWidget->setLayout(m_columnsStackedLayout);
	columnsStackedWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	sidePanelsLayout->addWidget(columnsStackedWidget, 1, 0, 1, 4, Qt::AlignTop);

	// Side panels status
	m_sidePanelStatusAct = new QCheckBox("Side panels at level 3.5 only");
	sidePanelsLayout->addWidget(m_sidePanelStatusAct, 2, 0, 1, 4, Qt::AlignTop);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	connect(m_sidePanelStatusAct, &QCheckBox::checkStateChanged, m_parentMainWidget, &TeletextWidget::setSidePanelAtL35Only);
#else
	connect(m_sidePanelStatusAct, &QCheckBox::stateChanged, m_parentMainWidget, &TeletextWidget::setSidePanelAtL35Only);
#endif

	// Add side panels group box to the main layout
	sidePanelsGroupBox->setLayout(sidePanelsLayout);
	pageEnhancementsLayout->addWidget(sidePanelsGroupBox);

	pageEnhancementsLayout->addStretch(1);

	pageEnhancementsWidget->setLayout(pageEnhancementsLayout);
	this->setWidget(pageEnhancementsWidget);
}

void PageEnhancementsDockWidget::setSidePanels(int newSidePanels)
{
	m_parentMainWidget->setSidePanels(newSidePanels & 1, newSidePanels & 2);

	switch (newSidePanels) {
		case 0:
			m_columnsStackedLayout->setCurrentIndex(0);
			break;
		case 1:
		case 2:
			m_columnsStackedLayout->setCurrentIndex(1);
			break;
		case 3:
			m_columnsStackedLayout->setCurrentIndex(2);
			break;
	}

	updateSidePanelWidgets(newSidePanels);
}

void PageEnhancementsDockWidget::setOneSidePanelWidth(int newPanelWidth)
{
	if (m_parentMainWidget->document()->currentSubPage()->leftSidePanelDisplayed())
		if (newPanelWidth == 16)
			m_parentMainWidget->setSidePanelColumns(0);
		else
			m_parentMainWidget->setSidePanelColumns(newPanelWidth);

	if (m_parentMainWidget->document()->currentSubPage()->rightSidePanelDisplayed())
		m_parentMainWidget->setSidePanelColumns(16 - newPanelWidth);
}

void PageEnhancementsDockWidget::setLeftSidePanelWidth(int newLeftPanelSize)
{
	// Both side panels, and left-columns spinbox was changed.
	// Update the right-columns spinbox to maintain 16 additional column total.
	m_rightSidePanelSpinBox->blockSignals(true);
	m_rightSidePanelSpinBox->setValue(16 - newLeftPanelSize);
	m_rightSidePanelSpinBox->blockSignals(false);

	m_parentMainWidget->setSidePanelColumns(newLeftPanelSize);
}

void PageEnhancementsDockWidget::setRightSidePanelWidth(int newRightPanelSize)
{
	// Both side panels, and right-columns spinbox was changed.
	// Update the left-columns spinbox to maintain 16 additional column total.
	const int newLeftPanelSize = 16 - newRightPanelSize;

	m_leftSidePanelSpinBox->blockSignals(true);
	m_leftSidePanelSpinBox->setValue(newLeftPanelSize);
	m_leftSidePanelSpinBox->blockSignals(false);

	m_parentMainWidget->setSidePanelColumns(newLeftPanelSize);
}

// Called when the radio button changes, either by being clicked or when a new page is shown.
void PageEnhancementsDockWidget::updateSidePanelWidgets(int sidePanelRadio)
{
	switch (sidePanelRadio) {
		case 0:
			m_columnsStackedLayout->setCurrentIndex(0);
			break;
		case 1:
			m_oneSidePanelSpinBox->blockSignals(true);
			if (m_parentMainWidget->document()->currentSubPage()->sidePanelColumns() == 0)
				m_oneSidePanelSpinBox->setValue(16);
			else
				m_oneSidePanelSpinBox->setValue(m_parentMainWidget->document()->currentSubPage()->sidePanelColumns());
			m_oneSidePanelSpinBox->blockSignals(false);

			m_columnsStackedLayout->setCurrentIndex(1);
			break;
		case 2:
			m_oneSidePanelSpinBox->blockSignals(true);
			m_oneSidePanelSpinBox->setValue(16 - m_parentMainWidget->document()->currentSubPage()->sidePanelColumns());
			m_oneSidePanelSpinBox->blockSignals(false);

			m_columnsStackedLayout->setCurrentIndex(1);
			break;
		case 3:
			m_leftSidePanelSpinBox->blockSignals(true);
			m_rightSidePanelSpinBox->blockSignals(true);
			m_leftSidePanelSpinBox->setValue(m_parentMainWidget->document()->currentSubPage()->sidePanelColumns());
			m_rightSidePanelSpinBox->setValue(16 - m_parentMainWidget->document()->currentSubPage()->sidePanelColumns());
			m_leftSidePanelSpinBox->blockSignals(false);
			m_rightSidePanelSpinBox->blockSignals(false);

			m_columnsStackedLayout->setCurrentIndex(2);
			break;
	}
}

void PageEnhancementsDockWidget::updateWidgets()
{
	m_defaultScreenColourCombo->blockSignals(true);
	m_defaultScreenColourCombo->setCurrentIndex(m_parentMainWidget->document()->currentSubPage()->defaultScreenColour());
	m_defaultScreenColourCombo->blockSignals(false);
	m_defaultRowColourCombo->blockSignals(true);
	m_defaultRowColourCombo->setCurrentIndex(m_parentMainWidget->document()->currentSubPage()->defaultRowColour());
	m_defaultRowColourCombo->blockSignals(false);
	m_colourTableCombo->blockSignals(true);
	m_colourTableCombo->setCurrentIndex(m_parentMainWidget->document()->currentSubPage()->colourTableRemap());
	m_colourTableCombo->blockSignals(false);
	m_blackBackgroundSubstAct->blockSignals(true);
	m_blackBackgroundSubstAct->setChecked(m_parentMainWidget->document()->currentSubPage()->blackBackgroundSubst());
	m_blackBackgroundSubstAct->blockSignals(false);

	int sidePanelRadio = 0;

	if (m_parentMainWidget->document()->currentSubPage()->leftSidePanelDisplayed())
		sidePanelRadio |= 1;
	if (m_parentMainWidget->document()->currentSubPage()->rightSidePanelDisplayed())
		sidePanelRadio |= 2;

	m_sidePanelRadioButton[sidePanelRadio]->blockSignals(true);
	m_sidePanelRadioButton[sidePanelRadio]->toggle();
	m_sidePanelRadioButton[sidePanelRadio]->blockSignals(false);

	updateSidePanelWidgets(sidePanelRadio);

	m_sidePanelStatusAct->blockSignals(true);
	m_sidePanelStatusAct->setChecked(!m_parentMainWidget->document()->currentSubPage()->sidePanelStatusL25());
	m_sidePanelStatusAct->blockSignals(false);
}
