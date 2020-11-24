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
#include <QColor>
#include <QList>
#include <QString>
#include <algorithm>

#include "levelonepage.h"

LevelOnePage::LevelOnePage()
{
	m_paddingX26Triplet.setAddress(41);
	m_paddingX26Triplet.setMode(0x1e);
	m_paddingX26Triplet.setData(0);
	localEnhance.reserve(208);
	clearPage();
}

LevelOnePage::LevelOnePage(const PageBase &other)
{
	m_paddingX26Triplet.setAddress(41);
	m_paddingX26Triplet.setMode(0x1e);
	m_paddingX26Triplet.setData(0);
	localEnhance.reserve(208);
	clearPage();

	for (int i=PageBase::C4ErasePage; i<=PageBase::C14NOS; i++)
		setControlBit(i, other.controlBit(i));
	for (int i=0; i<90; i++)
		if (other.packetNeededArrayIndex(i))
			setPacketArrayIndex(i, other.packetArrayIndex(i));
}

// So far we only call clearPage() once, within the constructor
void LevelOnePage::clearPage()
{
	for (int r=0; r<25; r++)
		for (int c=0; c<40; c++)
			m_level1Page[r][c] = 0x20;
	for (int i=C4ErasePage; i<=C14NOS; i++)
		setControlBit(i, false);
	for (int i=0; i<8; i++)
		m_composeLink[i] = { (i<4) ? i : 0, false, i>=4, 0x0ff, 0x0000 };
	for (int i=0; i<6; i++)
		m_fastTextLink[i] = { 0x0ff, 0x3f7f };

/*	m_subPageNumber = 0x0000; */
	m_cycleValue = 8;
	m_cycleType = CTseconds;
	m_defaultCharSet = 0;
	m_defaultNOS = 0;
	m_secondCharSet = 0xf;
	m_secondNOS = 0x7;

	m_defaultScreenColour = 0;
	m_defaultRowColour = 0;
	m_blackBackgroundSubst = false;
	m_colourTableRemap = 0;
	m_leftSidePanelDisplayed = m_rightSidePanelDisplayed = false;
	m_sidePanelStatusL25 = true;
	m_sidePanelColumns = 0;
	std::copy(defaultCLUT, defaultCLUT+32, m_CLUT);
//	If clearPage() is called outside constructor, we need to implement localEnhance.clear();
}

