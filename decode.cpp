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

#include "decode.h"

#include <QList>
#include <QMultiMap>

TeletextPageDecode::Invocation::Invocation()
{
	m_tripletList = nullptr;
	m_startTripletNumber = 0;
	m_endTripletNumber = -1;
	m_originRow = 0;
	m_originColumn = 0;
	m_fullScreenCLUT = -1;
}

void TeletextPageDecode::Invocation::clear()
{
	m_characterMap.clear();
	m_attributeMap.clear();
	m_rightMostColumn.clear();
	m_fullScreenCLUT = -1;
	m_fullRowCLUTMap.clear();
}

void TeletextPageDecode::Invocation::setTripletList(X26TripletList *tripletList)
{
	m_tripletList = tripletList;
}

void TeletextPageDecode::Invocation::setStartTripletNumber(int n)
{
	m_startTripletNumber = n;
}

void TeletextPageDecode::Invocation::setEndTripletNumber(int n)
{
	m_endTripletNumber = n;
}

void TeletextPageDecode::Invocation::setOrigin(int row, int column)
{
	m_originRow = row;
	m_originColumn = column;
}

void TeletextPageDecode::Invocation::buildMap(int level)
{
	int endTripletNumber;

	if (m_endTripletNumber == -1)
		endTripletNumber = m_tripletList->size()-1;
	else
		endTripletNumber = m_endTripletNumber;

	clear();

	for (int i=m_startTripletNumber; i<=endTripletNumber; i++) {
		const X26Triplet triplet = m_tripletList->at(i);

		if (triplet.error() != 0)
			continue;

		int targetRow, targetColumn;

		if (level == 1) {
			targetRow = m_originRow + triplet.activePositionRow1p5();
			targetColumn = m_originColumn + triplet.activePositionColumn1p5();
		} else {
			targetRow = m_originRow + triplet.activePositionRow();
			targetColumn = m_originColumn + triplet.activePositionColumn();
		}

		if (triplet.activePositionRow() == -1)
			targetRow++;
		if (triplet.activePositionColumn() == -1)
			targetColumn++;

		if (targetRow > 24 || targetColumn > 71)
			continue;

		switch (triplet.modeExt()) {
			case 0x21: // G1 character
			case 0x22: // G3 character at Level 1.5
			case 0x29: // G0 character
			case 0x2b: // G3 character at Level 2.5
			case 0x2f: // G2 character
			case 0x30 ... 0x3f: // G0 character with diacritical
				m_characterMap.insert(qMakePair(targetRow, targetColumn), triplet);
				// Store rightmost column in this row for Adaptive Object attribute tracking
				// QMap stores one value per key, QMap::insert will replace the value if the key already exists
				m_rightMostColumn.insert(targetRow, targetColumn);
				break;
			case 0x20: // Foreground colour
			case 0x23: // Background colour
			case 0x27: // Additional flash functions
			case 0x28: // Modified G0 and G2 character set designation
			case 0x2c: // Display attributes
			case 0x2e: // Font style
				m_attributeMap.insert(qMakePair(targetRow, targetColumn), triplet);
				m_rightMostColumn.insert(targetRow, targetColumn);
				break;
			case 0x00: // Full screen colour
				if ((triplet.data() & 0x60) != 0x00)
					break;
				m_fullScreenCLUT = triplet.data();
				// Full Screen Colour triplet overrides both the X/28 Full Screen Colour AND Full Row Colour.
				// For the latter, place a Full Row Colour "down to bottom" at the Active Position.
				m_fullRowCLUTMap.insert(targetRow, X26Triplet(triplet.address(), triplet.mode(), triplet.data() | 0x60));
				break;
			case 0x01: // Full row colour
				m_fullRowCLUTMap.insert(targetRow, triplet);
				break;
			case 0x07: // Address row 0
				if (targetRow == 0)
					m_fullRowCLUTMap.insert(targetRow, triplet);
				break;
		}
	}
}


int TeletextPageDecode::s_instances = 0;

TeletextPageDecode::textPainter TeletextPageDecode::s_blankPainter;

TeletextPageDecode::TeletextPageDecode()
{
	if (s_instances == 0) {
		for (int c=0; c<72; c++) {
			s_blankPainter.bottomHalfCell[c].character.code = 0x00;
			s_blankPainter.setProportionalRows[c] = 0;
			s_blankPainter.clearProportionalRows[c] = 0;
			s_blankPainter.setBoldRows[c] = 0;
			s_blankPainter.clearBoldRows[c] = 0;
			s_blankPainter.setItalicRows[c] = 0;
			s_blankPainter.clearItalicRows[c] = 0;
		}
		s_blankPainter.rightHalfCell.character.code = 0x00;
	}
	s_instances++;

	m_level = 0;

	for (int r=0; r<25; r++) {
		m_rowHeight[r] = NormalHeight;
		for (int c=0; c<72; c++) {
			if (c < 40) {
				m_cellLevel1Mosaic[r][c] = false;
				m_cellLevel1CharSet[r][c] = 0;
			}
			m_refresh[r][c] = true;
		}
	}

	m_finalFullScreenColour = 0;
	m_finalFullScreenQColor.setRgb(0, 0, 0);
	for (int r=0; r<25; r++) {
		m_fullRowColour[r] = 0;
		m_fullRowQColor[r].setRgb(0, 0, 0);
	}
	m_leftSidePanelColumns = m_rightSidePanelColumns = 0;
}

TeletextPageDecode::~TeletextPageDecode()
{
	s_instances--;
}

void TeletextPageDecode::setRefresh(int r, int c, bool refresh)
{
	m_refresh[r][c] = refresh;
}

void TeletextPageDecode::setTeletextPage(LevelOnePage *newCurrentPage)
{
	m_levelOnePage = newCurrentPage;
	m_localEnhancements.setTripletList(m_levelOnePage->enhancements());
	updateSidePanels();
}

