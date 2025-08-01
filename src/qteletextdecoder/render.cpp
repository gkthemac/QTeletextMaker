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

#include <QBitmap>
#include <QColor>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QPixmap>

#include "render.h"

#include "decode.h"

int TeletextFontBitmap::s_instances = 0;

QBitmap *TeletextFontBitmap::s_fontBitmap = nullptr;
QImage *TeletextFontBitmap::s_fontImage = nullptr;

TeletextFontBitmap::TeletextFontBitmap()
{
	Q_INIT_RESOURCE(teletextfonts);

	if (s_instances == 0) {
		s_fontBitmap = new QBitmap(":/fontimages/teletextfont.png");
		s_fontImage = new QImage(s_fontBitmap->toImage());
	}
	s_instances++;
}

TeletextFontBitmap::~TeletextFontBitmap()
{
	s_instances--;
	if (s_instances == 0) {
		delete s_fontImage;
		delete s_fontBitmap;
	}
}


TeletextPageRender::TeletextPageRender()
{
	for (int i=0; i<6; i++)
		m_pageImage[i] = new QImage(864, 250, QImage::Format_ARGB32_Premultiplied);

	m_reveal = false;
	m_renderMode = RenderNormal;
	m_showControlCodes = false;
	m_flashBuffersHz = 0;

	for (int r=0; r<25; r++) {
		m_flashingRow[r] = 0;
		for (int c=0; c<40; c++)
			m_controlCodeCache[r][c] = 0x7f;
	}
}

TeletextPageRender::~TeletextPageRender()
{
	for (int i=0; i<6; i++)
		delete m_pageImage[i];
}

void TeletextPageRender::setDecoder(TeletextPageDecode *decoder)
{
	m_decoder = decoder;
}

inline void TeletextPageRender::drawFromBitmap(QPainter &painter, int r, int c, const QImage image, TeletextPageDecode::CharacterFragment characterFragment)
{
	switch (characterFragment) {
		case TeletextPageDecode::NormalSize:
			painter.drawImage(c*12, r*10, image);
			break;
		case TeletextPageDecode::DoubleHeightTopHalf:
			painter.drawImage(QRect(c*12, r*10, 12, 10), image, QRect(0, 0, 12, 5));
			break;
		case TeletextPageDecode::DoubleHeightBottomHalf:
			painter.drawImage(QRect(c*12, r*10, 12, 10), image, QRect(0, 5, 12, 5));
			break;
		case TeletextPageDecode::DoubleWidthLeftHalf:
			painter.drawImage(QRect(c*12, r*10, 12, 10), image, QRect(0, 0, 6, 10));
			break;
		case TeletextPageDecode::DoubleWidthRightHalf:
			painter.drawImage(QRect(c*12, r*10, 12, 10), image, QRect(6, 0, 6, 10));
			break;
		case TeletextPageDecode::DoubleSizeTopLeftQuarter:
			painter.drawImage(QRect(c*12, r*10, 12, 10), image, QRect(0, 0, 6, 5));
			break;
		case TeletextPageDecode::DoubleSizeTopRightQuarter:
			painter.drawImage(QRect(c*12, r*10, 12, 10), image, QRect(6, 0, 6, 5));
			break;
		case TeletextPageDecode::DoubleSizeBottomLeftQuarter:
			painter.drawImage(QRect(c*12, r*10, 12, 10), image, QRect(0, 5, 6, 5));
			break;
		case TeletextPageDecode::DoubleSizeBottomRightQuarter:
			painter.drawImage(QRect(c*12, r*10, 12, 10), image, QRect(6, 5, 6, 5));
			break;
	}
}

