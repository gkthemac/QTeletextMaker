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

TeletextPage::TeletextPage()
{
	m_paddingX26Triplet.setAddress(41);
	m_paddingX26Triplet.setMode(0x1e);
	m_paddingX26Triplet.setData(0);
	localEnhance.reserve(208);
	clearPage();
}

// So far we only call clearPage() once, within the constructor
void TeletextPage::clearPage()
{
	for (int r=0; r<25; r++)
		for (int c=0; c<40; c++)
			m_level1Page[r][c] = 0x20;
	for (int i=0; i<8; i++)
		m_controlBits[i] = false;
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

bool TeletextPage::setPacket(int packetNumber, QByteArray packetContents)
{
	if (packetNumber <= 24) {
		for (int c=0; c<40; c++)
			m_level1Page[packetNumber][c] = packetContents.at(c);
		return true;
	}

	return PageBase::setPacket(packetNumber, packetContents);
}

bool TeletextPage::setPacket(int packetNumber, int designationCode, QByteArray packetContents)
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
	if (packetNumber == 28 && (designationCode == 0 || designationCode == 4)) {
		int CLUToffset = (designationCode == 0) ? 16 : 0;

		m_defaultCharSet = ((packetContents.at(2) >> 4) & 0x3) | ((packetContents.at(3) << 2) & 0xc);
		m_defaultNOS = (packetContents.at(2) >> 1) & 0x7;
		m_secondCharSet = ((packetContents.at(3) >> 5) & 0x1) | ((packetContents.at(4) << 1) & 0xe);
		m_secondNOS = (packetContents.at(3) >> 2) & 0x7;

		m_leftSidePanelDisplayed = (packetContents.at(4) >> 3) & 1;
		m_rightSidePanelDisplayed = (packetContents.at(4) >> 4) & 1;
		m_sidePanelStatusL25 = (packetContents.at(4) >> 5) & 1;
		m_sidePanelColumns = packetContents.at(5) & 0xf;

		for (int c=0; c<16; c++)
			m_CLUT[CLUToffset+c] = ((packetContents.at(c*2+5) << 4) & 0x300) | ((packetContents.at(c*2+6) << 10) & 0xc00) | ((packetContents.at(c*2+6) << 2) & 0x0f0) | (packetContents.at(c*2+7) & 0xf);

		m_defaultScreenColour = (packetContents.at(37) >> 4) | ((packetContents.at(38) << 2) & 0x1c);
		m_defaultRowColour = ((packetContents.at(38)) >> 3) | ((packetContents.at(39) << 3) & 0x18);
		m_blackBackgroundSubst = (packetContents.at(39) >> 2) & 1;
		m_colourTableRemap = (packetContents.at(39) >> 3) & 7;

		return true;
	}

	qDebug("LevelOnePage unhandled packet X%d/%d", packetNumber, designationCode);
	return PageBase::setPacket(packetNumber, designationCode, packetContents);
}

bool TeletextPage::packetNeeded(int packetNumber, int designationCode) const
{
	if (packetNumber <= 24) {
		for (int c=0; c<40; c++)
			if (m_level1Page[packetNumber][c] != 0x20)
				return true;
		return false;
	}
	// TODO packets 26 and 27
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
	return true;
}

void TeletextPage::loadPagePacket(QByteArray &inLine)
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
				// trimmed() helpfully removes CRLF line endings from the just-read line
				// but it also (un)helpfully removes spaces at end of a line, so put them back
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
			for (int i=1; i<=39; i++)
				inLine[i] = inLine.at(i) & 0x3f;
			setPacket(lineNumber, designationCode, inLine);
		}
	}
}

