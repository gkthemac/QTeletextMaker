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
#include <QColor>
#include <QList>
#include <QString>
#include <algorithm>

#include "levelonepage.h"

#include "x26triplets.h"

LevelOnePage::LevelOnePage()
{
	m_enhancements.reserve(maxEnhancements());
	clearPage();
}

// So far we only call clearPage() once, within the constructor
void LevelOnePage::clearPage()
{
	for (int r=0; r<25; r++)
		for (int c=0; c<40; c++)
			m_level1Page[r][c] = 0x20;
	for (int b=C4ErasePage; b<=C14NOS; b++)
		setControlBit(b, false);
	for (int i=0; i<8; i++)
		m_composeLink[i] = { (i<4) ? i : 0, false, i>=4, 0x0ff, 0x0000 };
	for (int i=0; i<6; i++)
		m_fastTextLink[i] = { 0x0ff, 0x3f7f };

/*	m_subPageNumber = 0x0000; */
	m_cycleValue = 20;
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
	std::copy(m_defaultCLUT, m_defaultCLUT+32, m_CLUT);
//	If clearPage() is called outside constructor, we need to implement m_enhancements.clear();
}

bool LevelOnePage::isEmpty() const
{
	if (!m_enhancements.isEmpty())
		return false;

	if (!isPaletteDefault(0, 31))
		return false;

	for (int r=0; r<25; r++)
		for (int c=0; c<40; c++)
			if (m_level1Page[r][c] != 0x20)
				return false;

	return true;
}

QByteArray LevelOnePage::packet(int y) const
{
	QByteArray result(40, 0x00);

	if (y <= 24) {
		for (int c=0; c<40; c++)
			result[c] = m_level1Page[y][c];
		return result;
	}

	return PageX26Base::packet(y);
}

