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

#include <QApplication>
#include <QByteArray>
#include <QByteArrayList>
#include <QClipboard>
#include <QImage>
#include <QMimeData>
#include <QRegularExpression>

#include "levelonecommands.h"

#include "document.h"
#include "keymap.h"

LevelOneCommand::LevelOneCommand(TeletextDocument *teletextDocument, QUndoCommand *parent) : QUndoCommand(parent)
{
	m_teletextDocument = teletextDocument;
	m_subPageIndex = teletextDocument->currentSubPageIndex();
	m_row = teletextDocument->cursorRow();
	m_column = teletextDocument->cursorColumn();
	m_firstDo = true;
}


StoreOldCharactersCommand::StoreOldCharactersCommand(TeletextDocument *teletextDocument, QUndoCommand *parent) : LevelOneCommand(teletextDocument, parent)
{
}

void StoreOldCharactersCommand::storeOldCharacters(int topRow, int leftColumn, int bottomRow, int rightColumn)
{
	for (int r=topRow; r<=bottomRow; r++) {
		QByteArray rowArray;

		for (int c=leftColumn; c<=rightColumn; c++)
			// Guard against size of pasted block going beyond last line or column
			if (r < 25 && c < 40)
				rowArray.append(m_teletextDocument->currentSubPage()->character(r, c));
			else
				// Gone beyond last line or column - store a filler character which we won't see
				// Not sure if this is really necessary as out-of-bounds access might not occur?
				rowArray.append(0x7f);

		m_oldCharacters.append(rowArray);
	}
}

void StoreOldCharactersCommand::retrieveOldCharacters(int topRow, int leftColumn, int bottomRow, int rightColumn)
{
	int arrayR = 0;
	int arrayC;

	for (int r=topRow; r<=bottomRow; r++) {
		arrayC = 0;
		for (int c=leftColumn; c<=rightColumn; c++)
			// Guard against size of pasted block going beyond last line or column
			if (r < 25 && c < 40) {
				m_teletextDocument->currentSubPage()->setCharacter(r, c, m_oldCharacters[arrayR].at(arrayC));

				arrayC++;
			}

		arrayR++;
	}
}


TypeCharacterCommand::TypeCharacterCommand(TeletextDocument *teletextDocument, unsigned char newCharacter, bool insertMode, QUndoCommand *parent) : LevelOneCommand(teletextDocument, parent)
{
	m_columnStart = m_columnEnd = m_column;
	m_newCharacter = newCharacter;
	m_insertMode = insertMode;

	for (int c=0; c<40; c++)
		m_oldRowContents[c] = m_newRowContents[c] = m_teletextDocument->currentSubPage()->character(m_row, c);

	if (m_insertMode)
		setText(QObject::tr("insert character"));
	else
		setText(QObject::tr("overwrite character"));
}

void TypeCharacterCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);

	// Only apply the typed character on the first do, m_newRowContents will remember it if we redo
	if (m_firstDo) {
		if (m_insertMode) {
			// Insert - Move characters rightwards
			for (int c=39; c>m_columnEnd; c--)
				m_newRowContents[c] = m_newRowContents[c-1];
		}
		m_newRowContents[m_columnEnd] = m_newCharacter;
		m_firstDo = false;
	}
	for (int c=0; c<40; c++)
		m_teletextDocument->currentSubPage()->setCharacter(m_row, c, m_newRowContents[c]);

	m_teletextDocument->moveCursor(m_row, m_columnEnd);
	m_teletextDocument->cursorRight();
	emit m_teletextDocument->contentsChanged();
}

void TypeCharacterCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);

	for (int c=0; c<40; c++)
		m_teletextDocument->currentSubPage()->setCharacter(m_row, c, m_oldRowContents[c]);

	m_teletextDocument->moveCursor(m_row, m_columnStart);
	emit m_teletextDocument->contentsChanged();
}

bool TypeCharacterCommand::mergeWith(const QUndoCommand *command)
{
	const TypeCharacterCommand *newerCommand = static_cast<const TypeCharacterCommand *>(command);

	// Has to be the next typed column on the same row
	if (m_subPageIndex != newerCommand->m_subPageIndex || m_row != newerCommand->m_row || m_columnEnd != newerCommand->m_columnEnd-1)
		return false;

	m_columnEnd = newerCommand->m_columnEnd;
	for (int c=0; c<40; c++)
		m_newRowContents[c] = newerCommand->m_newRowContents[c];

	return true;
}