QByteArray LevelOnePage::packet(int packetNumber, int designationCode)
{
	QByteArray result(40, 0x00);

	if (packetNumber <= 24) {
		for (int c=0; c<40; c++)
			result[c] = m_level1Page[packetNumber][c];
		return result;
	}

	if (packetNumber == 26) {
		if (!packetNeeded(26, designationCode))
			return result; // Blank result

		int enhanceListPointer;
		X26Triplet lastTriplet;

		for (int i=0; i<13; i++) {
			enhanceListPointer = designationCode*13+i;

			if (enhanceListPointer < localEnhance.size()) {
				result[i*3+1] = localEnhance.at(enhanceListPointer).address();
				result[i*3+2] = localEnhance.at(enhanceListPointer).mode() | ((localEnhance.at(enhanceListPointer).data() & 1) << 5);
				result[i*3+3] = localEnhance.at(enhanceListPointer).data() >> 1;

				// If this is the last triplet, get a copy to repeat to the end of the packet
				if (enhanceListPointer == localEnhance.size()-1) {
					lastTriplet = localEnhance.at(enhanceListPointer);
					// If the last triplet was NOT a Termination Marker, make up one
					if (lastTriplet.mode() != 0x1f || lastTriplet.address() != 0x3f) {
						lastTriplet.setAddress(0x3f);
						lastTriplet.setMode(0x1f);
						lastTriplet.setData(0x07);
					}
				}
			} else {
				// We've gone past the end of the triplet list, so repeat the Termination Marker to the end
				result[i*3+1] = lastTriplet.address();
				result[i*3+2] = lastTriplet.mode() | ((lastTriplet.data() & 1) << 5);
				result[i*3+3] = lastTriplet.data() >> 1;
			}
		}
		return result;
	}

	if (packetNumber == 27 && designationCode == 0) {
		for (int i=0; i<6; i++) {
			result[i*6+1] = m_fastTextLink[i].pageNumber & 0x00f;
			result[i*6+2] = (m_fastTextLink[i].pageNumber & 0x0f0) >> 4;
			result[i*6+3] = m_fastTextLink[i].subPageNumber & 0x000f;
			result[i*6+4] = ((m_fastTextLink[i].subPageNumber & 0x0070) >> 4) | ((m_fastTextLink[i].pageNumber & 0x100) >> 8);
			result[i*6+5] = (m_fastTextLink[i].subPageNumber & 0x0f00) >> 8;
			result[i*6+6] = ((m_fastTextLink[i].subPageNumber & 0x3000) >> 12) | ((m_fastTextLink[i].pageNumber & 0x600) >> 7);
		}
		result[43] = 0xf;
		result[44] = result[45] = 0;

		return result;
	}

	if (packetNumber == 27 && (designationCode == 4 || designationCode == 5)) {
		for (int i=0; i<(designationCode == 4 ? 6 : 2); i++) {
			int pageLinkNumber = i+(designationCode == 4 ? 0 : 6);

			result[i*6+1] = (m_composeLink[pageLinkNumber].level3p5 << 3) | (m_composeLink[pageLinkNumber].level2p5 << 2) | m_composeLink[pageLinkNumber].function;
			result[i*6+2] = ((m_composeLink[pageLinkNumber].pageNumber & 0x100) >> 3) | 0x10 | (m_composeLink[pageLinkNumber].pageNumber & 0x00f);
			result[i*6+3] = ((m_composeLink[pageLinkNumber].pageNumber & 0x0f0) >> 2) | ((m_composeLink[pageLinkNumber].pageNumber & 0x600) >> 9);

			result[i*6+4] = ((m_composeLink[pageLinkNumber].subPageCodes & 0x000f) << 2);
			result[i*6+5] = ((m_composeLink[pageLinkNumber].subPageCodes & 0x03f0) >> 4);
			result[i*6+6] = ((m_composeLink[pageLinkNumber].subPageCodes & 0xfc00) >> 10);
		}

		return result;
	}

	if (packetNumber == 28 && (designationCode == 0 || designationCode == 4)) {
		int CLUToffset = (designationCode == 0) ? 16 : 0;

		result[1] = 0x00;
		result[2] = ((m_defaultCharSet & 0x3) << 4) | (m_defaultNOS << 1);
		result[3] = ((m_secondCharSet & 0x1) << 5) | (m_secondNOS << 2) | (m_defaultCharSet >> 2);
		result[4] = (m_sidePanelStatusL25 << 5) | (m_rightSidePanelDisplayed << 4) | (m_leftSidePanelDisplayed << 3) | (m_secondCharSet >> 1);
		result[5] = m_sidePanelColumns | ((m_CLUT[CLUToffset] & 0x300) >> 4);

		for (int c=0; c<16; c++) {
			result[c*2+6] = ((m_CLUT[CLUToffset+c] & 0x0f0) >> 2) | ((m_CLUT[CLUToffset+c] & 0xf00) >> 10);
			result[c*2+7] = ((m_CLUT[CLUToffset+c+1] & 0x300) >> 4) | (m_CLUT[CLUToffset+c] & 0x00f);
		}

		result[37] = ((m_defaultScreenColour & 0x03) << 4) | (m_CLUT[CLUToffset+15] & 0x00f);
		result[38] = ((m_defaultRowColour & 0x07) << 3) | (m_defaultScreenColour >> 2);
		result[39] = (m_colourTableRemap << 3) | (m_blackBackgroundSubst << 2) | (m_defaultRowColour >> 3);

		return result;
	}

	return PageBase::packet(packetNumber, designationCode);
}

bool LevelOnePage::setPacket(int packetNumber, QByteArray packetContents)
{
	if (packetNumber <= 24) {
		for (int c=0; c<40; c++)
			m_level1Page[packetNumber][c] = packetContents.at(c);
		return true;
	}

	return PageBase::setPacket(packetNumber, packetContents);
}

