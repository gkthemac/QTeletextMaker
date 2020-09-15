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

#ifndef PAGEBASE_H
#define PAGEBASE_H

#include <QByteArray>

// If we inherit from QObject then we can't copy construct, so "make a new subpage that's a copy of this one" wouldn't work
class PageBase //: public QObject
{
	//Q_OBJECT

public:
	// Available Page Functions according to 9.4.2.1 of the spec
	enum PageFunctionEnum { PFLOP, PFData, PFGPOP, PFPOP, PFGDRCS, PFDRCS, PFMOT, PFMIP, PFBTT, PFAIT, PFMPT, PFMPTEX, PFTriggers };
	// Available Page Codings of X/1 to X/25 according to 9.4.2.1 of the spec
	enum PacketCodingEnum { PC7bit, PC8bit, PC18bit, PC4bit, PC4bit7bit, PCMixed };

	PageBase();
	~PageBase();
	QByteArray packet(int, int=0) const;
	bool packetNeeded(int, int=0) const;
	bool setPacket(int, QByteArray);
	bool setPacket(int, int, QByteArray);
	bool deletePacket(int, int=0);

	PageFunctionEnum pageFunction() const { return m_pageFunction; }
	bool setPageFunction(PageFunctionEnum);
	PacketCodingEnum packetCoding(int=0, int=0) const;
	bool setPacketCoding(PacketCodingEnum);

private:
	PageFunctionEnum m_pageFunction;
	PacketCodingEnum m_packetCoding;
	QByteArray *m_packets[90]; // X/0 to X/25, plus 16 packets for X/26, another 16 for X/27, for X28 and for X/29
};

#endif
