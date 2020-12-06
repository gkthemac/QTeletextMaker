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
	enum ControlBitsEnum { C4ErasePage, C5Newsflash, C6Subtitle, C7SuppressHeader, C8Update, C9InterruptedSequence, C10InhibitDisplay, C11SerialMagazine, C12NOS, C13NOS, C14NOS };

	PageBase();
	PageBase(const PageBase &);
	~PageBase();

	virtual bool isEmpty() const;

	virtual QByteArray packet(int, int=0) const;
	virtual bool packetNeeded(int, int=0) const;
	virtual bool setPacket(int, QByteArray);
	virtual bool setPacket(int, int, QByteArray);
//	bool deletePacket(int, int=0);

	virtual bool controlBit(int bitNumber) const { return m_controlBits[bitNumber]; }
	virtual bool setControlBit(int, bool);

	QByteArray packetArrayIndex(int) const;
	bool packetNeededArrayIndex(int) const;
	bool setPacketArrayIndex(int, QByteArray);
//	bool deletePacketArrayIndex(int);

private:
	bool m_controlBits[11];
	QByteArray *m_packets[90]; // X/0 to X/25, plus 16 packets for X/26, another 16 for X/27, for X28 and for X/29
};

#endif
