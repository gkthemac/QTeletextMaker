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

#ifndef RENDER_H
#define RENDER_H

#include <QBitmap>
#include <QPixmap>

#include "decode.h"

class TeletextFontBitmap
{
public:
	TeletextFontBitmap();
	~TeletextFontBitmap();

	QBitmap *rawBitmap() const { return s_fontBitmap; }

private:
	static int s_instances;
	static QBitmap* s_fontBitmap;
};

class TeletextPageRender : public QObject
{
	Q_OBJECT

public:
	TeletextPageRender();
	~TeletextPageRender();

	QPixmap* pagePixmap(int i) const { return m_pagePixmap[i]; };
	void setDecoder(TeletextPageDecode *);
	void renderPage(int rowUnused=0);

protected:
	TeletextFontBitmap m_fontBitmap;
	QPixmap* m_pagePixmap[6];
	textCell m_cell[25][72];

private:
	TeletextPageDecode *m_decoder;
};

#endif