ToggleMosaicBitCommand::ToggleMosaicBitCommand(TeletextDocument *teletextDocument, unsigned char bitToToggle, QUndoCommand *parent) : LevelOneCommand(teletextDocument, parent)
{
	m_oldCharacter = teletextDocument->currentSubPage()->character(m_row, m_column);

	if (bitToToggle == 0x20 || bitToToggle == 0x7f)
		// Clear or fill the whole mosaic character
		m_newCharacter = bitToToggle;
	else if (bitToToggle == 0x66)
		// Dither
		m_newCharacter = (m_row & 1) ? 0x66 : 0x39;
	else if (m_oldCharacter & 0x20)
		// Previous character was mosaic, just toggle the bit(s)
		m_newCharacter = m_oldCharacter ^ bitToToggle;
	else
		// Previous character was blast-through, change to mosaic and set bit alone
		m_newCharacter = bitToToggle | 0x20;

	setText(QObject::tr("mosaic"));

}

void ToggleMosaicBitCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setCharacter(m_row, m_column, m_newCharacter);

	m_teletextDocument->moveCursor(m_row, m_column);
	emit m_teletextDocument->contentsChanged();
}

void ToggleMosaicBitCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setCharacter(m_row, m_column, m_oldCharacter);

	m_teletextDocument->moveCursor(m_row, m_column);
	emit m_teletextDocument->contentsChanged();
}

bool ToggleMosaicBitCommand::mergeWith(const QUndoCommand *command)
{
	const ToggleMosaicBitCommand *newerCommand = static_cast<const ToggleMosaicBitCommand *>(command);

	if (m_subPageIndex != newerCommand->m_subPageIndex || m_row != newerCommand->m_row || m_column != newerCommand->m_column)
		return false;
	m_newCharacter = newerCommand->m_newCharacter;
	return true;
}


BackspaceKeyCommand::BackspaceKeyCommand(TeletextDocument *teletextDocument, bool insertMode, QUndoCommand *parent) : LevelOneCommand(teletextDocument, parent)
{
	m_columnStart = m_column - 1;

	if (m_columnStart == -1) {
		m_columnStart = 39;
		if (--m_row == 0)
			m_row = 24;
	}
	m_columnEnd = m_columnStart;
	m_insertMode = insertMode;

	for (int c=0; c<40; c++)
		m_oldRowContents[c] = m_newRowContents[c] = m_teletextDocument->currentSubPage()->character(m_row, c);

	setText(QObject::tr("backspace"));
}

void BackspaceKeyCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);

	if (m_firstDo) {
		if (m_insertMode) {
			// Insert - Move characters leftwards and put a space on the far right
			for (int c=m_columnEnd; c<39; c++)
				m_newRowContents[c] = m_newRowContents[c+1];
			m_newRowContents[39] = 0x20;
		} else
			// Replace - Overwrite backspaced character with a space
			m_newRowContents[m_columnEnd] = 0x20;
		m_firstDo = false;
	}

	for (int c=0; c<40; c++)
		m_teletextDocument->currentSubPage()->setCharacter(m_row, c, m_newRowContents[c]);

	m_teletextDocument->moveCursor(m_row, m_columnEnd);
	emit m_teletextDocument->contentsChanged();
}

void BackspaceKeyCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);

	for (int c=0; c<40; c++)
		m_teletextDocument->currentSubPage()->setCharacter(m_row, c, m_oldRowContents[c]);

	m_teletextDocument->moveCursor(m_row, m_columnStart);
	m_teletextDocument->cursorRight();
	emit m_teletextDocument->contentsChanged();
}

bool BackspaceKeyCommand::mergeWith(const QUndoCommand *command)
{
	const BackspaceKeyCommand *newerCommand = static_cast<const BackspaceKeyCommand *>(command);

	// Has to be the next backspaced column on the same row
	if (m_subPageIndex != newerCommand->m_subPageIndex || m_row != newerCommand->m_row || m_columnEnd != newerCommand->m_columnEnd+1)
		return false;

	// For backspacing m_columnStart is where we began backspacing and m_columnEnd is where we ended backspacing
	// so m_columnEnd will be less than m_columnStart
	m_columnEnd = newerCommand->m_columnEnd;
	for (int c=0; c<40; c++)
		m_newRowContents[c] = newerCommand->m_newRowContents[c];

	return true;
}


DeleteKeyCommand::DeleteKeyCommand(TeletextDocument *teletextDocument, QUndoCommand *parent) : LevelOneCommand(teletextDocument, parent)
{
	for (int c=0; c<40; c++)
		m_oldRowContents[c] = m_newRowContents[c] = m_teletextDocument->currentSubPage()->character(m_row, c);

	setText(QObject::tr("delete"));
}

void DeleteKeyCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);

	// Move characters leftwards and put a space on the far right
	for (int c=m_column; c<39; c++)
		m_newRowContents[c] = m_newRowContents[c+1];
	m_newRowContents[39] = 0x20;

	for (int c=0; c<40; c++)
		m_teletextDocument->currentSubPage()->setCharacter(m_row, c, m_newRowContents[c]);

	m_teletextDocument->moveCursor(m_row, m_column);
	emit m_teletextDocument->contentsChanged();
}

void DeleteKeyCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);

	for (int c=0; c<40; c++)
		m_teletextDocument->currentSubPage()->setCharacter(m_row, c, m_oldRowContents[c]);

	m_teletextDocument->moveCursor(m_row, m_column);
	emit m_teletextDocument->contentsChanged();
}

bool DeleteKeyCommand::mergeWith(const QUndoCommand *command)
{
	const DeleteKeyCommand *newerCommand = static_cast<const DeleteKeyCommand *>(command);

	if (m_subPageIndex != newerCommand->m_subPageIndex || m_row != newerCommand->m_row || m_column != newerCommand->m_column)
		return false;

	for (int c=0; c<40; c++)
		m_newRowContents[c] = newerCommand->m_newRowContents[c];

	return true;
}


InsertRowCommand::InsertRowCommand(TeletextDocument *teletextDocument, bool copyRow, QUndoCommand *parent) : LevelOneCommand(teletextDocument, parent)
{
	m_copyRow = copyRow;

	if (m_copyRow)
		setText(QObject::tr("insert copy row"));
	else
		setText(QObject::tr("insert blank row"));
}

void InsertRowCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->moveCursor(m_row, -1);
	// Store copy of the bottom row we're about to push out, for undo
	for (int c=0; c<40; c++)
		m_deletedBottomRow[c] = m_teletextDocument->currentSubPage()->character(23, c);
	// Move lines below the inserting row downwards without affecting the FastText row
	for (int r=22; r>=m_row; r--)
		for (int c=0; c<40; c++)
			m_teletextDocument->currentSubPage()->setCharacter(r+1, c, m_teletextDocument->currentSubPage()->character(r, c));
	if (!m_copyRow)
		// The above movement leaves a duplicate of the current row, so blank it if requested
		for (int c=0; c<40; c++)
			m_teletextDocument->currentSubPage()->setCharacter(m_row, c, ' ');

	emit m_teletextDocument->contentsChanged();
}

void InsertRowCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->moveCursor(m_row, -1);
	// Move lines below the deleting row upwards without affecting the FastText row
	for (int r=m_row; r<23; r++)
		for (int c=0; c<40; c++)
			m_teletextDocument->currentSubPage()->setCharacter(r, c, m_teletextDocument->currentSubPage()->character(r+1, c));
	// Now repair the bottom row we pushed out
	for (int c=0; c<40; c++)
		m_teletextDocument->currentSubPage()->setCharacter(23, c, m_deletedBottomRow[c]);

	emit m_teletextDocument->contentsChanged();
}


DeleteRowCommand::DeleteRowCommand(TeletextDocument *teletextDocument, QUndoCommand *parent) : LevelOneCommand(teletextDocument, parent)
{
	setText(QObject::tr("delete row"));
}

void DeleteRowCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->moveCursor(m_row, -1);
	// Store copy of the row we're going to delete, for undo
	for (int c=0; c<40; c++)
		m_deletedRow[c] = m_teletextDocument->currentSubPage()->character(m_row, c);
	// Move lines below the deleting row upwards without affecting the FastText row
	for (int r=m_row; r<23; r++)
		for (int c=0; c<40; c++)
			m_teletextDocument->currentSubPage()->setCharacter(r, c, m_teletextDocument->currentSubPage()->character(r+1, c));
	// If we deleted the FastText row blank that row, otherwise blank the last row
	int blankingRow = (m_row < 24) ? 23 : 24;
		for (int c=0; c<40; c++)
			m_teletextDocument->currentSubPage()->setCharacter(blankingRow, c, ' ');

	emit m_teletextDocument->contentsChanged();
}

void DeleteRowCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->moveCursor(m_row, -1);
	// Move lines below the inserting row downwards without affecting the FastText row
	for (int r=22; r>=m_row; r--)
		for (int c=0; c<40; c++)
			m_teletextDocument->currentSubPage()->setCharacter(r+1, c, m_teletextDocument->currentSubPage()->character(r, c));
	// Now repair the row we deleted
	for (int c=0; c<40; c++)
		m_teletextDocument->currentSubPage()->setCharacter(m_row, c, m_deletedRow[c]);

	emit m_teletextDocument->contentsChanged();
}


