/*
 * Copyright (C) 2020-2025 Gavin MacGregor
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

#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QGridLayout>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "dclutdockwidget.h"

#include "mainwidget.h"
#include "x26menus.h"
#include "x28commands.h"

DClutDockWidget::DClutDockWidget(TeletextWidget *parent): QDockWidget(parent)
{
	QVBoxLayout *dClutLayout = new QVBoxLayout;
	QWidget *dClutWidget = new QWidget;

	m_parentMainWidget = parent;

	this->setObjectName("DClutWidget");
	this->setWindowTitle("Level 3.5 DCLUTs");

	QStackedWidget *stackedWidget = new QStackedWidget;
	QGridLayout *pageLayout[4];

	for (int p=0; p<4; p++) {
		pageLayout[p] = new QGridLayout;

		for (int i=0; i<16; i++) {
			m_dClutButton[p][i] = new QPushButton;
			m_dClutButton[p][i]->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
			pageLayout[p]->addWidget(m_dClutButton[p][i], i/4, i%4);

			m_dClutMenu[p][i] = new TripletCLUTQMenu(false, this);
			m_dClutButton[p][i]->setMenu(m_dClutMenu[p][i]);

			for (int c=0; c<32; c++)
				connect(static_cast<TripletCLUTQMenu *>(m_dClutMenu[p][i])->action(c), &QAction::triggered, [=]() { m_parentMainWidget->document()->undoStack()->push(new SetDCLUTCommand(m_parentMainWidget->document(), (p % 2 == 0), p/2 + 1, i, c)); });

			connect(m_dClutMenu[p][i], &QMenu::aboutToShow, [=]() { updateDClutMenu(p, i); });

			if (i == 3 && p < 2)
				break;
		}

		QWidget *pageWidget = new QWidget;
		pageWidget->setLayout(pageLayout[p]);
		stackedWidget->addWidget(pageWidget);
	}

	QComboBox *dClutPageSelect = new QComboBox;
	dClutPageSelect->addItem(tr("Global DRCS mode 1"));
	dClutPageSelect->addItem(tr("Normal DRCS mode 1"));
	dClutPageSelect->addItem(tr("Global DRCS modes 2 & 3"));
	dClutPageSelect->addItem(tr("Normal DRCS modes 2 & 3"));
	dClutLayout->addWidget(dClutPageSelect);

	dClutLayout->addWidget(stackedWidget);

	dClutWidget->setLayout(dClutLayout);
	this->setWidget(dClutWidget);

	connect(dClutPageSelect, &QComboBox::activated, stackedWidget, &QStackedWidget::setCurrentIndex);

	connect(m_parentMainWidget->document(), &TeletextDocument::dClutChanged, this, &DClutDockWidget::dClutChanged);
	connect(m_parentMainWidget->document(), &TeletextDocument::colourChanged, this, &DClutDockWidget::colourChanged);
}

void DClutDockWidget::updateDClutMenu(int p, int i)
{
	for (int c=0; c<32; c++)
		static_cast<TripletCLUTQMenu *>(m_dClutMenu[p][i])->setColour(c, m_parentMainWidget->document()->currentSubPage()->CLUTtoQColor(c));
}

void DClutDockWidget::updateColourButton(int p, int i)
{
	const int dIndex = m_parentMainWidget->document()->currentSubPage()->dCLUT((p % 2 == 0), p/2 + 1, i);
	m_dClutButton[p][i]->setText(QString("%1:%2").arg(dIndex / 8).arg(dIndex % 8));
	const QString colourString = QString("%1").arg(m_parentMainWidget->document()->currentSubPage()->CLUT(dIndex), 3, 16, QChar('0'));

	if (dIndex != 8) {
		// FIXME duplicated in palettedockwidget.cpp
		const int r = m_parentMainWidget->document()->currentSubPage()->CLUT(dIndex) >> 8;
		const int g = (m_parentMainWidget->document()->currentSubPage()->CLUT(dIndex) >> 4) & 0xf;
		const int b = m_parentMainWidget->document()->currentSubPage()->CLUT(dIndex) & 0xf;
		// Set text itself to black or white so it can be seen over background colour - http://alienryderflex.com/hsp.html
		const char blackOrWhite = (sqrt(r*r*0.299 + g*g*0.587 + b*b*0.114) > 7.647) ? '0' : 'f';

		m_dClutButton[p][i]->setStyleSheet(QString("background-color: #%1; color: #%2%2%2; border: none").arg(colourString).arg(blackOrWhite));;
	} else
		m_dClutButton[p][i]->setStyleSheet("border: none");
}

void DClutDockWidget::updateAllColourButtons()
{
	for (int p=0; p<4; p++)
		for (int i=0; i<16; i++) {
			updateColourButton(p, i);

			if (i == 3 && p < 2)
				break;
		}
}

void DClutDockWidget::dClutChanged(bool g, int m, int i)
{
	updateColourButton(!g + m*2-2, i);

	if (m_parentMainWidget->pageDecode()->level() == 3)
		for (int r=0; r<25; r++)
			for (int c=0; c<72; c++)
				if (m_parentMainWidget->pageDecode()->cellDrcsSource(r, c) != TeletextPageDecode::NoDRCS)
					m_parentMainWidget->pageDecode()->setRefresh(r, c, true);

	emit m_parentMainWidget->document()->contentsChanged();
}

void DClutDockWidget::colourChanged(int c)
{
	const QString searchString = QString("%1:%2").arg(c / 8).arg(c % 8);

	for (int p=0; p<4; p++)
		for (int i=0; i<16; i++) {
			if (m_dClutButton[p][i]->text() == searchString)
				updateColourButton(p, i);

			if (i == 3 && p < 2)
				break;
		}
}
