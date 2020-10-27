/*
 * Copyright (C) 2020 Gavin MacGregor
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

#include <QFile>
#include <QTextStream>
#include <vector>

#include "document.h"

#include "levelonepage.h"

TeletextDocument::TeletextDocument()
{
	m_pageNumber = 0x198;
	m_description.clear();
	for (int i=0; i<6; i++)
		m_fastTextLink[i] = 0x8ff;
	m_empty = true;
	m_subPages.push_back(new LevelOnePage);
	m_subPages[0]->setPageNumber(m_pageNumber);
	m_currentSubPageIndex = 0;
	m_undoStack = new QUndoStack(this);
	m_cursorRow = 1;
	m_cursorColumn = 0;
	m_selectionSubPage = nullptr;
}

TeletextDocument::~TeletextDocument()
{
	for (auto &subPage : m_subPages)
		delete(subPage);
}

void TeletextDocument::loadDocument(QFile *inFile)
{
	QByteArray inLine;
	bool firstSubPageFound = false;
	int cycleCommandsFound = 0;
	int mostRecentCycleValue = -1;
	LevelOnePage::CycleTypeEnum mostRecentCycleType;
	
	LevelOnePage* loadingPage = m_subPages[0];

	for (;;) {
		inLine = inFile->readLine(160).trimmed();
		if (inLine.isEmpty())
			break;
		if (inLine.startsWith("DE,"))
			m_description = QString(inLine.remove(0, 3));
		if (inLine.startsWith("PN,")) {
			bool pageNumberOk;
			int pageNumberRead = inLine.mid(3, 3).toInt(&pageNumberOk, 16);
			if ((!pageNumberOk) || pageNumberRead < 0x100 || pageNumberRead > 0x8ff)
				pageNumberRead = 0x199;
			// When second and subsequent PN commands are found, firstSubPageFound==true at this point
			// This assumes that PN is the first command of a new subpage...
			if (firstSubPageFound) {
				m_subPages.push_back(new LevelOnePage);
				m_subPages.back()->setPageNumber(m_pageNumber);
				loadingPage = m_subPages.back();
			}
			m_pageNumber = pageNumberRead;
			firstSubPageFound = true;
		}
/*		if (lineType == "SC,") {
			bool subPageNumberOk;
			int subPageNumberRead = inLine.mid(3, 4).toInt(&subPageNumberOk, 16);
			if ((!subPageNumberOk) || subPageNumberRead > 0x3f7f)
				subPageNumberRead = 0;
			loadingPage->setSubPageNumber(subPageNumberRead);
		}*/
		if (inLine.startsWith("PS,")) {
			bool pageStatusOk;
			int pageStatusRead = inLine.mid(3, 4).toInt(&pageStatusOk, 16);
			if (pageStatusOk) {
				loadingPage->setControlBit(0, pageStatusRead & 0x4000);
				for (int i=1, pageStatusBit=0x0001; i<8; i++, pageStatusBit<<=1)
					loadingPage->setControlBit(i, pageStatusRead & pageStatusBit);
				loadingPage->setDefaultNOS(((pageStatusRead & 0x0200) >> 9) | ((pageStatusRead & 0x0100) >> 7) | ((pageStatusRead & 0x0080) >> 5));
			}
		}
		if (inLine.startsWith("CT,") && (inLine.endsWith(",C") || inLine.endsWith(",T"))) {
			bool cycleValueOk;
			int cycleValueRead = inLine.mid(3, inLine.size()-5).toInt(&cycleValueOk);
			if (cycleValueOk) {
				cycleCommandsFound++;
				// House-keep CT command values, in case it's the only one within multiple subpages
				mostRecentCycleValue = cycleValueRead;
				loadingPage->setCycleValue(cycleValueRead);
				mostRecentCycleType = inLine.endsWith("C") ? LevelOnePage::CTcycles : LevelOnePage::CTseconds;
				loadingPage->setCycleType(mostRecentCycleType);
			}
		}
		if (inLine.startsWith("FL,")) {
			bool fastTextLinkOk;
			int fastTextLinkRead;
			QString flLine = QString(inLine.remove(0, 3));
			if (flLine.count(',') == 5)
				for (int i=0; i<6; i++) {
					fastTextLinkRead = flLine.section(',', i, i).toInt(&fastTextLinkOk, 16);
					if (fastTextLinkOk)
						m_fastTextLink[i] = fastTextLinkRead == 0 ? 0x8ff : fastTextLinkRead;
				}
		}
		if (inLine.startsWith("OL,"))
			loadingPage->loadPagePacket(inLine);
	}
	// If there's more than one subpage but only one valid CT command was found, apply it to all subpages
	// I don't know if this is correct
	if (cycleCommandsFound == 1 && m_subPages.size()>1)
		for (auto &subPage : m_subPages) {
			subPage->setCycleValue(mostRecentCycleValue);
			subPage->setCycleType(mostRecentCycleType);
		}
	subPageSelected();
}

