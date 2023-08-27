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
	virtual ~PageBase();

	virtual bool isEmpty() const;

	virtual QByteArray packet(int i) const;
	virtual QByteArray packet(int i, int j) const;
	virtual bool packetExists(int i) const { return m_displayPackets[i] != nullptr; }
	virtual bool packetExists(int i, int j) const { return m_designationPackets[i-26][j] != nullptr; }
	virtual bool setPacket(int i, QByteArray packetContents);
	virtual bool setPacket(int i, int j, QByteArray packetContents);
//	bool deletePacket(int);
//	bool deletePacket(int, int);

	virtual bool controlBit(int bitNumber) const { return m_controlBits[bitNumber]; }
	virtual bool setControlBit(int bitNumber, bool active);

private:
	bool m_controlBits[11];
	QByteArray *m_displayPackets[26], *m_designationPackets[4][16];
};

#endif