inline void TeletextPageRender::drawFromFontBitmap(QPainter &painter, int r, int c, unsigned char characterCode, int characterSet, TeletextPageDecode::CharacterFragment characterFragment)
{
	switch (characterFragment) {
		case TeletextPageDecode::NormalSize:
			painter.drawImage(c*12, r*10, *m_fontBitmap.image(), (characterCode-32)*12, characterSet*10, 12, 10);
			break;
		case TeletextPageDecode::DoubleHeightTopHalf:
			painter.drawImage(QRect(c*12, r*10, 12, 10), *m_fontBitmap.image(), QRect((characterCode-32)*12, characterSet*10, 12, 5));
			break;
		case TeletextPageDecode::DoubleHeightBottomHalf:
			painter.drawImage(QRect(c*12, r*10, 12, 10), *m_fontBitmap.image(), QRect((characterCode-32)*12, characterSet*10+5, 12, 5));
			break;
		case TeletextPageDecode::DoubleWidthLeftHalf:
			painter.drawImage(QRect(c*12, r*10, 12, 10), *m_fontBitmap.image(), QRect((characterCode-32)*12, characterSet*10, 6, 10));
			break;
		case TeletextPageDecode::DoubleWidthRightHalf:
			painter.drawImage(QRect(c*12, r*10, 12, 10), *m_fontBitmap.image(), QRect((characterCode-32)*12+6, characterSet*10, 6, 10));
			break;
		case TeletextPageDecode::DoubleSizeTopLeftQuarter:
			painter.drawImage(QRect(c*12, r*10, 12, 10), *m_fontBitmap.image(), QRect((characterCode-32)*12, characterSet*10, 6, 5));
			break;
		case TeletextPageDecode::DoubleSizeTopRightQuarter:
			painter.drawImage(QRect(c*12, r*10, 12, 10), *m_fontBitmap.image(), QRect((characterCode-32)*12+6, characterSet*10, 6, 5));
			break;
		case TeletextPageDecode::DoubleSizeBottomLeftQuarter:
			painter.drawImage(QRect(c*12, r*10, 12, 10), *m_fontBitmap.image(), QRect((characterCode-32)*12, characterSet*10+5, 6, 5));
			break;
		case TeletextPageDecode::DoubleSizeBottomRightQuarter:
			painter.drawImage(QRect(c*12, r*10, 12, 10), *m_fontBitmap.image(), QRect((characterCode-32)*12+6, characterSet*10+5, 6, 5));
			break;
	}
}

inline void TeletextPageRender::drawCharacter(QPainter &painter, int r, int c, unsigned char characterCode, int characterSet, int characterDiacritical, TeletextPageDecode::CharacterFragment characterFragment)
{
	const bool dontUnderline = characterCode == 0x00;
	if (dontUnderline)
		characterCode = 0x20;

	// Set 24 has reduced height Latin G0 capital letters for diacritical marks to sit on top of
	if (characterDiacritical != 0  && // Not for no-diacritical-mark
	    characterDiacritical != 9  && // Not for these diacriticals that go under the letter
	    characterDiacritical != 11 &&
	    characterDiacritical != 12 &&
	    characterDiacritical != 14 &&
	    (characterSet == 0 || characterSet == 6) &&     // Only for Latin G0 and Hebrew G0
	    characterCode >= 0x41 && characterCode <= 0x5a) // and only for the capital letters A-Z
		characterSet = 24;

	if (characterCode == 0x20 && characterSet < 25 && characterDiacritical == 0)
		painter.fillRect(c*12, r*10, 12, 10, m_backgroundQColor);
	else if (characterCode == 0x7f && characterSet == 24)
		painter.fillRect(c*12, r*10, 12, 10, m_foregroundQColor);
	else if ((m_decoder->cellBold(r, c) || m_decoder->cellItalic(r, c)))
		drawBoldOrItalicCharacter(painter, r, c, characterCode, characterSet, characterFragment);
	else {
		m_fontBitmap.image()->setColorTable(QList<QRgb>{m_backgroundQColor.rgba(), m_foregroundQColor.rgba()});
		drawFromFontBitmap(painter, r, c, characterCode, characterSet, characterFragment);
	}

	if (m_decoder->cellUnderlined(r, c) && !dontUnderline) {
		painter.setPen(m_foregroundQColor);
		switch (characterFragment) {
			case TeletextPageDecode::NormalSize:
			case TeletextPageDecode::DoubleWidthLeftHalf:
			case TeletextPageDecode::DoubleWidthRightHalf:
				painter.drawLine(c*12, r*10+9, c*12+11, r*10+9);
				break;
			case TeletextPageDecode::DoubleHeightBottomHalf:
			case TeletextPageDecode::DoubleSizeBottomLeftQuarter:
			case TeletextPageDecode::DoubleSizeBottomRightQuarter:
				painter.drawRect(c*12, r*10+8, 11, 1);
				break;
			default:
				break;
		}
	}

	if (characterDiacritical != 0) {
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		m_fontBitmap.image()->setColorTable(QList<QRgb>{0x00000000, m_foregroundQColor.rgba()});
		drawFromFontBitmap(painter, r, c, characterDiacritical+64, 7, characterFragment);
		painter.setCompositionMode(QPainter::CompositionMode_Source);
	}
}