#ifndef QT_NO_CLIPBOARD
CutCommand::CutCommand(TeletextDocument *teletextDocument, QUndoCommand *parent) : StoreOldCharactersCommand(teletextDocument, parent)
{
	m_selectionTopRow = m_teletextDocument->selectionTopRow();
	m_selectionBottomRow = m_teletextDocument->selectionBottomRow();
	m_selectionLeftColumn = m_teletextDocument->selectionLeftColumn();
	m_selectionRightColumn = m_teletextDocument->selectionRightColumn();

	m_selectionCornerRow = m_teletextDocument->selectionCornerRow();
	m_selectionCornerColumn = m_teletextDocument->selectionCornerColumn();

	storeOldCharacters(m_selectionTopRow, m_selectionLeftColumn, m_selectionBottomRow, m_selectionRightColumn);

	setText(QObject::tr("cut"));
}

void CutCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);

	for (int r=m_selectionTopRow; r<=m_selectionBottomRow; r++) {
		for (int c=m_selectionLeftColumn; c<=m_selectionRightColumn; c++)
			m_teletextDocument->currentSubPage()->setCharacter(r, c, 0x20);
	}

	emit m_teletextDocument->contentsChanged();
}

void CutCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);

	retrieveOldCharacters(m_selectionTopRow, m_selectionLeftColumn, m_selectionBottomRow, m_selectionRightColumn);

	emit m_teletextDocument->contentsChanged();

	m_teletextDocument->setSelectionCorner(m_selectionCornerRow, m_selectionCornerColumn);
	m_teletextDocument->moveCursor(m_row, m_column, true);
}


