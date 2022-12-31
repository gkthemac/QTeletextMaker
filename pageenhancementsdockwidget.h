/*
 * Copyright (C) 2020-2023 Gavin MacGregor
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

#ifndef PAGEENHANCEMENTSDOCKWIDGET_H
#define PAGEENHANCEMENTSDOCKWIDGET_H

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QSpinBox>

#include "mainwidget.h"

class PageEnhancementsDockWidget : public QDockWidget
{
	Q_OBJECT

public:
	PageEnhancementsDockWidget(TeletextWidget *parent);
	void updateWidgets();

private:
	void setLeftSidePanelWidth(int);
	void setRightSidePanelWidth(int);

	TeletextWidget *m_parentMainWidget;
	QComboBox *m_defaultScreenColourCombo, *m_defaultRowColourCombo, *m_colourTableCombo;
	QCheckBox *m_blackBackgroundSubstAct, *m_sidePanelStatusAct;
	QSpinBox *m_leftSidePanelSpinBox, *m_rightSidePanelSpinBox;
};

#endif
