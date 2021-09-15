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

void TeletextPageRender::renderPage(int rowUnused)
{
	QPainter pixmapPainter;

	pixmapPainter.begin(m_pagePixmap[0]);
	pixmapPainter.setBackgroundMode(Qt::OpaqueMode);

	pixmapPainter.setPen(QColor(255, 255, 255));
	pixmapPainter.setBackground(QColor(0, 0, 0));

	for (int r=0; r<25; r++)
		for (int c=0; c<72; c++)
			if (m_decoder->cellRefreshNeeded(r, c)) {
				const textCell cell = m_decoder->cell(r, c);

				if (cell.character.code >= 32)
					pixmapPainter.drawPixmap(c*12, r*10, 12, 10, *m_fontBitmap.rawBitmap(), (cell.character.code-32)*12, cell.character.set*10, 12, 10);
				else
					qDebug("R %d C %d code %d", r, c, cell.character.code);
				m_decoder->clearRefresh(r, c);
			}

	pixmapPainter.end();
}