QByteArray LevelOnePage::packet(int y, int d) const
{
	QByteArray result(40, 0x00);

	if (y == 26) {
		if (!packetFromEnhancementListNeeded(d))
			return result; // Blank result

		return packetFromEnhancementList(d);
	}

	if (y == 27 && d == 0) {
		for (int i=0; i<6; i++) {
			result[i*6+1] = m_fastTextLink[i].pageNumber & 0x00f;
			result[i*6+2] = (m_fastTextLink[i].pageNumber & 0x0f0) >> 4;
			result[i*6+3] = m_fastTextLink[i].subPageNumber & 0x000f;
			result[i*6+4] = ((m_fastTextLink[i].subPageNumber & 0x0070) >> 4) | ((m_fastTextLink[i].pageNumber & 0x100) >> 5);
			result[i*6+5] = (m_fastTextLink[i].subPageNumber & 0x0f00) >> 8;
			result[i*6+6] = ((m_fastTextLink[i].subPageNumber & 0x3000) >> 12) | ((m_fastTextLink[i].pageNumber & 0x600) >> 7);
		}
		result[37] = 0xf;
		result[38] = result[39] = 0;

		return result;
	}

	if (y == 27 && (d == 4 || d == 5)) {
		for (int i=0; i<(d == 4 ? 6 : 2); i++) {
			int pageLinkNumber = i+(d == 4 ? 0 : 6);

			result[i*6+1] = (m_composeLink[pageLinkNumber].level3p5 << 3) | (m_composeLink[pageLinkNumber].level2p5 << 2) | m_composeLink[pageLinkNumber].function;
			result[i*6+2] = ((m_composeLink[pageLinkNumber].pageNumber & 0x100) >> 3) | 0x10 | (m_composeLink[pageLinkNumber].pageNumber & 0x00f);
			result[i*6+3] = ((m_composeLink[pageLinkNumber].pageNumber & 0x0f0) >> 2) | ((m_composeLink[pageLinkNumber].pageNumber & 0x600) >> 9);

			result[i*6+4] = ((m_composeLink[pageLinkNumber].subPageCodes & 0x000f) << 2);
			result[i*6+5] = ((m_composeLink[pageLinkNumber].subPageCodes & 0x03f0) >> 4);
			result[i*6+6] = ((m_composeLink[pageLinkNumber].subPageCodes & 0xfc00) >> 10);
		}

		return result;
	}

	if (y == 28 && (d == 0 || d == 4)) {
		int CLUToffset = (d == 0) ? 16 : 0;

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

	return PageX26Base::packet(y, d);
}

bool LevelOnePage::setPacket(int y, QByteArray pkt)
{
	if (y <= 24) {
		for (int c=0; c<40; c++)
			m_level1Page[y][c] = pkt.at(c);
		return true;
	}

	qDebug("LevelOnePage unhandled setPacket X/%d", y);
	return PageX26Base::setPacket(y, pkt);
}

bool LevelOnePage::setPacket(int y, int d, QByteArray pkt)
{
	if (y == 26) {
		setEnhancementListFromPacket(d, pkt);
		return true;
	}

	if (y == 27 && d == 0) {
		for (int i=0; i<6; i++) {
			int relativeMagazine = (pkt.at(i*6+4) >> 3) | ((pkt.at(i*6+6) & 0xc) >> 1);
			int pageNumber = (pkt.at(i*6+2) << 4) | pkt.at(i*6+1);
			m_fastTextLink[i].pageNumber = (relativeMagazine << 8) | pageNumber;
			m_fastTextLink[i].subPageNumber = pkt.at(i*6+3) | ((pkt.at(i*6+4) & 0x7) << 4) | (pkt.at(i*6+5) << 8) | ((pkt.at(i*6+6) & 0x3) << 12);
			// TODO remove this warning when we can preserve FastText subpage links
			if (m_fastTextLink[i].subPageNumber != 0x3f7f)
				qDebug("FastText link %d has custom subPageNumber %x - will NOT be saved!", i, m_fastTextLink[i].subPageNumber);
		}
		return true;
	}

	if (y == 27 && (d == 4 || d == 5)) {
		for (int i=0; i<(d == 4 ? 6 : 2); i++) {
			int pageLinkNumber = i+(d == 4 ? 0 : 6);
			int pageFunction = pkt.at(i*6+1) & 0x03;
			if (i >= 4)
				m_composeLink[pageLinkNumber].function = pageFunction;
			else if (i != pageFunction)
				qDebug("X/27/4 link number %d fixed at function %d. Attempted to set to %d.", pageLinkNumber, pageLinkNumber, pageFunction);

			m_composeLink[pageLinkNumber].level2p5 = pkt.at(i*6+1) & 0x04;
			m_composeLink[pageLinkNumber].level3p5 = pkt.at(i*6+1) & 0x08;

			m_composeLink[pageLinkNumber].pageNumber = ((pkt.at(i*6+3) & 0x03) << 9) | ((pkt.at(i*6+2) & 0x20) << 3) | ((pkt.at(i*6+3) & 0x3c) << 2) | (pkt.at(i*6+2) & 0x0f);

			m_composeLink[pageLinkNumber].subPageCodes = (pkt.at(i*6+4) >> 2) | (pkt.at(i*6+5) << 4) | (pkt.at(i*6+6) << 10);
		}
		return true;
	}

	if (y == 28 && (d == 0 || d == 4)) {
		int CLUToffset = (d == 0) ? 16 : 0;

		m_defaultCharSet = ((pkt.at(2) >> 4) & 0x3) | ((pkt.at(3) << 2) & 0xc);
		m_defaultNOS = (pkt.at(2) >> 1) & 0x7;
		m_secondCharSet = ((pkt.at(3) >> 5) & 0x1) | ((pkt.at(4) << 1) & 0xe);
		m_secondNOS = (pkt.at(3) >> 2) & 0x7;

		m_leftSidePanelDisplayed = (pkt.at(4) >> 3) & 1;
		m_rightSidePanelDisplayed = (pkt.at(4) >> 4) & 1;
		m_sidePanelStatusL25 = (pkt.at(4) >> 5) & 1;
		m_sidePanelColumns = pkt.at(5) & 0xf;

		for (int c=0; c<16; c++)
			m_CLUT[CLUToffset+c] = ((pkt.at(c*2+5) << 4) & 0x300) | ((pkt.at(c*2+6) << 10) & 0xc00) | ((pkt.at(c*2+6) << 2) & 0x0f0) | (pkt.at(c*2+7) & 0x00f);

		m_defaultScreenColour = (pkt.at(37) >> 4) | ((pkt.at(38) << 2) & 0x1c);
		m_defaultRowColour = ((pkt.at(38)) >> 3) | ((pkt.at(39) << 3) & 0x18);
		m_blackBackgroundSubst = (pkt.at(39) >> 2) & 1;
		m_colourTableRemap = (pkt.at(39) >> 3) & 7;

		return true;
	}

	qDebug("LevelOnePage unhandled setPacket X/%d/%d", y, d);
	return PageX26Base::setPacket(y, d, pkt);
}

bool LevelOnePage::packetExists(int y) const
{
	if (y <= 24) {
		for (int c=0; c<40; c++)
			if (m_level1Page[y][c] != 0x20)
				return true;
		return false;
	}

	return PageX26Base::packetExists(y);
}

bool LevelOnePage::packetExists(int y, int d) const
{
	if (y == 26)
		return packetFromEnhancementListNeeded(d);

	if (y == 27 && d == 0) {
		for (int i=0; i<6; i++)
			if ((m_fastTextLink[i].pageNumber & 0x0ff) != 0xff)
				return true;

		return false;
	}

	if (y == 27 && (d == 4 || d == 5)) {
		for (int i=0; i<(d == 4 ? 6 : 2); i++) {
			int pageLinkNumber = i+(d == 4 ? 0 : 6);
			if ((m_composeLink[pageLinkNumber].pageNumber & 0x0ff) != 0x0ff)
				return true;
		}
		return false;
	}

	if (y == 28) {
		if (d == 0) {
			if (m_leftSidePanelDisplayed || m_rightSidePanelDisplayed || m_defaultScreenColour !=0 || m_defaultRowColour !=0 || m_blackBackgroundSubst || m_colourTableRemap !=0 || m_defaultCharSet != 0 || m_secondCharSet != 0xf)
				return true;
			return !isPaletteDefault(16, 31);
		}
		if (d == 4)
			return !isPaletteDefault(0, 15);
	}

	return PageX26Base::packetExists(y, d);
}

bool LevelOnePage::controlBit(int b) const
{
	switch (b) {
		case C12NOS:
			return (m_defaultNOS & 1) == 1;
		case C13NOS:
			return (m_defaultNOS & 2) == 2;
		case C14NOS:
			return (m_defaultNOS & 4) == 4;
		default:
			return PageX26Base::controlBit(b);
	}
}

bool LevelOnePage::setControlBit(int b, bool active)
{
	switch (b) {
		case C12NOS:
			m_defaultNOS &= 0x06;
			if (active)
				m_defaultNOS |= 0x01;
			return true;
		case C13NOS:
			m_defaultNOS &= 0x05;
			if (active)
				m_defaultNOS |= 0x02;
			return true;
		case C14NOS:
			m_defaultNOS &= 0x03;
			if (active)
				m_defaultNOS |= 0x04;
			return true;
		default:
			return PageX26Base::setControlBit(b, active);
	}
}

/* void LevelOnePage::setSubPageNumber(int newSubPageNumber) { m_subPageNumber = newSubPageNumber; } */
void LevelOnePage::setCycleValue(int newValue) { m_cycleValue = newValue; };
void LevelOnePage::setCycleType(CycleTypeEnum newType) { m_cycleType = newType; }
void LevelOnePage::setDefaultCharSet(int newDefaultCharSet) { m_defaultCharSet = newDefaultCharSet; }

void LevelOnePage::setDefaultNOS(int defaultNOS)
{
	m_defaultNOS = defaultNOS;
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
		return index>=16 ? m_CLUT[index] : m_defaultCLUT[index];
	else
		return renderLevel==3 ? m_CLUT[index] : m_defaultCLUT[index];
}

void LevelOnePage::setCLUT(int index, int newColour)
{
	if (index == 8)
		return;
	m_CLUT[index] = newColour;
}

QColor LevelOnePage::CLUTtoQColor(int index, int renderLevel) const
{
	int colour12Bit = CLUT(index, renderLevel);

	if (index == 8)
		return QColor(Qt::transparent);

	return QColor(((colour12Bit & 0xf00) >> 8) * 17, ((colour12Bit & 0x0f0) >> 4) * 17, (colour12Bit & 0x00f) * 17);
}

bool LevelOnePage::isPaletteDefault(int colour) const
{
	return m_CLUT[colour] == m_defaultCLUT[colour];
}

bool LevelOnePage::isPaletteDefault(int fromColour, int toColour) const
{
	for (int i=fromColour; i<=toColour; i++)
		if (m_CLUT[i] != m_defaultCLUT[i])
			return false;

	return true;
}

int LevelOnePage::levelRequired() const
{
	// X/28/4 present i.e. CLUTs 0 or 1 redefined - Level 3.5
	if (!isPaletteDefault(0, 15))
		return 3;

	// TODO Check for X/28/1 for DCLUT for mode 1-3 DRCS characters - return 3

	// Assume Level 2.5 if any X/28 page enhancements are present, otherwise assume Level 1
	int levelSeen = (!isPaletteDefault(16, 31) || m_leftSidePanelDisplayed || m_rightSidePanelDisplayed || m_defaultScreenColour !=0 || m_defaultRowColour !=0 || m_blackBackgroundSubst || m_colourTableRemap !=0 || m_defaultCharSet != 0 || m_secondCharSet != 0xf) ? 2 : 0;

	// If there's no X/26 triplets, exit here as Level 1 or 2.5
	if (m_enhancements.isEmpty())
		return levelSeen;

	for (int i=0; i<m_enhancements.size(); i++) {
		// Font style - Level 3.5 only triplet
		if (m_enhancements.at(i).modeExt() == 0x2e) // Font style
			return 3;

		if (levelSeen == 0)
			// Check for Level 1.5 triplets
			switch (m_enhancements.at(i).modeExt()) {
				case 0x04: // Set Active Position
				case 0x07: // Address Row 0
				case 0x1f: // Termination
				case 0x22: // G3 character @ Level 1.5
				case 0x2f: // G2 character
					levelSeen = 1;
					break;
				default:
					if (m_enhancements.at(i).modeExt() >= 0x30 && m_enhancements.at(i).modeExt() <= 0x3f)
						// G0 character with diacritical
						levelSeen = 1;
			}

		if (levelSeen < 2)
			switch (m_enhancements.at(i).modeExt()) {
				// Check for Level 2.5 triplets
				case 0x00: // Full screen colour
				case 0x01: // Full row colour
				case 0x10: // Origin Modifier
				case 0x11: // Invoke Active Object
				case 0x12: // Invoke Adaptive Object
				case 0x13: // Invoke Passive Object
				case 0x15: // Define Active Object
				case 0x16: // Define Adaptive Object
				case 0x17: // Define Passive Object
				case 0x18: // DRCS Mode
				case 0x20: // Foreground colour
				case 0x21: // G1 character
				case 0x23: // Background colour
				case 0x27: // Flash functions
				case 0x28: // G0 and G2 charset designation
				case 0x29: // G0 character @ Level 2.5
				case 0x2b: // G3 character @ Level 2.5
				case 0x2c: // Display attributes
				case 0x2d: // DRCS character
					levelSeen = 2;
					break;
			}

		if (levelSeen == 2)
			switch (m_enhancements.at(i).modeExt()) {
				// Check for triplets with "required at Level 3.5 only" parameters
				case 0x15: // Define Active Object
				case 0x16: // Define Adaptive Object
				case 0x17: // Define Passive Object
					if ((m_enhancements.at(i).address() & 0x18) == 0x10)
						return 3;
					break;
				case 0x18: // DRCS Mode
					if ((m_enhancements.at(i).data() & 0x30) == 0x20)
						return 3;
					break;
			}
	}

	return levelSeen;
}

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