bool LevelOnePage::setPacket(int packetNumber, int designationCode, QByteArray packetContents)
{
	if (packetNumber == 26) {
		// Preallocate entries in the localEnhance list to hold our incoming triplets.
		// We write "dummy" reserved 11110 Row Triplets in the allocated entries which then get overwritten by the packet contents.
		// This is in case of missing packets so we can keep Local Object pointers valid.
		while (localEnhance.size() < (designationCode+1)*13)
			localEnhance.append(m_paddingX26Triplet);

		int enhanceListPointer;
		X26Triplet newX26Triplet;

		for (int i=0; i<13; i++) {
			enhanceListPointer = designationCode*13+i;

			newX26Triplet.setAddress(packetContents.at(i*3+1) & 0x3f);
			newX26Triplet.setMode(packetContents.at(i*3+2) & 0x1f);
			newX26Triplet.setData(((packetContents.at(i*3+3) & 0x3f) << 1) | ((packetContents.at(i*3+2) & 0x20) >> 5));
			localEnhance[enhanceListPointer] = newX26Triplet;
		}
		if (newX26Triplet.mode() == 0x1f && newX26Triplet.address() == 0x3f && newX26Triplet.data() & 0x01)
			// Last triplet was a Termination Marker (without ..follows) so clean up the repeated ones
			while (localEnhance.size()>1 && localEnhance.at(localEnhance.size()-2).mode() == 0x1f && localEnhance.at(localEnhance.size()-2).address() == 0x3f && localEnhance.at(localEnhance.size()-2).data() == newX26Triplet.data())
				localEnhance.removeLast();

		return true;
	}

	if (packetNumber == 27 && designationCode == 0) {
		for (int i=0; i<6; i++) {
			int relativeMagazine = (packetContents.at(i*6+4) >> 3) | ((packetContents.at(i*6+6) & 0xc) >> 1);
			int pageNumber = (packetContents.at(i*6+2) << 4) | packetContents.at(i*6+1);
			m_fastTextLink[i].pageNumber = (relativeMagazine << 8) | pageNumber;
			m_fastTextLink[i].subPageNumber = packetContents.at(i*6+3) | ((packetContents.at(i*6+4) & 0x7) << 4) | (packetContents.at(i*6+5) << 8) | ((packetContents.at(i*6+6) & 0x3) << 12);
			// TODO remove this warning when we can preserve FastText subpage links
			if (m_fastTextLink[i].subPageNumber != 0x3f7f)
				qDebug("FastText link %d has custom subPageNumber %x - will NOT be saved!", i, m_fastTextLink[i].subPageNumber);
		}
		return true;
	}

	if (packetNumber == 27 && (designationCode == 4 || designationCode == 5)) {
		for (int i=0; i<(designationCode == 4 ? 6 : 2); i++) {
			int pageLinkNumber = i+(designationCode == 4 ? 0 : 6);
			int pageFunction = packetContents.at(i*6+1) & 0x03;
			if (i >= 4)
				m_composeLink[pageLinkNumber].function = pageFunction;
			else if (i != pageFunction)
				qDebug("X/27/4 link number %d fixed at function %d. Attempted to set to %d.", pageLinkNumber, pageLinkNumber, pageFunction);

			m_composeLink[pageLinkNumber].level2p5 = packetContents.at(i*6+1) & 0x04;
			m_composeLink[pageLinkNumber].level3p5 = packetContents.at(i*6+1) & 0x08;

			m_composeLink[pageLinkNumber].pageNumber = ((packetContents.at(i*6+3) & 0x03) << 9) | ((packetContents.at(i*6+2) & 0x20) << 3) | ((packetContents.at(i*6+3) & 0x3c) << 2) | (packetContents.at(i*6+2) & 0x0f);

			m_composeLink[pageLinkNumber].subPageCodes = (packetContents.at(i*6+4) >> 2) | (packetContents.at(i*6+5) << 4) | (packetContents.at(i*6+6) << 10);
		}
		return true;
	}

	if (packetNumber == 28 && (designationCode == 0 || designationCode == 4)) {
		int CLUToffset = (designationCode == 0) ? 16 : 0;

		m_defaultCharSet = ((packetContents.at(2) >> 4) & 0x3) | ((packetContents.at(3) << 2) & 0xc);
		// Don't set m_defaultNOS directly as we need to keep control bits in subclass in sync
		setDefaultNOS((packetContents.at(2) >> 1) & 0x7);
		m_secondCharSet = ((packetContents.at(3) >> 5) & 0x1) | ((packetContents.at(4) << 1) & 0xe);
		m_secondNOS = (packetContents.at(3) >> 2) & 0x7;

		m_leftSidePanelDisplayed = (packetContents.at(4) >> 3) & 1;
		m_rightSidePanelDisplayed = (packetContents.at(4) >> 4) & 1;
		m_sidePanelStatusL25 = (packetContents.at(4) >> 5) & 1;
		m_sidePanelColumns = packetContents.at(5) & 0xf;

		for (int c=0; c<16; c++)
			m_CLUT[CLUToffset+c] = ((packetContents.at(c*2+5) << 4) & 0x300) | ((packetContents.at(c*2+6) << 10) & 0xc00) | ((packetContents.at(c*2+6) << 2) & 0x0f0) | (packetContents.at(c*2+7) & 0x00f);

		m_defaultScreenColour = (packetContents.at(37) >> 4) | ((packetContents.at(38) << 2) & 0x1c);
		m_defaultRowColour = ((packetContents.at(38)) >> 3) | ((packetContents.at(39) << 3) & 0x18);
		m_blackBackgroundSubst = (packetContents.at(39) >> 2) & 1;
		m_colourTableRemap = (packetContents.at(39) >> 3) & 7;

		return true;
	}

	qDebug("LevelOnePage unhandled packet X/%d/%d", packetNumber, designationCode);
	return PageBase::setPacket(packetNumber, designationCode, packetContents);
}