PasteCommand::PasteCommand(TeletextDocument *teletextDocument, int pageCharSet, QUndoCommand *parent) : StoreOldCharactersCommand(teletextDocument, parent)
{
	const QClipboard *clipboard = QApplication::clipboard();
	const QMimeData *mimeData = clipboard->mimeData();
	QByteArray nativeData;

	m_selectionActive = m_teletextDocument->selectionActive();
	if (m_selectionActive) {
		m_selectionCornerRow = m_teletextDocument->selectionCornerRow();
		m_selectionCornerColumn = m_teletextDocument->selectionCornerColumn();
		m_pasteTopRow = m_teletextDocument->selectionTopRow();
		m_pasteBottomRow = m_teletextDocument->selectionBottomRow();
		m_pasteLeftColumn = m_teletextDocument->selectionLeftColumn();
		m_pasteRightColumn = m_teletextDocument->selectionRightColumn();
	} else {
		m_pasteTopRow = m_row;
		m_pasteLeftColumn = m_column;
		// m_pasteBottomRow and m_pasteRightColumn will be filled in later
		// when the size of the clipboard data is known
	}

	// Zero size here represents invalid or empty clipboard data
	m_clipboardDataHeight = m_clipboardDataWidth = 0;

	// Try to get something from the clipboard
	// FIXME is this a correct "custom" mime type? Or should we use vnd?
	nativeData = mimeData->data("application/x-teletext");
	if (nativeData.size() > 2) {
		// Native clipboard data: we put it there ourselves
		m_plainText = false;
		m_clipboardDataHeight = nativeData.at(0);
		m_clipboardDataWidth = nativeData.at(1);

		// Guard against invalid dimensions or total size not matching stated dimensions
		if (m_clipboardDataHeight > 0 && m_clipboardDataWidth > 0 && m_clipboardDataHeight <= 25 && m_clipboardDataWidth <= 40 && nativeData.size() == m_clipboardDataHeight * m_clipboardDataWidth + 2)
			for (int r=0; r<m_clipboardDataHeight; r++)
				m_pastingCharacters.append(nativeData.mid(2 + r * m_clipboardDataWidth, m_clipboardDataWidth));
		else {
			// Invalidate
			m_clipboardDataHeight = m_clipboardDataWidth = 0;
			return;
		}

		if (!m_selectionActive) {
			m_pasteBottomRow = m_row + m_clipboardDataHeight - 1;
			m_pasteRightColumn = m_column + m_clipboardDataWidth - 1;
		}
	} else if (mimeData->hasText()) {
		// Plain text
		m_plainText = true;

		const int rightColumn = m_selectionActive ? m_pasteRightColumn : 39;

		// Parse line-feeds in the clipboard data
		QStringList plainTextData = mimeData->text().split(QRegularExpression("\n|\r\n|\r"));

		// "if" statement will be false if clipboard data is a single line of text
		// that will fit from the cursor position
		if (plainTextData.size() != 1 || m_pasteLeftColumn + plainTextData.at(0).size() - 1 > rightColumn) {
			bool wrappingNeeded = false;

			if (!m_selectionActive) {
				// If selection is NOT active, use the full width of the page to paste.
				// The second and subsequent lines will start at column 1
				m_pasteLeftColumn = 1;

				// Check if first word in the first line will fit from the cursor position
				bool firstWordFits = true;
				const int firstSpace = plainTextData.at(0).indexOf(' ');

				if (firstSpace == -1 && m_column + plainTextData.at(0).size() > 40)
					firstWordFits = false; // Only one word in first line, and it won't fit
				else if (m_column + firstSpace > 40)
					firstWordFits = false; // First word in first line won't fit

				// If the first word WILL fit at the cursor position, pad the first line
				// to match the cursor position using null characters.
				// In the QString null characters represent character cells in the
				// pasting rectangle that won't overwrite what's on the page.
				// If the first word WON'T fit, start pasting at the beginning of the next row.
				if (firstWordFits)
					plainTextData[0] = QString(m_column-m_pasteLeftColumn, QChar::Null) + plainTextData.at(0);
				else if (m_pasteTopRow < 24)
					m_pasteTopRow++;
				else
					return;
			}

			const int pasteWidth = rightColumn - m_pasteLeftColumn + 1;

			// Find out if we need to word-wrap
			for (int i=0; i<plainTextData.size(); i++)
				if (plainTextData.at(i).size() > pasteWidth) {
					wrappingNeeded = true;
					break;
				}

			if (wrappingNeeded) {
				QStringList wrappedText;

				for (int i=0; i<plainTextData.size(); i++) {
					// Split this line into individual words
					QStringList lineWords = plainTextData.at(i).split(' ');

					// If there's any words which are too long to fit,
					// break them across multiple lines
					for (int j=0; j<lineWords.size(); j++)
						if (lineWords.at(j).size() > pasteWidth) {
							lineWords.insert(j+1, lineWords.at(j).mid(pasteWidth));
							lineWords[j].truncate(pasteWidth);
						}

					// Now reassemble the words into lines that will fit
					QString currentLine = lineWords.at(0);

					for (int j=1; j<lineWords.size(); j++)
						if (currentLine.size() + 1 + lineWords.at(j).size() <= pasteWidth)
							currentLine.append(' ' + lineWords.at(j));
						else {
							wrappedText.append(currentLine);
							currentLine = lineWords.at(j);
						}

					wrappedText.append(currentLine);
				}
				plainTextData.swap(wrappedText);
			}
		}

		m_clipboardDataHeight = plainTextData.size();
		m_clipboardDataWidth = 0;

		// Convert the unicode clipboard text into teletext bytes matching the current Level 1
		// character set of this page
		for (int r=0; r<m_clipboardDataHeight; r++) {
			m_pastingCharacters.append(QByteArray());
			for (int c=0; c<plainTextData.at(r).size(); c++) {
				char convertedChar;
				const QChar charToConvert = plainTextData.at(r).at(c);

				// Map a null character in the QString to 0xff (or -1)
				// In the QByteArray 0xff bytes represent character cells in the pasting rectangle
				// that won't overwrite what's on the page
				if (charToConvert == QChar::Null)
					convertedChar = -1;
				else if (charToConvert >= QChar(0x01) && charToConvert <= QChar(0x1f))
					convertedChar = ' ';
				else if (keymapping[pageCharSet].contains(charToConvert))
					// Remapped character or non-Latin character converted successfully
					convertedChar = keymapping[pageCharSet].value(charToConvert);
				else {
					// Either a Latin character or non-Latin character that can't be converted
					// See if it's a Latin character
					convertedChar = charToConvert.toLatin1();
					if (convertedChar <= 0)
						// Couldn't convert - make it a block character so it doesn't need to be inserted-between later on
						convertedChar = 0x7f;
				}

				m_pastingCharacters[r].append(convertedChar);
			}
			m_clipboardDataWidth = qMax(m_pastingCharacters.at(r).size(), m_clipboardDataWidth);
		}
		// Pad the end of short lines with spaces to make a box
		for (int r=0; r<m_clipboardDataHeight; r++)
			m_pastingCharacters[r] = m_pastingCharacters.at(r).leftJustified(m_clipboardDataWidth);

		if (!m_selectionActive) {
			m_pasteBottomRow = m_pasteTopRow + m_clipboardDataHeight - 1;
			m_pasteRightColumn = m_pasteLeftColumn + m_clipboardDataWidth - 1;
		}
	} else if (mimeData->hasImage()) {
		QImage imageData = qvariant_cast<QImage>(mimeData->imageData());
		m_plainText = false;
		// Round up when dividing pixel size into character cell size
		m_clipboardDataHeight = (imageData.height() + 2) / 3;
		m_clipboardDataWidth = (imageData.width() + 1) / 2;

		// Format_MonoLSB reverses the bits which makes them easier to shuffle into sixels
		if (imageData.depth() == 1)
			imageData.convertTo(QImage::Format_MonoLSB);
		else
			// Only pure black and white images convert reliably this way...
			imageData = imageData.convertToFormat(QImage::Format_MonoLSB, QVector<QRgb>{0x000000ff, 0xffffffff});

		for (int r=0; r<m_clipboardDataHeight; r++)
			m_pastingCharacters.append(QByteArray(m_clipboardDataWidth, 0x00));

		// Directly read the pixel-bits and convert them to sixels with some funky bit manipulation
		for (int y=0; y<imageData.height(); y++) {
			const unsigned char *bytePointer = imageData.constScanLine(y);
			// Three rows of sixels per character cell
			const int r = y / 3;
			// Where to shuffle the bits into the top, middle or bottom row of sixels
			// Yes it does put the bottom right sixel into bit 5 instead of bit 6;
			// this gets remedied further on
			const int yShift = (y % 3) * 2;
			// The loop does eight horizontal pixels into four character cells at a time
			for (int x=0; x<imageData.width(); x+=8) {
				const unsigned char byteScanned = *bytePointer;
				const int c = x / 2;

				m_pastingCharacters[r][c] = m_pastingCharacters[r][c] | ((byteScanned & 0x03) << yShift);
				// Since we're doing four character cells at a time, we need to exit the loop
				// early before we go out of bounds.
				// Yes it does leave an undefined last column of sixels from images that are an
				// odd numbered number of pixels wide; this gets remedied further on
				if (x + 2 >= imageData.width())
					continue;
				m_pastingCharacters[r][c+1] = m_pastingCharacters[r][c+1] | (((byteScanned >> 2) & 0x03) << yShift);
				if (x + 4 >= imageData.width())
					continue;
				m_pastingCharacters[r][c+2] = m_pastingCharacters[r][c+2] | (((byteScanned >> 4) & 0x03) << yShift);
				if (x + 6 >= imageData.width())
					continue;
				m_pastingCharacters[r][c+3] = m_pastingCharacters[r][c+3] | (((byteScanned >> 6) & 0x03) << yShift);

				bytePointer++;
			}
		}

		for (int r=0; r<m_clipboardDataHeight; r++) {
			for (int c=0; c<m_clipboardDataWidth; c++)
				if (m_pastingCharacters.at(r).at(c) & 0x20)
					// If bit 5 was set, correct this to bit 6
					// but we keep bit 5 set as all mosaic characters have bit 5 set
					m_pastingCharacters[r][c] = m_pastingCharacters.at(r).at(c) | 0x40;
				else
					// Set bit 5 to have it recognised as a mosaic character
					m_pastingCharacters[r][c] = m_pastingCharacters.at(r).at(c) | 0x20;
			// If image was an odd numbered width, neutralise the undefined sixels
			// on the right half
			if (imageData.width() & 0x01)
				m_pastingCharacters[r][m_clipboardDataWidth-1] = m_pastingCharacters.at(r).at(m_clipboardDataWidth-1) & 0x35;
		}

		if (!m_selectionActive) {
			m_pasteBottomRow = m_row + m_clipboardDataHeight - 1;
			m_pasteRightColumn = m_column + m_clipboardDataWidth - 1;
		}
	}

	if (m_clipboardDataWidth == 0 || m_clipboardDataHeight == 0)
		return;

	storeOldCharacters(m_pasteTopRow, m_pasteLeftColumn, m_pasteBottomRow, m_pasteRightColumn);

	setText(QObject::tr("paste"));
}

