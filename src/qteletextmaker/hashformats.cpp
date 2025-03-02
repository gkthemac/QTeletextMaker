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

#include "hashformats.h"

#include <QByteArray>
#include <QString>

#include "levelonepage.h"
#include "pagebase.h"

QString exportHashStringPage(LevelOnePage *subPage)
{
	int hashDigits[1167]={0};
	int totalBits, charBit;
	const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	QString hashString;
	QByteArray rowPacket;

	// TODO int editTFCharacterSet = 5;
	bool blackForeground = false;

	for (int r=0; r<25; r++) {
		if (subPage->packetExists(r))
			rowPacket = subPage->packet(r);
		else
			rowPacket = QByteArray(40, 0x20).constData();

		for (int c=0; c<40; c++) {
			if (rowPacket.at(c) == 0x00 || rowPacket.at(c) == 0x10)
				blackForeground = true;
			for (int b=0; b<7; b++) {
				totalBits = (r * 40 + c) * 7 + b;
				charBit = ((rowPacket.at(c)) >> (6 - b)) & 0x01;
				hashDigits[totalBits / 6] |= charBit << (5 - (totalBits % 6));
			}
		}
	}

	hashString.append(QString("#%1:").arg(blackForeground ? 8 : 0, 1, 16));

	for (int i=0; i<1167; i++)
		hashString.append(base64[hashDigits[i]]);

	return hashString;
}

QString exportHashStringPackets(LevelOnePage *subPage)
{
	auto colourToHexString=[&](int whichCLUT)
	{
		QString resultHexString;

		for (int i=whichCLUT*8; i<whichCLUT*8+8; i++)
			resultHexString.append(QString("%1").arg(subPage->CLUT(i), 3, 16, QChar('0')));
		return resultHexString;
	};

	const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	QString result;

	if (subPage->packetExists(28,0) || subPage->packetExists(28,4)) {
		// X/28/0 and X/28/4 are duplicates apart from the CLUT definitions
		// Assemble the duplicate beginning and ending of both packets
		QString x28StringBegin, x28StringEnd;

		x28StringBegin.append(QString("00%1").arg((subPage->defaultCharSet() << 3) | subPage->defaultNOS(), 2, 16, QChar('0')).toUpper());
		x28StringBegin.append(QString("%1").arg((subPage->secondCharSet() << 3) | subPage->secondNOS(), 2, 16, QChar('0')).toUpper());
		x28StringBegin.append(QString("%1%2%3%4").arg(subPage->leftSidePanelDisplayed(), 1, 10).arg(subPage->rightSidePanelDisplayed(), 1, 10).arg(subPage->sidePanelStatusL25(), 1, 10).arg(subPage->sidePanelColumns(), 1, 16));

		x28StringEnd = QString("%1%2%3%4").arg(subPage->defaultScreenColour(), 2, 16, QChar('0')).arg(subPage->defaultRowColour(), 2, 16, QChar('0')).arg(subPage->blackBackgroundSubst(), 1, 10).arg(subPage->colourTableRemap(), 1, 10);

		if (subPage->packetExists(28,0))
			result.append(":X280=" + x28StringBegin + colourToHexString(2) + colourToHexString(3) + x28StringEnd);
		if (subPage->packetExists(28,4))
			result.append(":X284=" + x28StringBegin + colourToHexString(0) + colourToHexString(1) + x28StringEnd);
	}

	if (!subPage->enhancements()->isEmpty()) {
		result.append(":X26=");
		for (int i=0; i<subPage->enhancements()->size(); i++) {
			result.append(base64[subPage->enhancements()->at(i).data() >> 1]);
			result.append(base64[subPage->enhancements()->at(i).mode() | ((subPage->enhancements()->at(i).data() & 1) << 5)]);
			result.append(base64[subPage->enhancements()->at(i).address()]);
		}
	}

	// Assemble PS
	// C4 Erase page is stored in bit 14
	int pageStatus = 0x8000 | (subPage->controlBit(PageBase::C4ErasePage) << 14);
	// C5 to C11 stored in order from bits 1 to 6
	for (int i=PageBase::C5Newsflash; i<=PageBase::C11SerialMagazine; i++)
		pageStatus |= subPage->controlBit(i) << (i-1);
	// Apparently the TTI format stores the NOS bits backwards
	pageStatus |= subPage->controlBit(PageBase::C12NOS) << 9;
	pageStatus |= subPage->controlBit(PageBase::C13NOS) << 8;
	pageStatus |= subPage->controlBit(PageBase::C14NOS) << 7;

	result.append(QString(":PS=%1").arg(0x8000 | pageStatus, 0, 16, QChar('0')));
	return result;
}
