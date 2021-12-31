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

#ifndef X26MODEL_H
#define X26MODEL_H

#include <QAbstractListModel>
#include "mainwidget.h"
#include "render.h"

class X26Model : public QAbstractListModel
{
	Q_OBJECT

public:
	X26Model(TeletextWidget *parent);
	void setX26ListLoaded(bool);
	int rowCount(const QModelIndex &parent = QModelIndex()) const override ;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	bool insertFirstRow();
	bool insertRows(int position, int rows, const QModelIndex &parent);
	bool removeRows(int position, int rows, const QModelIndex &index);
//	Qt::ItemFlags flags(const QModelIndex &index) const;

	const QString modeTripletName(int i) const { return m_modeTripletName[i]; }

	// The x26commands classes manipulate the model but beginInsertRows and endInsertRows
	// are protected methods, so we need to friend them
	friend class InsertTripletCommand;
	friend class DeleteTripletCommand;
	friend class EditTripletCommand;

private:
	TeletextWidget *m_parentMainWidget;
	bool m_listLoaded;
	TeletextFontBitmap m_fontBitmap;

	const QString m_modeTripletName[64] {
		// Row triplet modes
		"Full screen colour",
		"Full row colour",
		"Reserved 0x02",
		"Reserved 0x03",

		"Set Active Position",
		"Reserved 0x05",
		"Reserved 0x06",
		"Address row 0",

		"PDC origin, source",
		"PDC month and day",
		"PDC cursor and start hour",
		"PDC cursor and end hour",

		"PDC cursor local offset",
		"PDC series ID and code",
		"Reserved 0x0e",
		"Reserved 0x0f",

		"Origin modifier",
		"Invoke active object",
		"Invoke adaptive object",
		"Invoke passive object",

		"Reserved 0x14",
		"Define active object",
		"Define adaptive object",
		"Define passive object",

		"DRCS mode",
		"Reserved 0x19",
		"Reserved 0x1a",
		"Reserved 0x1b",

		"Reserved 0x1c",
		"Reserved 0x1d",
		"Reserved 0x1e",
		"Termination marker",

		// Column triplet modes
		"Foreground colour",
		"G1 block mosaic",
		"G3 smooth mosaic, level 1.5",
		"Background colour",

		"Reserved 0x04",
		"Reserved 0x05",
		"PDC cursor, start end min",
		"Additional flash functions",

		"Modified G0/G2 character set",
		"G0 character",
		"Reserved 0x0a",
		"G3 smooth mosaic, level 2.5",

		"Display attributes",
		"DRCS character",
		"Font style, level 3.5",
		"G2 supplementary character",

		"G0 character no diacritical",
		"G0 character diacritical 1",
		"G0 character diacritical 2",
		"G0 character diacritical 3",
		"G0 character diacritical 4",
		"G0 character diacritical 5",
		"G0 character diacritical 6",
		"G0 character diacritical 7",
		"G0 character diacritical 8",
		"G0 character diacritical 9",
		"G0 character diacritical A",
		"G0 character diacritical B",
		"G0 character diacritical C",
		"G0 character diacritical D",
		"G0 character diacritical E",
		"G0 character diacritical F"
	};

	struct tripletErrorShow {
		QString message;
		int columnHighlight;
	};

	const tripletErrorShow m_tripletErrors[3] {
		{ "", 0 }, // No error
		{ "Active Position can't move up", 0 },
		{ "Active Position can't move left within row", 1 }
	};
};

#endif
