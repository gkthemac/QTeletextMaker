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
