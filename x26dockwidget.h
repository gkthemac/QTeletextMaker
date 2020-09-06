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

#ifndef X26DOCKWIDGET_H
#define X26DOCKWIDGET_H

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QGroupBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTableView>

#include "mainwidget.h"
#include "x26model.h"

class X26DockWidget : public QDockWidget
{
	Q_OBJECT

public:
	X26DockWidget(TeletextWidget *parent);

public slots:
	void insertTriplet();
	void deleteTriplet();
	void customMenuRequested(QPoint pos);
	void loadX26List();
	void unloadX26List();
	void rowClicked(const QModelIndex &);
	void updateRawTripletWidgets(const QModelIndex &);
	void rawTripletAddressSpinBoxChanged(int);
	void rawTripletModeSpinBoxChanged(int);
	void rawTripletDataSpinBoxChanged(int);
	void tripletDataChanged(int, int=0, int=0);

private:
	QTableView *m_x26View;
	X26Model *m_x26Model;
	// "Temporary" widgets to edit raw triplet values
	QSpinBox *m_rawTripletAddressSpinBox, *m_rawTripletModeSpinBox, *m_rawTripletDataSpinBox;
	QPushButton *m_insertPushButton, *m_deletePushButton;

	TeletextWidget *m_parentMainWidget;
};

#endif