void TeletextPageDecode::setLevel(int level)
{
	if (level == m_level)
		return;

	m_level = level;

	for (int r=0; r<25; r++)
		for (int c=0; c<72; c++)
			m_refresh[r][c] = true;

	updateSidePanels();
	decodePage();
}

void TeletextPageDecode::updateSidePanels()
{
	int oldLeftSidePanelColumns = m_leftSidePanelColumns;
	int oldRightSidePanelColumns = m_rightSidePanelColumns;

	if (m_level >= (3-m_levelOnePage->sidePanelStatusL25()) && m_levelOnePage->leftSidePanelDisplayed())
		m_leftSidePanelColumns = (m_levelOnePage->sidePanelColumns() == 0) ? 16 : m_levelOnePage->sidePanelColumns();
	else
		m_leftSidePanelColumns = 0;

	if (m_level >= (3-m_levelOnePage->sidePanelStatusL25()) && m_levelOnePage->rightSidePanelDisplayed())
		m_rightSidePanelColumns = 16-m_levelOnePage->sidePanelColumns();
	else
		m_rightSidePanelColumns = 0;

	if (m_leftSidePanelColumns != oldLeftSidePanelColumns || m_rightSidePanelColumns != oldRightSidePanelColumns) {
		emit sidePanelsChanged();
		decodePage();
	}
}

void TeletextPageDecode::buildInvocationList(Invocation &invocation, int objectType)
{
	if (invocation.tripletList()->isEmpty()) {
		invocation.clear();
		return;
	}

	int i;

	for (i=invocation.startTripletNumber(); i<invocation.tripletList()->size(); i++) {
		const X26Triplet triplet = invocation.tripletList()->at(i);

		if (triplet.modeExt() == 0x1f && triplet.address() == 63)
			// Termination marker
			break;
		if (triplet.modeExt() >= 0x15 && triplet.modeExt() <= 0x17)
			// Object Definition, also used as terminator
			break;
		if (m_level >= 2 && triplet.modeExt() >= 0x11 && triplet.modeExt() <= 0x13 && triplet.error() == 0) {
			// Object Invocation
			//TODO POP and GPOP objects
			if (triplet.objectSource() != X26Triplet::LocalObject) {
				qDebug("POP or GPOP");
				continue;
			}

			// Check if (sub)Object type can be invoked by Object type we're within
			if (triplet.modeExt() - 0x11 <= objectType)
				continue;

			// See if Object Definition is required at selected level
			if (m_level == 2 && (invocation.tripletList()->at(triplet.objectLocalIndex()).address() & 0x08) == 0x00)
				continue;
			if (m_level == 3 && (invocation.tripletList()->at(triplet.objectLocalIndex()).address() & 0x10) == 0x00)
				continue;

			// Work out the absolute position where the Object is invoked
			int originRow = invocation.originRow() + triplet.activePositionRow();
			int originColumn = invocation.originColumn() + triplet.activePositionColumn();
			// -1, -1 happens if Object is invoked before the Active Position is deployed
			if (triplet.activePositionRow() == -1)
				originRow++;
			if (triplet.activePositionColumn() == -1)
				originColumn++;
			// Use Origin Modifier in previous triplet if there's one there
			if (i > 0 && invocation.tripletList()->at(i-1).modeExt() == 0x10) {
				originRow += invocation.tripletList()->at(i-1).address()-40;
				originColumn += invocation.tripletList()->at(i-1).data();
			}

			// Add the Invocation to the list, and recurse
			Invocation newInvocation;
			const int newObjectType = triplet.modeExt() - 0x11;

			newInvocation.setTripletList(invocation.tripletList());
			newInvocation.setStartTripletNumber(triplet.objectLocalIndex()+1);
			newInvocation.setOrigin(originRow, originColumn);
			m_invocations[newObjectType].append(newInvocation);
			buildInvocationList(m_invocations[newObjectType].last(), newObjectType);
		}
	}

	invocation.setEndTripletNumber(i-1);
	invocation.buildMap(m_level);
}

TeletextPageDecode::textCharacter TeletextPageDecode::characterFromTriplets(const QList<X26Triplet> triplets)
{
	textCharacter result;
	result.code = 0x00;

	// QMultiMap::values returns a QList with the most recently inserted value sorted first,
	// so do the loop backwards to iterate from least to most recent value
	for (int a=triplets.size()-1; a>=0; a--) {
		const X26Triplet triplet = triplets.at(a);

		if (triplet.data() < 0x20)
			continue;

		const unsigned char charCode = triplet.data();

		// Deal with Level 1.5 valid characters first
		switch (triplet.modeExt()) {
			case 0x22: // G3 character at Level 1.5
				result = { charCode, 26, 0 };
				break;
			case 0x2f: // G2 character
				result = { charCode, 2, 0 };
				break;
			case 0x30 ... 0x3f: // G0 character with diacritical
				result = { charCode, 0, triplet.mode() & 0xf };
				break;
		}

		if (m_level == 1)
			continue;

		// Now deal with Level 2.5 characters
		switch (triplet.modeExt()) {
			case 0x21: // G1 character
				result.code = charCode;
				if (triplet.data() & 0x20)
					result.set = 24;
				else
					result.set = 0;
				result.diacritical = 0;
				break;
			case 0x29: // G0 character
				result = { charCode, 0, 0 };
				break;
			case 0x2b: // G3 character at Level 2.5
				result = { charCode, 26, 0 };
				break;
		}
	}

	return result;
}

