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


void X26TripletList::updateInternalData()
{
	ActivePosition activePosition;
	X26Triplet *triplet;

	// Check for errors, and fill in where the Active Position goes for Level 2.5 and above
	for (int i=0; i < m_list.size(); i++) {
		triplet = &m_list[i];
		triplet->m_error = X26Triplet::NoError;
		triplet->m_reservedMode = false;
		triplet->m_reservedData = false;

		if (triplet->isRowTriplet()) {
			switch (triplet->modeExt()) {
				case 0x00: // Full screen colour
					if (activePosition.isDeployed())
						// TODO more specific error needed
						triplet->m_error = X26Triplet::ActivePositionMovedUp;
					if (triplet->m_data & 0x60)
						triplet->m_reservedData = true;
					break;
				case 0x01: // Full row colour
					if (!activePosition.setRow(triplet->addressRow()))
						triplet->m_error = X26Triplet::ActivePositionMovedUp;
					if ((triplet->m_data & 0x60) != 0x00 && (triplet->m_data & 0x60) != 0x60)
						triplet->m_reservedData = true;
					break;
				case 0x04: // Set Active Position;
					if (!activePosition.setRow(triplet->addressRow()))
						triplet->m_error = X26Triplet::ActivePositionMovedUp;
					else if (triplet->data() >= 40)
						// FIXME data column highlighted?
						triplet->m_reservedData = true;
					else if (!activePosition.setColumn(triplet->data()))
						triplet->m_error = X26Triplet::ActivePositionMovedLeft;
					break;
				case 0x07: // Address row 0
					if (triplet->m_address != 63)
						// FIXME data column highlighted?
						triplet->m_reservedData = true;
					else if (activePosition.isDeployed())
						triplet->m_error = X26Triplet::ActivePositionMovedUp;
					else {
						activePosition.setRow(0);
						activePosition.setColumn(8);
					}
					if ((triplet->m_data & 0x60) != 0x00 && (triplet->m_data & 0x60) != 0x60)
						triplet->m_reservedData = true;
					break;
				case 0x10: // Origin Modifier
					if (i == m_list.size()-1 ||
					    m_list.at(i+1).modeExt() < 0x11 ||
					    m_list.at(i+1).modeExt() > 0x13)
						triplet->m_error = X26Triplet::OriginModifierAlone;
					break;
				case 0x11 ... 0x13: // Invoke Object
					if (triplet->objectSource() == X26Triplet::LocalObject) {
						if (triplet->objectLocalTripletNumber() > 12 ||
							triplet->objectLocalIndex() > (m_list.size()-1) ||
							m_list.at(triplet->objectLocalIndex()).modeExt() < 0x15 ||
							m_list.at(triplet->objectLocalIndex()).modeExt() > 0x17)
							triplet->m_error = X26Triplet::InvokePointerInvalid;
						else if ((triplet->modeExt() | 0x04) != m_list.at(triplet->objectLocalIndex()).modeExt())
							triplet->m_error = X26Triplet::InvokeTypeMismatch;
					}
					break;
				case 0x15 ... 0x17: // Define Object
					activePosition.reset();
					// Make sure data field holds correct place of triplet
					// otherwise the object won't appear
					triplet->setObjectLocalIndex(i);
					break;
				case 0x18: // DRCS mode
					if ((triplet->m_data & 0x30) == 0x00)
						triplet->m_reservedData = true;
				case 0x1f: // Termination marker
				case 0x08 ... 0x0d: // PDC
					break;
				default:
					triplet->m_reservedMode = true;
			};
		// Column triplet: all triplets modes except PDC and reserved move the Active Position
		} else if (triplet->modeExt() == 0x24 || triplet->modeExt() == 0x25 || triplet->modeExt() == 0x2a)
			triplet->m_reservedMode = true;
		else if (triplet->modeExt() != 0x26 && !activePosition.setColumn(triplet->addressColumn()))
			triplet->m_error = X26Triplet::ActivePositionMovedLeft;
		else
			switch (triplet->modeExt()) {
				case 0x20: // Foreground colour
				case 0x23: // Foreground colour
					if (triplet->m_data & 0x60)
						triplet->m_reservedData = true;
					break;
				case 0x21: // G1 mosaic character
				case 0x22: // G3 mosaic character at level 1.5
				case 0x29: // G0 character
				case 0x2b: // G3 mosaic character at level >=2.5
				case 0x2f ... 0x3f: // G2 character or G0 diacritical mark
					if (triplet->m_data < 0x20)
						triplet->m_reservedData = true;
					break;
				case 0x27: // Additional flash functions
					if (triplet->m_data >= 0x18)
						triplet->m_reservedData = true;
					break;
				case 0x2d: // DRCS character
					if ((triplet->m_data & 0x3f) >= 48)
						// Should really be an error?
						triplet->m_reservedData = true;
			}

		triplet->m_activePositionRow = activePosition.row();
		triplet->m_activePositionColumn = activePosition.column();
	}

	// Now work out where the Active Position goes on a Level 1.5 decoder
	activePosition.reset();

	for (int i=0; i < m_list.size(); i++) {
		triplet = &m_list[i];

		if (triplet->modeExt() == 0x1f) // Termination marker
			break;

		switch (triplet->modeExt()) {
			case 0x04: // Set Active Position;
				activePosition.setRow(triplet->addressRow());
				break;
			case 0x07: // Address row 0
				if (triplet->m_address == 63 && activePosition.setRow(0))
					activePosition.setColumn(8);
				break;
			case 0x22: // G3 mosaic character at level 1.5
			case 0x2f ... 0x3f: // G2 character or G0 diacritical mark
				activePosition.setColumn(triplet->addressColumn());
				break;
		}

		triplet->m_activePositionRow1p5 = activePosition.row();
		triplet->m_activePositionColumn1p5 = activePosition.column();
	}

}

void X26TripletList::append(const X26Triplet &value)
{
	m_list.append(value);
	updateInternalData();
}

void X26TripletList::insert(int i, const X26Triplet &value)
{
	m_list.insert(i, value);
	updateInternalData();
}

void X26TripletList::removeAt(int i)
{
	m_list.removeAt(i);
	if (m_list.size() != 0 && i < m_list.size())
		updateInternalData();
}

void X26TripletList::replace(int i, const X26Triplet &value)
{
	m_list.replace(i, value);
	updateInternalData();
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