bool LevelOnePage::packetNeeded(int packetNumber, int designationCode) const
{
	if (packetNumber <= 24) {
		for (int c=0; c<40; c++)
			if (m_level1Page[packetNumber][c] != 0x20)
				return true;
		return false;
	}

	if (packetNumber == 26)
		return ((localEnhance.size()+12) / 13) > designationCode;

	if (packetNumber == 27 && designationCode == 0) {
		for (int i=0; i<6; i++)
			if ((m_fastTextLink[i].pageNumber & 0x0ff) != 0xff)
				return true;

		return false;
	}

	if (packetNumber == 27 && (designationCode == 4 || designationCode == 5)) {
		for (int i=0; i<(designationCode == 4 ? 6 : 2); i++) {
			int pageLinkNumber = i+(designationCode == 4 ? 0 : 6);
			if ((m_composeLink[pageLinkNumber].pageNumber & 0x0ff) != 0x0ff)
				return true;
		}
		return false;
	}

	if (packetNumber == 28) {
		if (designationCode == 0) {
			if (m_leftSidePanelDisplayed || m_rightSidePanelDisplayed || m_defaultScreenColour !=0 || m_defaultRowColour !=0 || m_blackBackgroundSubst || m_colourTableRemap !=0 || m_defaultCharSet != 0 || m_secondCharSet != 0xf)
				return true;
			for (int i=16; i<32; i++)
				if (m_CLUT[i] != defaultCLUT[i])
					return true;
			return false;
		}
		if (designationCode == 4) {
			for (int i=0; i<16; i++)
				if (m_CLUT[i] != defaultCLUT[i])
					return true;
			return false;
		}
	}
	return PageBase::packetNeeded(packetNumber, designationCode);
}

void LevelOnePage::loadPagePacket(QByteArray &inLine)
{
	bool lineNumberOk;
	int lineNumber, secondCommaPosition;

	secondCommaPosition = inLine.indexOf(",", 3);
	if (secondCommaPosition != 4 && secondCommaPosition != 5)
		return;

	lineNumber = inLine.mid(3, secondCommaPosition-3).toInt(&lineNumberOk, 10);
	if (lineNumberOk && lineNumber>=0 && lineNumber<=29) {
		inLine.remove(0, secondCommaPosition+1);
		if (lineNumber <= 25) {
			for (int c=0; c<40; c++) {
				// trimmed() helpfully removes CRLF line endings from the just-read line for us
				// But it also (un)helpfully removes spaces at the end of a 40 character line, so put them back
				if (c >= inLine.size())
					inLine.append(' ');
				if (inLine.at(c) & 0x80)
					inLine[c] = inLine.at(c) & 0x7f;
				else if (inLine.at(c) == 0x10)
					inLine[c] = 0x0d;
				else if (inLine.at(c) == 0x1b) {
					inLine.remove(c, 1);
					inLine[c] = inLine.at(c) & 0xbf;
				}
			}
			setPacket(lineNumber, inLine);
		} else {
			int designationCode = inLine.at(0) & 0x3f;
			if (inLine.size() < 40) {
				// OL is too short!
				if (lineNumber == 26) {
					// For a too-short enhancement triplets OL, first trim the line down to nearest whole triplet
					inLine.resize((inLine.size() / 3 * 3) + 1);
					// Then use "dummy" enhancement triplets to extend the line to the proper length
					for (int i=inLine.size(); i<40; i+=3)
						inLine.append("i^@"); // Address 41, Mode 0x1e, Data 0
				} else
					// For other triplet OLs and Hamming 8/4 OLs, just pad with zero data
					inLine.leftJustified(40, '@');
			}
			for (int i=1; i<=39; i++)
				inLine[i] = inLine.at(i) & 0x3f;
			setPacket(lineNumber, designationCode, inLine);
		}
	}
}

