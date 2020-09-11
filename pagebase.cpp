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
	m_pageFunction = PFLOP;
	m_packetCoding = PC7bit;
	// We use nullptrs to keep track of allocated packets, so initialise them this way
	for (int i=0; i<89; i++)
		m_packets[i] = nullptr;
}

PageBase::~PageBase()
{
	for (int i=0; i<89; i++)
		if (m_packets[i] != nullptr)
			delete m_packets[i];
}

bool PageBase::setPacket(int packetNumber, QByteArray packetContents)
{
	// X/26 and above need a designation code
	if (packetNumber >= 26)
		return false;

	return setPacket(packetNumber, 0, packetContents);
}

bool PageBase::setPacket(int packetNumber, int designationCode, QByteArray packetContents)
{
	if (packetNumber >= 30)
		return false;

	int packetArrayNumber = packetNumber;
	if (packetNumber >= 26)
		packetArrayNumber += (packetNumber - 26) * 16;
	if (m_packets[packetArrayNumber] == nullptr)
		m_packets[packetArrayNumber] = new QByteArray;
	*m_packets[packetArrayNumber] = packetContents;

	return true;
}

PageBase::PacketCodingEnum PageBase::packetCoding(int packetNumber, int designationCode) const
{
	switch (packetNumber) {
		case 26:
		case 28:
			return PC18bit;
		case 27:
			return PC4bit;
		default:
			return m_packetCoding;
	}
}

bool PageBase::setPageFunction(PageBase::PageFunctionEnum newPageFunction)
{
	m_pageFunction = newPageFunction;
	return true;
}

bool PageBase::setPacketCoding(PageBase::PacketCodingEnum newPacketEncoding)
{
	m_packetCoding = newPacketEncoding;
	return true;
}
