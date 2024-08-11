/*
 * Copyright (C) 2020-2024 Gavin MacGregor
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

#ifndef X26MODEL_H
#define X26MODEL_H

#include <QAbstractListModel>

#include "mainwidget.h"
#include "x26menus.h"

class X26Model : public QAbstractListModel
{
	Q_OBJECT

public:
	X26Model(TeletextWidget *parent);
	void setX26ListLoaded(bool newListLoaded);
	int rowCount(const QModelIndex &parent = QModelIndex()) const override ;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	bool insertRows(int position, int rows, const QModelIndex &parent);
	bool insertRows(int position, int rows, const QModelIndex &parent, X26Triplet triplet);
	bool removeRows(int position, int rows, const QModelIndex &index);
//	Qt::ItemFlags flags(const QModelIndex &index) const;

	// The x26commands classes manipulate the model but beginInsertRows and endInsertRows
	// are protected methods, so we need to friend them
	friend class InsertTripletCommand;
	friend class DeleteTripletCommand;
	friend class EditTripletCommand;

private:
	TeletextWidget *m_parentMainWidget;
	bool m_listLoaded;
	TeletextFontBitmap m_fontBitmap;
	ModeTripletNames m_modeTripletNames;

	struct tripletErrorShow {
		QString message;
		int columnHighlight;
	};

	// Needs to be in the same order as enum X26TripletError in x26triplets.h
	const tripletErrorShow m_tripletErrors[6] {
		{ "", 0 }, // No error
		{ "Active Position can't move up", 0 },
		{ "Active Position can't move left within row", 1 },
		{ "Invocation not pointing to Object Definition", 3 },
		{ "Invoked and Defined Object types don't match", 2 },
		{ "Origin Modifier not followed by Object Invocation", 2 }
	};
};

#endif
