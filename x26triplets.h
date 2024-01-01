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

	int address() const { return m_address; }
	int mode() const { return m_mode; }
	int modeExt() const { return (m_address >= 40) ? m_mode : (m_mode | 0x20); }
	int data() const { return m_data; }
	int addressRow() const { return (m_address == 40) ? 24 :m_address-40; }
	int addressColumn() const { return (m_address); }
	bool isRowTriplet() const { return (m_address >= 40); }

	void setAddress(int address);
	void setMode(int mode);
	void setData(int data);
	void setAddressRow(int addressRow);
	void setAddressColumn(int addressColumn);

	int objectSource() const { return (m_address & 0x18) >> 3; }

	int objectLocalDesignationCode() const { return (((m_address & 0x01) << 3) | (m_data >> 4)); }
	int objectLocalTripletNumber() const { return m_data & 0x0f; }
	int objectLocalIndex() const { return objectLocalDesignationCode() * 13 + objectLocalTripletNumber(); }
	void setObjectLocalDesignationCode(int i);
	void setObjectLocalTripletNumber(int i);
	void setObjectLocalIndex(int i);

	int activePositionRow() const { return m_activePositionRow; }
	int activePositionColumn() const { return m_activePositionColumn; }
	int activePositionRow1p5() const { return m_activePositionRow1p5; }
	int activePositionColumn1p5() const { return m_activePositionColumn1p5; }
	X26TripletError error() const { return m_error; }
	bool reservedMode() const { return m_reservedMode; }
	bool reservedData() const { return m_reservedData; }

	friend class X26TripletList;

private:
	// Only these variables are manipulated by the X26Triplet class
	int m_address, m_mode, m_data;
	// and the following are filled in by X26TripletList::updateInternalData()
	int m_activePositionRow = -1;
	int m_activePositionColumn = -1;
	int m_activePositionRow1p5 = -1;
	int m_activePositionColumn1p5 = -1;
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

	void removeLast() { m_list.removeLast(); }

	const X26Triplet &at(int i) const { return m_list.at(i); }
	bool isEmpty() const { return m_list.isEmpty(); }
	void reserve(int alloc) { m_list.reserve(alloc); }
	int size() const { return m_list.size(); }

private:
	void updateInternalData();

	QList<X26Triplet> m_list;

	class ActivePosition
	{
	public:
		ActivePosition();
		void reset();
//		int row() const { return (m_row == -1) ? 0 : m_row; }
//		int column() const { return (m_column == -1) ? 0 : m_column; }
		int row() const { return m_row; }
		int column() const { return m_column; }
		bool isDeployed() const { return m_row != -1; }
		bool setRow(int);
		bool setColumn(int);
//		bool setRowAndColumn(int, int);

	private:
		int m_row, m_column;
	};
};

#endif