inline bool TeletextPageRender::drawDRCSCharacter(QPainter &painter, int r, int c, TeletextPageDecode::DRCSSource drcsSource, int drcsSubTable, int drcsChar, TeletextPageDecode::CharacterFragment characterFragment, bool flashPhOn)
{
	QImage drcsImage = m_decoder->drcsImage(drcsSource, drcsSubTable, drcsChar, flashPhOn);

	if (drcsImage.isNull())
		return false;

	if (drcsImage.format() == QImage::Format_Mono)
		// mode 0 (12x10x1) returned here has no colours of its own
		// so apply the foreground and background colours of the cell it appears in
		drcsImage.setColorTable(QVector<QRgb>{m_backgroundQColor.rgba(), m_foregroundQColor.rgba()});
	else if (m_renderMode >= RenderWhiteOnBlack)
		// modes 1-3: crudely convert colours to monochrome
		for (int i=0; i<16; i++)
			drcsImage.setColor(i, qGray(drcsImage.color(i)) > 127 ? 0xffffffff : 0xff000000);

	drawFromBitmap(painter, r, c, drcsImage, characterFragment);

	return true;
}

inline void TeletextPageRender::drawBoldOrItalicCharacter(QPainter &painter, int r, int c, unsigned char characterCode, int characterSet, TeletextPageDecode::CharacterFragment characterFragment)
{
	QImage styledImage = QImage(12, 10, QImage::Format_Mono);
	QPainter styledPainter;

	// Don't apply style to mosaics
	const bool mosaic = characterSet > 24 || (characterSet == 24 && (characterCode < 0x41 || characterCode > 0x5a));

	m_fontBitmap.image()->setColorTable(QList<QRgb>{m_backgroundQColor.rgba(), m_foregroundQColor.rgba()});
	styledImage.setColorTable(QList<QRgb>{m_backgroundQColor.rgba(), m_foregroundQColor.rgba()});

	if (!mosaic && m_decoder->cellItalic(r, c)) {
		styledImage.fill(0);

		styledPainter.begin(&styledImage);
		styledPainter.setCompositionMode(QPainter::CompositionMode_Source);
		styledPainter.drawImage(1, 0, *m_fontBitmap.image(), (characterCode-32)*12, characterSet*10, 11, 3);
		styledPainter.drawImage(0, 3, *m_fontBitmap.image(), (characterCode-32)*12, characterSet*10+3, 12, 3);
		styledPainter.drawImage(0, 6, *m_fontBitmap.image(), (characterCode-32)*12+1, characterSet*10+6, 11, 4);
		styledPainter.end();
	} else
		styledImage = m_fontBitmap.image()->copy((characterCode-32)*12, characterSet*10, 12, 10);

	// We have either an unstyled or italic character. Now bolden if needed.
	if (!mosaic && m_decoder->cellBold(r, c)) {
		QImage boldeningImage;

		boldeningImage = styledImage.copy();
		styledPainter.begin(&styledImage);
		styledPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		boldeningImage.setColorTable(QList<QRgb>{0x00000000, m_foregroundQColor.rgba()});
		styledPainter.drawImage(1, 0, boldeningImage);
		styledPainter.end();
	}
	drawFromBitmap(painter, r, c, styledImage, characterFragment);
}

void TeletextPageRender::renderPage(bool force)
{
	if (m_renderMode == RenderWhiteOnBlack) {
		m_foregroundQColor = Qt::white;
		m_backgroundQColor = Qt::black;
	} else if (m_renderMode == RenderBlackOnWhite) {
		m_foregroundQColor = Qt::black;
		m_backgroundQColor = Qt::white;
	}
	for (int r=0; r<25; r++)
		renderRow(r, 0, force);
}

