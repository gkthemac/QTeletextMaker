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

#ifndef RENDER_H
#define RENDER_H

#include <QBitmap>
#include <QColor>
#include <QIcon>
#include <QImage>
#include <QPixmap>

#include "decode.h"

class TeletextFontBitmap
{
public:
	TeletextFontBitmap();
	~TeletextFontBitmap();

	QImage *image() const { return s_fontImage; }
	QPixmap charBitmap(int c, int s) const { return s_fontBitmap->copy((c-32)*12, s*10, 12, 10); }
	QIcon charIcon(int c, int s) const { return QIcon(charBitmap(c, s)); }

private:
	static int s_instances;
	static QBitmap* s_fontBitmap;
	static QImage* s_fontImage;
};

class TeletextPageRender : public QObject
{
	Q_OBJECT

public:
	enum RenderMode { RenderNormal, RenderMix, RenderWhiteOnBlack, RenderBlackOnWhite };

	TeletextPageRender();
	~TeletextPageRender();

	QImage* image(int i) const { return m_pageImage[i]; };
	RenderMode renderMode() const { return m_renderMode; };
	void setDecoder(TeletextPageDecode *decoder);
	void renderPage(bool force=false);
	bool showControlCodes() const { return m_showControlCodes; };

public slots:
	void colourChanged(int index);
	void setReveal(bool reveal);
	void setRenderMode(RenderMode renderMode);
	void setShowControlCodes(bool showControlCodes);

signals:
	void flashChanged(int newFlashHz);

protected:
	TeletextFontBitmap m_fontBitmap;
	QImage* m_pageImage[6];
	unsigned char m_controlCodeCache[25][40];
	RenderMode m_renderMode;
	bool m_reveal, m_showControlCodes;
	int m_flashBuffersHz;
	int m_flashingRow[25];

private:
	inline void drawFromBitmap(QPainter &painter, int r, int c, const QImage image, TeletextPageDecode::CharacterFragment characterFragment);
	inline void drawFromFontBitmap(QPainter &painter, int r, int c, unsigned char characterCode, int characterSet, TeletextPageDecode::CharacterFragment characterFragment);
	inline void drawCharacter(QPainter &painter, int r, int c, unsigned char characterCode, int characterSet, int characterDiacritical, TeletextPageDecode::CharacterFragment characterFragment);
	inline bool drawDRCSCharacter(QPainter &painter, int r, int c, TeletextPageDecode::DRCSSource drcsSource, int drcsSubTable, int drcsChar, TeletextPageDecode::CharacterFragment characterFragment, bool flashPhOn = true);
	inline void drawBoldOrItalicCharacter(QPainter &painter, int r, int c, unsigned char characterCode, int characterSet, TeletextPageDecode::CharacterFragment characterFragment);
	void renderRow(int r, int ph, bool force=false);
	void setRowFlashStatus(int r, int rowFlashHz);

	QColor m_foregroundQColor, m_backgroundQColor;
	TeletextPageDecode *m_decoder;
};

#endif