void TeletextPageDecode::decodePage()
{
	m_invocations[0].clear();
	m_invocations[1].clear();
	m_invocations[2].clear();

	buildInvocationList(m_localEnhancements, -1);

	// Append Local Enhancement Data to end of Active Object QList
	m_invocations[0].append(m_localEnhancements);

	m_level1ActivePainter = s_blankPainter;

	m_adapPassPainter[0].clear();
	m_adapPassPainter[1].clear();

	for (int t=1; t<3; t++)
		for (int i=0; i<m_invocations[t].size(); i++)
			m_adapPassPainter[t-1].append(s_blankPainter);

	if (m_level >= 2) {
		// Pick up default full screen/row colours from X/28
		setFullScreenColour(m_levelOnePage->defaultScreenColour());
		int downwardsRowCLUT = m_levelOnePage->defaultRowColour();

		// Check for Full Screen Colour X/26 triplets in Local Enhancement Data and Active Objects
		for (int i=0; i<m_invocations[0].size(); i++)
			if (m_invocations[0].at(i).fullScreenColour() != -1)
				setFullScreenColour(m_invocations[0].at(i).fullScreenColour());

		// Now do the Full Row Colours
		for (int r=0; r<25; r++) {
			int thisFullRowColour = downwardsRowCLUT;

			for (int i=0; i<m_invocations[0].size(); i++) {
				const QList<X26Triplet> fullRowColoursHere = m_invocations[0].at(i).fullRowColoursMappedAt(r);

				// QMultiMap::values returns QList with most recent value first...
				for (int a=fullRowColoursHere.size()-1; a>=0; a--) {
					thisFullRowColour = fullRowColoursHere.at(a).data() & 0x1f;
					if ((fullRowColoursHere.at(a).data() & 0x60) == 0x60)
						downwardsRowCLUT = thisFullRowColour;
				}
			}

			setFullRowColour(r, thisFullRowColour);
		}
	} else {
		setFullScreenColour(0);
		for (int r=0; r<25; r++)
			setFullRowColour(r, 0);
	}

	m_defaultG0andG2 = (m_levelOnePage->defaultCharSet() << 3) | m_levelOnePage->defaultNOS();
	m_secondG0andG2 = -1;

	m_level1DefaultCharSet = m_level1CharacterMap.value(m_defaultG0andG2, 0);
	if (m_levelOnePage->secondCharSet() != 0xf)
		m_level1SecondCharSet = m_level1CharacterMap.value((m_levelOnePage->secondCharSet() << 3) | m_levelOnePage->secondNOS(), 0);
	else
		m_level1SecondCharSet = m_level1DefaultCharSet;

	// Work out rows containing top and bottom halves of Level 1 double height characters
	for (int r=1; r<24; r++) {
		bool doubleHeightAttributeFound = false;

		for (int c=0; c<40; c++)
			if (m_levelOnePage->character(r, c) == 0x0d || m_levelOnePage->character(r, c) == 0x0f) {
				doubleHeightAttributeFound = true;
				break;
			}

		if (doubleHeightAttributeFound && r < 23) {
			m_rowHeight[r] = TopHalf;
			r++;
			m_rowHeight[r] = BottomHalf;
		} else
			m_rowHeight[r] = NormalHeight;
	}

	for (int r=0; r<25; r++)
		decodeRow(r);
}

