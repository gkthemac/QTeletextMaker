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

#include <QByteArray>

#include "pagebase.h"

PageBase::PageBase()
{
	for (int b=PageBase::C4ErasePage; b<=PageBase::C14NOS; b++)
		m_controlBits[b] = false;
}

bool PageBase::isEmpty() const
{
	for (int y=0; y<26; y++)
		if (!m_displayPackets[y].isEmpty())
			return false;
	for (int y=0; y<3; y++)
		for (int d=0; d<16; d++)
			if (!m_designationPackets[y][d].isEmpty())
				return false;

	return true;
}

bool PageBase::setPacket(int y, QByteArray pkt)
{
	m_displayPackets[y] = pkt;

	return true;
}

bool PageBase::setPacket(int y, int d, QByteArray pkt)
{
	m_designationPackets[y-26][d] = pkt;

	return true;
}

bool PageBase::clearPacket(int y)
{
	m_displayPackets[y] = QByteArray();

	return true;
}

bool PageBase::clearPacket(int y, int d)
{
	m_designationPackets[y-26][d] = QByteArray();

	return true;
}

void PageBase::clearAllPackets()
{
	for (int y=0; y<26; y++)
		clearPacket(y);
	for (int y=0; y<3; y++)
		for (int d=0; d<16; d++)
			clearPacket(y, d);
}

bool PageBase::setControlBit(int b, bool active)
{
	m_controlBits[b] = active;
	return true;
}