// This will be gradually be converted to just getting the raw packets from the Page class and writing out a TTI file.
void TeletextPage::savePage(QTextStream *outStream, int pageNumber, int subPageNumber)
{
//	int pageStatus = 0x8000 | (controlBits[0] << 14) | ((defaultPageNOS & 1) << 9) | ((defaultPageNOS & 2) << 7) | ((defaultPageNOS & 4) << 5);
//	for (int i=1; i<8; i++)
//		pageStatus |= controlBits[i] << i;
	*outStream << QString("PN,%1%2").arg(pageNumber, 3, 16, QChar('0')).arg(subPageNumber & 0xff, 2, 16, QChar('0')) << endl;
	*outStream << QString("SC,%1").arg(subPageNumber, 4, 16, QChar('0')) << endl;
	*outStream << QString("PS,%1").arg(0x8000 | controlBitsToPS(), 4, 16, QChar('0')) << endl;
	*outStream << QString("CT,%1,%2").arg(m_cycleValue).arg(m_cycleType==CTcycles ? 'C' : 'T') << endl;
	//TODO RE and maybe FLOF?

	if (packetNeeded(28, 0))
		*outStream << "OL,28," << x28toTTI(0) << endl;
	if (packetNeeded(28, 4))
		*outStream << "OL,28," << x28toTTI(4) << endl;

	if (!localEnhance.isEmpty()) {
		int tripletNumber = 0;
		X26Triplet lastTriplet = localEnhance.at(localEnhance.size()-1);
		int terminatorNeeded = true; // Becomes false if a termination marker (without a "...follow") is already at the end

		if (lastTriplet.mode() == 0x1f && lastTriplet.address() == 0x3f)
			if (lastTriplet.data() & 0x01)
				terminatorNeeded = false;
			else
				// Last termination marker has "follow" set but nothing follows, so write another one afterwards
				lastTriplet.setData(lastTriplet.data() | 0x01);
		else {
			// No termination marker there, so make up one
			lastTriplet.setAddress(0x3f);
			lastTriplet.setMode(0x1f);
			lastTriplet.setData(0x07);
		}

		for (int d=0; d<16; d++) {
			*outStream << "OL,26," << (char)(d | 0x40);
			for (int t=0; t<13; t++) {
				if (tripletNumber < localEnhance.size()) {
					*outStream << (char)(0x40 | localEnhance.at(tripletNumber).address());
					*outStream << (char)(0x40 | (localEnhance.at(tripletNumber).mode() | ((localEnhance.at(tripletNumber).data() & 1) << 5)));
					*outStream << (char)(0x40 | (localEnhance.at(tripletNumber).data() >> 1));
				} else {
					*outStream << (char)(0x40 | lastTriplet.address());
					*outStream << (char)(0x40 | (lastTriplet.mode() | ((lastTriplet.data() & 1) << 5)));
					*outStream << (char)(0x40 | (lastTriplet.data() >> 1));
					terminatorNeeded = false;
				}
				tripletNumber++;
			}
			*outStream << endl;
			// If the last triplet of the last X26 row wasn't a termination marker,
			// terminatorNeeded ensures we write an additional X26 row full of termination markers.
			if (!terminatorNeeded && tripletNumber >= localEnhance.size())
				break;
		}
	}
	for (int r=1; r<25; r++)
		if (packetNeeded(r)) {
			QString rowString;

			rowString.append(QString("OL,%1,").arg(r));
			for (int c=0; c<40; c++) {
				unsigned char myChar = m_level1Page[r][c];
				if (myChar < 32) {
					rowString.append((char)0x1b);
					rowString.append((char)(myChar+0x40));
				} else
					rowString.append((char)myChar);
			}
			*outStream << rowString << endl;
	}
}

int TeletextPage::controlBitsToPS() const
{
	//TODO map page language for regions other than 0
	int pageStatus = 0x8000 | (m_controlBits[0] << 14) | ((m_defaultNOS & 1) << 9) | ((m_defaultNOS & 2) << 7) | ((m_defaultNOS & 4) << 5);
	for (int i=1; i<8; i++)
		pageStatus |= m_controlBits[i] << (i-1);
	return pageStatus;
}