void TeletextPageDecode::decodeRow(int r)
{
	int level1ForegroundCLUT = 7;
	bool level1Mosaics = false;
	bool level1SeparatedMosaics = false;
	bool level1HoldMosaics = false;
	unsigned char level1HoldMosaicCharacter = 0x20;
	bool level1HoldMosaicSeparated = false;
	int level1CharSet = 0;
	bool level1EscapeSwitch = false;

	textPainter *painter;

	// Used for tracking which Adaptive Invocation is applying which attribute type(s)
	// A.7.1 and A.7.2 of the spec says Adaptive Objects can't overlap but can be interleaved
	// if they don't have attributes, so we only need to track one
	int adapInvokeAttrs = -1;
	bool adapForeground = false;
	bool adapBackground = false;
	bool adapFlash = false;
	bool adapDisplayAttrs = false;
	bool adapStyle = false;

	for (int c=0; c<72; c++) {
		textCell previousCellContents = m_cell[r][c];

		// Start of row default conditions, also when crossing into and across side panels
		if (c == 0 || c == 40 || c == 56) {
			level1CharSet = m_level1DefaultCharSet;

			m_level1ActivePainter.result.g0Set = m_g0CharacterMap.value(m_defaultG0andG2, 0);
			m_level1ActivePainter.result.g2Set = m_g2CharacterMap.value(m_defaultG0andG2, 7);

			m_level1ActivePainter.attribute.flash.mode = 0;
			m_level1ActivePainter.attribute.flash.ratePhase = 0;
			m_level1ActivePainter.attribute.display.doubleHeight = false;
			m_level1ActivePainter.attribute.display.doubleWidth = false;
			m_level1ActivePainter.attribute.display.boxingWindow = false;
			m_level1ActivePainter.attribute.display.conceal = false;
			m_level1ActivePainter.attribute.display.invert = false;
			m_level1ActivePainter.attribute.display.underlineSeparated = false;
			m_level1ActivePainter.attribute.style.proportional = false;
			m_level1ActivePainter.attribute.style.bold = false;
			m_level1ActivePainter.attribute.style.italic = false;

			if (m_level >= 2) {
				m_level1ActivePainter.attribute.foregroundCLUT = 7 | m_foregroundRemap[m_levelOnePage->colourTableRemap()];
				if (m_levelOnePage->blackBackgroundSubst() || c >= 40)
					m_level1ActivePainter.attribute.backgroundCLUT = m_fullRowColour[r];
				else
					m_level1ActivePainter.attribute.backgroundCLUT = m_backgroundRemap[m_levelOnePage->colourTableRemap()];
			} else {
				m_level1ActivePainter.attribute.foregroundCLUT = 7;
				m_level1ActivePainter.attribute.backgroundCLUT = 0;
			}
		}

		// Level 1 set-at and "set-between" spacing attributes
		if (c < 40 && m_rowHeight[r] != BottomHalf)
			switch (m_levelOnePage->character(r, c)) {
				case 0x09: // Steady
					m_level1ActivePainter.attribute.flash.mode = 0;
					m_level1ActivePainter.attribute.flash.ratePhase = 0;
					break;
				case 0x0a: // End box
					// "Set-between" - requires two consecutive "end box" codes
					if (c > 0 && m_levelOnePage->character(r, c-1) == 0x0a)
						m_level1ActivePainter.attribute.display.boxingWindow = false;
					break;
				case 0x0b: // Start box
					// "Set-between" - requires two consecutive "start box" codes
					if (c > 0 && m_levelOnePage->character(r, c-1) == 0x0b)
						m_level1ActivePainter.attribute.display.boxingWindow = true;
					break;
				case 0x0c: // Normal size
					if (m_level1ActivePainter.attribute.display.doubleHeight || m_level1ActivePainter.attribute.display.doubleWidth) {
						// Change of size resets hold mosaic character
						level1HoldMosaicCharacter = 0x20;
						level1HoldMosaicSeparated = false;
					}
					m_level1ActivePainter.attribute.display.doubleHeight = false;
					m_level1ActivePainter.attribute.display.doubleWidth = false;
					break;
				case 0x18: // Conceal
					m_level1ActivePainter.attribute.display.conceal = true;
					break;
				case 0x19: // Contiguous mosaics
					// This spacing attribute cannot cancel an X/26 underlined/separated attribute
					if (!m_level1ActivePainter.attribute.display.underlineSeparated)
						level1SeparatedMosaics = false;
					break;
				case 0x1a: // Separated mosaics
					level1SeparatedMosaics = true;
					break;
				case 0x1c: // Black background
					if (m_level >= 2) {
						if (m_levelOnePage->blackBackgroundSubst())
							m_level1ActivePainter.attribute.backgroundCLUT = m_fullRowColour[r];
						else
							m_level1ActivePainter.attribute.backgroundCLUT = m_backgroundRemap[m_levelOnePage->colourTableRemap()];
					} else
						m_level1ActivePainter.attribute.backgroundCLUT = 0;
					break;
				case 0x1d: // New background
					if (m_level >= 2)
						m_level1ActivePainter.attribute.backgroundCLUT = level1ForegroundCLUT | m_backgroundRemap[m_levelOnePage->colourTableRemap()];
					else
						m_level1ActivePainter.attribute.backgroundCLUT = level1ForegroundCLUT;
					break;
				case 0x1e: // Hold mosaics
					level1HoldMosaics = true;
					break;
			}

		if (m_level < 2)
			m_level1ActivePainter.result.attribute = m_level1ActivePainter.attribute;
		else{
			// Deal with incremental and decremental flash
			rotateFlashMovement(m_level1ActivePainter.attribute.flash);

			for (int t=0; t<2; t++)
				for (int i=0; i<m_adapPassPainter[t].size(); i++)
					rotateFlashMovement(m_adapPassPainter[t][i].attribute.flash);

			// X/26 attributes
			for (int t=0; t<3; t++)
				for (int i=0; i<m_invocations[t].size(); i++) {
					QList<X26Triplet> attributesHere = m_invocations[t].at(i).attributesMappedAt(r, c);

					painter = (t == 0) ? &m_level1ActivePainter : &m_adapPassPainter[t-1][i];

					if (m_level == 3) {
						// Reset font style "row spread" at start of row and side panels
						if (c == 0 || c == 40 || c == 56)
							painter->styleSpreadRows = 0;

						// Apply any font style attributes from previous rows
						// For m_level1ActivePainter, ensure we deal with font style row counters only once
						if (t >= 1 || i == 0) {
							if (painter->clearProportionalRows[c] != 0) {
								painter->attribute.style.proportional = false;
								painter->clearProportionalRows[c]--;
							}
							if (painter->setProportionalRows[c] != 0) {
								painter->attribute.style.proportional = true;
								painter->setProportionalRows[c]--;
							}
							if (painter->clearBoldRows[c] != 0) {
								painter->attribute.style.bold = false;
								painter->clearBoldRows[c]--;
							}
							if (painter->setBoldRows[c] != 0) {
								painter->attribute.style.bold = true;
								painter->setBoldRows[c]--;
							}
							if (painter->clearItalicRows[c] != 0) {
								painter->attribute.style.italic = false;
								painter->clearItalicRows[c]--;
							}
							if (painter->setItalicRows[c] != 0) {
								painter->attribute.style.italic = true;
								painter->setItalicRows[c]--;
							}
						}
					}

					// Adaptive Invocation painter: pick up the attributes we're NOT adapting from
					// m_level1ActivePainter, which by now has taken into account all the attributes
					// from the Level 1 page, Active Objects and the Local Enhancement Data
					if (t == 1) {
						if (!adapForeground)
							painter->attribute.foregroundCLUT = m_level1ActivePainter.attribute.foregroundCLUT;
						if (!adapBackground)
							painter->attribute.backgroundCLUT = m_level1ActivePainter.attribute.backgroundCLUT;
						if (!adapFlash)
							painter->attribute.flash = m_level1ActivePainter.attribute.flash;
						if (!adapDisplayAttrs)
							painter->attribute.display = m_level1ActivePainter.attribute.display;
						if (!adapStyle)
							painter->attribute.style = m_level1ActivePainter.attribute.style;
					}

					// QMultiMap::values returns QList with most recent value first...
					for (int a=attributesHere.size()-1; a>=0; a--) {
						const X26Triplet triplet = attributesHere.at(a);

						bool applyAdapt = false;

						// Adaptive Invocation that is applying an attribute
						// If we're not tracking an Adaptive Invocation yet, start tracking this one
						// Otherwise check if this Invocation is the the same one as we are tracking
						if (t == 1) {
							if (adapInvokeAttrs == -1) {
								adapInvokeAttrs = i;
								applyAdapt = true;
							} else if (adapInvokeAttrs == i)
								applyAdapt = true;
//							else
//								qDebug("Multiple adaptive object attributes");
						}

						switch (triplet.modeExt()) {
							case 0x20: // Foreground colour
								if (applyAdapt)
									adapForeground = true;
								painter->attribute.foregroundCLUT = triplet.data();
								break;
							case 0x23: // Background colour
								if (applyAdapt)
									adapBackground = true;
								painter->attribute.backgroundCLUT = triplet.data();
								break;
							case 0x27: // Additional flash functions
								if (applyAdapt)
									adapFlash = true;
								painter->attribute.flash.mode = triplet.data() & 0x03;
								painter->attribute.flash.ratePhase = triplet.data() >> 2;
								// For incremental/decremental 2Hz flash, start at phase 1
								if (painter->attribute.flash.mode != 0 && painter->attribute.flash.ratePhase & 0x4)
									painter->attribute.flash.phase2HzShown = 1;
								else
									painter->attribute.flash.phase2HzShown = painter->attribute.flash.ratePhase;
								break;
							case 0x28: // Modified G0 and G2 character set designation
								if (m_level == 3 || triplet.data() == m_defaultG0andG2 || triplet.data() == m_secondG0andG2) {
									painter->result.g0Set = m_g0CharacterMap.value(triplet.data(), 0);
									painter->result.g2Set = m_g2CharacterMap.value(triplet.data(), 7);
								} else if (m_secondG0andG2 == -1) {
									m_secondG0andG2 = triplet.data();
									painter->result.g0Set = m_g0CharacterMap.value(triplet.data(), 0);
									painter->result.g2Set = m_g2CharacterMap.value(triplet.data(), 7);
								}
								break;
							case 0x2c: // Display attributes
								if (applyAdapt)
									adapDisplayAttrs = true;
								painter->attribute.display.doubleHeight = triplet.data() & 0x01;
								painter->attribute.display.boxingWindow = triplet.data() & 0x02;
								painter->attribute.display.conceal = triplet.data() & 0x04;
								painter->attribute.display.invert = triplet.data() & 0x10;
								painter->attribute.display.underlineSeparated = triplet.data() & 0x20;
								painter->attribute.display.doubleWidth = triplet.data() & 0x40;
								// Cancelling separated mosaics with X/26 attribute
								// also cancels the Level 1 separated mosaic attribute
								if (t == 0 && !painter->attribute.display.underlineSeparated)
									level1SeparatedMosaics = false;
								break;
							case 0x2e: // Font style
								if (m_level != 3)
									break;
								if (applyAdapt)
									adapStyle = true;
								painter->attribute.style.proportional = triplet.data() & 0x01;
								painter->attribute.style.bold = triplet.data() & 0x02;
								painter->attribute.style.italic = triplet.data() & 0x04;
								painter->styleSpreadRows = triplet.data() >> 4;
								break;
						}
					}

					painter->result.attribute = painter->attribute;

					// Font style attribute that spreads across more than one row
					if (m_level == 3 && painter->styleSpreadRows != 0) {
						if (painter->attribute.style.proportional)
							painter->setProportionalRows[c] = painter->styleSpreadRows;
						else
							painter->clearProportionalRows[c] = painter->styleSpreadRows;
						if (painter->attribute.style.bold)
							painter->setBoldRows[c] = painter->styleSpreadRows;
						else
							painter->clearBoldRows[c] = painter->styleSpreadRows;
						if (painter->attribute.style.italic)
							painter->setItalicRows[c] = painter->styleSpreadRows;
						else
							painter->clearItalicRows[c] = painter->styleSpreadRows;
					}
				}
			}

		// Level 1 character
		if (c < 40 && m_rowHeight[r] != BottomHalf) {
			m_level1ActivePainter.result.character.diacritical = 0;
			if (m_levelOnePage->character(r, c) >= 0x20) {
				m_level1ActivePainter.result.character.code = m_levelOnePage->character(r, c);
				// Set to true on mosaic character - not on blast through alphanumerics
				m_cellLevel1Mosaic[r][c] = level1Mosaics && (m_levelOnePage->character(r, c) & 0x20);
				if (m_cellLevel1Mosaic[r][c]) {
					m_level1ActivePainter.result.character.set = 24 + (level1SeparatedMosaics || m_level1ActivePainter.attribute.display.underlineSeparated);
					level1HoldMosaicCharacter = m_levelOnePage->character(r, c);
					level1HoldMosaicSeparated = level1SeparatedMosaics;
				} else
					m_level1ActivePainter.result.character.set = level1CharSet;
			} else if (level1HoldMosaics) {
				m_level1ActivePainter.result.character = { level1HoldMosaicCharacter, 24 + level1HoldMosaicSeparated, 0 };
			} else
				m_level1ActivePainter.result.character = { 0x20, 0, 0 };
		} else
			// In side panel or on bottom half of Level 1 double height row, no Level 1 characters here
			m_level1ActivePainter.result.character = { 0x20, 0, 0 };

		if (c < 40)
			m_cellLevel1CharSet[r][c] = level1CharSet;

		// X/26 characters

		// Used to track if character was placed by X/26 data
		// 0=Level 1 character, 1=Active Object or Local Enhancement Data, 2=Adaptive Object
		int x26Character = 0;

		if (m_level == 1 && !m_invocations[0].isEmpty()) {
			// For Level 1.5 only do characters from Local Enhancements
			// which is the last entry on the Active Objects QList
			const textCharacter result = characterFromTriplets(m_invocations[0].constLast().charactersMappedAt(r, c));

			if (result.code != 0x00) {
				m_level1ActivePainter.result.character = result;
				if (result.set == 0)
					m_level1ActivePainter.result.character.set = m_level1ActivePainter.result.g0Set;
				else if (result.set == 2)
					m_level1ActivePainter.result.character.set = m_level1ActivePainter.result.g2Set;
				x26Character = 1;
			}
		} else if (m_level >= 2)
			for (int t=0; t<3; t++)
				for (int i=0; i<m_invocations[t].size(); i++) {
					painter = (t == 0) ? &m_level1ActivePainter : &m_adapPassPainter[t-1][i];

					const textCharacter result = characterFromTriplets(m_invocations[t].at(i).charactersMappedAt(r, c));

					if (t == 0 && result.code == 0x00)
						continue;
					// For an Adaptive Invocation that is applying attribute(s) but not a character here
					// pick up the character underneath so we can still place the attributes
					if (t == 1 && adapInvokeAttrs == i && result.code == 0x00) {
						painter->result.character = m_level1ActivePainter.result.character;
						x26Character = 2;
						continue;
					}

					painter->result.character = result;
					switch (result.set) {
						case 0:
							painter->result.character.set = painter->result.g0Set;
							break;
						case 2:
							painter->result.character.set = painter->result.g2Set;
							break;
						case 24:
							if (painter->attribute.display.underlineSeparated)
								painter->result.character.set = 25;
							break;
					}

					if (t < 2 && result.code != 0x00)
						x26Character = t + 1;
				}

		// Allow Active Objects or Local Enhancement Data to overlap bottom half of a Level 1 double height row
		// where the character on the top half is normal size or double width
		if (m_rowHeight[r] == BottomHalf && c < 40 && x26Character == 1 && m_level1ActivePainter.bottomHalfCell[c].fragment == NormalSize)
			m_level1ActivePainter.bottomHalfCell[c].character.code = 0x00;

		// Allow Adaptive Objects to always overlap bottom half of a Level 1 double height row
		if (m_rowHeight[r] == BottomHalf && c < 40 && x26Character == 2)
			m_level1ActivePainter.bottomHalfCell[c].character.code = 0x00;

		// Work out which fragment of an enlarged character to display
		for (int t=0; t<3; t++) {
			for (int i=0; i<m_invocations[t].size(); i++) {
				// This loop will only iterate once when t == 0 because m_level1ActivePainter is
				// now the overall result of Level 1, Active Objects and Local Enhancement Data
				painter = (t == 0) ? &m_level1ActivePainter : &m_adapPassPainter[t-1][i];

				bool cellCovered = false;

				// Deal with non-origin parts of enlarged characters if placed during previous iteration
				if (painter->rightHalfCell.character.code != 0x00) {
					painter->result = painter->rightHalfCell;
					// Corner cases of right half of double-width characters overlapping
					// a Level 1 double height row need this to avoid spurious characters below
					if (painter->result.fragment == DoubleWidthRightHalf)
						painter->bottomHalfCell[c].character.code = 0x00;
					painter->rightHalfCell.character.code = 0x00;
					cellCovered = true;
				} else if (painter->bottomHalfCell[c].character.code != 0x00) {
					painter->result = painter->bottomHalfCell[c];
					painter->bottomHalfCell[c].character.code = 0x00;
					cellCovered = true;
				}

				if (!cellCovered) {
					// Cell is not covered by previous enlarged character
					// Work out which fragments of enlarged characters are needed from size attributes,
					// place origin of character here and other fragments into right half or bottom half
					// painter buffer ready to be picked up on the next iteration
					bool doubleHeight = painter->attribute.display.doubleHeight;
					bool doubleWidth = painter->attribute.display.doubleWidth;

					if (r == 0 || r > 22)
						doubleHeight = false;
					if (c == 39 || c == 39+m_rightSidePanelColumns || c == 71-m_leftSidePanelColumns || c == 71)
						doubleWidth = false;

					if (doubleHeight) {
						if (doubleWidth) {
							// Double size
							painter->result.fragment = DoubleSizeTopLeftQuarter;
							painter->bottomHalfCell[c] = painter->result;
							painter->bottomHalfCell[c].fragment = DoubleSizeBottomLeftQuarter;
							painter->rightHalfCell = painter->result;
							painter->rightHalfCell.fragment = DoubleSizeTopRightQuarter;
							painter->bottomHalfCell[c+1] = painter->result;
							painter->bottomHalfCell[c+1].fragment = DoubleSizeBottomRightQuarter;
						} else {
							// Double height
							painter->result.fragment = DoubleHeightTopHalf;
							painter->bottomHalfCell[c] = painter->result;
							painter->bottomHalfCell[c].fragment = DoubleHeightBottomHalf;
						}
					} else if (doubleWidth) {
						// Double width
						painter->result.fragment = DoubleWidthLeftHalf;
						painter->rightHalfCell = painter->result;
						painter->rightHalfCell.fragment = DoubleWidthRightHalf;
					} else
						// Normal size
						painter->result.fragment = NormalSize;

					// Now the enlargements and fragments are worked out, prevent Adaptive Objects that
					// are NOT applying Display Attributes from trying to overlap wrong fragments of characters
					// on the underlying page
					if (t == 1 && !adapDisplayAttrs && painter->result.fragment != m_level1ActivePainter.result.fragment) {
						painter->result.character.code = 0x00;
						if (painter->result.fragment == DoubleWidthLeftHalf || painter->result.fragment == DoubleSizeTopLeftQuarter)
							painter->rightHalfCell.character.code = 0x00;
						if (painter->result.fragment == DoubleHeightTopHalf || painter->result.fragment == DoubleSizeTopLeftQuarter)
							painter->bottomHalfCell[c].character.code = 0x00;
						if (painter->result.fragment == DoubleSizeTopLeftQuarter && c < 71)
							painter->bottomHalfCell[c+1].character.code = 0x00;
					}
				}

				if (t == 0)
					break;
			}
		}

		// Top half of Level 1 double height row: normal or double width characters will cause a space
		// with the same attributes to be on the bottom half
		// Also occurs with bottom halves of X/26 double height characters overlapping a
		// Level 1 top half row
		if (m_rowHeight[r] == TopHalf && c < 40) {
			if (m_level1ActivePainter.result.fragment != DoubleHeightTopHalf && m_level1ActivePainter.result.fragment != DoubleSizeTopLeftQuarter && m_level1ActivePainter.result.fragment != DoubleSizeTopRightQuarter) {
				m_level1ActivePainter.bottomHalfCell[c] = m_level1ActivePainter.result;
				m_level1ActivePainter.bottomHalfCell[c].character = { 0x20, 0, 0 };
				m_level1ActivePainter.bottomHalfCell[c].fragment = NormalSize;
			}

		}

		// Now we've finally worked out what characters and attributes are in place on
		// the underlying page and Invoked Objects, work out which of those to actually render
		if (m_level < 2)
			m_cell[r][c] = m_level1ActivePainter.result;
		else {
			bool objectCell = false;

			// Passive Objects highest priority, followed by Adaptive Objects
			// Most recently invoked Object has priority
			for (int t=1; t>=0; t--) {
				for (int i=m_adapPassPainter[t].size()-1; i>=0; i--)
					if (m_adapPassPainter[t][i].result.character.code != 0x00) {
						m_cell[r][c] = m_adapPassPainter[t][i].result;
						objectCell = true;
						break;
					}
				if (objectCell)
					break;
			}

			if (!objectCell)
				// No Adaptive or Passive Object here: will either be Local Enhancement Data, Active Object
				// or underlying Level 1 page
				m_cell[r][c] = m_level1ActivePainter.result;
		}

		// Check for end of Adaptive Object row
		if (adapInvokeAttrs != -1 && c == m_invocations[1].at(adapInvokeAttrs).rightMostColumn(r)) {
			// Neutralise size attributes as they could interfere with double height stuff
			// not sure if this is really necessary
			m_adapPassPainter[0][adapInvokeAttrs].attribute.display.doubleHeight = false;
			m_adapPassPainter[0][adapInvokeAttrs].attribute.display.doubleWidth = false;
			adapInvokeAttrs = -1;
			adapForeground = adapBackground = adapFlash = adapDisplayAttrs = adapStyle = false;
		}

		// Level 1 set-after spacing attributes
		if (c < 40 && m_rowHeight[r] != BottomHalf)
			switch (m_levelOnePage->character(r, c)) {
				case 0x00 ... 0x07: // Alphanumeric and foreground colour
					level1Mosaics = false;
					level1ForegroundCLUT = m_levelOnePage->character(r, c);
					if (m_level >= 2)
						m_level1ActivePainter.attribute.foregroundCLUT = level1ForegroundCLUT | m_foregroundRemap[m_levelOnePage->colourTableRemap()];
					else
						m_level1ActivePainter.attribute.foregroundCLUT = level1ForegroundCLUT;
					m_level1ActivePainter.attribute.display.conceal = false;
					// Switch from mosaics to alpha resets hold mosaic character
					level1HoldMosaicCharacter = 0x20;
					level1HoldMosaicSeparated = false;
					break;
				case 0x10 ... 0x17: // Mosaic and foreground colour
					level1Mosaics = true;
					level1ForegroundCLUT = m_levelOnePage->character(r, c) & 0x07;
					if (m_level >= 2)
						m_level1ActivePainter.attribute.foregroundCLUT = level1ForegroundCLUT | m_foregroundRemap[m_levelOnePage->colourTableRemap()];
					else
						m_level1ActivePainter.attribute.foregroundCLUT = level1ForegroundCLUT;
					m_level1ActivePainter.attribute.display.conceal = false;
					break;
				case 0x08: // Flashing
					m_level1ActivePainter.attribute.flash.mode = 1;
					m_level1ActivePainter.attribute.flash.ratePhase = 0;
					break;
				case 0x0d: // Double height
					if (!m_level1ActivePainter.attribute.display.doubleHeight || m_level1ActivePainter.attribute.display.doubleWidth) {
						// Change of size resets hold mosaic character
						level1HoldMosaicCharacter = 0x20;
						level1HoldMosaicSeparated = false;
					}
					m_level1ActivePainter.attribute.display.doubleHeight = true;
					m_level1ActivePainter.attribute.display.doubleWidth = false;
					break;
				case 0x0e: // Double width
					if (m_level1ActivePainter.attribute.display.doubleHeight || !m_level1ActivePainter.attribute.display.doubleWidth) {
						// Change of size resets hold mosaic character
						level1HoldMosaicCharacter = 0x20;
						level1HoldMosaicSeparated = false;
					}
					m_level1ActivePainter.attribute.display.doubleHeight = false;
					m_level1ActivePainter.attribute.display.doubleWidth = true;
					break;
				case 0x0f: // Double size
					if (!m_level1ActivePainter.attribute.display.doubleHeight || !m_level1ActivePainter.attribute.display.doubleWidth) {
						// Change of size resets hold mosaic character
						level1HoldMosaicCharacter = 0x20;
						level1HoldMosaicSeparated = false;
					}
					m_level1ActivePainter.attribute.display.doubleHeight = true;
					m_level1ActivePainter.attribute.display.doubleWidth = true;
					break;
				case 0x1b: // ESC/switch
					level1EscapeSwitch ^= true;
					if (level1EscapeSwitch)
						level1CharSet = m_level1SecondCharSet;
					else
						level1CharSet = m_level1DefaultCharSet;
					break;
				case 0x1f: // Release mosaics
					level1HoldMosaics = false;
					break;
			}

		if (m_cell[r][c] != previousCellContents)
			setRefresh(r, c, true);
	}
}

