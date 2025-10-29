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

#ifndef X26TRIPLETS_H
#define X26TRIPLETS_H

#include <QList>

class X26Triplet
{
public:
	// x26model.h has the Plain English descriptions of these errors
	enum X26TripletError { NoError, ActivePositionMovedUp, ActivePositionMovedLeft, InvokePointerInvalid, InvokeTypeMismatch, OriginModifierAlone };
	enum ObjectSource { InvalidObjectSource, LocalObject, POPObject, GPOPObject };

	X26Triplet() {}
//	X26Triplet(const X26Triplet &other);

//	X26Triplet &operator=(const X26Triplet &other);

	X26Triplet(int address, int mode, int data);

	int address() const;
	int mode() const;
	int modeExt() const;
	int data() const;
	int addressRow() const;
	int addressColumn() const;
	bool isRowTriplet() const;

	void setAddress(int address);
	void setMode(int mode);
	void setData(int data);
	void setAddressRow(int addressRow);
	void setAddressColumn(int addressColumn);

	int objectSource() const;

	int objectLocalDesignationCode() const;
	int objectLocalTripletNumber() const;
	int objectLocalIndex() const;
	void setObjectLocalDesignationCode(int i);
	void setObjectLocalTripletNumber(int i);
	void setObjectLocalIndex(int i);

	int activePositionRow() const;
	int activePositionColumn() const;
	int activePositionRow1p5() const;
	int activePositionColumn1p5() const;
	X26TripletError error() const;
	bool reservedMode() const;
	bool reservedData() const;
	bool activePosition1p5Differs() const;

	friend class X26TripletList;

private:
	// Only these variables are manipulated by the X26Triplet class
	int m_address, m_mode, m_data;
	// and the following are filled in by X26TripletList::updateInternalData()
	int m_activePositionRow = -1;
	int m_activePositionColumn = -1;
	int m_activePositionRow1p5 = -1;
	int m_activePositionColumn1p5 = -1;
	bool m_activePosition1p5Differs = false;
	X26TripletError m_error = NoError;
	bool m_reservedMode = false;
	bool m_reservedData = false;
};

class X26TripletList
{
public:
	void append(const X26Triplet &value);
	void insert(int i, const X26Triplet &value);
	void removeAt(int i);
	void replace(int i, const X26Triplet &value);
	void removeLast();
	const X26Triplet &at(int i) const;
	bool isEmpty() const;
	void reserve(int alloc);
	int size() const;

	const QList<int> &objects(int t) const;

private:
	void updateInternalData();

	QList<X26Triplet> m_list;
	QList<int> m_objects[3];

	class ActivePosition
	{
	public:
		ActivePosition();
		void reset();
		int row() const;
		int column() const;
		bool isDeployed() const;
		bool setRow(int);
		bool setColumn(int);
//		bool setRowAndColumn(int, int);

	private:
		int m_row, m_column;
	};
};

#endif