void TeletextPageRender::renderRow(int r, int ph, bool force)
{
	QPainter painter;
	int flashingRow = 0;
	bool rowRefreshed = false;

	painter.begin(m_pageImage[ph]);
	painter.setCompositionMode(QPainter::CompositionMode_Source);

	for (int c=0; c<72; c++) {
		bool controlCodeChanged = false;

		// Ensure that shown control codes are refreshed
		if (ph == 0 && m_showControlCodes && c < 40 && (m_controlCodeCache[r][c] != 0x7f || m_decoder->teletextPage()->character(r, c) < 0x20)) {
			controlCodeChanged = m_controlCodeCache[r][c] != m_decoder->teletextPage()->character(r, c);
			if (controlCodeChanged) {
				if (m_decoder->teletextPage()->character(r, c) < 0x20)
					m_controlCodeCache[r][c] = m_decoder->teletextPage()->character(r, c);
				else
					m_controlCodeCache[r][c] = 0x7f;
			}
		}

		// Second part of "if" suppresses all flashing on monochrome render modes
		if (ph == 0 && m_renderMode < RenderWhiteOnBlack) {
			if (m_decoder->cellFlashMode(r, c) != 0)
				flashingRow = qMax(flashingRow, (m_decoder->cellFlashRatePhase(r, c) == 0) ? 1 : 2);
//		} else if (!force)
		} else
			force = m_decoder->cellFlashMode(r, c) != 0;

		// If drawing into a flash pixmap buffer, "force" is set on a flashing cell only
		// and since the refresh and controlCodeChanged variables will be false at this point
		// only flashing cells will be drawn
		if (m_decoder->refresh(r, c) || force || controlCodeChanged) {
			bool flashPhOn = true; // Must remain "true" on non-flashing cell
			const bool concealed = !m_reveal && m_decoder->cellConceal(r, c);

			rowRefreshed = true;

			if (m_renderMode < RenderWhiteOnBlack) {
				if (m_decoder->cellFlashMode(r, c) == 0)
					m_foregroundQColor = m_decoder->cellForegroundQColor(r, c);
				else {
					// Flashing cell, decide if phase in this cycle is on or off
					if (m_decoder->cellFlashRatePhase(r, c) == 0)
						flashPhOn = (ph < 3) ^ (m_decoder->cellFlashMode(r, c) == 2);
					else
						flashPhOn = ((ph == m_decoder->cellFlash2HzPhaseNumber(r, c)-1) || (ph == m_decoder->cellFlash2HzPhaseNumber(r, c)+2)) ^ (m_decoder->cellFlashMode(r, c) == 2);

					// If flashing to adjacent CLUT select the appropriate foreground colour
					if (m_decoder->cellFlashMode(r, c) == 3 && !flashPhOn)
						m_foregroundQColor = m_decoder->cellFlashForegroundQColor(r, c);
					else
						m_foregroundQColor = m_decoder->cellForegroundQColor(r, c);
				}

				if (m_renderMode != RenderMix || m_decoder->cellBoxed(r, c))
					m_backgroundQColor = m_decoder->cellBackgroundQColor(r, c);
				else
					m_backgroundQColor = Qt::transparent;
			}

			if (((m_decoder->cellFlashMode(r, c) == 1 || m_decoder->cellFlashMode(r, c) == 2) && !flashPhOn))
				// If flashing mode is Normal or Invert, draw a space instead of a character on phase
				// Character 0x00 draws space without underline
				drawCharacter(painter, r, c, 0x00, 0, 0, m_decoder->cellCharacterFragment(r, c));
			else if (concealed)
				drawCharacter(painter, r, c, 0x20, 0, 0, m_decoder->cellCharacterFragment(r, c));
			else if (m_decoder->cellDrcsSource(r, c) == TeletextPageDecode::NoDRCS || !drawDRCSCharacter(painter, r, c, m_decoder->cellDrcsSource(r, c), m_decoder->cellDrcsSubTable(r, c), m_decoder->cellDrcsCharacter(r, c), m_decoder->cellCharacterFragment(r, c), flashPhOn))
				drawCharacter(painter, r, c, m_decoder->cellCharacterCode(r, c), m_decoder->cellCharacterSet(r, c), m_decoder->cellCharacterDiacritical(r, c), m_decoder->cellCharacterFragment(r, c));

			if (m_showControlCodes && c < 40 && m_decoder->teletextPage()->character(r, c) < 0x20) {
				painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
				m_fontBitmap.image()->setColorTable(QList<QRgb>{0x7f000000, 0xe0ffffff});
				painter.drawImage(c*12, r*10, *m_fontBitmap.image(), (m_decoder->teletextPage()->character(r, c)+32)*12, 250, 12, 10);
				painter.setCompositionMode(QPainter::CompositionMode_Source);
			}
		}
	}

	painter.end();

	if (ph != 0)
		return;

	if (flashingRow == 3)
		flashingRow = 2;
	if (flashingRow != m_flashingRow[r])
		setRowFlashStatus(r, flashingRow);

	for (int c=0; c<72; c++)
		m_decoder->setRefresh(r, c, false);

	// If row had changes rendered and flashing is present anywhere on the screen,
	// copy this rendered line into the other flash pixmap buffers and then re-render
	// the flashing cells in those buffers
	if (rowRefreshed && m_flashBuffersHz > 0) {
		painter.begin(m_pageImage[3]);
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.drawImage(0, r*10, *m_pageImage[0], 0, r*10, 864, 10);
		painter.end();

		renderRow(r, 3);

		if (m_flashBuffersHz == 2) {
			painter.begin(m_pageImage[1]);
			painter.setCompositionMode(QPainter::CompositionMode_Source);
			painter.drawImage(0, r*10, *m_pageImage[0], 0, r*10, 864, 10);
			painter.end();
			painter.begin(m_pageImage[2]);
			painter.setCompositionMode(QPainter::CompositionMode_Source);
			painter.drawImage(0, r*10, *m_pageImage[0], 0, r*10, 864, 10);
			painter.end();
			painter.begin(m_pageImage[4]);
			painter.setCompositionMode(QPainter::CompositionMode_Source);
			painter.drawImage(0, r*10, *m_pageImage[3], 0, r*10, 864, 10);
			painter.end();
			painter.begin(m_pageImage[5]);
			painter.setCompositionMode(QPainter::CompositionMode_Source);
			painter.drawImage(0, r*10, *m_pageImage[3], 0, r*10, 864, 10);
			painter.end();

			renderRow(r, 1);
			renderRow(r, 2);
			renderRow(r, 4);
			renderRow(r, 5);
		}
	}
}

