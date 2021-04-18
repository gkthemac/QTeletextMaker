/*
 * Copyright (C) 2020, 2021 Gavin MacGregor
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

#include <QMultiMap>
#include <QPainter>
//#include <QTime>
#include <QPair>
#include <algorithm>
#include <vector>

#include "render.h"

TeletextPageRender::TeletextPageRender()
{
	QPainter pixmapPainter;

	m_fontBitmap = new QBitmap(":/images/teletextfont.png");
	for (int i=0; i<6; i++)
		m_pagePixmap[i] = new QPixmap(864, 250);
	m_pagePixmap[0]->fill(Qt::transparent);
	m_mix = m_reveal = m_showCodes = false;
	m_renderLevel = 0;
	m_flashRequired = 0;
	m_finalFullScreenColour = 0;
	m_finalFullScreenQColor.setRgb(0, 0, 0);
	for (int r=0; r<25; r++) {
		m_flashRow[r] = 0;
		m_fullRowColour[r] = 0;
		m_fullRowQColor[r].setRgb(0, 0, 0);
		m_concealRow[r] = false;
	}
	m_leftSidePanelColumns = m_rightSidePanelColumns = 0;
	m_textLayer.push_back(&m_level1Layer);
	m_textLayer.push_back(new EnhanceLayer);
}

TeletextPageRender::~TeletextPageRender()
{
	while (m_textLayer.size()>1) {
		delete m_textLayer.back();
		m_textLayer.pop_back();
	}
	for (int i=0; i<6; i++)
		delete m_pagePixmap[i];
	delete m_fontBitmap;
}

void TeletextPageRender::setTeletextPage(LevelOnePage *newCurrentPage)
{
	m_levelOnePage = newCurrentPage;
	m_level1Layer.setTeletextPage(newCurrentPage);
	updateSidePanels();
}

void TeletextPageRender::setReveal(bool newReveal)
{
	m_reveal = newReveal;
	for (int r=0; r<25; r++)
		if (m_concealRow[r])
			renderPage(r);
}

void TeletextPageRender::setMix(bool newMix)
{
	m_mix = newMix;
	decodePage();
	renderPage();
}

void TeletextPageRender::setRenderLevel(int newRenderLevel)
{
	if (newRenderLevel == m_renderLevel)
		return;
	m_renderLevel = newRenderLevel;
	decodePage();
	renderPage();
}

void TeletextPageRender::setShowCodes(bool newShowCodes)
{
	m_showCodes = newShowCodes;
	renderPage();
}

void TeletextPageRender::updateSidePanels()
{
	int oldLeftSidePanelColumns = m_leftSidePanelColumns;
	int oldRightSidePanelColumns = m_rightSidePanelColumns;

	if (m_renderLevel >= (3-m_levelOnePage->sidePanelStatusL25()) && m_levelOnePage->leftSidePanelDisplayed())
		m_leftSidePanelColumns = (m_levelOnePage->sidePanelColumns() == 0) ? 16 : m_levelOnePage->sidePanelColumns();
	else
		m_leftSidePanelColumns = 0;

	if (m_renderLevel >= (3-m_levelOnePage->sidePanelStatusL25()) && m_levelOnePage->rightSidePanelDisplayed())
		m_rightSidePanelColumns = 16-m_levelOnePage->sidePanelColumns();
	else
		m_rightSidePanelColumns = 0;

	if (m_leftSidePanelColumns != oldLeftSidePanelColumns || m_rightSidePanelColumns != oldRightSidePanelColumns) {
		emit sidePanelsChanged();
		renderPage();
	}
}

void TeletextPageRender::buildEnhanceMap(TextLayer *enhanceLayer, int tripletNumber)
{
	bool terminatorFound=false;
	ActivePosition activePosition;
	const X26Triplet *x26Triplet;
	int originModifierR=0;
	int originModifierC=0;

	do {
		x26Triplet = &m_levelOnePage->enhancements()->at(tripletNumber);
		if (x26Triplet->isRowTriplet())
			// Row address group
			switch (x26Triplet->mode()) {
				case 0x00: // Full screen colour
					if (m_renderLevel >= 2 && ((x26Triplet->data() & 0x60) == 0x00) && !activePosition.isDeployed())
						enhanceLayer->setFullScreenColour(x26Triplet->data());
					break;
				case 0x01: // Full row colour
					if (m_renderLevel >= 2 && activePosition.setRow(x26Triplet->addressRow()) && ((x26Triplet->data() & 0x60) == 0x00 || (x26Triplet->data() & 0x60) == 0x60))
						enhanceLayer->setFullRowColour(activePosition.row(), x26Triplet->data() & 0x1f, (x26Triplet->data() & 0x60) == 0x60);
					break;
				case 0x04: // Set active position
					if (activePosition.setRow(x26Triplet->addressRow()) && m_renderLevel >= 2 && x26Triplet->data() < 40)
						activePosition.setColumn(x26Triplet->data());
					break;
				case 0x07: // Address row 0
					if (x26Triplet->address() == 0x3f && !activePosition.isDeployed()) {
						activePosition.setRow(0);
						activePosition.setColumn(8);
						if (m_renderLevel >= 2 && ((x26Triplet->data() & 0x60) == 0x00 || (x26Triplet->data() & 0x60) == 0x60))
							enhanceLayer->setFullRowColour(0, x26Triplet->data() & 0x1f, (x26Triplet->data() & 0x60) == 0x60);
					}
					break;
				case 0x10: // Origin modifier
					if (m_renderLevel >= 2 && (tripletNumber+1) < m_levelOnePage->enhancements()->size() && m_levelOnePage->enhancements()->at(tripletNumber+1).mode() >= 0x11 && m_levelOnePage->enhancements()->at(tripletNumber+1).mode() <= 0x13 && x26Triplet->address() >= 40 && x26Triplet->data() < 72) {
						originModifierR = x26Triplet->address()-40;
						originModifierC = x26Triplet->data();
					}
					break;
				case 0x11 ... 0x13: // Invoke Object
					if (m_renderLevel >= 2) {
						if ((x26Triplet->address() & 0x18) == 0x08) {
							// Local Object
							// Check if the pointer in the Invocation triplet is valid
							// Can't point to triplets 13-15; only 12 triplets per packet
							if ((x26Triplet->data() & 0x0f) > 12)
								break;
							int tripletPointer = ((x26Triplet->data() >> 4) | ((x26Triplet->address() & 1) << 3)) * 13 + (x26Triplet->data() & 0x0f);
							// Can't point to triplet beyond the end of the Local Enhancement Data
							if ((tripletPointer+1) >= m_levelOnePage->enhancements()->size())
								break;
							// Check if we're pointing to an actual Object Definition of the same type
							if ((x26Triplet->mode() | 0x04) != m_levelOnePage->enhancements()->at(tripletPointer).mode())
								break;
							// The Object Definition can't declare it's at triplet 13-15; only 12 triplets per packet
							if ((m_levelOnePage->enhancements()->at(tripletPointer).data() & 0x0f) > 12)
								break;
							// Check if the Object Definition triplet is where it declares it is
							if ((((m_levelOnePage->enhancements()->at(tripletPointer).data() >> 4) | ((m_levelOnePage->enhancements()->at(tripletPointer).address() & 1) << 3)) * 13 + (m_levelOnePage->enhancements()->at(tripletPointer).data() & 0x0f)) != tripletPointer)
								break;
							// Is the object required at the current presentation Level?
							if (m_renderLevel == 2 && (m_levelOnePage->enhancements()->at(tripletPointer).address() & 0x08) == 0x00)
								break;
							if (m_renderLevel == 3 && (m_levelOnePage->enhancements()->at(tripletPointer).address() & 0x10) == 0x00)
								break;
							EnhanceLayer *newLayer = new EnhanceLayer;
							m_textLayer.push_back(newLayer);
							newLayer->setObjectType(x26Triplet->mode() & 0x03);
							newLayer->setOrigin(activePosition.row()+originModifierR, activePosition.column()+originModifierC);
							buildEnhanceMap(newLayer, tripletPointer+1);
						} else
							qDebug("POP or GPOP");
						originModifierR = originModifierC = 0;
					}
					break;
				case 0x15 ... 0x17: // Define Object, also used as terminator
					terminatorFound = true;
					break;
				case 0x1f: // Terminator
					if (x26Triplet->address() == 63)
						terminatorFound = true;
					break;
			}
		else {
			// Column address group
			bool columnTripletActioned = true;
			switch (x26Triplet->mode()) {
				// First we deal with column triplets that are also valid at Level 1.5
				case 0x0b: // G3 mosaic character at Level 2.5
					if (m_renderLevel <= 1)
						break;
					// fall-through
				case 0x02: // G3 mosaic character at Level 1.5
				case 0x0f: // G2 character
				case 0x10 ... 0x1f: // Diacritical mark
					if (activePosition.setColumn(x26Triplet->addressColumn()) && x26Triplet->data() >= 0x20)
						enhanceLayer->enhanceMap.insert(qMakePair(activePosition.row(), activePosition.column()), qMakePair(x26Triplet->mode() | 0x20, x26Triplet->data()));
					break;
				// Make sure that PDC and reserved triplets don't affect the Active Position
				case 0x04 ... 0x06: // 0x04 and 0x05 are reserved, 0x06 for PDC
				case 0x0a: // Reserved
					break;
				default:
					columnTripletActioned = false;
			}
			// All remaining possible column triplets at Level 2.5 affect the Active Position
			if (m_renderLevel >= 2 && !columnTripletActioned && activePosition.setColumn(x26Triplet->addressColumn()))
				enhanceLayer->enhanceMap.insert(qMakePair(activePosition.row(), activePosition.column()), qMakePair(x26Triplet->mode() | 0x20, x26Triplet->data()));
		}
		tripletNumber++;
	} while (!terminatorFound && tripletNumber < m_levelOnePage->enhancements()->size());
}

void TeletextPageRender::decodePage()
{
	QPainter pixmapPainter;
	int currentFullRowColour, downwardsFullRowColour;
	int renderedFullScreenColour;

	struct {
		bool operator() (TextLayer *i, TextLayer *j) { return (i->objectType() < j->objectType()); }
	} compareLayer;

	updateSidePanels();

	while (m_textLayer.size()>2) {
		delete m_textLayer.back();
		m_textLayer.pop_back();
	}

	renderedFullScreenColour = (m_renderLevel >= 2) ? m_levelOnePage->defaultScreenColour() : 0;
	downwardsFullRowColour = (m_renderLevel >= 2) ? m_levelOnePage->defaultRowColour() : 0;
	setFullScreenColour(renderedFullScreenColour);
	for (int r=0; r<25; r++)
		setFullRowColour(r ,downwardsFullRowColour);

	m_textLayer[1]->enhanceMap.clear();
	if (m_renderLevel == 0 || m_levelOnePage->enhancements()->empty())
		return;

	m_textLayer[1]->setFullScreenColour(-1);
	for (int r=0; r<25; r++)
		m_textLayer[1]->setFullRowColour(r, -1, false);
	buildEnhanceMap(m_textLayer[1]);

	if (m_textLayer.size() > 2)
		std::stable_sort(m_textLayer.begin()+2, m_textLayer.end(), compareLayer);

	if (m_renderLevel <= 1)
		return;

	 if (m_textLayer[1]->fullScreenColour() != -1)
		downwardsFullRowColour = m_textLayer[1]->fullScreenColour();
	for (int r=0; r<25; r++) {
		for (int l=0; l<2; l++) {
			if (r == 0 && m_textLayer[l]->fullScreenColour() != - 1)
				renderedFullScreenColour = m_textLayer[l]->fullScreenColour();
			if (m_textLayer[l]->fullRowColour(r) == - 1)
				currentFullRowColour = downwardsFullRowColour;
			else {
				currentFullRowColour = m_textLayer[l]->fullRowColour(r);
				if (m_textLayer[l]->fullRowDownwards(r))
					downwardsFullRowColour = currentFullRowColour;
			}
		}
		setFullRowColour(r ,currentFullRowColour);
	}
	setFullScreenColour(renderedFullScreenColour);
}

void TeletextPageRender::renderPage()
{
//	QTime renderPageTime;

//	renderPageTime.start();
	for (int r=0; r<25; r++)
		renderPage(r);
//	qDebug("Full page render: %d ms", renderPageTime.elapsed());
}

void TeletextPageRender::renderPage(int r)
{
	QPainter pixmapPainter;
	QColor foreQColour, backQColour;
	int c;
	int phaseNumberRender = 0;
	int flashRowRequired = 0;
	bool renderNextRow = false;
	bool applyRightHalf = false;
	bool previouslyDoubleHeight, previouslyBottomHalf, underlined;
	bool doubleHeightFound = false;
	textCharacter resultCharacter, layerCharacter;
	applyAttributes layerApplyAttributes;
	textAttributes underlyingAttributes, resultAttributes;
	int level1CharSet;

	auto renderCharacter=[&]()
	{
		if (m_cell[r][c].bottomHalf || m_cell[r][c].rightHalf)
			return;
		int charWidth = resultAttributes.display.doubleWidth ? 24 : 12;
		int charHeight = resultAttributes.display.doubleHeight ? 20 : 10;
		bool transparentBackground = (m_levelOnePage->controlBit(PageBase::C5Newsflash) || m_levelOnePage->controlBit(PageBase::C6Subtitle) || m_mix) && !resultAttributes.display.boxingWindow;
		// bool transparentForeground = false;
		if (transparentBackground) {
			pixmapPainter.setCompositionMode(QPainter::CompositionMode_Clear);
			pixmapPainter.eraseRect(c*12, r*10, charWidth, charHeight);
			pixmapPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		}
		if ((m_levelOnePage->controlBit(PageBase::C5Newsflash) || m_levelOnePage->controlBit(PageBase::C6Subtitle)) && !resultAttributes.display.boxingWindow) {
			// Outside a boxed area
			resultCharacter = { 0x20, 0, 0 };
			underlined = false;
		}
		if ((resultCharacter.code <= 0x20 && resultCharacter.diacritical == 0 && resultCharacter.set != 26) || (resultAttributes.display.conceal && !m_reveal)) {
			if (!transparentBackground) {
				pixmapPainter.fillRect(c*12, r*10, charWidth, charHeight, backQColour);
			}
		} else {
			if (transparentBackground)
				pixmapPainter.setBackgroundMode(Qt::TransparentMode);
			else {
				pixmapPainter.setBackgroundMode(Qt::OpaqueMode);
				pixmapPainter.setBackground(QBrush(backQColour));
			}
			pixmapPainter.setPen(foreQColour);
			pixmapPainter.drawPixmap(c*12, r*10, charWidth, charHeight, *m_fontBitmap, (resultCharacter.code-32)*12, resultCharacter.set*10, 12, 10);
			if (resultCharacter.diacritical) {
				pixmapPainter.setBackgroundMode(Qt::TransparentMode);
				pixmapPainter.drawPixmap(c*12, r*10, charWidth, charHeight, *m_fontBitmap, 384+resultCharacter.diacritical*12, 70, 12, 10);
				pixmapPainter.setBackgroundMode(Qt::OpaqueMode);
			}
		}
		if (underlined) {
			pixmapPainter.setPen(foreQColour);
			if (!resultAttributes.display.doubleHeight)
				pixmapPainter.drawLine(c*12, r*10+9, c*12+charWidth-1, r*10+9);
			else
				pixmapPainter.drawRect(c*12, r*10+18, charWidth-1, 1);
		}
	};

	pixmapPainter.begin(m_pagePixmap[0]);
	pixmapPainter.setBackgroundMode(Qt::OpaqueMode);

	m_concealRow[r] = false;
	for (c=0; c<72; c++) {
		resultAttributes = underlyingAttributes;
		for (int l=0; l<m_textLayer.size(); l++) {
			layerCharacter = m_textLayer[l]->character(r, c);
			if (layerCharacter.code != 0x00)
				resultCharacter = layerCharacter;
			if (l == 0) {
//				m_cell[r][c].level1Mosaic = (resultCharacter.set == 24 || resultCharacter.set == 25) && m_levelOnePage->character(r, c) >= 0x20;
				m_cell[r][c].level1Mosaic = (resultCharacter.set == 24 || resultCharacter.set == 25);
				if (!m_cell[r][c].level1Mosaic)
					level1CharSet = resultCharacter.set;
				m_cell[r][c].level1CharSet = level1CharSet;
			}

			layerApplyAttributes = { false, false, false, false, false, false, false, false };
			m_textLayer[l]->attributes(r, c, &layerApplyAttributes);
			if (layerApplyAttributes.copyAboveAttributes) {
				resultAttributes = m_cell[r-1][c].attribute;
				layerApplyAttributes.copyAboveAttributes = false;
				break;
			}
			if (layerApplyAttributes.applyForeColour) {
				resultAttributes.foreColour = layerApplyAttributes.attribute.foreColour;
				if (l == 0 && m_renderLevel >= 2)
					resultAttributes.foreColour |= m_foregroundRemap[m_levelOnePage->colourTableRemap()];
			}
			if (layerApplyAttributes.applyBackColour) {
				resultAttributes.backColour = layerApplyAttributes.attribute.backColour;
				if (l == 0) {
					if (m_renderLevel >= 2)
						if (resultAttributes.backColour == 0x20)
							resultAttributes.backColour = (c > 39 || m_levelOnePage->blackBackgroundSubst()) ? m_fullRowColour[r] : m_backgroundRemap[m_levelOnePage->colourTableRemap()];
						else
							resultAttributes.backColour |= m_backgroundRemap[m_levelOnePage->colourTableRemap()];
					else
						if (resultAttributes.backColour == 0x20)
							resultAttributes.backColour = 0x00;
				}
			}
			if (resultAttributes.foreColour == 8)
				resultAttributes.foreColour = m_fullRowColour[r];
			if (resultAttributes.backColour == 8)
				resultAttributes.backColour = m_fullRowColour[r];

			if (layerApplyAttributes.applyFlash) {
				//BUG Adaptive Objects disrupt inc/dec flash
				resultAttributes.flash = layerApplyAttributes.attribute.flash;
				if (resultAttributes.flash.mode != 0)
					phaseNumberRender = (resultAttributes.flash.ratePhase == 4 || resultAttributes.flash.ratePhase == 5) ? 1 : resultAttributes.flash.ratePhase;
			}
			if (layerApplyAttributes.applyDisplayAttributes)
				resultAttributes.display = layerApplyAttributes.attribute.display;
			else {
				// Selecting contiguous mosaics wih a triplet will override an earlier Level 1 separated mosaics attribute until a further Level 1 contiguous mosaic attribute is encountered
				resultAttributes.display.forceContiguous = (layerApplyAttributes.applyContiguousOnly) ? false : underlyingAttributes.display.forceContiguous;
				if (layerApplyAttributes.applyTextSizeOnly) {
					resultAttributes.display.doubleHeight = layerApplyAttributes.attribute.display.doubleHeight;
					resultAttributes.display.doubleWidth = layerApplyAttributes.attribute.display.doubleWidth;
				}
				if (layerApplyAttributes.applyBoxingOnly)
					resultAttributes.display.boxingWindow = layerApplyAttributes.attribute.display.boxingWindow;
				if (layerApplyAttributes.applyConcealOnly || layerApplyAttributes.applyForeColour)
					resultAttributes.display.conceal = layerApplyAttributes.attribute.display.conceal;
			}
			if (m_textLayer[l]->objectType() <= 1)
				underlyingAttributes = resultAttributes;

			if (m_renderLevel == 0)
				break;
		}

		underlined = false;
		if (resultAttributes.display.underlineSeparated) {
			if (resultCharacter.set == 24)
				resultCharacter.set = 25;
			else
				underlined = resultCharacter.set < 24;
		}
		if (resultAttributes.display.forceContiguous && resultCharacter.set == 25)
			resultCharacter.set = 24;

		resultAttributes.flash.phaseNumber = phaseNumberRender;

		previouslyDoubleHeight = m_cell[r][c].attribute.display.doubleHeight;
		previouslyBottomHalf = m_cell[r][c].bottomHalf;

		m_cell[r][c].character = resultCharacter;
		m_cell[r][c].attribute = resultAttributes;

		if (resultAttributes.flash.ratePhase == 4 && ++phaseNumberRender == 4)
			phaseNumberRender = 1;
		if (resultAttributes.flash.ratePhase == 5 && --phaseNumberRender == 0)
			phaseNumberRender = 3;
		if (resultAttributes.flash.mode > 0)
			flashRowRequired |= (resultAttributes.flash.ratePhase == 0) ? 1 : 2;

		if (r > 0)
			m_cell[r][c].bottomHalf = m_cell[r-1][c].attribute.display.doubleHeight && !m_cell[r-1][c].bottomHalf;
		if ((resultAttributes.display.doubleHeight != previouslyDoubleHeight) || (m_cell[r][c].bottomHalf != previouslyBottomHalf))
			renderNextRow = true;
		m_cell[r][c].rightHalf = applyRightHalf;

		if (!resultAttributes.display.invert) {
			foreQColour = m_levelOnePage->CLUTtoQColor(resultAttributes.foreColour, m_renderLevel);
			backQColour = m_levelOnePage->CLUTtoQColor(resultAttributes.backColour, m_renderLevel);
		} else {
			foreQColour = m_levelOnePage->CLUTtoQColor(resultAttributes.backColour, m_renderLevel);
			backQColour = m_levelOnePage->CLUTtoQColor(resultAttributes.foreColour, m_renderLevel);
		}

		renderCharacter();

		if (m_showCodes && c < 40 && m_levelOnePage->character(r, c)<0x20 && !m_level1Layer.isRowBottomHalf(r) && !m_cell[r][c].bottomHalf) {
			pixmapPainter.setBackground(QBrush(QColor(0, 0, 0, 128)));
			pixmapPainter.setPen(QColor(255, 255, 255, 224));
			pixmapPainter.drawPixmap(c*12, r*10, 12, 10, *m_fontBitmap, (m_levelOnePage->character(r, c)+32)*12, 250, 12, 10);
		}

		if (resultAttributes.display.doubleHeight)
			doubleHeightFound = true;
		if (resultAttributes.display.doubleWidth)
			applyRightHalf ^= true;
		else
			applyRightHalf = false;
		if (resultAttributes.display.conceal)
			m_concealRow[r] = true;
	}

	pixmapPainter.end();

	if (flashRowRequired == 3)
		flashRowRequired = 2;
	if (flashRowRequired != m_flashRow[r]) {
		m_flashRow[r] = flashRowRequired;
		updateFlashRequired(flashRowRequired);
	}
	// If flashing is present, copy the rendered line into the other buffers
	if (m_flashRequired > 0) {
		pixmapPainter.begin(m_pagePixmap[3]);
		pixmapPainter.setCompositionMode(QPainter::CompositionMode_Clear);
		pixmapPainter.eraseRect(0, r*10, 864, doubleHeightFound ? 20 : 10);
		pixmapPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		pixmapPainter.drawPixmap(0, r*10, *m_pagePixmap[0], 0, r*10, 864, doubleHeightFound ? 20 : 10);
		if (m_flashRequired == 2) {
			pixmapPainter.end();
			pixmapPainter.begin(m_pagePixmap[1]);
			pixmapPainter.setCompositionMode(QPainter::CompositionMode_Clear);
			pixmapPainter.eraseRect(0, r*10, 864, doubleHeightFound ? 20 : 10);
			pixmapPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
			pixmapPainter.drawPixmap(0, r*10, *m_pagePixmap[0], 0, r*10, 864, doubleHeightFound ? 20 : 10);
			pixmapPainter.end();
			pixmapPainter.begin(m_pagePixmap[2]);
			pixmapPainter.setCompositionMode(QPainter::CompositionMode_Clear);
			pixmapPainter.eraseRect(0, r*10, 864, doubleHeightFound ? 20 : 10);
			pixmapPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
			pixmapPainter.drawPixmap(0, r*10, *m_pagePixmap[0], 0, r*10, 864, doubleHeightFound ? 20 : 10);
			pixmapPainter.end();
			pixmapPainter.begin(m_pagePixmap[4]);
			pixmapPainter.setCompositionMode(QPainter::CompositionMode_Clear);
			pixmapPainter.eraseRect(0, r*10, 864, doubleHeightFound ? 20 : 10);
			pixmapPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
			pixmapPainter.drawPixmap(0, r*10, *m_pagePixmap[3], 0, r*10, 864, doubleHeightFound ? 20 : 10);
			pixmapPainter.end();
			pixmapPainter.begin(m_pagePixmap[5]);
			pixmapPainter.setCompositionMode(QPainter::CompositionMode_Clear);
			pixmapPainter.eraseRect(0, r*10, 864, doubleHeightFound ? 20 : 10);
			pixmapPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
			pixmapPainter.drawPixmap(0, r*10, *m_pagePixmap[3], 0, r*10, 864, doubleHeightFound ? 20 : 10);
		}
		pixmapPainter.end();
	}
	if (flashRowRequired)
		for (c=0; c<72; c++) {
			if (m_cell[r][c].attribute.flash.mode == 0 || m_cell[r][c].bottomHalf || m_cell[r][c].rightHalf)
				continue;
			// Compose other phase of flash, either a space or adjacent CLUT
			resultAttributes = m_cell[r][c].attribute;
			if (m_cell[r][c].attribute.flash.mode == 3) {
				resultAttributes.foreColour ^= 8;
				resultCharacter = m_cell[r][c].character;
				underlined = resultAttributes.display.underlineSeparated && (m_cell[r][c].character.set < 24);
			} else { // flash modes 1 and 2
				resultCharacter = { 0x20, 0, 0 };
				underlined = false;
			}
			if (!resultAttributes.display.invert) {
				foreQColour = m_levelOnePage->CLUTtoQColor(resultAttributes.foreColour, m_renderLevel);
				backQColour = m_levelOnePage->CLUTtoQColor(resultAttributes.backColour, m_renderLevel);
			} else {
				foreQColour = m_levelOnePage->CLUTtoQColor(resultAttributes.backColour, m_renderLevel);
				backQColour = m_levelOnePage->CLUTtoQColor(resultAttributes.foreColour, m_renderLevel);
			}
			if (resultAttributes.flash.ratePhase == 0) {
				// 1Hz flash
				int flashPixmap = resultAttributes.flash.mode == 2 ? 0 : 3;
				pixmapPainter.begin(m_pagePixmap[flashPixmap]);
				renderCharacter();
				if (m_flashRequired == 2) {
					pixmapPainter.end();
					pixmapPainter.begin(m_pagePixmap[flashPixmap+1]);
					renderCharacter();
					pixmapPainter.end();
					pixmapPainter.begin(m_pagePixmap[flashPixmap+2]);
					renderCharacter();
				}
				pixmapPainter.end();
			} else
				// 2Hz flash
				for (int p=1; p<=3; p++)
					if ((resultAttributes.flash.phaseNumber != p) ^ (resultAttributes.flash.mode == 2)) {
						pixmapPainter.begin(m_pagePixmap[p-1]);
						renderCharacter();
						pixmapPainter.end();
						pixmapPainter.begin(m_pagePixmap[p+2]);
						renderCharacter();
						pixmapPainter.end();
					}
		}

	if (renderNextRow && r<24)
		renderPage(r+1);
}

void TeletextPageRender::updateFlashRequired(int newFlashRequired)
{
	if (newFlashRequired == m_flashRequired)
		return;

	if (newFlashRequired < m_flashRequired) {
		int minimumFlashRequired = newFlashRequired;
		// If the flash rate for a row is reduced, check the other rows if they still need flashing
		for (int r=0; r<25; r++)
			if (m_flashRow[r] > minimumFlashRequired)
				minimumFlashRequired = m_flashRow[r];
		if (minimumFlashRequired > newFlashRequired)
			newFlashRequired = minimumFlashRequired;
		if (newFlashRequired == m_flashRequired)
			return;
		m_flashRequired = newFlashRequired;
		emit flashChanged(m_flashRequired);
		return;
	}

	// newFlashRequired > flashRequired
	if (m_flashRequired == 0)
		*m_pagePixmap[3] = m_pagePixmap[0]->copy();
	if (newFlashRequired == 2) {
		*m_pagePixmap[1] = m_pagePixmap[0]->copy();
		*m_pagePixmap[2] = m_pagePixmap[0]->copy();
		*m_pagePixmap[4] = m_pagePixmap[3]->copy();
		*m_pagePixmap[5] = m_pagePixmap[3]->copy();
	}
	m_flashRequired = newFlashRequired;
	emit flashChanged(m_flashRequired);
}

inline void TeletextPageRender::setFullScreenColour(int newColour)
{
	if (m_mix || m_levelOnePage->controlBit(PageBase::C5Newsflash) || m_levelOnePage->controlBit(PageBase::C6Subtitle)) {
		m_finalFullScreenQColor = QColor(0, 0, 0, 0);
		emit fullScreenColourChanged(QColor(0, 0, 0, 0));
		return;
	}
	QColor newFullScreenQColor = m_levelOnePage->CLUTtoQColor(newColour, m_renderLevel);
	m_finalFullScreenColour = newColour;
	if (m_finalFullScreenQColor != newFullScreenQColor) {
		m_finalFullScreenQColor = newFullScreenQColor;
		emit fullScreenColourChanged(m_finalFullScreenQColor);
	}
}

inline void TeletextPageRender::setFullRowColour(int row, int newColour)
{
	if (m_mix || m_levelOnePage->controlBit(PageBase::C5Newsflash) || m_levelOnePage->controlBit(PageBase::C6Subtitle)) {
		m_fullRowQColor[row] = QColor(0, 0, 0, 0);
		emit fullRowColourChanged(row, QColor(0, 0, 0, 0));
		return;
	}
	QColor newFullRowQColor = m_levelOnePage->CLUTtoQColor(newColour, m_renderLevel);
	m_fullRowColour[row] = newColour;
	if (m_fullRowQColor[row] != newFullRowQColor) {
		m_fullRowQColor[row] = newFullRowQColor;
		emit fullRowColourChanged(row, m_fullRowQColor[row]);
	}
}

void TextLayer::setTeletextPage(LevelOnePage *newCurrentPage) { m_levelOnePage = newCurrentPage; }
void TextLayer::setFullScreenColour(int newColour) { m_layerFullScreenColour = newColour; }

void TextLayer::setFullRowColour(int r, int newColour, bool newDownwards)
{
	m_layerFullRowColour[r] = newColour;
	m_layerFullRowDownwards[r] = newDownwards;
}

void EnhanceLayer::setObjectType(int newObjectType) { m_objectType = newObjectType; }

void EnhanceLayer::setOrigin(int r, int c)
{
	m_originR = r;
	m_originC = c;
}

Level1Layer::Level1Layer()
{
	for (int r=0; r<25; r++) {
		m_rowHasDoubleHeightAttr[r] = false;
		m_rowHeight[r] = RHnormal;
	}
}

EnhanceLayer::EnhanceLayer()
{
	for (int r=0; r<25; r++) {
		m_layerFullRowColour[r] = -1;
		m_layerFullRowDownwards[r] = false;
	}
}

textCharacter EnhanceLayer::character(int r, int c)
{
	r -= m_originR;
	c -= m_originC;
	if (r < 0 || c < 0)
		return { 0, 0 };

	// QPair.first is triplet mode, QPair.second is triplet data
	QList<QPair<int, int>> enhancements = enhanceMap.values(qMakePair(r, c));

	if (enhancements.size() > 0)
		for (int i=0; i<enhancements.size(); i++)
			switch (enhancements.at(i).first) {
				case 0x2b: // G3 mosaic character at Level 2.5
				case 0x22: // G3 mosaic character at Level 1.5
					return { enhancements.at(i).second, 26 };
				case 0x29: // G0 character at Level 2.5
					return { enhancements.at(i).second, 0 };
				case 0x2f: // G2 character
					return { enhancements.at(i).second, 7 };
				case 0x30 ... 0x3f: // Diacritical
					// Deal with @ symbol replacing * symbol - clause 15.6.1 note 3 in spec
					if (enhancements.at(i).first == 0x30 && enhancements.at(i).second == 0x2a)
						return { 0x40, 0 };
					else
						return { enhancements.at(i).second, 0, enhancements.at(i).first & 0x0f };
				case 0x21: // G1 character
					if ((enhancements.at(i).second) >= 0x20)
						return { enhancements.at(i).second, (enhancements.at(i).second & 0x20) ? 24 : 0 };
			}
	return { 0, 0 };
}

void EnhanceLayer::attributes(int r, int c, applyAttributes *layerApplyAttributes)
{
	r -= m_originR;
	c -= m_originC;
	if (r < 0 || c < 0)
		return;
	if (m_objectType == 2) {
		// Adaptive Object - find rightmost column addressed on this row if we haven't already
		if (r != m_rowCached) {
			m_rightMostColumn[r] = 0;
			m_rowCached = r;
			for (int cc=39; cc>0; cc--)
				if (enhanceMap.contains(qMakePair(r, cc))) {
					m_rightMostColumn[r] = cc;
					break;
				}
		}
		// On new row, default to attributes already on page
		// At end of rightmost column, let go of all attributes
		if (c == 0 || c == m_rightMostColumn[r]+1)
			m_applyAttributes = { false, false, false, false, false, false, false, false };
		else {
			// Re-apply attributes that Object has defined previously on this row
			if (m_applyAttributes.applyForeColour) {
				layerApplyAttributes->applyForeColour = true;
				layerApplyAttributes->attribute.foreColour = m_applyAttributes.attribute.foreColour;
			}
			if (m_applyAttributes.applyBackColour) {
				layerApplyAttributes->applyBackColour = true;
				layerApplyAttributes->attribute.backColour = m_applyAttributes.attribute.backColour;
			}
			//BUG Adaptive Objects disrupt inc/dec flash
			if (m_applyAttributes.applyFlash) {
				layerApplyAttributes->applyFlash = true;
				layerApplyAttributes->attribute.flash.mode = m_applyAttributes.attribute.flash.mode;
				layerApplyAttributes->attribute.flash.ratePhase = m_applyAttributes.attribute.flash.ratePhase;
			}
			if (m_applyAttributes.applyDisplayAttributes) {
				layerApplyAttributes->applyDisplayAttributes = true;
				layerApplyAttributes->attribute.display = m_applyAttributes.attribute.display;
			}
		}
	}
	if (m_objectType == 3) {
		if (r == 0 && c == 0) {
			// Passive Objects always start with all these default attributes
			m_applyAttributes.applyForeColour = true;
			m_applyAttributes.attribute.foreColour = 0x07;
			m_applyAttributes.applyBackColour = true;
			m_applyAttributes.attribute.backColour = 0x00;
			m_applyAttributes.applyDisplayAttributes = true;
			m_applyAttributes.applyFlash = true;
			m_applyAttributes.attribute.flash.mode = 0;
			m_applyAttributes.attribute.flash.ratePhase = 0;
			m_applyAttributes.attribute.display.doubleHeight = false;
			m_applyAttributes.attribute.display.doubleWidth = false;
			m_applyAttributes.attribute.display.boxingWindow = false;
			m_applyAttributes.attribute.display.conceal = false;
			m_applyAttributes.attribute.display.invert = false;
			m_applyAttributes.attribute.display.underlineSeparated = false;
			m_applyAttributes.attribute.display.forceContiguous = false;
		}
		if (character(r+m_originR, c+m_originC).code == 0x00)
			// Passive Object attributes only apply where it also defines a character
			// In this case, wrench the pointer-parameter to alter only the attributes of the Object
			layerApplyAttributes = &m_applyAttributes;
		else
			*layerApplyAttributes = m_applyAttributes;
	}

	// QPair.first is triplet mode, QPair.second is triplet data
	QList<QPair<int, int>> enhancements = enhanceMap.values(qMakePair(r, c));

	for (int i=0; i<enhancements.size(); i++)
		switch (enhancements.at(i).first) {
			case 0x20: // Foreground colour
				if ((enhancements.at(i).second & 0x60) == 0) {
					layerApplyAttributes->applyForeColour = true;
					layerApplyAttributes->attribute.foreColour = enhancements.at(i).second;
				}
				break;
			case 0x23: // Background colour
				if ((enhancements.at(i).second & 0x60) == 0) {
					layerApplyAttributes->applyBackColour = true;
					layerApplyAttributes->attribute.backColour = enhancements.at(i).second;
				}
				break;
			case 0x27: // Additional flash functions
				if ((enhancements.at(i).second & 0x60) == 0 && (enhancements.at(i).second & 0x18) != 0x18) { // Avoid reserved rate/phase
					layerApplyAttributes->applyFlash = true;
					layerApplyAttributes->attribute.flash.mode = enhancements.at(i).second & 0x03;
					layerApplyAttributes->attribute.flash.ratePhase = (enhancements.at(i).second >> 2) & 0x07;
				}
				break;
			case 0x2c: // Display attributes
				layerApplyAttributes->applyDisplayAttributes = true;
				layerApplyAttributes->attribute.display.doubleHeight = enhancements.at(i).second & 0x01;
				layerApplyAttributes->attribute.display.boxingWindow = enhancements.at(i).second & 0x02;
				layerApplyAttributes->attribute.display.conceal = enhancements.at(i).second & 0x04;
				layerApplyAttributes->attribute.display.invert = enhancements.at(i).second & 0x10;
				layerApplyAttributes->attribute.display.underlineSeparated = enhancements.at(i).second & 0x20;
				// Selecting contiguous mosaics wih a triplet will override an earlier Level 1 separated mosaics attribute
				layerApplyAttributes->attribute.display.forceContiguous = !layerApplyAttributes->attribute.display.underlineSeparated;
				layerApplyAttributes->attribute.display.doubleWidth = enhancements.at(i).second & 0x40;
				break;
		}
	if (m_objectType >= 2)
		m_applyAttributes = *layerApplyAttributes;
}


void Level1Layer::updateRowCache(int r)
{
	level1CacheAttributes buildCacheAttributes;
	bool doubleHeightAttrFound = false;

	for (int c=0; c<40; c++) {
		unsigned char charCode = m_levelOnePage->character(r, c);
		// Set at spacing attributes
		switch (charCode) {
			case 0x0c: // Normal size
				if (buildCacheAttributes.sizeCode != 0x0c) // Size CHANGE resets held mosaic to space
					buildCacheAttributes.holdChar = 0x20;
				buildCacheAttributes.sizeCode = 0x0c;
				break;
			case 0x19: // Contiguous mosaics
				buildCacheAttributes.separated = false;
				break;
			case 0x1a: // Separated mosaics
				buildCacheAttributes.separated = true;
				break;
			case 0x1c: // Black background
				buildCacheAttributes.backColour = 0x00;
				break;
			case 0x1d: // New background
				buildCacheAttributes.backColour = buildCacheAttributes.foreColour & 0x07;
				break;
			case 0x1e: // Hold mosaics
				buildCacheAttributes.held = true;
				break;
		}

		if (buildCacheAttributes.mosaics && (charCode & 0x20)) {
			buildCacheAttributes.holdChar = charCode;
			buildCacheAttributes.holdSeparated = buildCacheAttributes.separated;
		}

		m_attributeCache[c] = buildCacheAttributes;

		// Set-after spacing attributes
		switch (charCode) {
			case 0x00 ... 0x07: // Alphanumeric + foreground colour
				buildCacheAttributes.foreColour = charCode;
				buildCacheAttributes.mosaics = false;
				buildCacheAttributes.holdChar = 0x20; // Switch from mosaics to alpha resets held mosaic
				buildCacheAttributes.holdSeparated = false;
				break;
			case 0x10 ... 0x17: // Mosaic + foreground colour
				buildCacheAttributes.foreColour = charCode & 0x07;
				buildCacheAttributes.mosaics = true;
				break;
			case 0x0d: // Double height
			case 0x0f: // Double size
				doubleHeightAttrFound = true;
				// fall-through
			case 0x0e: // Double width
				if (buildCacheAttributes.sizeCode != charCode) // Size CHANGE resets held mosaic to space
					buildCacheAttributes.holdChar = 0x20;
				buildCacheAttributes.sizeCode = charCode;
				break;
			case 0x1b: // ESC/switch
				buildCacheAttributes.escSwitch ^= true;
				break;
			case 0x1f: // Release mosaics
				buildCacheAttributes.held = false;
				break;
		}
	}

	if (doubleHeightAttrFound != m_rowHasDoubleHeightAttr[r]) {
		m_rowHasDoubleHeightAttr[r] = doubleHeightAttrFound;
		for (int dr=r; dr<24; dr++)
			if (m_rowHasDoubleHeightAttr[dr]) {
				m_rowHeight[dr] = RHtophalf;
				m_rowHeight[++dr] = RHbottomhalf;
			} else
				m_rowHeight[dr] = RHnormal;
	}
}

textCharacter Level1Layer::character(int r, int c)
{
	textCharacter result;

	if (r != m_rowCached)
		updateRowCache(r);
	if (c > 39 || m_rowHeight[r] == RHbottomhalf)
		return { 0x20, 0 };
	result.code = m_levelOnePage->character(r, c);
	if (m_levelOnePage->secondCharSet() != 0xf && m_attributeCache[c].escSwitch)
		result.set = g0CharacterMap.value(((m_levelOnePage->secondCharSet() << 3) | m_levelOnePage->secondNOS()), 0);
	else
		result.set = g0CharacterMap.value(((m_levelOnePage->defaultCharSet() << 3) | m_levelOnePage->defaultNOS()), 0);
	if (result.code < 0x20) {
		result.code = m_attributeCache[c].held ? m_attributeCache[c].holdChar : 0x20;
		if (m_attributeCache[c].held && c > 0)
			result.set = 24+m_attributeCache[c].holdSeparated;
//		else
//			result.set = m_attributeCache[c].mosaics*24;
	} else if (m_attributeCache[c].mosaics && (result.code & 0x20))
		result.set = 24+m_attributeCache[c].separated;
	return result;
}

void Level1Layer::attributes(int r, int c, applyAttributes *layerApplyAttributes)
{
	unsigned char characterCode;

	if (m_rowHeight[r] == RHbottomhalf) {
		layerApplyAttributes->copyAboveAttributes = true;
		return;
	}
	if (r != m_rowCached)
		updateRowCache(r);
	if (c == 0 || c == 40 || c == 56) {
		// Start of row default conditions, also when crossing into side panels
		layerApplyAttributes->applyForeColour = true;
		layerApplyAttributes->attribute.foreColour = 0x07;
		layerApplyAttributes->applyBackColour = true;
		layerApplyAttributes->attribute.backColour = 0x20;
		layerApplyAttributes->applyDisplayAttributes = true;
		layerApplyAttributes->applyFlash = true;
		layerApplyAttributes->attribute.flash.mode = 0;
		layerApplyAttributes->attribute.flash.ratePhase = 0;
		layerApplyAttributes->attribute.display.doubleHeight = false;
		layerApplyAttributes->attribute.display.doubleWidth = false;
		layerApplyAttributes->attribute.display.boxingWindow = false;
		layerApplyAttributes->attribute.display.conceal = false;
		layerApplyAttributes->attribute.display.invert = false;
		layerApplyAttributes->attribute.display.underlineSeparated = false;
		layerApplyAttributes->attribute.display.forceContiguous = false;
		//TODO fontstyle
	}
	if (c > 39)
		return;
	if (c > 0) {
		// Set-after
		characterCode = m_levelOnePage->character(r, c-1);
		switch (characterCode) {
			case 0x00 ... 0x07: // Alphanumeric + Foreground colour
			case 0x10 ... 0x17: // Mosaic + Foreground colour
				layerApplyAttributes->applyForeColour = true;
				layerApplyAttributes->attribute.foreColour = characterCode & 0x07;
				layerApplyAttributes->attribute.display.conceal = false;
				break;
			case 0x08: // Flashing
				layerApplyAttributes->applyFlash = true;
				layerApplyAttributes->attribute.flash.mode = 1;
				layerApplyAttributes->attribute.flash.ratePhase = 0;
				break;
			case 0x0a: // End box
				if (m_levelOnePage->character(r, c) == 0x0a) {
					layerApplyAttributes->applyBoxingOnly = true;
					layerApplyAttributes->attribute.display.boxingWindow = false;
				}
				break;
			case 0x0b: // Start box
				if (m_levelOnePage->character(r, c) == 0x0b) {
					layerApplyAttributes->applyBoxingOnly = true;
					layerApplyAttributes->attribute.display.boxingWindow = true;
				}
				break;
			case 0x0d: // Double height
				layerApplyAttributes->applyTextSizeOnly = true;
				layerApplyAttributes->attribute.display.doubleHeight = true;
				layerApplyAttributes->attribute.display.doubleWidth = false;
				break;
			case 0x0e: // Double width
				layerApplyAttributes->applyTextSizeOnly = true;
				layerApplyAttributes->attribute.display.doubleHeight = false;
				layerApplyAttributes->attribute.display.doubleWidth = true;
				break;
			case 0x0f: // Double size
				layerApplyAttributes->applyTextSizeOnly = true;
				layerApplyAttributes->attribute.display.doubleHeight = true;
				layerApplyAttributes->attribute.display.doubleWidth = true;
				break;
		}
	}
	// Set-at
	characterCode = m_levelOnePage->character(r, c);
	switch (characterCode) {
		case 0x09: // Steady
			layerApplyAttributes->applyFlash = true;
			layerApplyAttributes->attribute.flash.mode = 0;
			layerApplyAttributes->attribute.flash.ratePhase = 0;
			break;
		case 0x0c: // Normal size
			layerApplyAttributes->applyTextSizeOnly = true;
			layerApplyAttributes->attribute.display.doubleHeight = false;
			layerApplyAttributes->attribute.display.doubleWidth = false;
			break;
		case 0x18: // Conceal
			layerApplyAttributes->applyConcealOnly = true;
			layerApplyAttributes->attribute.display.conceal = true;
			break;
		case 0x19: // Contiguous mosaics
			layerApplyAttributes->applyContiguousOnly = true;
			break;
		case 0x1c: // Black background
			layerApplyAttributes->applyBackColour = true;
			layerApplyAttributes->attribute.backColour = 0x20;
			break;
		case 0x1d: // New background
			layerApplyAttributes->applyBackColour = true;
			layerApplyAttributes->attribute.backColour = m_attributeCache[c].backColour;
			break;
	}
}


ActivePosition::ActivePosition()
{
	m_row = m_column = -1;
}

bool ActivePosition::setRow(int newRow)
{
	if (newRow < m_row)
		return false;
	if (newRow > m_row) {
		m_row = newRow;
		m_column = -1;
	}
	return true;
}

bool ActivePosition::setColumn(int newColumn)
{
	if (newColumn < m_column)
		return false;
	if (m_row == -1)
		m_row = 0;
	m_column = newColumn;
	return true;
}
/*
bool ActivePosition::setRowAndColumn(int newRow, int newColumn)
{
	if (!setRow(newRow))
		return false;
	return setColumn(newColumn);
}
*/