int LevelOnePage::controlBitsToPS() const
{
	//TODO map page language for regions other than 0
	int pageStatus = 0x8000 | (controlBit(PageBase::C4ErasePage) << 14) | ((m_defaultNOS & 1) << 9) | ((m_defaultNOS & 2) << 7) | ((m_defaultNOS & 4) << 5);
	for (int i=C5Newsflash; i<=C11SerialMagazine; i++)
		pageStatus |= controlBit(i) << (i-1);
	return pageStatus;
}

QString LevelOnePage::exportURLHash(QString pageHash)
{
	int hashDigits[1167]={0};
	int totalBits, charBit;
	const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

	//TODO deal with "black text allowed"
	pageHash.append(QString("#%1:").arg(m_defaultNOS, 1, 16));
	for (int r=0; r<25; r++) 
		for (int c=0; c<40; c++)
			for (int b=0; b<7; b++) {
				totalBits = (r * 40 + c) * 7 + b;
				charBit = ((m_level1Page[r][c]) >> (6 - b)) & 0x01;
				hashDigits[totalBits / 6] |= charBit << (5 - (totalBits % 6));
			}

	for (int i=0; i<1167; i++)
		pageHash.append(base64[hashDigits[i]]);

	if (packetNeeded(28,0) || packetNeeded(28,4)) {
		QString x28StringBegin, x28StringEnd;

		x28StringBegin.append(QString("00%1").arg((m_defaultCharSet << 3) | m_defaultNOS, 2, 16, QChar('0')).toUpper());
		x28StringBegin.append(QString("%1").arg((m_secondCharSet << 3) | m_secondNOS, 2, 16, QChar('0')).toUpper());
		x28StringBegin.append(QString("%1%2%3%4").arg(m_leftSidePanelDisplayed, 1, 10).arg(m_rightSidePanelDisplayed, 1, 10).arg(m_sidePanelStatusL25, 1, 10).arg(m_sidePanelColumns, 1, 16));

		x28StringEnd = QString("%1%2%3%4").arg(m_defaultScreenColour, 2, 16, QChar('0')).arg(m_defaultRowColour, 2, 16, QChar('0')).arg(m_blackBackgroundSubst, 1, 10).arg(m_colourTableRemap, 1, 10);

		if (packetNeeded(28,0)) {
			pageHash.append(":X280=");
			pageHash.append(x28StringBegin);
			pageHash.append(colourHash(1));
			pageHash.append(x28StringEnd);
		}
		if (packetNeeded(28,4)) {
			pageHash.append(":X284=");
			pageHash.append(x28StringBegin);
			pageHash.append(colourHash(0));
			pageHash.append(x28StringEnd);
		}
	}

	if (!localEnhance.isEmpty()) {
		pageHash.append(":X26=");
		for (int i=0; i<localEnhance.size(); i++) {
			pageHash.append(base64[localEnhance.at(i).data() >> 1]);
			pageHash.append(base64[localEnhance.at(i).mode() | ((localEnhance.at(i).data() & 1) << 5)]);
			pageHash.append(base64[localEnhance.at(i).address()]);
		}
		//TODO need to add one or more terminators to X26
	}
	//TODO check if 0x8000 | is needed for zxnet
	pageHash.append(QString(":PS=%1").arg(0x8000 | controlBitsToPS(), 0, 16, QChar('0')));
	return pageHash;
}

