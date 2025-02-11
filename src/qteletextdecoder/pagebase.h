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

	PageBase();
	virtual ~PageBase();

	virtual bool isEmpty() const;

	virtual QByteArray packet(int y) const;
	virtual QByteArray packet(int y, int d) const;
	virtual bool packetExists(int y) const { return m_displayPackets[y] != nullptr; }
	virtual bool packetExists(int y, int d) const { return m_designationPackets[y-26][d] != nullptr; }
	virtual bool setPacket(int y, QByteArray packet);
	virtual bool setPacket(int y, int d, QByteArray pkt);
//	bool clearPacket(int y);
//	bool clearPacket(int y, int d);
//	void clearAllPackets();

	virtual bool controlBit(int b) const { return m_controlBits[b]; }
	virtual bool setControlBit(int b, bool active);

private:
	bool m_controlBits[11];
	QByteArray *m_displayPackets[26], *m_designationPackets[3][16];
};

#endif