void TeletextDocument::saveDocument(QTextStream *outStream)
{
	if (!m_description.isEmpty())
		*outStream << "DE," << m_description << endl;
	//TODO DS and SP commands

	int subPageNumber = m_subPages.size()>1;

	for (auto &subPage : m_subPages) {
		subPage->savePage(outStream, m_pageNumber, subPageNumber++);
		if (m_fastTextLink[0] != 0x8ff) {
			*outStream << "FL,";
			for (int i=0; i<6; i++) {
				*outStream << QString("%1").arg(m_fastTextLink[i], 3, 16, QChar('0'));
				if (i<5)
					*outStream << ',';
			}
			*outStream << endl;
		}
	}
}

void TeletextDocument::selectSubPageIndex(int newSubPageIndex, bool forceRefresh)
{
	// forceRefresh overrides "beyond the last subpage" check, so inserting a subpage after the last one still shows - dangerous workaround?
	if (forceRefresh || (newSubPageIndex != m_currentSubPageIndex && newSubPageIndex < m_subPages.size()-1)) {
		emit aboutToChangeSubPage();
		m_currentSubPageIndex = newSubPageIndex;
		emit subPageSelected();
		return;
	}
}

void TeletextDocument::selectSubPageNext()
{
	if (m_currentSubPageIndex < m_subPages.size()-1) {
		emit aboutToChangeSubPage();
		m_currentSubPageIndex++;
		emit subPageSelected();
	}
}

void TeletextDocument::selectSubPagePrevious()
{
	if (m_currentSubPageIndex > 0) {
		emit aboutToChangeSubPage();
		m_currentSubPageIndex--;
		emit subPageSelected();
	}
}

void TeletextDocument::insertSubPage(int beforeSubPageIndex, bool copySubPage)
{
	LevelOnePage *insertedSubPage;

	if (copySubPage)
		insertedSubPage = new LevelOnePage(*m_subPages.at(beforeSubPageIndex));
	else
		insertedSubPage = new LevelOnePage;
	insertedSubPage->setPageNumber(m_pageNumber);
	if (beforeSubPageIndex == m_subPages.size())
		m_subPages.push_back(insertedSubPage);
	else
		m_subPages.insert(m_subPages.begin()+beforeSubPageIndex, insertedSubPage);
}

void TeletextDocument::deleteSubPage(int subPageToDelete)
{
	delete(m_subPages[subPageToDelete]);
	m_subPages.erase(m_subPages.begin()+subPageToDelete);
}

void TeletextDocument::setPageNumber(QString newPageNumberString)
{
	// The LineEdit should check if a valid hex number was entered, but just in case...
	bool newPageNumberOk;
	int newPageNumberRead = newPageNumberString.toInt(&newPageNumberOk, 16);
	if ((!newPageNumberOk) || newPageNumberRead < 0x100 || newPageNumberRead > 0x8fe)
		return;

	// If the magazine number was changed, we'll need to update the relative magazine numbers in X/27
	int oldMagazine = (m_pageNumber & 0xf00);
	int newMagazine = (newPageNumberRead & 0xf00);
	// Fix magazine 0 to 8
	if (oldMagazine == 0x800)
		oldMagazine = 0x000;
	if (newMagazine == 0x800)
		newMagazine = 0x000;
	int magazineFlip = oldMagazine ^ newMagazine;

	m_pageNumber = newPageNumberRead;

	for (auto &subPage : m_subPages) {
		subPage->setPageNumber(newPageNumberRead);
		if (magazineFlip)
			for (int i=0; i<8; i++)
				subPage->setComposeLinkPageNumber(i, subPage->composeLinkPageNumber(i) ^ magazineFlip);
	}
}

void TeletextDocument::setDescription(QString newDescription)
{
	m_description = newDescription;
}

