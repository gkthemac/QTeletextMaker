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

#include "drcspage.h"

DRCSPage::DRCSPage(const PageBase &other)
{
	for (int y=0; y<26; y++)
		if (other.packetExists(y))
			setPacket(y, other.packet(y));

	for (int y=26; y<29; y++)
		for (int d=0; d<16; d++)
			if (other.packetExists(y, d))
				setPacket(y, d, other.packet(y, d));

	for (int b=PageBase::C4ErasePage; b<=PageBase::C14NOS; b++)
		setControlBit(b, other.controlBit(b));
}

int DRCSPage::drcsMode(int c) const
{
	if (!packetExists(28, 3))
		return 0;

	const QByteArray pkt = packet(28, 3);

	// Some tricky bit juggling to extract 4 bits from part of a 6-bit triplet
	switch (c % 3) {
		case 0:
			return pkt.at(c/3*2 + 4) & 0xf;
		case 1:
			return ((pkt.at((c-1)/3*2 + 4) & 0x30) >> 4) | ((pkt.at((c-1)/3*2 + 5) & 0x3) << 2);
		case 2:
			return pkt.at(((c-2)/3*2 + 5) & 0x3f) >> 2;
	}

	return 0; // Won't get here; used to suppress a compiler warning
}

bool DRCSPage::ptu(int c, uchar *data) const
{
	const int pktNo = (c+2)/2;

	if (!packetExists(pktNo))
		return false;

	const int start = c%2 * 20;

	// FIXME should we check all 20 D-bytes for SPACE instead of just the first D-byte?
	if (packet(pktNo).at(start) < 0x40)
		return false;

	if (data != nullptr) {
		const int end = start + 20;

		for (int i=start, j=0; i<end; i+=2, j+=2) {
			data[j] = ((packet(pktNo).at(i) & 0x3f) << 2) | ((packet(pktNo).at(i+1) & 0x30) >> 4);
			data[j+1] = (packet(pktNo).at(i+1) & 0x0f) << 4;
		}
	}

	return true;
}
