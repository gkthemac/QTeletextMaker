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

#include <QActionGroup>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QShortcut>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QToolButton>
#include <QVBoxLayout>

#include "x26dockwidget.h"

X26DockWidget::X26DockWidget(TeletextWidget *parent): QDockWidget(parent)
{
	QVBoxLayout *x26Layout = new QVBoxLayout;
	QWidget *x26Widget = new QWidget;
	m_parentMainWidget = parent;

	this->setObjectName("x26DockWidget");
	this->setWindowTitle("X/26 triplets");

	// Table listing of local enhancements
	m_x26View = new QTableView;
	m_x26Model = new X26Model(m_parentMainWidget);
	m_x26View->setModel(m_x26Model);
	m_x26View->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_x26View->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_x26View->setColumnWidth(0, 50);
	m_x26View->setColumnWidth(1, 50);
	m_x26View->setColumnWidth(2, 200);
	m_x26View->setColumnWidth(3, 200);
	x26Layout->addWidget(m_x26View);

	// "Temporary" widgets to edit raw triplet values
	QHBoxLayout *rawTripletLayout = new QHBoxLayout;

	rawTripletLayout->addWidget(new QLabel(tr("Address")));
	m_rawTripletAddressSpinBox = new QSpinBox;
	m_rawTripletAddressSpinBox->setMaximum(63);
	rawTripletLayout->addWidget(m_rawTripletAddressSpinBox);

	rawTripletLayout->addWidget(new QLabel(tr("Mode")));
	m_rawTripletModeSpinBox = new QSpinBox;
	m_rawTripletModeSpinBox->setMaximum(31);
	rawTripletLayout->addWidget(m_rawTripletModeSpinBox);

	rawTripletLayout->addWidget(new QLabel(tr("Data")));
	m_rawTripletDataSpinBox = new QSpinBox;
	m_rawTripletDataSpinBox->setMaximum(127);
	rawTripletLayout->addWidget(m_rawTripletDataSpinBox);

	connect(m_rawTripletAddressSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &X26DockWidget::rawTripletAddressSpinBoxChanged);
	connect(m_rawTripletModeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &X26DockWidget::rawTripletModeSpinBoxChanged);
	connect(m_rawTripletDataSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &X26DockWidget::rawTripletDataSpinBoxChanged);

	x26Layout->addLayout(rawTripletLayout);

	// Insert and delete widgets
	QHBoxLayout *insertDeleteLayout = new QHBoxLayout;

	m_insertPushButton = new QPushButton(tr("Insert triplet"));
	insertDeleteLayout->addWidget(m_insertPushButton);
	m_deletePushButton = new QPushButton(tr("Delete triplet"));
	insertDeleteLayout->addWidget(m_deletePushButton);

	connect(m_insertPushButton, &QPushButton::clicked, this, &X26DockWidget::insertTriplet);
	connect(m_deletePushButton, &QPushButton::clicked, this, &X26DockWidget::deleteTriplet);

	x26Layout->addLayout(insertDeleteLayout);

	x26Widget->setLayout(x26Layout);
	this->setWidget(x26Widget);

	m_x26View->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_x26View, &QWidget::customContextMenuRequested, this, &X26DockWidget::customMenuRequested);

	connect(m_x26View, &QAbstractItemView::clicked, this, &X26DockWidget::rowClicked);

	QShortcut* insertShortcut = new QShortcut(QKeySequence(Qt::Key_Insert), m_x26View);
	connect(insertShortcut, &QShortcut::activated, this, &X26DockWidget::insertTriplet);
	QShortcut* deleteShortcut = new QShortcut(QKeySequence(Qt::Key_Delete), m_x26View);
	connect(deleteShortcut, &QShortcut::activated, this, &X26DockWidget::deleteTriplet);
}

void X26DockWidget::loadX26List()
{
	m_x26Model->setX26ListLoaded(true);
}

void X26DockWidget::unloadX26List()
{
	m_x26Model->setX26ListLoaded(false);
	m_rawTripletAddressSpinBox->setEnabled(false);
	m_rawTripletDataSpinBox->setEnabled(false);
	m_rawTripletModeSpinBox->setEnabled(false);
}

void X26DockWidget::rowClicked(const QModelIndex &index)
{
	updateRawTripletWidgets(index);
}

void X26DockWidget::updateRawTripletWidgets(const QModelIndex &index)
{
	m_rawTripletAddressSpinBox->setEnabled(true);
	m_rawTripletDataSpinBox->setEnabled(true);
	m_rawTripletModeSpinBox->setEnabled(true);
	m_rawTripletAddressSpinBox->blockSignals(true);
	m_rawTripletModeSpinBox->blockSignals(true);
	m_rawTripletDataSpinBox->blockSignals(true);
	m_rawTripletAddressSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole).toInt());
	m_rawTripletModeSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 1), Qt::UserRole).toInt());
	m_rawTripletDataSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 2), Qt::UserRole).toInt());
	m_rawTripletAddressSpinBox->blockSignals(false);
	m_rawTripletModeSpinBox->blockSignals(false);
	m_rawTripletDataSpinBox->blockSignals(false);
}
void X26DockWidget::rawTripletAddressSpinBoxChanged(int newValue)
{
	m_x26Model->setData(m_x26Model->index(m_x26View->currentIndex().row(), 0), newValue, Qt::UserRole);
}

void X26DockWidget::rawTripletModeSpinBoxChanged(int newValue)
{
	m_x26Model->setData(m_x26Model->index(m_x26View->currentIndex().row(), 1), newValue, Qt::UserRole);
}

void X26DockWidget::rawTripletDataSpinBoxChanged(int newValue)
{
	m_x26Model->setData(m_x26Model->index(m_x26View->currentIndex().row(), 2), newValue, Qt::UserRole);
}

void X26DockWidget::tripletDataChanged(int newValue, int bitMask, int bitShift)
{
	m_x26Model->setData(m_x26Model->index(m_x26View->currentIndex().row(), 2), (m_x26Model->data(m_x26Model->index(m_x26View->currentIndex().row(), 2), Qt::UserRole).toInt() & bitMask) | (newValue << bitShift), Qt::UserRole);
	updateRawTripletWidgets(m_x26View->currentIndex());
}
void X26DockWidget::insertTriplet()
{
	QModelIndex index = m_x26View->currentIndex();

	if (index.isValid())
		m_x26Model->insertRow(index.row(), QModelIndex());
	else
		m_x26Model->insertFirstRow();
}

void X26DockWidget::deleteTriplet()
{
	QModelIndex index = m_x26View->currentIndex();
	if (index.isValid())
		m_x26Model->removeRow(index.row(), index.parent());
}

void X26DockWidget::customMenuRequested(QPoint pos)
{
	QModelIndex index = m_x26View->indexAt(pos);

	QMenu *menu = new QMenu(this);

	QAction *insertAct = new QAction("Insert triplet", this);
	menu->addAction(insertAct);
	connect(insertAct, &QAction::triggered, this, &X26DockWidget::insertTriplet);
	if (index.isValid()) {
		QAction *deleteAct = new QAction("Delete triplet", this);
		menu->addAction(deleteAct);
		connect(deleteAct, &QAction::triggered, this, &X26DockWidget::deleteTriplet);
	}
	menu->popup(m_x26View->viewport()->mapToGlobal(pos));
}
