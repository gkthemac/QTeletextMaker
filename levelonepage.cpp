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

// At the moment this awkwardly combines parsing a TTI file and making sense of the packets within it.
// This will be gradually converted to just parsing a TTI file and passing the raw packets to the Page class.
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
		// Won't be a designation code for line numbers below 25!
		int designationCode = inLine.at(0) & 0x3f;

		if (lineNumber <= 24) {
			for (int i=0, j=0; j<=39; i++, j++) {
				if (i == inLine.size())
					break;
				int myChar = inLine.at(i);
				if (myChar & 0x80)
					myChar &= 0x7f;
				else if (myChar == 0x10)
					myChar = 0x0d;
				else if (myChar == 0x1b) {
					i++;
					myChar = inLine.at(i)-0x40;
				}
				setCharacter(lineNumber, j, myChar);
			}
		} else if (lineNumber == 26) {
			// TODO deal with gaps or out of order X26 designation codes in a more graceful way
			// At the moment gaps are dealt with by simply inserting "dummy" reserved 11110 Row
			// Triplets in case Local Objects are referenced within.
			// Out of order X26 designation codes aren't handled at all!
			if (designationCode*13 != localEnhance.size()) {
				qDebug("Gap or out of order X26 designation code");
				X26Triplet paddingX26Triplet;

				paddingX26Triplet.setAddress(41);
				paddingX26Triplet.setMode(0x1e);
				paddingX26Triplet.setData(0);
				while (localEnhance.size() < designationCode*13)
					localEnhance.append(paddingX26Triplet);
			}

			// Round loop counter to nearest multiple of 3 so an incomplete triplet doesn't crash us
			int inLineLength = (((inLine.size() <= 39) ? inLine.size() : 39) / 3) * 3;

			for (int i=1; i<inLineLength; i+=3) {
				X26Triplet newX26Triplet;

				newX26Triplet.setAddress(inLine.at(i) & 0x3f);
				newX26Triplet.setMode(inLine.at(i+1) & 0x1f);
				newX26Triplet.setData(((inLine.at(i+2) & 0x3f) << 1) | ((inLine.at(i+1) & 0x20) >> 5));
				localEnhance.append(newX26Triplet);
				// Break out of loop if termination marker (without a "...follow") is encountered
				if (newX26Triplet.mode() == 0x1f && newX26Triplet.address() == 0x3f && newX26Triplet.data() & 0x01)
					break;
			}
		} else if (lineNumber == 28 && (designationCode == 0 || designationCode == 4)) {
			int offset = (designationCode == 0) ? 16 : 0;

			int x28Triplets[13];

			for (int i=1, j=0; i<39; i+=3, j++)
				x28Triplets[j] = ((inLine.at(i+2) & 0x3f) << 12) | ((inLine.at(i+1) & 0x3f) << 6) | (inLine.at(i) & 0x3f);

			m_defaultCharSet = (x28Triplets[0] >> 10) & 0xF;
			m_defaultNOS = (x28Triplets[0] >> 7) & 0x7;
			m_secondCharSet = ((x28Triplets[1] << 1) & 0xE) | ((x28Triplets[0] >> 17) & 0x1);
			m_secondNOS = (x28Triplets[0] >> 14) & 0x7;

			m_leftSidePanelDisplayed = (x28Triplets[1] >> 3) & 1;
			m_rightSidePanelDisplayed = (x28Triplets[1] >> 4) & 1;
			m_sidePanelStatusL25 = (x28Triplets[1] >> 5) & 1;
			m_sidePanelColumns = (x28Triplets[1] >> 6) & 0xF;

			for (int c=0; c<16; c++) {
				int rtr = ((c * 12) + 28) / 18;
				int rsh = ((c * 12) + 28) % 18;
				int r = (x28Triplets[rtr] >> rsh) & 0xF;
				if (rsh == 16)
					r |= (x28Triplets[rtr+1] & 3) << 2;

				int gtr = ((c * 12) + 32) / 18;
				int gsh = ((c * 12) + 32) % 18;
				int g = (x28Triplets[gtr] >> gsh) & 0xF;
				if (gsh == 16)
					g |= (x28Triplets[gtr+1] & 3) << 2;

				int btr = ((c * 12) + 36) / 18;
				int bsh = ((c * 12) + 36) % 18;
				int b = (x28Triplets[btr] >> bsh) & 0xF;
				if (bsh == 16)
					b |= (x28Triplets[btr+1] & 3) << 2;

				m_CLUT[offset+c] = (r << 8) | (g << 4) | b;
			}
			m_defaultScreenColour = (x28Triplets[12] >> 4) & 0x1f;
			m_defaultRowColour = (x28Triplets[12] >> 9) & 0x1f;
			m_blackBackgroundSubst = ((x28Triplets[12] >> 14) & 1);
			m_colourTableRemap = (x28Triplets[12] >> 15) & 7;
		} else
			qDebug("Packet X/%d/%d unhandled (yet?)", lineNumber, designationCode);
	} //TODO panic if invalid line number is encountered
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