void TeletextDocument::setFastTextLink(int linkNumber, QString newPageNumberString)
{
	// The LineEdit should check if a valid hex number was entered, but just in case...
	bool newPageNumberOk;
	int newPageNumberRead = newPageNumberString.toInt(&newPageNumberOk, 16);
	if ((!newPageNumberOk) || newPageNumberRead < 0x100 || newPageNumberRead > 0x8ff)
		return;

	m_fastTextLink[linkNumber] = newPageNumberRead;
}

void TeletextDocument::cursorUp()
{
	if (--m_cursorRow == 0)
		m_cursorRow = 24;
	emit cursorMoved();
}

void TeletextDocument::cursorDown()
{
	if (++m_cursorRow == 25)
		m_cursorRow = 1;
	emit cursorMoved();
}

void TeletextDocument::cursorLeft()
{
	if (--m_cursorColumn == -1) {
		m_cursorColumn = 39;
		cursorUp();
	}
	emit cursorMoved();
}

void TeletextDocument::cursorRight()
{
	if (++m_cursorColumn == 40) {
		m_cursorColumn = 0;
		cursorDown();
	}
	emit cursorMoved();
}

void TeletextDocument::moveCursor(int cursorRow, int cursorColumn)
{
	if (cursorRow != -1)
		m_cursorRow = cursorRow;
	if (cursorColumn != -1)
		m_cursorColumn = cursorColumn;
	emit cursorMoved();
}

void TeletextDocument::setSelection(int topRow, int leftColumn, int bottomRow, int rightColumn)
{
	if (m_selectionTopRow != topRow || m_selectionBottomRow != bottomRow || m_selectionLeftColumn != leftColumn || m_selectionRightColumn != rightColumn) {
		m_selectionSubPage = currentSubPage();
		m_selectionTopRow = topRow;
		m_selectionBottomRow = bottomRow;
		m_selectionLeftColumn = leftColumn;
		m_selectionRightColumn = rightColumn;
		emit selectionMoved();
	}
}

void TeletextDocument::cancelSelection()
{
	m_selectionSubPage = nullptr;
}


OverwriteCharacterCommand::OverwriteCharacterCommand(TeletextDocument *teletextDocument, unsigned char newCharacter, QUndoCommand *parent) : QUndoCommand(parent)
{
	m_teletextDocument = teletextDocument;
	m_subPageIndex = teletextDocument->currentSubPageIndex();
	m_row = teletextDocument->cursorRow();
	m_column = teletextDocument->cursorColumn();
	m_oldCharacter = teletextDocument->currentSubPage()->character(m_row, m_column);
	m_newCharacter = newCharacter;
}

void OverwriteCharacterCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setCharacter(m_row, m_column, m_newCharacter);
	m_teletextDocument->moveCursor(m_row, m_column);
	m_teletextDocument->cursorRight();
	setText(QObject::tr("overwrite char"));
	emit m_teletextDocument->contentsChange(m_row);
}

void OverwriteCharacterCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setCharacter(m_row, m_column, m_oldCharacter);
	m_teletextDocument->moveCursor(m_row, m_column);
	setText(QObject::tr("overwrite char"));
	emit m_teletextDocument->contentsChange(m_row);
}


ToggleMosaicBitCommand::ToggleMosaicBitCommand(TeletextDocument *teletextDocument, unsigned char bitToToggle, QUndoCommand *parent) : QUndoCommand(parent)
{
	m_teletextDocument = teletextDocument;
	m_subPageIndex = teletextDocument->currentSubPageIndex();
	m_row = teletextDocument->cursorRow();
	m_column = teletextDocument->cursorColumn();
	m_oldCharacter = teletextDocument->currentSubPage()->character(m_row, m_column);
	if (bitToToggle == 0x20 || bitToToggle == 0x7f)
		m_newCharacter = bitToToggle;
	else
		m_newCharacter = m_oldCharacter ^ bitToToggle;
}

void ToggleMosaicBitCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setCharacter(m_row, m_column, m_newCharacter);
	m_teletextDocument->moveCursor(m_row, m_column);
	setText(QObject::tr("mosaic"));
	emit m_teletextDocument->contentsChange(m_row);
}

void ToggleMosaicBitCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setCharacter(m_row, m_column, m_oldCharacter);
	m_teletextDocument->moveCursor(m_row, m_column);
	setText(QObject::tr("mosaic"));
	emit m_teletextDocument->contentsChange(m_row);
}