/* void LevelOnePage::setSubPageNumber(int newSubPageNumber) { m_subPageNumber = newSubPageNumber; } */
void LevelOnePage::setCycleValue(int newValue) { m_cycleValue = newValue; };
void LevelOnePage::setCycleType(CycleTypeEnum newType) { m_cycleType = newType; }
void LevelOnePage::setDefaultCharSet(int newDefaultCharSet) { m_defaultCharSet = newDefaultCharSet; }

void LevelOnePage::setDefaultNOS(int defaultNOS)
{
	m_defaultNOS = defaultNOS;
	setControlBit(C12NOS, defaultNOS & 1);
	setControlBit(C13NOS, defaultNOS & 2);
	setControlBit(C14NOS, defaultNOS & 4);
}

void LevelOnePage::setSecondCharSet(int newSecondCharSet)
{
	m_secondCharSet = newSecondCharSet;
	if (m_secondCharSet == 0xf)
		m_secondNOS = 0x7;
}

void LevelOnePage::setSecondNOS(int newSecondNOS) { m_secondNOS = newSecondNOS; }
void LevelOnePage::setCharacter(int row, int column, unsigned char newCharacter) { m_level1Page[row][column] = newCharacter; }
void LevelOnePage::setDefaultScreenColour(int newDefaultScreenColour) { m_defaultScreenColour = newDefaultScreenColour; }
void LevelOnePage::setDefaultRowColour(int newDefaultRowColour) { m_defaultRowColour = newDefaultRowColour; }
void LevelOnePage::setColourTableRemap(int newColourTableRemap) { m_colourTableRemap = newColourTableRemap; }
void LevelOnePage::setBlackBackgroundSubst(bool newBlackBackgroundSubst) { m_blackBackgroundSubst = newBlackBackgroundSubst; }

int LevelOnePage::CLUT(int index, int renderLevel) const
{
	if (renderLevel == 2)
		return index>=16 ? m_CLUT[index] : defaultCLUT[index];
	else
		return renderLevel==3 ? m_CLUT[index] : defaultCLUT[index];
}

void LevelOnePage::setCLUT(int index, int newColour) { m_CLUT[index] = newColour; }
void LevelOnePage::setLeftSidePanelDisplayed(bool newLeftSidePanelDisplayed) { m_leftSidePanelDisplayed = newLeftSidePanelDisplayed; }
void LevelOnePage::setRightSidePanelDisplayed(bool newRightSidePanelDisplayed) { m_rightSidePanelDisplayed = newRightSidePanelDisplayed; }
void LevelOnePage::setSidePanelColumns(int newSidePanelColumns) { m_sidePanelColumns = newSidePanelColumns; }
void LevelOnePage::setSidePanelStatusL25(bool newSidePanelStatusL25) { m_sidePanelStatusL25 = newSidePanelStatusL25; }

void LevelOnePage::setFastTextLinkPageNumber(int linkNumber, int pageNumber)
{
	m_fastTextLink[linkNumber].pageNumber = pageNumber;
}

void LevelOnePage::setComposeLinkFunction(int linkNumber, int newFunction)
{
	m_composeLink[linkNumber].function = newFunction;
}

void LevelOnePage::setComposeLinkLevel2p5(int linkNumber, bool newRequired)
{
	m_composeLink[linkNumber].level2p5 = newRequired;
}

void LevelOnePage::setComposeLinkLevel3p5(int linkNumber, bool newRequired)
{
	m_composeLink[linkNumber].level3p5 = newRequired;
}

void LevelOnePage::setComposeLinkPageNumber(int linkNumber, int newPageNumber)
{
	m_composeLink[linkNumber].pageNumber = newPageNumber;
}

void LevelOnePage::setComposeLinkSubPageCodes(int linkNumber, int newSubPageCodes)
{
	m_composeLink[linkNumber].subPageCodes = newSubPageCodes;
}

QString LevelOnePage::colourHash(int whichCLUT)
{
	QString resultHash;

	for (int i=whichCLUT*16; i<whichCLUT*16+16; i++)
		resultHash.append(QString("%1").arg(m_CLUT[i], 3, 16, QChar('0')));
	return resultHash;
}

QColor CLUTtoQColor(int myColour)
{
	return QColor(((myColour & 0xf00) >> 8) * 17, ((myColour & 0x0f0) >> 4) * 17, (myColour & 0x00f) * 17);
}
