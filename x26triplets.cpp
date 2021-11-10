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

#include "x26triplets.h"

X26Triplet::X26Triplet(int address, int mode, int data)
{
	m_address = address;
	m_mode = mode;
	m_data = data;
}

void X26Triplet::setAddress(int address)
{
	m_address = address;
}

void X26Triplet::setMode(int mode)
{
	m_mode = mode;
}

void X26Triplet::setData(int data)
{
	m_data = data;
}

void X26Triplet::setAddressRow(int addressRow)
{
	m_address = (addressRow == 24) ? 40 : addressRow+40;
}

void X26Triplet::setAddressColumn(int addressColumn)
{
	m_address = addressColumn;
}

void X26Triplet::setObjectLocalDesignationCode(int i)
{
	m_address = (m_address & 0x38) | (i >> 3);
	m_data = (m_data & 0x0f) | ((i & 0x07) << 4);
}

void X26Triplet::setObjectLocalTripletNumber(int i)
{
	m_data = (m_data & 0x70) | i;
}

void X26Triplet::setObjectLocalIndex(int i)
{
	m_address = (m_address & 0x38) + (i >= 104); // Set bit 0 of address if triplet >= 8
	m_data = (((i / 13) & 0x07) << 4) | (i % 13);
}


void X26TripletList::updateInternalData(int r)
{
	ActivePosition activePosition;
	X26Triplet *triplet;

	if (r != 0) {
		activePosition.setRow(m_list[r-1].m_activePositionRow);
		activePosition.setColumn(m_list[r-1].m_activePositionColumn);
	}

	for (int i=r; i < m_list.size(); i++) {
		triplet = &m_list[i];
		triplet->m_error = X26Triplet::NoError;
		if (triplet->isRowTriplet()) {
			switch (m_list.at(i).modeExt()) {
				case 0x00: // Full screen colour
					if (activePosition.isDeployed())
						// TODO more specific error needed
						triplet->m_error = X26Triplet::ActivePositionMovedUp;
					break;
				case 0x01: // Full row colour
					if (!activePosition.setRow(triplet->addressRow()))
						triplet->m_error = X26Triplet::ActivePositionMovedUp;
					break;
				case 0x04: // Set Active Position;
					if (!activePosition.setRow(triplet->addressRow()))
						triplet->m_error = X26Triplet::ActivePositionMovedUp;
					else if (triplet->data() >= 40 || !activePosition.setColumn(triplet->data()))
						triplet->m_error = X26Triplet::ActivePositionMovedLeft;
					break;
				case 0x07: // Address row 0
					if (activePosition.isDeployed())
						triplet->m_error = X26Triplet::ActivePositionMovedUp;
					else {
						activePosition.setRow(0);
						activePosition.setColumn(8);
					}
					break;
				case 0x15 ... 0x17: // Define Object
					activePosition.reset();
					// Make sure data field holds correct place of triplet
					// otherwise the object won't appear
					triplet->setObjectLocalIndex(i);
					break;
			};
		// Column triplet: make sure that PDC and reserved triplets don't affect the Active Position
		} else if (triplet->modeExt() != 0x24 && triplet->modeExt() != 0x25 && triplet->modeExt() != 0x26 && triplet->modeExt() != 0x2a)
			if (!activePosition.setColumn(triplet->addressColumn()))
				triplet->m_error = X26Triplet::ActivePositionMovedLeft;
		triplet->m_activePositionRow = activePosition.row();
		triplet->m_activePositionColumn = activePosition.column();
	}
}

void X26TripletList::append(const X26Triplet &value)
{
	m_list.append(value);
	updateInternalData(m_list.size()-1);
}

void X26TripletList::insert(int i, const X26Triplet &value)
{
	m_list.insert(i, value);
	updateInternalData(i);
}

void X26TripletList::removeAt(int i)
{
	m_list.removeAt(i);
	if (m_list.size() != 0 && i < m_list.size())
		updateInternalData(i);
}

void X26TripletList::replace(int i, const X26Triplet &value)
{
	m_list.replace(i, value);
	updateInternalData(i);
}


X26TripletList::ActivePosition::ActivePosition()
{
	m_row = m_column = -1;
}

void X26TripletList::ActivePosition::reset()
{
	m_row = m_column = -1;
}

bool X26TripletList::ActivePosition::setRow(int row)
{
	if (row < m_row)
		return false;
	if (row > m_row) {
		m_row = row;
		m_column = -1;
	}
	return true;
}

bool X26TripletList::ActivePosition::setColumn(int column)
{
	if (column < m_column)
		return false;
	if (m_row == -1 and column >= 0)
		m_row = 0;
	m_column = column;
	return true;
}
/*
bool X26TripletList::ActivePosition::setRowAndColumn(int newRow, int newColumn)
{
	if (!setRow(newRow))
		return false;
	return setColumn(newColumn);
}
*/
