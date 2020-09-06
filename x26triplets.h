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

#ifndef X26TRIPLETS_H
#define X26TRIPLETS_H

class X26Triplet
{
public:
//	X26Triplet() {}
//	X26Triplet(const X26Triplet &other);

//	X26Triplet &operator=(const X26Triplet &other);

	int address() const { return myTriplet.m_address; }
	int mode() const { return myTriplet.m_mode; }
	int data() const { return myTriplet.m_data; }
	int addressRow() const { return (myTriplet.m_address == 40) ? 24 : myTriplet.m_address-40; }
	int addressColumn() const { return (myTriplet.m_address); }
	bool isRowTriplet() const { return (myTriplet.m_address >= 40); }

	void setAddress(int);
	void setMode(int);
	void setData(int);
	void setAddressRow(int);
	void setAddressColumn(int);

private:
	struct x26TripletStruct {
		int m_address;
		int m_mode;
		int m_data;
	} myTriplet;
};

#endif
