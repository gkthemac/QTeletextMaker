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
	m_showControlCodes = false;
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
	drawFromFontBitmap(pixmapPainter, r, c, characterCode, characterSet, characterFragment);

	if (characterDiacritical != 0) {
		pixmapPainter.setBackgroundMode(Qt::TransparentMode);
		drawFromFontBitmap(pixmapPainter, r, c, characterDiacritical+64, 7, characterFragment);
		pixmapPainter.setBackgroundMode(Qt::OpaqueMode);
	}
}

void TeletextPageRender::renderPage()
{
	QPainter pixmapPainter;

	pixmapPainter.begin(m_pagePixmap[0]);
	pixmapPainter.setBackgroundMode(Qt::OpaqueMode);

	for (int r=0; r<25; r++)
		for (int c=0; c<72; c++)
			if (m_decoder->refresh(r, c)) {
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

				pixmapPainter.setPen(m_decoder->cellForegroundQColor(r, c));
				pixmapPainter.setBackground(m_decoder->cellBackgroundQColor(r, c));

				drawCharacter(pixmapPainter, r, c, characterCode, characterSet, characterDiacritical, m_decoder->cellCharacterFragment(r, c));

				if (m_showControlCodes && c < 40 && m_decoder->teletextPage()->character(r, c) < 0x20) {
					pixmapPainter.setBackground(QColor(0, 0, 0, 128));
					pixmapPainter.setPen(QColor(255, 255, 255, 224));
					pixmapPainter.drawPixmap(c*12, r*10, *m_fontBitmap.rawBitmap(), (m_decoder->teletextPage()->character(r, c)+32)*12, 250, 12, 10);
				}

				m_decoder->setRefresh(r, c, false);
			}

	pixmapPainter.end();
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
