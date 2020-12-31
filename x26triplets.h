/*
 * Copyright (C) 2020, 2021 Gavin MacGregor
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
	X26Triplet() {}
//	X26Triplet(const X26Triplet &other);

//	X26Triplet &operator=(const X26Triplet &other);

	X26Triplet(int, int, int);

	int address() const { return m_address; }
	int mode() const { return m_mode; }
	int modeExt() const { return (m_address >= 40) ? m_mode : (m_mode | 0x20); }
	int data() const { return m_data; }
	int addressRow() const { return (m_address == 40) ? 24 :m_address-40; }
	int addressColumn() const { return (m_address); }
	bool isRowTriplet() const { return (m_address >= 40); }

	void setAddress(int);
	void setMode(int);
	void setData(int);
	void setAddressRow(int);
	void setAddressColumn(int);

private:
	int m_address, m_mode, m_data;

};

#endif
