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

#include <QByteArray>

#include "pagebase.h"

PageBase::PageBase()
{
	// We use nullptrs to keep track of allocated packets, so initialise them this way
	for (int i=0; i<90; i++)
		m_packets[i] = nullptr;
	for (int i=PageBase::C4ErasePage; i<=PageBase::C11SerialMagazine; i++)
		m_controlBits[i] = false;
}

PageBase::PageBase(const PageBase &other)
{
	for (int i=PageBase::C4ErasePage; i<=PageBase::C11SerialMagazine; i++)
		setControlBit(i, other.controlBit(i));
	for (int i=0; i<90; i++)
		if (other.packetNeededArrayIndex(i))
			setPacketArrayIndex(i, other.packetArrayIndex(i));
		else
			m_packets[i] = nullptr;
}

PageBase::~PageBase()
{
	for (int i=0; i<90; i++)
		if (m_packets[i] != nullptr)
			delete m_packets[i];
}
QByteArray PageBase::packet(int packetNumber, int designationCode) const
{
	int arrayIndex = packetNumber;

	if (packetNumber >= 26)
		arrayIndex += (packetNumber - 26) * 16 + designationCode;
	return packetArrayIndex(arrayIndex);
}

QByteArray PageBase::packetArrayIndex(int arrayIndex) const
{
	if (m_packets[arrayIndex] == nullptr)
		return QByteArray(); // Blank result
	return *m_packets[arrayIndex];
}

bool PageBase::packetNeeded(int packetNumber, int designationCode) const
{
	int arrayIndex = packetNumber;

	if (packetNumber >= 26)
		arrayIndex += (packetNumber - 26) * 16 + designationCode;
	return packetNeededArrayIndex(arrayIndex);
}

bool PageBase::packetNeededArrayIndex(int arrayIndex) const
{
	return m_packets[arrayIndex] != nullptr;
}

bool PageBase::setPacket(int packetNumber, QByteArray packetContents)
{
	return setPacket(packetNumber, 0, packetContents);
}

bool PageBase::setPacket(int packetNumber, int designationCode, QByteArray packetContents)
{
	int arrayIndex = packetNumber;

	if (packetNumber >= 26)
		arrayIndex += (packetNumber - 26) * 16 + designationCode;
	return setPacketArrayIndex(arrayIndex, packetContents);
}

bool PageBase::setPacketArrayIndex(int arrayIndex, QByteArray packetContents)
{
	if (m_packets[arrayIndex] == nullptr)
		m_packets[arrayIndex] = new QByteArray(40, 0x00);
	*m_packets[arrayIndex] = packetContents;

	return true;
}

bool PageBase::deletePacket(int packetNumber, int designationCode)
{
	int arrayIndex = packetNumber;

	if (packetNumber >= 26)
		arrayIndex += (packetNumber - 26) * 16 + designationCode;

	return deletePacketArrayIndex(arrayIndex);
}

bool PageBase::deletePacketArrayIndex(int arrayIndex)
{
	if (m_packets[arrayIndex] != nullptr) {
		delete m_packets[arrayIndex];
		m_packets[arrayIndex] = nullptr;
	}

	return true;
}

bool PageBase::setControlBit(int bitNumber, bool active)
{
	m_controlBits[bitNumber] = active;
	return true;
}
