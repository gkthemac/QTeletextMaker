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

#include "x26triplets.h"

void X26Triplet::setAddress(int newAddress) { myTriplet.m_address = newAddress; }
void X26Triplet::setMode(int newMode) { myTriplet.m_mode = newMode; }
void X26Triplet::setData(int newData) { myTriplet.m_data = newData; }
void X26Triplet::setAddressRow(int newAddressRow) { myTriplet.m_address = (newAddressRow == 24) ? 40 : newAddressRow+40; }
void X26Triplet::setAddressColumn(int newAddressColumn) { myTriplet.m_address = newAddressColumn; }
