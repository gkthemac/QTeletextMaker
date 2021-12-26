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

void TeletextPageRender::renderPage()
{
	QPainter pixmapPainter;

	pixmapPainter.begin(m_pagePixmap[0]);
	pixmapPainter.setBackgroundMode(Qt::OpaqueMode);

	for (int r=0; r<25; r++)
		for (int c=0; c<72; c++)
			if (m_decoder->cellRefreshNeeded(r, c)) {
				const unsigned char characterCode = m_decoder->cellCharacterCode(r, c);
				int characterSet = m_decoder->cellCharacterSet(r, c);

				if (characterCode >= 32) {
					pixmapPainter.setPen(m_decoder->cellForegroundQColor(r, c));
					pixmapPainter.setBackground(m_decoder->cellBackgroundQColor(r, c));
					switch (m_decoder->cellCharacterFragment(r, c)) {
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
				m_decoder->clearRefresh(r, c);
			}

	pixmapPainter.end();
}