void PasteCommand::redo()
{
	if (m_clipboardDataWidth == 0 || m_clipboardDataHeight == 0)
		return;

	m_teletextDocument->selectSubPageIndex(m_subPageIndex);

	int arrayR = 0;
	int arrayC;

	for (int r=m_pasteTopRow; r<=m_pasteBottomRow; r++) {
		arrayC = 0;
		for (int c=m_pasteLeftColumn; c<=m_pasteRightColumn; c++)
			// Guard against size of pasted block going beyond last line or column
			if (r < 25 && c < 40) {
				// Check for 0xff bytes using "-1"
				// gcc complains about "comparision always true due to limited range"
				if (m_pastingCharacters.at(arrayR).at(arrayC) != -1)
					m_teletextDocument->currentSubPage()->setCharacter(r, c, m_pastingCharacters.at(arrayR).at(arrayC));

				arrayC++;

				// If paste area is wider than clipboard data, repeat the pattern
				// if it wasn't plain text
				if (arrayC == m_clipboardDataWidth) {
					if (!m_plainText)
						arrayC = 0;
					else
						break;
				}
			}

		arrayR++;
		// If paste area is taller than clipboard data, repeat the pattern
		// if it wasn't plain text
		if (arrayR == m_clipboardDataHeight) {
			if (!m_plainText)
				arrayR = 0;
			else
				break;
		}
	}

	emit m_teletextDocument->contentsChanged();

	if (m_selectionActive) {
		m_teletextDocument->setSelectionCorner(m_selectionCornerRow, m_selectionCornerColumn);
		m_teletextDocument->moveCursor(m_row, m_column, true);
	} else {
		m_teletextDocument->moveCursor(m_row, qMin(m_column+m_clipboardDataWidth-1, 39));
		m_teletextDocument->cursorRight();
	}
}

