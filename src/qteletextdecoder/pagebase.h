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

#ifndef PAGEBASE_H
#define PAGEBASE_H

#include <QByteArray>

// If we inherit from QObject then we can't copy construct, so "make a new subpage that's a copy of this one" wouldn't work
class PageBase //: public QObject
{
	//Q_OBJECT

public:
	enum ControlBitsEnum { C4ErasePage, C5Newsflash, C6Subtitle, C7SuppressHeader, C8Update, C9InterruptedSequence, C10InhibitDisplay, C11SerialMagazine, C12NOS, C13NOS, C14NOS };
	// Available Page Functions according to 9.4.2.1 of the spec
	enum PageFunctionEnum { PFUnknown = -1, PFLevelOnePage, PFDataBroadcasting, PFGlobalPOP, PFNormalPOP, PFGlobalDRCS, PFNormalDRCS, PFMOT, PFMIP, PFBasicTOPTable, PFAdditionalInformationTable, PFMultiPageTable, PFMultiPageExtensionTable, PFTriggerMessages };
	// Available Page Codings of X/1 to X/25 according to 9.4.2.1 of the spec
	enum PacketCodingEnum { CodingUnknown = -1, Coding7bit, Coding8bit, Coding18bit, Coding4bit, Coding4bitThen7bit, CodingPerPacket };

	PageBase();

	virtual PageFunctionEnum pageFunction() const { return PFUnknown; }
	virtual PacketCodingEnum packetCoding() const { return CodingUnknown; }

	virtual bool isEmpty() const;

	virtual QByteArray packet(int y) const { return m_displayPackets[y]; }
	virtual QByteArray packet(int y, int d) const { return m_designationPackets[y-26][d]; }
	virtual bool setPacket(int y, QByteArray pkt);
	virtual bool setPacket(int y, int d, QByteArray pkt);
	virtual bool packetExists(int y) const { return !m_displayPackets[y].isEmpty(); }
	virtual bool packetExists(int y, int d) const { return !m_designationPackets[y-26][d].isEmpty(); }
	virtual bool clearPacket(int y);
	virtual bool clearPacket(int y, int d);
	virtual void clearAllPackets();

	virtual bool controlBit(int b) const { return m_controlBits[b]; }
	virtual bool setControlBit(int b, bool active);

private:
	bool m_controlBits[11];
	QByteArray m_displayPackets[26], m_designationPackets[3][16];
};

#endif
