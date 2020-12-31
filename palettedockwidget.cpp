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

#include <QColorDialog>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QUndoCommand>
#include <math.h>

#include "palettedockwidget.h"

#include "levelonecommands.h"
#include "document.h"
#include "mainwidget.h"

PaletteDockWidget::PaletteDockWidget(TeletextWidget *parent): QDockWidget(parent)
{
	QGridLayout *paletteGridLayout = new QGridLayout;
	QWidget *paletteGridWidget = new QWidget;

	m_parentMainWidget = parent;

	this->setObjectName("PaletteDockWidget");
	this->setWindowTitle("Palette");
	for (int r=0, i=0; r<=3; r++) {
		paletteGridLayout->addWidget(new QLabel(tr("CLUT %1").arg(r)), r, 0);
		m_resetButton[r] = new QPushButton(tr("Reset"));
		paletteGridLayout->addWidget(m_resetButton[r], r, 9);
		connect(m_resetButton[r], &QAbstractButton::clicked, [=]() { resetCLUT(r); });
		for (int c=0; c<=7; c++, i++) {
			if (i == 8)
				continue;
			m_colourButton[i] = new QPushButton();
			m_colourButton[i]->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
			paletteGridLayout->addWidget(m_colourButton[i], r, c+1);
			connect(m_colourButton[i], &QAbstractButton::clicked, [=]() { selectColour(i); });
		}
	}
	paletteGridWidget->setLayout(paletteGridLayout);
	this->setWidget(paletteGridWidget);
	connect(m_parentMainWidget->document(), &TeletextDocument::colourChanged, this, &PaletteDockWidget::updateColourButton);
}

void PaletteDockWidget::updateColourButton(int colourIndex)
{
	if (colourIndex == 8)
		return;
	QString colourString = QString("%1").arg(m_parentMainWidget->document()->currentSubPage()->CLUT(colourIndex), 3, 16, QChar('0'));
	m_colourButton[colourIndex]->setText(colourString);

	// Set text itself to black or white so it can be seen over background colour - http://alienryderflex.com/hsp.html
	int r = m_parentMainWidget->document()->currentSubPage()->CLUT(colourIndex) >> 8;
	int g = (m_parentMainWidget->document()->currentSubPage()->CLUT(colourIndex) >> 4) & 0xf;
	int b = m_parentMainWidget->document()->currentSubPage()->CLUT(colourIndex) & 0xf;
	char blackOrWhite = (sqrt(r*r*0.299 + g*g*0.587 + b*b*0.114) > 7.647) ? '0' : 'f';

	QString qss = QString("background-color: #%1; color: #%2%2%2; border: none").arg(colourString).arg(blackOrWhite);
	m_colourButton[colourIndex]->setStyleSheet(qss);

	if (m_parentMainWidget->document()->currentSubPage()->isPaletteDefault(colourIndex))
		// Default colour was set, disable Reset button if all colours in row are default as well
		m_resetButton[colourIndex>>3]->setEnabled(!m_parentMainWidget->document()->currentSubPage()->isPaletteDefault(colourIndex & 0x18, colourIndex | 0x07));
	else
		m_resetButton[colourIndex>>3]->setEnabled(true);
}

void PaletteDockWidget::updateAllColourButtons()
{
	for (int i=0; i<32; i++)
		updateColourButton(i);
}

void PaletteDockWidget::showEvent(QShowEvent *event)
{
	Q_UNUSED(event);
	updateAllColourButtons();
}

void PaletteDockWidget::selectColour(int colourIndex)
{
    const QColor newColour = QColorDialog::getColor(CLUTtoQColor(m_parentMainWidget->document()->currentSubPage()->CLUT(colourIndex)), this, "Select Colour");

	if (newColour.isValid()) {
		QUndoCommand *setColourCommand = new SetColourCommand(m_parentMainWidget->document(), colourIndex, ((newColour.red() & 0xf0) << 4) | (newColour.green() & 0xf0) | ((newColour.blue() & 0xf0) >> 4));
		m_parentMainWidget->document()->undoStack()->push(setColourCommand);
	}
}

void PaletteDockWidget::resetCLUT(int colourTable)
{
	QUndoCommand *resetCLUTCommand = new ResetCLUTCommand(m_parentMainWidget->document(), colourTable);
	m_parentMainWidget->document()->undoStack()->push(resetCLUTCommand);
}