void TeletextPageRender::setRowFlashStatus(int r, int rowFlashHz)
{
	m_flashingRow[r] = rowFlashHz;

	if (rowFlashHz == m_flashBuffersHz)
		return;

	if (rowFlashHz < m_flashBuffersHz) {
		// New flash Hz for this row is lower than the entire screen flash Hz
		// Check the other rows if they still need flashing at the current flash Hz
		// If not, reduce the screen flash Hz
		int highestRowFlashHz = rowFlashHz;

		for (int ri=0; ri<25; ri++)
			if (m_flashingRow[ri] > highestRowFlashHz) {
				highestRowFlashHz = m_flashingRow[ri];
				if (highestRowFlashHz == 2)
					break;
			}

		if (highestRowFlashHz > rowFlashHz)
			rowFlashHz = highestRowFlashHz;
		if (rowFlashHz == m_flashBuffersHz)
			return;

		m_flashBuffersHz = rowFlashHz;
		emit flashChanged(m_flashBuffersHz);
		return;
	}

	// If we get here, new flash Hz for this row is higher than the entire flash Hz
	// so prepare the pixmap flash buffers
	if (m_flashBuffersHz == 0)
		*m_pageImage[3] = m_pageImage[0]->copy();
	if (rowFlashHz == 2) {
		*m_pageImage[1] = m_pageImage[0]->copy();
		*m_pageImage[2] = m_pageImage[0]->copy();
		*m_pageImage[4] = m_pageImage[3]->copy();
		*m_pageImage[5] = m_pageImage[3]->copy();
	}

	m_flashBuffersHz = rowFlashHz;
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

	if (m_decoder->level() == 3)
		// TODO don't refresh mode 0 DRCS
		for (int r=0; r<25; r++)
			for (int c=0; c<72; c++)
				if (m_decoder->cellDrcsSource(r, c) != TeletextPageDecode::NoDRCS)
					m_decoder->setRefresh(r, c, true);
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

void TeletextPageRender::setRenderMode(RenderMode renderMode)
{
	if (renderMode == m_renderMode)
		return;

	m_renderMode = renderMode;

	for (int r=0; r<25; r++)
		for (int c=0; c<72; c++)
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
