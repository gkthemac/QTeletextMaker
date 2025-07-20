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

#ifndef DCLUTDOCKWIDGET_H
#define DCLUTDOCKWIDGET_H

#include <QComboBox>
#include <QDockWidget>
#include <QMenu>
#include <QPainter>
#include <QPushButton>

#include "mainwidget.h"

class DClutDockWidget : public QDockWidget
{
	Q_OBJECT

public:
	DClutDockWidget(TeletextWidget *parent);
	void updateAllColourButtons();

public slots:
	void updateColourButton(int p, int i);
	void dClutChanged(bool g, int m, int i);
	void colourChanged(int c);

private:
	void updateDClutMenu(int p, int i);

	TeletextWidget *m_parentMainWidget;
	QPushButton *m_dClutButton[4][16];
	QMenu *m_dClutMenu[4][16];
};

#endif
