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
	m_pageFunction = PFLevelOnePage;
	m_packetCoding = Coding7bit;
	m_empty = true;
	m_subPages.push_back(new LevelOnePage);
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

/*
void TeletextDocument::setPageFunction(PageFunctionEnum newPageFunction)
{
	m_pageFunction = newPageFunction;
}

void TeletextDocument::setPacketCoding(PacketCodingEnum newPacketEncoding)
{
	m_packetCoding = newPacketEncoding;
}
*/

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
				loadingPage->setControlBit(PageBase::C4ErasePage, pageStatusRead & 0x4000);
				for (int i=PageBase::C5Newsflash, pageStatusBit=0x0001; i<=PageBase::C11SerialMagazine; i++, pageStatusBit<<=1)
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
					if (fastTextLinkOk) {
						if (fastTextLinkRead == 0)
							fastTextLinkRead = 0x8ff;
						// Stored as page link with relative magazine number, convert from absolute page number that was read
						fastTextLinkRead ^= m_pageNumber & 0x700;
						fastTextLinkRead &= 0x7ff; // Fixes magazine 8 to 0
						loadingPage->setFastTextLinkPageNumber(i, fastTextLinkRead);
					}
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
		if ((subPage->fastTextLinkPageNumber(0) & 0x0ff) != 0x0ff) {
			*outStream << "FL,";
			for (int i=0; i<6; i++) {
				// Stored as page link with relative magazine number, convert to absolute page number for display
				int absoluteLinkPageNumber = subPage->fastTextLinkPageNumber(i) ^ (m_pageNumber & 0x700);
				// Fix magazine 0 to 8
				if ((absoluteLinkPageNumber & 0x700) == 0x000)
					absoluteLinkPageNumber |= 0x800;

				*outStream << QString("%1").arg(absoluteLinkPageNumber, 3, 16, QChar('0'));
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

void TeletextDocument::setPageNumber(QString pageNumberString)
{
	// The LineEdit should check if a valid hex number was entered, but just in case...
	bool pageNumberOk;
	int pageNumberRead = pageNumberString.toInt(&pageNumberOk, 16);
	if ((!pageNumberOk) || pageNumberRead < 0x100 || pageNumberRead > 0x8fe)
		return;

	// If the magazine number was changed, we need to update the relative magazine numbers in FastText
	// and page enhancement links
	int oldMagazine = (m_pageNumber & 0xf00);
	int newMagazine = (pageNumberRead & 0xf00);
	// Fix magazine 0 to 8
	if (oldMagazine == 0x800)
		oldMagazine = 0x000;
	if (newMagazine == 0x800)
		newMagazine = 0x000;
	int magazineFlip = oldMagazine ^ newMagazine;

	m_pageNumber = pageNumberRead;

	for (auto &subPage : m_subPages)
		if (magazineFlip) {
			for (int i=0; i<6; i++)
				subPage->setFastTextLinkPageNumber(i, subPage->fastTextLinkPageNumber(i) ^ magazineFlip);
			for (int i=0; i<8; i++)
				subPage->setComposeLinkPageNumber(i, subPage->composeLinkPageNumber(i) ^ magazineFlip);
	}
}

void TeletextDocument::setDescription(QString newDescription)
{
	m_description = newDescription;
}

void TeletextDocument::setFastTextLinkPageNumberOnAllSubPages(int linkNumber, int pageNumber)
{
	for (auto &subPage : m_subPages)
		subPage->setFastTextLinkPageNumber(linkNumber, pageNumber);
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