bool ToggleMosaicBitCommand::mergeWith(const QUndoCommand *command)
{
	const ToggleMosaicBitCommand *previousCommand = static_cast<const ToggleMosaicBitCommand *>(command);

	if (m_subPageIndex != previousCommand->m_subPageIndex || m_row != previousCommand->m_row || m_column != previousCommand->m_column)
		return false;
	m_newCharacter = previousCommand->m_newCharacter;
	return true;
}


BackspaceCommand::BackspaceCommand(TeletextDocument *teletextDocument, QUndoCommand *parent) : QUndoCommand(parent)
{
	m_teletextDocument = teletextDocument;
	m_subPageIndex = teletextDocument->currentSubPageIndex();
	m_row = teletextDocument->cursorRow();
	m_column = teletextDocument->cursorColumn()-1;
	if (m_column == -1) {
		m_column = 39;
		if (--m_row == 0)
			m_row = 24;
	}
	m_deletedCharacter = teletextDocument->currentSubPage()->character(m_row, m_column);
}

void BackspaceCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setCharacter(m_row, m_column, 0x20);
	m_teletextDocument->moveCursor(m_row, m_column);
	setText(QObject::tr("backspace"));
	emit m_teletextDocument->contentsChange(m_row);
}

void BackspaceCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setCharacter(m_row, m_column, m_deletedCharacter);
	m_teletextDocument->moveCursor(m_row, m_column);
	m_teletextDocument->cursorRight();
	setText(QObject::tr("backspace"));
	emit m_teletextDocument->contentsChange(m_row);
}


InsertRowCommand::InsertRowCommand(TeletextDocument *teletextDocument, bool copyRow, QUndoCommand *parent) : QUndoCommand(parent)
{
	m_teletextDocument = teletextDocument;
	m_subPageIndex = teletextDocument->currentSubPageIndex();
	m_row = teletextDocument->cursorRow();
	m_copyRow = copyRow;
}

void InsertRowCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->moveCursor(m_row, -1);
	// Store copy of the bottom row we're about to push out, for undo
	for (int c=0; c<40; c++)
		m_deletedBottomRow[c] = m_teletextDocument->currentSubPage()->character(23, c);
	// Shuffle lines below the inserting row downwards without affecting the FastText row
	for (int r=22; r>=m_row; r--)
		for (int c=0; c<40; c++)
			m_teletextDocument->currentSubPage()->setCharacter(r+1, c, m_teletextDocument->currentSubPage()->character(r, c));
	if (m_copyRow)
		setText(QObject::tr("insert copy row"));
	else {
		// The above shuffle leaves a duplicate of the current row, so blank it if requested
		for (int c=0; c<40; c++)
			m_teletextDocument->currentSubPage()->setCharacter(m_row, c, ' ');
		setText(QObject::tr("insert blank row"));
	}
	emit m_teletextDocument->refreshNeeded();
}

void InsertRowCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->moveCursor(m_row, -1);
	// Shuffle lines below the deleting row upwards without affecting the FastText row
	for (int r=m_row; r<23; r++)
		for (int c=0; c<40; c++)
			m_teletextDocument->currentSubPage()->setCharacter(r, c, m_teletextDocument->currentSubPage()->character(r+1, c));
	// Now repair the bottom row we pushed out
	for (int c=0; c<40; c++)
		m_teletextDocument->currentSubPage()->setCharacter(23, c, m_deletedBottomRow[c]);
	if (m_copyRow)
		setText(QObject::tr("insert copy row"));
	else
		setText(QObject::tr("insert blank row"));
	emit m_teletextDocument->refreshNeeded();
}


DeleteRowCommand::DeleteRowCommand(TeletextDocument *teletextDocument, QUndoCommand *parent) : QUndoCommand(parent)
{
	m_teletextDocument = teletextDocument;
	m_subPageIndex = teletextDocument->currentSubPageIndex();
	m_row = teletextDocument->cursorRow();
}

void DeleteRowCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->moveCursor(m_row, -1);
	// Store copy of the row we're going to delete, for undo
	for (int c=0; c<40; c++)
		m_deletedRow[c] = m_teletextDocument->currentSubPage()->character(m_row, c);
	// Shuffle lines below the deleting row upwards without affecting the FastText row
	for (int r=m_row; r<23; r++)
		for (int c=0; c<40; c++)
			m_teletextDocument->currentSubPage()->setCharacter(r, c, m_teletextDocument->currentSubPage()->character(r+1, c));
	// If we deleted the FastText row blank that row, otherwise blank the last row
	int blankingRow = (m_row < 24) ? 23 : 24;
		for (int c=0; c<40; c++)
			m_teletextDocument->currentSubPage()->setCharacter(blankingRow, c, ' ');
	setText(QObject::tr("delete row"));
	emit m_teletextDocument->refreshNeeded();
}

void DeleteRowCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->moveCursor(m_row, -1);
	// Shuffle lines below the inserting row downwards without affecting the FastText row
	for (int r=22; r>=m_row; r--)
		for (int c=0; c<40; c++)
			m_teletextDocument->currentSubPage()->setCharacter(r+1, c, m_teletextDocument->currentSubPage()->character(r, c));
	// Now repair the row we deleted
	for (int c=0; c<40; c++)
		m_teletextDocument->currentSubPage()->setCharacter(m_row, c, m_deletedRow[c]);
	setText(QObject::tr("delete row"));
	emit m_teletextDocument->refreshNeeded();
}


InsertSubPageCommand::InsertSubPageCommand(TeletextDocument *teletextDocument, bool afterCurrentSubPage, bool copySubPage, QUndoCommand *parent) : QUndoCommand(parent)
{
	m_teletextDocument = teletextDocument;
	m_newSubPageIndex = teletextDocument->currentSubPageIndex()+afterCurrentSubPage;
	m_copySubPage = copySubPage;
}

void InsertSubPageCommand::redo()
{
	m_teletextDocument->insertSubPage(m_newSubPageIndex, m_copySubPage);
	m_teletextDocument->selectSubPageIndex(m_newSubPageIndex, true);
	setText(QObject::tr("insert subpage"));
}

void InsertSubPageCommand::undo()
{
	m_teletextDocument->deleteSubPage(m_newSubPageIndex);
	//TODO should we always wrench to "subpage viewed when we inserted"? Or just if subpage viewed is being deleted?
	m_teletextDocument->selectSubPageIndex(m_newSubPageIndex, true);
	setText(QObject::tr("insert subpage"));
}


SetColourCommand::SetColourCommand(TeletextDocument *teletextDocument, int colourIndex, int newColour, QUndoCommand *parent) : QUndoCommand(parent)
{
	m_teletextDocument = teletextDocument;
	m_subPageIndex = teletextDocument->currentSubPageIndex();
	m_colourIndex = colourIndex;
	m_oldColour = teletextDocument->currentSubPage()->CLUT(colourIndex);
	m_newColour = newColour;
}

void SetColourCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setCLUT(m_colourIndex, m_newColour);
	emit m_teletextDocument->colourChanged(m_colourIndex);
	setText(QObject::tr("colour change"));
	emit m_teletextDocument->refreshNeeded();
}

void SetColourCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	m_teletextDocument->currentSubPage()->setCLUT(m_colourIndex, m_oldColour);
	emit m_teletextDocument->colourChanged(m_colourIndex);
	setText(QObject::tr("colour change"));
	emit m_teletextDocument->refreshNeeded();
}


ResetCLUTCommand::ResetCLUTCommand(TeletextDocument *teletextDocument, int colourTable, QUndoCommand *parent) : QUndoCommand(parent)
{
	m_teletextDocument = teletextDocument;
	m_subPageIndex = teletextDocument->currentSubPageIndex();
	m_colourTable = colourTable;
	for (int i=m_colourTable*8; i<m_colourTable*8+8; i++)
		m_oldColourEntry[i&7] = teletextDocument->currentSubPage()->CLUT(i);
}

void ResetCLUTCommand::redo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	for (int i=m_colourTable*8; i<m_colourTable*8+8; i++) {
		m_teletextDocument->currentSubPage()->setCLUT(i, m_teletextDocument->currentSubPage()->CLUT(i, 0));
		emit m_teletextDocument->colourChanged(i);
	}
	setText(QObject::tr("CLUT %1 reset").arg(m_colourTable));
	emit m_teletextDocument->refreshNeeded();
}

void ResetCLUTCommand::undo()
{
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	for (int i=m_colourTable*8; i<m_colourTable*8+8; i++) {
		m_teletextDocument->currentSubPage()->setCLUT(i, m_oldColourEntry[i&7]);
		emit m_teletextDocument->colourChanged(i);
	}
	setText(QObject::tr("CLUT %1 reset").arg(m_colourTable));
	emit m_teletextDocument->refreshNeeded();
}
