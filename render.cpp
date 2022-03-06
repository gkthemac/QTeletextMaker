/*
 * Copyright (C) 2020-2022 Gavin MacGregor
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

#include <QBitmap>
#include <QPainter>
#include <QPixmap>

#include "render.h"

#include "decode.h"

int TeletextFontBitmap::s_instances = 0;

QBitmap *TeletextFontBitmap::s_fontBitmap = nullptr;

TeletextFontBitmap::TeletextFontBitmap()
{
	if (s_instances == 0)
		s_fontBitmap = new QBitmap(":/images/teletextfont.png");
	s_instances++;
}

TeletextFontBitmap::~TeletextFontBitmap()
{
	s_instances--;
	if (s_instances == 0)
		delete s_fontBitmap;
}


TeletextPageRender::TeletextPageRender()
{
	for (int i=0; i<6; i++)
		m_pagePixmap[i] = new QPixmap(864, 250);
	m_pagePixmap[0]->fill(Qt::transparent);

	m_reveal = false;
	m_mix = false;
	m_showControlCodes = false;
	m_flashBuffersHz = 0;

	for (int r=0; r<25; r++)
		for (int c=0; c<40; c++)
			m_controlCodeCache[r][c] = 0x7f;
}

TeletextPageRender::~TeletextPageRender()
{
	for (int i=0; i<6; i++)
		delete m_pagePixmap[i];
}

void TeletextPageRender::setDecoder(TeletextPageDecode *decoder)
{
	m_decoder = decoder;
}

inline void TeletextPageRender::drawFromFontBitmap(QPainter &pixmapPainter, int r, int c, unsigned char characterCode, int characterSet, TeletextPageDecode::CharacterFragment characterFragment)
{
	switch (characterFragment) {
		case TeletextPageDecode::NormalSize:
			pixmapPainter.drawPixmap(c*12, r*10, *m_fontBitmap.rawBitmap(), (characterCode-32)*12, characterSet*10, 12, 10);
			break;
		case TeletextPageDecode::DoubleHeightTopHalf:
			pixmapPainter.drawPixmap(c*12, r*10, 12, 10, *m_fontBitmap.rawBitmap(), (characterCode-32)*12, characterSet*10, 12, 5);
			break;
		case TeletextPageDecode::DoubleHeightBottomHalf:
			pixmapPainter.drawPixmap(c*12, r*10, 12, 10, *m_fontBitmap.rawBitmap(), (characterCode-32)*12, characterSet*10+5, 12, 5);
			break;
		case TeletextPageDecode::DoubleWidthLeftHalf:
			pixmapPainter.drawPixmap(c*12, r*10, 12, 10, *m_fontBitmap.rawBitmap(), (characterCode-32)*12, characterSet*10, 6, 10);
			break;
		case TeletextPageDecode::DoubleWidthRightHalf:
			pixmapPainter.drawPixmap(c*12, r*10, 12, 10, *m_fontBitmap.rawBitmap(), (characterCode-32)*12+6, characterSet*10, 6, 10);
			break;
		case TeletextPageDecode::DoubleSizeTopLeftQuarter:
			pixmapPainter.drawPixmap(c*12, r*10, 12, 10, *m_fontBitmap.rawBitmap(), (characterCode-32)*12, characterSet*10, 6, 5);
			break;
		case TeletextPageDecode::DoubleSizeTopRightQuarter:
			pixmapPainter.drawPixmap(c*12, r*10, 12, 10, *m_fontBitmap.rawBitmap(), (characterCode-32)*12+6, characterSet*10, 6, 5);
			break;
		case TeletextPageDecode::DoubleSizeBottomLeftQuarter:
			pixmapPainter.drawPixmap(c*12, r*10, 12, 10, *m_fontBitmap.rawBitmap(), (characterCode-32)*12, characterSet*10+5, 6, 5);
			break;
		case TeletextPageDecode::DoubleSizeBottomRightQuarter:
			pixmapPainter.drawPixmap(c*12, r*10, 12, 10, *m_fontBitmap.rawBitmap(), (characterCode-32)*12+6, characterSet*10+5, 6, 5);
			break;
	}
}

inline void TeletextPageRender::drawCharacter(QPainter &pixmapPainter, int r, int c, unsigned char characterCode, int characterSet, int characterDiacritical, TeletextPageDecode::CharacterFragment characterFragment)
{
	const bool dontUnderline = characterCode == 0x00;
	if (dontUnderline)
		characterCode = 0x20;

	// If either foreground or background is set to transparent
	// tinker with the QPainter settings so we get the desired result
	if (!pixmapPainter.background().isOpaque()) {
		if (pixmapPainter.pen().color().alpha() == 0) {
			// Transparent foreground and background
			pixmapPainter.setCompositionMode(QPainter::CompositionMode_Clear);
			pixmapPainter.eraseRect(c*12, r*10, 12, 10);
			pixmapPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

			return;
		} else
			// Transparent background, opaque foreground
			pixmapPainter.setCompositionMode(QPainter::CompositionMode_Source);
	} else if (pixmapPainter.pen().color().alpha() == 0) {
		// Transparent foreground, opaque background
		// Deal with optimising G1 solid 7/F blocks and spaces now
		// otherwise the same optimisations later on won't work with
		// our tinkered QPainter settings
		if (characterCode == 0x7f && characterSet == 24) {
			pixmapPainter.setCompositionMode(QPainter::CompositionMode_Clear);
			pixmapPainter.eraseRect(c*12, r*10, 12, 10);
			pixmapPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

			return;
		}

		pixmapPainter.fillRect(c*12, r*10, 12, 10, m_decoder->cellBackgroundQColor(r, c));

		if (characterCode == 0x20 && characterSet < 25 && characterDiacritical == 0)
			return;

		pixmapPainter.setBackground(QColor(0, 0, 0, 0));
		pixmapPainter.setPen(QColor(255, 255, 255, 255));
		pixmapPainter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
	}

	if (characterCode == 0x20 && characterSet < 25 && characterDiacritical == 0)
		pixmapPainter.fillRect(c*12, r*10, 12, 10, pixmapPainter.background().color());
	else if (characterCode == 0x7f && characterSet == 24)
		pixmapPainter.fillRect(c*12, r*10, 12, 10, pixmapPainter.pen().color());
	else
		drawFromFontBitmap(pixmapPainter, r, c, characterCode, characterSet, characterFragment);

	if (m_decoder->cellUnderlined(r, c) && !dontUnderline)
		switch (characterFragment) {
			case TeletextPageDecode::NormalSize:
			case TeletextPageDecode::DoubleWidthLeftHalf:
			case TeletextPageDecode::DoubleWidthRightHalf:
				pixmapPainter.drawLine(c*12, r*10+9, c*12+11, r*10+9);
				break;
			case TeletextPageDecode::DoubleHeightBottomHalf:
			case TeletextPageDecode::DoubleSizeBottomLeftQuarter:
			case TeletextPageDecode::DoubleSizeBottomRightQuarter:
				pixmapPainter.drawRect(c*12, r*10+8, 11, 1);
				break;
			default:
				break;
		}

	if (characterDiacritical != 0) {
		pixmapPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		pixmapPainter.setBackgroundMode(Qt::TransparentMode);
		drawFromFontBitmap(pixmapPainter, r, c, characterDiacritical+64, 7, characterFragment);
		pixmapPainter.setBackgroundMode(Qt::OpaqueMode);
	}

	if (pixmapPainter.compositionMode() != QPainter::CompositionMode_SourceOver)
		pixmapPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
}

void TeletextPageRender::renderPage(bool force)
{
	QPainter pixmapPainter[6];
	int previousFlashBuffersHz = m_flashBuffersHz;

	// If force-rendering (such as showing a new or different subpage) then
	// we don't render flashing cells initially as it greatly speeds things up
	// when rendering on top of an already flashing page.
	// At the end of this method, if flashing cells are present then this method
	// recursively calls itself with force=false to render the flashing cells.
	if (force) {
		m_flashBuffersHz = 0;
		previousFlashBuffersHz = 0;
		emit flashChanged(0);
	}

	pixmapPainter[0].begin(m_pagePixmap[0]);
	pixmapPainter[0].setBackgroundMode(Qt::OpaqueMode);
	if (m_flashBuffersHz != 0) {
		pixmapPainter[3].begin(m_pagePixmap[3]);
		pixmapPainter[3].setBackgroundMode(Qt::OpaqueMode);
		if (m_flashBuffersHz == 2) {
			pixmapPainter[1].begin(m_pagePixmap[1]);
			pixmapPainter[1].setBackgroundMode(Qt::OpaqueMode);
			pixmapPainter[2].begin(m_pagePixmap[2]);
			pixmapPainter[2].setBackgroundMode(Qt::OpaqueMode);
			pixmapPainter[4].begin(m_pagePixmap[4]);
			pixmapPainter[4].setBackgroundMode(Qt::OpaqueMode);
			pixmapPainter[5].begin(m_pagePixmap[5]);
			pixmapPainter[5].setBackgroundMode(Qt::OpaqueMode);
		}
	}

	for (int r=0; r<25; r++)
		for (int c=0; c<72; c++) {

			bool controlCodeChanged = false;

			// Ensure that shown control codes are refreshed
			if (m_showControlCodes && c < 40 && (m_controlCodeCache[r][c] != 0x7f || m_decoder->teletextPage()->character(r, c) < 0x20)) {
				controlCodeChanged = m_controlCodeCache[r][c] != m_decoder->teletextPage()->character(r, c);
				if (controlCodeChanged) {
					if (m_decoder->teletextPage()->character(r, c) < 0x20)
						m_controlCodeCache[r][c] = m_decoder->teletextPage()->character(r, c);
					else
						m_controlCodeCache[r][c] = 0x7f;
				}
			}

			if (m_decoder->refresh(r, c) || force || controlCodeChanged) {
				unsigned char characterCode;
				int characterSet, characterDiacritical;

				if (!m_reveal && m_decoder->cellConceal(r, c)) {
					characterCode = 0x20;
					characterSet = 0;
					characterDiacritical = 0;
				} else {
					characterCode = m_decoder->cellCharacterCode(r, c);
					characterSet = m_decoder->cellCharacterSet(r, c);
					characterDiacritical = m_decoder->cellCharacterDiacritical(r, c);
				}

				// QSet::insert won't insert a duplicate value already in the set
				// QSet::remove doesn't mind if we try to remove a value that's not there
				if (m_flashBuffersHz == 0 && m_decoder->cellFlashMode(r, c) != 0) {
					if (m_decoder->cellFlashRatePhase(r, c) == 0)
						m_flash1HzCells.insert(qMakePair(r, c));
					else
						m_flash2HzCells.insert(qMakePair(r, c));
					if (!force)
						updateFlashBuffers();
				} else if (m_decoder->cellFlashMode(r, c) == 0) {
					m_flash1HzCells.remove(qMakePair(r, c));
					m_flash2HzCells.remove(qMakePair(r, c));
					if (!force)
						updateFlashBuffers();
				} else if (m_decoder->cellFlashRatePhase(r, c) == 0) {
					m_flash1HzCells.insert(qMakePair(r, c));
					m_flash2HzCells.remove(qMakePair(r, c));
					if (!force)
						updateFlashBuffers();
				} else {
					m_flash1HzCells.remove(qMakePair(r, c));
					m_flash2HzCells.insert(qMakePair(r, c));
					if (!force)
						updateFlashBuffers();
				}

				// If flash rate has gone up, prepare painters for the other buffers
				if (m_flashBuffersHz > previousFlashBuffersHz) {
					if (previousFlashBuffersHz == 0) {
						pixmapPainter[3].begin(m_pagePixmap[3]);
						pixmapPainter[3].setBackgroundMode(Qt::OpaqueMode);
					}
					if (m_flashBuffersHz == 2) {
						pixmapPainter[1].begin(m_pagePixmap[1]);
						pixmapPainter[1].setBackgroundMode(Qt::OpaqueMode);
						pixmapPainter[2].begin(m_pagePixmap[2]);
						pixmapPainter[2].setBackgroundMode(Qt::OpaqueMode);
						pixmapPainter[4].begin(m_pagePixmap[4]);
						pixmapPainter[4].setBackgroundMode(Qt::OpaqueMode);
						pixmapPainter[5].begin(m_pagePixmap[5]);
						pixmapPainter[5].setBackgroundMode(Qt::OpaqueMode);
					}
					previousFlashBuffersHz = m_flashBuffersHz;
				}

				// If flash rate has gone down, end the painters so we don't crash
				// if the pixmaps get copied due to the flash rate going up again
				if (m_flashBuffersHz < previousFlashBuffersHz) {
					if (previousFlashBuffersHz == 2) {
						pixmapPainter[1].end();
						pixmapPainter[2].end();
						pixmapPainter[4].end();
						pixmapPainter[5].end();
					}
					if (m_flashBuffersHz == 0)
						pixmapPainter[3].end();

					previousFlashBuffersHz = m_flashBuffersHz;
				}

				if (m_flashBuffersHz == 0) {
					pixmapPainter[0].setPen(m_decoder->cellForegroundQColor(r, c));
					if (!m_mix || m_decoder->cellBoxed(r, c))
						pixmapPainter[0].setBackground(m_decoder->cellBackgroundQColor(r, c));
					else
						pixmapPainter[0].setBackground(Qt::transparent);
					drawCharacter(pixmapPainter[0], r, c, characterCode, characterSet, characterDiacritical, m_decoder->cellCharacterFragment(r, c));
				} else {
					for (int i=0; i<6; i++) {
						if (m_flashBuffersHz == 1 && (i == 1 || i == 2 || i == 4 || i == 5))
							continue;

						bool phaseOn;

						if (m_decoder->cellFlashRatePhase(r, c) == 0)
							phaseOn = (i < 3) ^ (m_decoder->cellFlashMode(r, c) == 2);
						else
							phaseOn = ((i == m_decoder->cellFlash2HzPhaseNumber(r, c)-1) || (i == m_decoder->cellFlash2HzPhaseNumber(r, c)+2)) ^ (m_decoder->cellFlashMode(r, c) == 2);

						if (!m_mix || m_decoder->cellBoxed(r, c))
							pixmapPainter[i].setBackground(m_decoder->cellBackgroundQColor(r, c));
						else
							pixmapPainter[i].setBackground(Qt::transparent);
						if (m_decoder->cellFlashMode(r, c) == 3 && !phaseOn)
							pixmapPainter[i].setPen(m_decoder->cellFlashForegroundQColor(r, c));
						else
							pixmapPainter[i].setPen(m_decoder->cellForegroundQColor(r, c));
						if ((m_decoder->cellFlashMode(r, c) == 1 || m_decoder->cellFlashMode(r, c) == 2) && !phaseOn)
							// Character 0x00 draws space without underline
							drawCharacter(pixmapPainter[i], r, c, 0x00, 0, 0, m_decoder->cellCharacterFragment(r, c));
						else
							drawCharacter(pixmapPainter[i], r, c, characterCode, characterSet, characterDiacritical, m_decoder->cellCharacterFragment(r, c));
					}
				}

				if (m_showControlCodes && c < 40 && m_decoder->teletextPage()->character(r, c) < 0x20) {
					pixmapPainter[0].setBackground(QColor(0, 0, 0, 128));
					pixmapPainter[0].setPen(QColor(255, 255, 255, 224));
					pixmapPainter[0].drawPixmap(c*12, r*10, *m_fontBitmap.rawBitmap(), (m_decoder->teletextPage()->character(r, c)+32)*12, 250, 12, 10);
					if (m_flashBuffersHz == 1) {
						pixmapPainter[3].setBackground(QColor(0, 0, 0, 128));
						pixmapPainter[3].setPen(QColor(255, 255, 255, 224));
						pixmapPainter[3].drawPixmap(c*12, r*10, *m_fontBitmap.rawBitmap(), (m_decoder->teletextPage()->character(r, c)+32)*12, 250, 12, 10);
					} else if (m_flashBuffersHz == 2)
						for (int i=1; i<6; i++) {
							pixmapPainter[i].setBackground(QColor(0, 0, 0, 128));
							pixmapPainter[i].setPen(QColor(255, 255, 255, 224));
							pixmapPainter[i].drawPixmap(c*12, r*10, *m_fontBitmap.rawBitmap(), (m_decoder->teletextPage()->character(r, c)+32)*12, 250, 12, 10);
						}
				}

				if (!force || m_decoder->cellFlashMode(r, c) == 0)
					m_decoder->setRefresh(r, c, false);
			}
		}

	pixmapPainter[0].end();
	if (m_flashBuffersHz != 0) {
		pixmapPainter[3].end();
		if (m_flashBuffersHz == 2) {
			pixmapPainter[1].end();
			pixmapPainter[2].end();
			pixmapPainter[4].end();
			pixmapPainter[5].end();
		}
	}

	if (force && (!m_flash1HzCells.isEmpty() || !m_flash2HzCells.isEmpty()))
		renderPage();
}

void TeletextPageRender::updateFlashBuffers()
{
	int highestFlashHz;

	if (!m_flash2HzCells.isEmpty())
		highestFlashHz = 2;
	else
		highestFlashHz = !m_flash1HzCells.isEmpty();

	if (highestFlashHz == m_flashBuffersHz)
		return;

	if (highestFlashHz > m_flashBuffersHz) {
		if (m_flashBuffersHz == 0)
			*m_pagePixmap[3] = m_pagePixmap[0]->copy();
		if (highestFlashHz == 2) {
			*m_pagePixmap[1] = m_pagePixmap[0]->copy();
			*m_pagePixmap[2] = m_pagePixmap[0]->copy();
			*m_pagePixmap[4] = m_pagePixmap[3]->copy();
			*m_pagePixmap[5] = m_pagePixmap[3]->copy();
		}
	}

	m_flashBuffersHz = highestFlashHz;
	emit flashChanged(m_flashBuffersHz);
}

void TeletextPageRender::colourChanged(int index)
{
	for (int r=0; r<25; r++)
		for (int c=0; c<72; c++) {
			if (m_decoder->cellForegroundCLUT(r, c) == index || m_decoder->cellBackgroundCLUT(r, c) == index || m_decoder->cellForegroundCLUT(r, c) == 8 || m_decoder->cellBackgroundCLUT(r, c) == 8)
				m_decoder->setRefresh(r, c, true);
			if (m_decoder->cellFlashMode(r, c) == 3 && ((m_decoder->cellForegroundCLUT(r, c) ^ 8) == index || (m_decoder->cellForegroundCLUT(r, c) ^ 8) == 8))
				m_decoder->setRefresh(r, c, true);
		}
}

void TeletextPageRender::setReveal(bool reveal)
{
	if (reveal == m_reveal)
		return;

	m_reveal = reveal;

	for (int r=0; r<25; r++)
		for (int c=0; c<72; c++)
			if (m_decoder->cellConceal(r, c))
				m_decoder->setRefresh(r, c, true);
}

void TeletextPageRender::setMix(bool mix)
{
	if (mix  == m_mix)
		return;

	m_mix = mix;

	for (int r=0; r<25; r++)
		for (int c=0; c<72; c++)
			if (!m_decoder->cellBoxed(r, c))
				m_decoder->setRefresh(r, c, true);
}


void TeletextPageRender::setShowControlCodes(bool showControlCodes)
{
	if (showControlCodes == m_showControlCodes)
		return;

	m_showControlCodes = showControlCodes;

	for (int r=0; r<25; r++)
		for (int c=0; c<40; c++)
			if (m_decoder->teletextPage()->character(r, c) < 0x20)
				m_decoder->setRefresh(r, c, true);
}