QString TeletextPage::x28toTTI(int designationCode)
{
	QString result;
	int x28Triplets[13] = {0};

	int offset;

	switch (designationCode) {
		case 0:
			offset = 16;
			break;
		case 4:
			offset = 0;
			break;
	}
	result.append(designationCode + 0x40);

	x28Triplets[0] = ((m_secondCharSet & 1) << 17) | (m_secondNOS << 14) | (m_defaultCharSet << 10) | (m_defaultNOS << 7);
	x28Triplets[1] = (m_secondCharSet >> 1) | (m_leftSidePanelDisplayed << 3) | (m_rightSidePanelDisplayed << 4) | (m_sidePanelStatusL25 << 5) | (m_sidePanelColumns << 6);

	for (int c=0; c<16; c++){
		int r = (m_CLUT[offset+c] & 0xF00) >> 8;
		int g = (m_CLUT[offset+c] & 0xF0) >> 4;
		int b = m_CLUT[offset+c] & 0xF;

		int rtr = ((c * 12) + 28) / 18;
		int rsh = ((c * 12) + 28) % 18;
		x28Triplets[rtr] |= (r << rsh);
		if (rsh == 16)
			x28Triplets[rtr+1] |= (r >> 2) & 3;

		int gtr = ((c * 12) + 32) / 18;
		int gsh = ((c * 12) + 32) % 18;
		x28Triplets[gtr] |= (g << gsh);
		if (gsh == 16)
			x28Triplets[gtr+1] |= (g >> 2) & 3;

		int btr = ((c * 12) + 36) / 18;
		int bsh = ((c * 12) + 36) % 18;
		x28Triplets[btr] |= (b << bsh);
		if (bsh == 16)
			x28Triplets[btr+1] |= (b >> 2) & 3;
	}

	x28Triplets[12] |= (m_defaultScreenColour << 4) | (m_defaultRowColour << 9) | (m_blackBackgroundSubst << 14) | (m_colourTableRemap << 15);
	
	for (int i=0; i<13; i++) {
		result.append(0x40 | (x28Triplets[i] & 0x3F));
		result.append(0x40 | ((x28Triplets[i] & 0xFC0) >> 6));
		result.append(0x40 | ((x28Triplets[i] & 0x3F000) >> 12));
	}

	return result;
}

QString TeletextPage::exportURLHash(QString pageHash)
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

//	CLUTChangedResult = CLUTChanged();
//	if (leftSidePanel || rightSidePanel || defaultScreenColour !=0 || defaultRowColour !=0 || blackBackgroundSubst || colourTableRemap !=0 || CLUTChangedResult) {

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

/* void TeletextPage::setSubPageNumber(int newSubPageNumber) { m_subPageNumber = newSubPageNumber; } */
void TeletextPage::setControlBit(int bitNumber, bool active) { m_controlBits[bitNumber] = active; }
void TeletextPage::setCycleValue(int newValue) { m_cycleValue = newValue; };
void TeletextPage::setCycleType(CycleTypeEnum newType) { m_cycleType = newType; }
void TeletextPage::setDefaultCharSet(int newDefaultCharSet) { m_defaultCharSet = newDefaultCharSet; }
void TeletextPage::setDefaultNOS(int newDefaultNOS) { m_defaultNOS = newDefaultNOS; }

void TeletextPage::setSecondCharSet(int newSecondCharSet)
{
	m_secondCharSet = newSecondCharSet;
	if (m_secondCharSet == 0xf)
		m_secondNOS = 0x7;
}

void TeletextPage::setSecondNOS(int newSecondNOS) { m_secondNOS = newSecondNOS; }
void TeletextPage::setCharacter(int row, int column, unsigned char newCharacter) { m_level1Page[row][column] = newCharacter; }
void TeletextPage::setDefaultScreenColour(int newDefaultScreenColour) { m_defaultScreenColour = newDefaultScreenColour; }
void TeletextPage::setDefaultRowColour(int newDefaultRowColour) { m_defaultRowColour = newDefaultRowColour; }
void TeletextPage::setColourTableRemap(int newColourTableRemap) { m_colourTableRemap = newColourTableRemap; }
void TeletextPage::setBlackBackgroundSubst(bool newBlackBackgroundSubst) { m_blackBackgroundSubst = newBlackBackgroundSubst; }

int TeletextPage::CLUT(int index, int renderLevel) const
{
	if (renderLevel == 2)
		return index>=16 ? m_CLUT[index] : defaultCLUT[index];
	else
		return renderLevel==3 ? m_CLUT[index] : defaultCLUT[index];
}

void TeletextPage::setCLUT(int index, int newColour) { m_CLUT[index] = newColour; }
void TeletextPage::setLeftSidePanelDisplayed(bool newLeftSidePanelDisplayed) { m_leftSidePanelDisplayed = newLeftSidePanelDisplayed; }
void TeletextPage::setRightSidePanelDisplayed(bool newRightSidePanelDisplayed) { m_rightSidePanelDisplayed = newRightSidePanelDisplayed; }
void TeletextPage::setSidePanelColumns(int newSidePanelColumns) { m_sidePanelColumns = newSidePanelColumns; }
void TeletextPage::setSidePanelStatusL25(bool newSidePanelStatusL25) { m_sidePanelStatusL25 = newSidePanelStatusL25; }

QString TeletextPage::colourHash(int whichCLUT)
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
