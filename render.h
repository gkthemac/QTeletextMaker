/*
 * Copyright (C) 2020-2024 Gavin MacGregor
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
	bool mix() const { return m_mix; };
	void setDecoder(TeletextPageDecode *decoder);
	void renderPage(bool force=false);
	bool showControlCodes() const { return m_showControlCodes; };

public slots:
	void colourChanged(int index);
	void setReveal(bool reveal);
	void setMix(bool mix);
	void setShowControlCodes(bool showControlCodes);

signals:
	void flashChanged(int newFlashHz);

protected:
	TeletextFontBitmap m_fontBitmap;
	QPixmap* m_pagePixmap[6];
	unsigned char m_controlCodeCache[25][40];
	bool m_reveal, m_mix, m_showControlCodes;
	int m_flashBuffersHz;
	int m_flashingRow[25];

private:
	inline void drawFromBitmap(QPainter &, int, int, const QBitmap, TeletextPageDecode::CharacterFragment);
	inline void drawFromFontBitmap(QPainter &, int, int, unsigned char, int, TeletextPageDecode::CharacterFragment);
	inline void drawCharacter(QPainter &, int, int, unsigned char, int, int, TeletextPageDecode::CharacterFragment);
	inline void drawBoldOrItalicCharacter(QPainter &, int, int, unsigned char, int, TeletextPageDecode::CharacterFragment);
	void renderRow(int, int, bool force=false);
	void setRowFlashStatus(int, int);

	TeletextPageDecode *m_decoder;
};

#endif