inline void TeletextPageDecode::rotateFlashMovement(flashFunctions &flash)
{
	if (flash.ratePhase == 4) {
		flash.phase2HzShown++;
		if (flash.phase2HzShown == 4)
			flash.phase2HzShown = 1;
	} else if (flash.ratePhase == 5) {
		flash.phase2HzShown--;
		if (flash.phase2HzShown == 0)
			flash.phase2HzShown = 3;
	}
}

QColor TeletextPageDecode::cellQColor(int r, int c, ColourPart colourPart)
{
	const bool newsFlashOrSubtitle = m_levelOnePage->controlBit(PageBase::C5Newsflash) || m_levelOnePage->controlBit(PageBase::C6Subtitle);
	int resultCLUT = 0;

	switch (colourPart) {
		case Foreground:
			if (!m_cell[r][c].attribute.display.invert)
				resultCLUT = m_cell[r][c].attribute.foregroundCLUT;
			else
				resultCLUT = m_cell[r][c].attribute.backgroundCLUT;
			break;
		case Background:
			if (!m_cell[r][c].attribute.display.invert)
				resultCLUT = m_cell[r][c].attribute.backgroundCLUT;
			else
				resultCLUT = m_cell[r][c].attribute.foregroundCLUT;
			break;
		case FlashForeground:
			if (!m_cell[r][c].attribute.display.invert)
				resultCLUT = m_cell[r][c].attribute.foregroundCLUT ^ 8;
			else
				resultCLUT = m_cell[r][c].attribute.backgroundCLUT ^ 8;
			break;
	}

	if (resultCLUT == 8) {
		// Transparent CLUT - either Full Row Colour or Video
		// Logic of table C.1 in spec implemented to find out which it is
		if (m_cell[r][c].attribute.display.boxingWindow != newsFlashOrSubtitle)
			return QColor(Qt::transparent);

		int rowColour;

		if (cellCharacterFragment(r, c) == TeletextPageDecode::DoubleHeightBottomHalf ||
		    cellCharacterFragment(r, c) == TeletextPageDecode::DoubleSizeBottomLeftQuarter ||
		    cellCharacterFragment(r, c) == TeletextPageDecode::DoubleSizeBottomRightQuarter)
			rowColour = m_fullRowColour[r-1];
		else
			rowColour = m_fullRowColour[r];

		if (rowColour == 8)
			return QColor(Qt::transparent);
		else
			return m_levelOnePage->CLUTtoQColor(rowColour, m_level);
	} else if (!m_cell[r][c].attribute.display.boxingWindow && newsFlashOrSubtitle)
		return QColor(Qt::transparent);

	return m_levelOnePage->CLUTtoQColor(resultCLUT, m_level);
}