void PasteCommand::undo()
{
	if (m_clipboardDataWidth == 0 || m_clipboardDataHeight == 0)
		return;

	m_teletextDocument->selectSubPageIndex(m_subPageIndex);

	retrieveOldCharacters(m_pasteTopRow, m_pasteLeftColumn, m_pasteBottomRow, m_pasteRightColumn);

	emit m_teletextDocument->contentsChanged();

	if (!m_selectionActive)
		m_teletextDocument->moveCursor(m_row, m_column);
}
#endif // !QT_NO_CLIPBOARD


InsertSubPageCommand::InsertSubPageCommand(TeletextDocument *teletextDocument, bool afterCurrentSubPage, bool copySubPage, QUndoCommand *parent) : LevelOneCommand(teletextDocument, parent)
{
	m_newSubPageIndex = m_subPageIndex + afterCurrentSubPage;
	m_copySubPage = copySubPage;

	setText(QObject::tr("insert subpage"));

}

void InsertSubPageCommand::redo()
{
	m_teletextDocument->insertSubPage(m_newSubPageIndex, m_copySubPage);

	m_teletextDocument->selectSubPageIndex(m_newSubPageIndex, true);
}

void InsertSubPageCommand::undo()
{
	m_teletextDocument->deleteSubPage(m_newSubPageIndex);
	//TODO should we always wrench to "subpage viewed when we inserted"? Or just if subpage viewed is being deleted?

	m_teletextDocument->selectSubPageIndex(qMin(m_newSubPageIndex, m_teletextDocument->numberOfSubPages()-1), true);
}


DeleteSubPageCommand::DeleteSubPageCommand(TeletextDocument *teletextDocument, QUndoCommand *parent) : LevelOneCommand(teletextDocument, parent)
{
	setText(QObject::tr("delete subpage"));
}

void DeleteSubPageCommand::redo()
{
	m_teletextDocument->deleteSubPageToRecycle(m_subPageIndex);
	m_teletextDocument->selectSubPageIndex(qMin(m_subPageIndex, m_teletextDocument->numberOfSubPages()-1), true);
}

void DeleteSubPageCommand::undo()
{
	m_teletextDocument->unDeleteSubPageFromRecycle(m_subPageIndex);
	m_teletextDocument->selectSubPageIndex(m_subPageIndex, true);
}


SetFullScreenColourCommand::SetFullScreenColourCommand(TeletextDocument *teletextDocument, int newColour, QUndoCommand *parent) : LevelOneCommand(teletextDocument, parent)
{
	m_oldColour = teletextDocument->currentSubPage()->defaultScreenColour();
	m_newColour = newColour;

	setText(QObject::tr("full screen colour"));
}

void SetFullScreenColourCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setDefaultScreenColour(m_newColour);

	emit m_teletextDocument->contentsChanged();
	emit m_teletextDocument->pageOptionsChanged();
}

void SetFullScreenColourCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setDefaultScreenColour(m_oldColour);

	emit m_teletextDocument->contentsChanged();
	emit m_teletextDocument->pageOptionsChanged();
}

bool SetFullScreenColourCommand::mergeWith(const QUndoCommand *command)
{
	const SetFullScreenColourCommand *newerCommand = static_cast<const SetFullScreenColourCommand *>(command);

	if (m_subPageIndex != newerCommand->m_subPageIndex)
		return false;

	m_newColour = newerCommand->m_newColour;

	return true;
}


SetFullRowColourCommand::SetFullRowColourCommand(TeletextDocument *teletextDocument, int newColour, QUndoCommand *parent) : LevelOneCommand(teletextDocument, parent)
{
	m_oldColour = teletextDocument->currentSubPage()->defaultRowColour();
	m_newColour = newColour;

	setText(QObject::tr("full row colour"));
}

void SetFullRowColourCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setDefaultRowColour(m_newColour);

	emit m_teletextDocument->contentsChanged();
	emit m_teletextDocument->pageOptionsChanged();
}

void SetFullRowColourCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setDefaultRowColour(m_oldColour);

	emit m_teletextDocument->contentsChanged();
	emit m_teletextDocument->pageOptionsChanged();
}

bool SetFullRowColourCommand::mergeWith(const QUndoCommand *command)
{
	const SetFullRowColourCommand *newerCommand = static_cast<const SetFullRowColourCommand *>(command);

	if (m_subPageIndex != newerCommand->m_subPageIndex)
		return false;

	m_newColour = newerCommand->m_newColour;

	return true;
}


SetCLUTRemapCommand::SetCLUTRemapCommand(TeletextDocument *teletextDocument, int newMap, QUndoCommand *parent) : LevelOneCommand(teletextDocument, parent)
{
	m_oldMap = teletextDocument->currentSubPage()->colourTableRemap();
	m_newMap = newMap;

	setText(QObject::tr("CLUT remapping"));
}

void SetCLUTRemapCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setColourTableRemap(m_newMap);

	emit m_teletextDocument->contentsChanged();
	emit m_teletextDocument->pageOptionsChanged();
}

void SetCLUTRemapCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setColourTableRemap(m_oldMap);

	emit m_teletextDocument->contentsChanged();
	emit m_teletextDocument->pageOptionsChanged();
}

bool SetCLUTRemapCommand::mergeWith(const QUndoCommand *command)
{
	const SetCLUTRemapCommand *newerCommand = static_cast<const SetCLUTRemapCommand *>(command);

	if (m_subPageIndex != newerCommand->m_subPageIndex)
		return false;

	m_newMap = newerCommand->m_newMap;

	return true;
}


SetBlackBackgroundSubstCommand::SetBlackBackgroundSubstCommand(TeletextDocument *teletextDocument, bool newSub, QUndoCommand *parent) : LevelOneCommand(teletextDocument, parent)
{
	m_oldSub = teletextDocument->currentSubPage()->blackBackgroundSubst();
	m_newSub = newSub;

	setText(QObject::tr("black background substitution"));
}

void SetBlackBackgroundSubstCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setBlackBackgroundSubst(m_newSub);

	emit m_teletextDocument->contentsChanged();
	emit m_teletextDocument->pageOptionsChanged();
}

void SetBlackBackgroundSubstCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setBlackBackgroundSubst(m_oldSub);

	emit m_teletextDocument->contentsChanged();
	emit m_teletextDocument->pageOptionsChanged();
}

bool SetBlackBackgroundSubstCommand::mergeWith(const QUndoCommand *command)
{
	const SetBlackBackgroundSubstCommand *newerCommand = static_cast<const SetBlackBackgroundSubstCommand *>(command);

	if (m_subPageIndex != newerCommand->m_subPageIndex)
		return false;

	setObsolete(true);
	return true;
}


SetColourCommand::SetColourCommand(TeletextDocument *teletextDocument, int colourIndex, int newColour, QUndoCommand *parent) : LevelOneCommand(teletextDocument, parent)
{
	m_colourIndex = colourIndex;
	m_oldColour = teletextDocument->currentSubPage()->CLUT(colourIndex);
	m_newColour = newColour;

	setText(QObject::tr("colour change"));

}

void SetColourCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setCLUT(m_colourIndex, m_newColour);

	emit m_teletextDocument->colourChanged(m_colourIndex);
	emit m_teletextDocument->contentsChanged();
}

void SetColourCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setCLUT(m_colourIndex, m_oldColour);

	emit m_teletextDocument->colourChanged(m_colourIndex);
	emit m_teletextDocument->contentsChanged();
}


ResetCLUTCommand::ResetCLUTCommand(TeletextDocument *teletextDocument, int colourTable, QUndoCommand *parent) : LevelOneCommand(teletextDocument, parent)
{
	m_colourTable = colourTable;
	for (int i=m_colourTable*8; i<m_colourTable*8+8; i++)
		m_oldColourEntry[i&7] = teletextDocument->currentSubPage()->CLUT(i);

	setText(QObject::tr("CLUT %1 reset").arg(m_colourTable));
}

void ResetCLUTCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	for (int i=m_colourTable*8; i<m_colourTable*8+8; i++) {
		m_teletextDocument->currentSubPage()->setCLUT(i, m_teletextDocument->currentSubPage()->CLUT(i, 0));
		emit m_teletextDocument->colourChanged(i);
	}

	emit m_teletextDocument->contentsChanged();
}

void ResetCLUTCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	for (int i=m_colourTable*8; i<m_colourTable*8+8; i++) {
		m_teletextDocument->currentSubPage()->setCLUT(i, m_oldColourEntry[i&7]);
		emit m_teletextDocument->colourChanged(i);
	}

	emit m_teletextDocument->contentsChanged();
}
