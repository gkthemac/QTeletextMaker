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

#include <QByteArray>

#include "pagebase.h"

PageBase::PageBase()
{
	// We use nullptrs to keep track of allocated packets, so initialise them this way
	for (int i=0; i<26; i++)
		m_displayPackets[i] = nullptr;
	for (int i=0; i<4; i++)
		for (int j=0; j<16; j++)
			m_designationPackets[i][j] = nullptr;

	for (int i=PageBase::C4ErasePage; i<=PageBase::C14NOS; i++)
		m_controlBits[i] = false;
}

PageBase::~PageBase()
{
	for (int i=0; i<26; i++)
		if (m_displayPackets[i] != nullptr)
			delete m_displayPackets[i];
	for (int i=0; i<4; i++)
		for (int j=0; j<16; j++)
			if (m_designationPackets[i][j] != nullptr)
				delete m_designationPackets[i][j];
}

bool PageBase::isEmpty() const
{
	for (int i=0; i<26; i++)
		if (m_displayPackets[i] != nullptr)
			return false;
	for (int i=0; i<4; i++)
		for (int j=0; j<16; j++)
			if (m_designationPackets[i][j] != nullptr)
				return false;

	return true;
}

QByteArray PageBase::packet(int i) const
{
	if (m_displayPackets[i] == nullptr)
		return QByteArray(); // Blank result

	return *m_displayPackets[i];
}

QByteArray PageBase::packet(int i, int j) const
{
	if (m_designationPackets[i-26][j] == nullptr)
		return QByteArray(); // Blank result

	return *m_designationPackets[i-26][j];
}


bool PageBase::setPacket(int i, QByteArray packetContents)
{
	if (m_displayPackets[i] == nullptr)
		m_displayPackets[i] = new QByteArray(40, 0x00);
	*m_displayPackets[i] = packetContents;

	return true;
}

bool PageBase::setPacket(int i, int j, QByteArray packetContents)
{
	if (m_designationPackets[i-26][j] == nullptr)
		m_designationPackets[i-26][j] = new QByteArray(40, 0x00);
	*m_designationPackets[i-26][j] = packetContents;

	return true;
}

/*
bool PageBase::deletePacket(int i)
{
	if (m_displayPackets[i] != nullptr) {
		delete m_displayPackets[i];
		m_displayPackets[i] = nullptr;
	}

	return true;
}

bool PageBase::deletePacket(int i)
{
	if (m_designationPackets[i-26][j] != nullptr) {
		delete m_designationPackets[i-26][j];
		m_designationPackets[i-26][j] = nullptr;
	}

	return true;
}
*/

bool PageBase::setControlBit(int bitNumber, bool active)
{
	m_controlBits[bitNumber] = active;
	return true;
}
