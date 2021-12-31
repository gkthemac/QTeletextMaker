/*
 * Copyright (C) 2020-2022 Gavin MacGregor
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

#ifndef PALETTEDOCKWIDGET_H
#define PALETTEDOCKWIDGET_H

#include <QCheckBox>
#include <QDockWidget>
#include <QPushButton>

#include "mainwidget.h"

class PaletteDockWidget : public QDockWidget
{
	Q_OBJECT

public:
	PaletteDockWidget(TeletextWidget *parent);
	void updateAllColourButtons();

public slots:
	void updateColourButton(int);

protected:
	void showEvent(QShowEvent *);

private slots:
	void selectColour(int);

private:
	void resetCLUT(int);

	QPushButton *m_colourButton[32], *m_resetButton[4];
	QCheckBox *m_showHexValuesCheckBox;

	TeletextWidget *m_parentMainWidget;
};

#endif
