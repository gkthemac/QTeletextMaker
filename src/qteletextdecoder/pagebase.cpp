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
	// We use nullptrs to keep track of allocated packets, so initialise them this way
	for (int y=0; y<26; y++)
		m_displayPackets[y] = nullptr;
	for (int y=0; y<3; y++)
		for (int d=0; d<16; d++)
			m_designationPackets[y][d] = nullptr;

	for (int b=PageBase::C4ErasePage; b<=PageBase::C14NOS; b++)
		m_controlBits[b] = false;
}

PageBase::~PageBase()
{
	for (int y=0; y<26; y++)
		if (m_displayPackets[y] != nullptr)
			delete m_displayPackets[y];
	for (int y=0; y<3; y++)
		for (int d=0; d<16; d++)
			if (m_designationPackets[y][d] != nullptr)
				delete m_designationPackets[y][d];
}

bool PageBase::isEmpty() const
{
	for (int y=0; y<26; y++)
		if (m_displayPackets[y] != nullptr)
			return false;
	for (int y=0; y<3; y++)
		for (int d=0; d<16; d++)
			if (m_designationPackets[y][d] != nullptr)
				return false;

	return true;
}

QByteArray PageBase::packet(int y) const
{
	if (m_displayPackets[y] == nullptr)
		return QByteArray(); // Blank result

	return *m_displayPackets[y];
}

QByteArray PageBase::packet(int y, int d) const
{
	if (m_designationPackets[y-26][d] == nullptr)
		return QByteArray(); // Blank result

	return *m_designationPackets[y-26][d];
}


bool PageBase::setPacket(int y, QByteArray pkt)
{
	if (m_displayPackets[y] == nullptr)
		m_displayPackets[y] = new QByteArray(40, 0x00);
	*m_displayPackets[y] = pkt;

	return true;
}

bool PageBase::setPacket(int y, int d, QByteArray pkt)
{
	if (m_designationPackets[y-26][d] == nullptr)
		m_designationPackets[y-26][d] = new QByteArray(40, 0x00);
	*m_designationPackets[y-26][d] = pkt;

	return true;
}

/*
bool PageBase::clearPacket(int y)
{
	if (m_displayPackets[y] != nullptr) {
		delete m_displayPackets[y];
		m_displayPackets[y] = nullptr;
	}

	return true;
}

bool PageBase::clearPacket(int y, int d)
{
	if (m_designationPackets[y-26][d] != nullptr) {
		delete m_designationPackets[y-26][d];
		m_designationPackets[y-26][d] = nullptr;
	}

	return true;
}

void SubPage::clearAllPackets()
{
	for (int y=0; y<26; y++)
		clearPacket(y);
	for (int y=0; y<3; y++)
		for (int d=0; d<16; d++)
			clearPacket(y, d);
}
*/

bool PageBase::setControlBit(int b, bool active)
{
	m_controlBits[b] = active;
	return true;
}
