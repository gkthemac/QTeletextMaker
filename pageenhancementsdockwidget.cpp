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

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

#include "pageenhancementsdockwidget.h"

PageEnhancementsDockWidget::PageEnhancementsDockWidget(TeletextWidget *parent): QDockWidget(parent)
{
	QGridLayout *pageEnhancementsLayout = new QGridLayout;
	QWidget *pageEnhancementsWidget = new QWidget;

	m_parentMainWidget = parent;

	this->setObjectName("PageEnhancementsDockWidget");
	this->setWindowTitle("Page enhancements");
	pageEnhancementsLayout->addWidget(new QLabel(tr("Default screen colour")), 0, 0, 1, 1);
	pageEnhancementsLayout->addWidget(new QLabel(tr("Default row colour")), 1, 0, 1, 1);
	m_defaultScreenColourCombo = new QComboBox;
	m_defaultRowColourCombo = new QComboBox;
	for (int r=0; r<=3; r++)
		for (int c=0; c<=7; c++) {
			m_defaultScreenColourCombo->addItem(tr("CLUT %1:%2").arg(r).arg(c));
			m_defaultRowColourCombo->addItem(tr("CLUT %1:%2").arg(r).arg(c));
		}
	pageEnhancementsLayout->addWidget(m_defaultScreenColourCombo, 0, 1, 1, 1, Qt::AlignTop);
	connect(m_defaultScreenColourCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index){ m_parentMainWidget->setDefaultScreenColour(index); });
	pageEnhancementsLayout->addWidget(m_defaultRowColourCombo, 1, 1, 1, 1, Qt::AlignTop);
	connect(m_defaultRowColourCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index){ m_parentMainWidget->setDefaultRowColour(index); });
	pageEnhancementsLayout->addWidget(new QLabel(tr("Colour remapping")), 2, 0, 1, 1);
	m_colourTableCombo = new QComboBox;
	m_colourTableCombo->addItem("Fore 0  Back 0");
	m_colourTableCombo->addItem("Fore 0  Back 1");
	m_colourTableCombo->addItem("Fore 0  Back 2");
	m_colourTableCombo->addItem("Fore 1  Back 1");
	m_colourTableCombo->addItem("Fore 1  Back 2");
	m_colourTableCombo->addItem("Fore 2  Back 1");
	m_colourTableCombo->addItem("Fore 2  Back 2");
	m_colourTableCombo->addItem("Fore 2  Back 3");
	pageEnhancementsLayout->addWidget(m_colourTableCombo, 2, 1, 1, 1, Qt::AlignTop);
	connect(m_colourTableCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index){ m_parentMainWidget->setColourTableRemap(index); });
	m_blackBackgroundSubstAct = new QCheckBox("Black background colour substitution");
	pageEnhancementsLayout->addWidget(m_blackBackgroundSubstAct, 3, 0, 1, 2, Qt::AlignTop);
	connect(m_blackBackgroundSubstAct, &QCheckBox::stateChanged, m_parentMainWidget, &TeletextWidget::setBlackBackgroundSubst);

	pageEnhancementsLayout->addWidget(new QLabel(tr("Left side panel columns")), 4, 0, 1, 1);
	m_leftSidePanelSpinBox = new QSpinBox(this);
	m_leftSidePanelSpinBox->setMaximum(16);
	pageEnhancementsLayout->addWidget(m_leftSidePanelSpinBox, 4, 1, 1, 1, Qt::AlignTop);
	connect(m_leftSidePanelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int index){ setLeftSidePanelWidth(index); });

	pageEnhancementsLayout->addWidget(new QLabel(tr("Right side panel columns")), 5, 0, 1, 1);
	m_rightSidePanelSpinBox = new QSpinBox(this);
	m_rightSidePanelSpinBox->setMaximum(16);
	pageEnhancementsLayout->addWidget(m_rightSidePanelSpinBox, 5, 1, 1, 1, Qt::AlignTop);
	connect(m_rightSidePanelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int index){ setRightSidePanelWidth(index); });

	m_sidePanelStatusAct = new QCheckBox("Side panels at level 3.5 only");
	pageEnhancementsLayout->addWidget(m_sidePanelStatusAct, 6, 0, 1, 2, Qt::AlignTop);
	connect(m_sidePanelStatusAct, &QCheckBox::stateChanged, m_parentMainWidget, &TeletextWidget::setSidePanelAtL35Only);

	pageEnhancementsLayout->setRowStretch(6, 1);
	pageEnhancementsWidget->setLayout(pageEnhancementsLayout);
	this->setWidget(pageEnhancementsWidget);
}

void PageEnhancementsDockWidget::setLeftSidePanelWidth(int newLeftPanelSize)
{
	if (newLeftPanelSize && m_rightSidePanelSpinBox->value()) {
		int newRightPanelSize = 16-newLeftPanelSize;
		m_rightSidePanelSpinBox->blockSignals(true);
		m_rightSidePanelSpinBox->setValue(newRightPanelSize);
		m_rightSidePanelSpinBox->blockSignals(false);
	}
	m_parentMainWidget->setSidePanelWidths(newLeftPanelSize, m_rightSidePanelSpinBox->value());
}

void PageEnhancementsDockWidget::setRightSidePanelWidth(int newRightPanelSize)
{
	if (newRightPanelSize && m_leftSidePanelSpinBox->value()) {
		int newLeftPanelSize = 16-newRightPanelSize;
		m_leftSidePanelSpinBox->blockSignals(true);
		m_leftSidePanelSpinBox->setValue(newLeftPanelSize);
		m_leftSidePanelSpinBox->blockSignals(false);
	}
	m_parentMainWidget->setSidePanelWidths(m_leftSidePanelSpinBox->value(), newRightPanelSize);
}

void PageEnhancementsDockWidget::updateWidgets()
{
	int leftSidePanelColumnsResult = 0;
	int rightSidePanelColumnsResult = 0;

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

	if (m_parentMainWidget->document()->currentSubPage()->leftSidePanelDisplayed())
		leftSidePanelColumnsResult = (m_parentMainWidget->document()->currentSubPage()->sidePanelColumns() == 0) ? 16 : m_parentMainWidget->document()->currentSubPage()->sidePanelColumns();

	if (m_parentMainWidget->document()->currentSubPage()->rightSidePanelDisplayed())
		rightSidePanelColumnsResult = 16-m_parentMainWidget->document()->currentSubPage()->sidePanelColumns();

	m_leftSidePanelSpinBox->blockSignals(true);
	m_leftSidePanelSpinBox->setValue(leftSidePanelColumnsResult);
	m_leftSidePanelSpinBox->blockSignals(false);
	m_rightSidePanelSpinBox->blockSignals(true);
	m_rightSidePanelSpinBox->setValue(rightSidePanelColumnsResult);
	m_rightSidePanelSpinBox->blockSignals(false);
	m_sidePanelStatusAct->blockSignals(true);
	m_sidePanelStatusAct->setChecked(!m_parentMainWidget->document()->currentSubPage()->sidePanelStatusL25());
	m_sidePanelStatusAct->blockSignals(false);
}