QColor TeletextPageDecode::cellForegroundQColor(int r, int c)
{
	return cellQColor(r, c, Foreground);
}

QColor TeletextPageDecode::cellBackgroundQColor(int r, int c)
{
	return cellQColor(r, c, Background);
}

QColor TeletextPageDecode::cellFlashForegroundQColor(int r, int c)
{
	return cellQColor(r, c, FlashForeground);
}

TeletextPageDecode::textCell& TeletextPageDecode::cellAtCharacterOrigin(int r, int c)
{
	switch (cellCharacterFragment(r, c)) {
		case TeletextPageDecode::DoubleHeightBottomHalf:
		case TeletextPageDecode::DoubleSizeBottomLeftQuarter:
			return m_cell[r-1][c];
		case TeletextPageDecode::DoubleWidthRightHalf:
		case TeletextPageDecode::DoubleSizeTopRightQuarter:
			return m_cell[r][c-1];
		case TeletextPageDecode::DoubleSizeBottomRightQuarter:
			return m_cell[r-1][c-1];
		default:
			return m_cell[r][c];
	}
}

inline void TeletextPageDecode::setFullScreenColour(int newColour)
{
	if (newColour == 8 || m_levelOnePage->controlBit(PageBase::C5Newsflash) || m_levelOnePage->controlBit(PageBase::C6Subtitle)) {
		m_finalFullScreenQColor = QColor(0, 0, 0, 0);
		emit fullScreenColourChanged(QColor(0, 0, 0, 0));
		return;
	}
	QColor newFullScreenQColor = m_levelOnePage->CLUTtoQColor(newColour, m_level);
	m_finalFullScreenColour = newColour;
	if (m_finalFullScreenQColor != newFullScreenQColor) {
		m_finalFullScreenQColor = newFullScreenQColor;
		emit fullScreenColourChanged(m_finalFullScreenQColor);
	}
}

inline void TeletextPageDecode::setFullRowColour(int row, int newColour)
{
	m_fullRowColour[row] = newColour;

	if (newColour == 8 || m_levelOnePage->controlBit(PageBase::C5Newsflash) || m_levelOnePage->controlBit(PageBase::C6Subtitle)) {
		m_fullRowQColor[row] = QColor(0, 0, 0, 0);
		emit fullRowColourChanged(row, QColor(0, 0, 0, 0));
		return;
	}
	QColor newFullRowQColor = m_levelOnePage->CLUTtoQColor(newColour, m_level);
	if (m_fullRowQColor[row] != newFullRowQColor) {
		for (int c=0; c<72; c++) {
			if (m_cell[row][c].attribute.foregroundCLUT == 8 || m_cell[row][c].attribute.backgroundCLUT == 8)
				setRefresh(row, c, true);
		}
		m_fullRowQColor[row] = newFullRowQColor;
		emit fullRowColourChanged(row, m_fullRowQColor[row]);
	}
}
