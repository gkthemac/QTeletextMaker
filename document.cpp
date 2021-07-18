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

#include <QAbstractListModel>
#include <vector>

#include "document.h"

#include "levelonepage.h"

ClutModel::ClutModel(QObject *parent): QAbstractListModel(parent)
{
	m_subPage = nullptr;
}

int ClutModel::rowCount(const QModelIndex & /*parent*/) const
{
	return 32;
}

QVariant ClutModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role == Qt::DisplayRole)
		return QString("CLUT %1:%2").arg(index.row() >> 3).arg(index.row() & 0x07);

	if (role == Qt::DecorationRole && m_subPage != nullptr)
		return m_subPage->CLUTtoQColor(index.row());

	return QVariant();
}

void ClutModel::setSubPage(LevelOnePage *subPage)
{
	if (subPage != m_subPage) {
		m_subPage = subPage;
		emit dataChanged(createIndex(0, 0), createIndex(31, 0), QVector<int>(Qt::DecorationRole));
	}
}


TeletextDocument::TeletextDocument()
{
	m_pageNumber = 0x199;
	m_description.clear();
	m_pageFunction = PFLevelOnePage;
	m_packetCoding = Coding7bit;
	m_subPages.push_back(new LevelOnePage);
	m_currentSubPageIndex = 0;
	m_undoStack = new QUndoStack(this);
	m_cursorRow = 1;
	m_cursorColumn = 0;
	m_selectionCornerRow = m_selectionCornerColumn = -1;
	m_selectionSubPage = nullptr;

	m_clutModel = new ClutModel;
	m_clutModel->setSubPage(m_subPages[0]);
}

TeletextDocument::~TeletextDocument()
{
	delete m_clutModel;

	for (auto &subPage : m_subPages)
		delete(subPage);
	for (auto &recycleSubPage : m_recycleSubPages)
		delete(recycleSubPage);
}

bool TeletextDocument::isEmpty() const
{
	for (auto &subPage : m_subPages)
		if (!subPage->isEmpty())
			return false;

	return true;
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

void TeletextDocument::selectSubPageIndex(int newSubPageIndex, bool forceRefresh)
{
	// forceRefresh overrides "beyond the last subpage" check, so inserting a subpage after the last one still shows - dangerous workaround?
	if (forceRefresh || (newSubPageIndex != m_currentSubPageIndex && newSubPageIndex < m_subPages.size())) {
		emit aboutToChangeSubPage();

		m_currentSubPageIndex = newSubPageIndex;

		m_clutModel->setSubPage(m_subPages[m_currentSubPageIndex]);
		emit subPageSelected();
		emit selectionMoved();
		return;
	}
}

void TeletextDocument::selectSubPageNext()
{
	if (m_currentSubPageIndex < m_subPages.size()-1) {
		emit aboutToChangeSubPage();

		m_currentSubPageIndex++;

		m_clutModel->setSubPage(m_subPages[m_currentSubPageIndex]);
		emit subPageSelected();
		emit selectionMoved();
	}
}

void TeletextDocument::selectSubPagePrevious()
{
	if (m_currentSubPageIndex > 0) {
		emit aboutToChangeSubPage();

		m_currentSubPageIndex--;

		m_clutModel->setSubPage(m_subPages[m_currentSubPageIndex]);
		emit subPageSelected();
		emit selectionMoved();
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
	m_clutModel->setSubPage(nullptr);

	delete(m_subPages[subPageToDelete]);
	m_subPages.erase(m_subPages.begin()+subPageToDelete);
}

void TeletextDocument::deleteSubPageToRecycle(int subPageToRecycle)
{
	m_recycleSubPages.push_back(m_subPages[subPageToRecycle]);
	m_subPages.erase(m_subPages.begin()+subPageToRecycle);
}

void TeletextDocument::unDeleteSubPageFromRecycle(int subPage)
{
	m_subPages.insert(m_subPages.begin()+subPage, m_recycleSubPages.back());
	m_recycleSubPages.pop_back();
}

void TeletextDocument::setPageNumber(QString pageNumberString)
{
	bool pageNumberOk;
	int pageNumberRead = pageNumberString.toInt(&pageNumberOk, 16);
	if ((!pageNumberOk) || pageNumberRead < 0x100 || pageNumberRead > 0x8ff)
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

void TeletextDocument::cursorUp(bool shiftKey)
{
	if (shiftKey && !selectionActive())
		setSelectionCorner(m_cursorRow, m_cursorColumn);

	if (--m_cursorRow == 0)
		m_cursorRow = 24;

	if (shiftKey)
		emit selectionMoved();
	else
		cancelSelection();

	emit cursorMoved();
}

void TeletextDocument::cursorDown(bool shiftKey)
{
	if (shiftKey && !selectionActive())
		setSelectionCorner(m_cursorRow, m_cursorColumn);

	if (++m_cursorRow == 25)
		m_cursorRow = 1;

	if (shiftKey)
		emit selectionMoved();
	else
		cancelSelection();

	emit cursorMoved();
}

void TeletextDocument::cursorLeft(bool shiftKey)
{
	if (shiftKey && !selectionActive())
		setSelectionCorner(m_cursorRow, m_cursorColumn);

	if (--m_cursorColumn == -1) {
		m_cursorColumn = 39;
		cursorUp(shiftKey);
	}

	if (shiftKey)
		emit selectionMoved();
	else
		cancelSelection();

	emit cursorMoved();
}

void TeletextDocument::cursorRight(bool shiftKey)
{
	if (shiftKey && !selectionActive())
		setSelectionCorner(m_cursorRow, m_cursorColumn);

	if (++m_cursorColumn == 40) {
		m_cursorColumn = 0;
		cursorDown(shiftKey);
	}

	if (shiftKey)
		emit selectionMoved();
	else
		cancelSelection();

	emit cursorMoved();
}

void TeletextDocument::moveCursor(int cursorRow, int cursorColumn, bool selectionInProgress)
{
	if (selectionInProgress && !selectionActive())
		setSelectionCorner(m_cursorRow, m_cursorColumn);

	if (cursorRow != -1)
		m_cursorRow = cursorRow;
	if (cursorColumn != -1)
		m_cursorColumn = cursorColumn;

	if (selectionInProgress)
		emit selectionMoved();
	else
		cancelSelection();

	emit cursorMoved();
}

void TeletextDocument::setSelectionCorner(int row, int column)
{
	if (m_selectionCornerRow != row || m_selectionCornerColumn != column) {
		m_selectionSubPage = currentSubPage();
		m_selectionCornerRow = row;
		m_selectionCornerColumn = column;

//		emit selectionMoved();
	}
}

void TeletextDocument::setSelection(int topRow, int leftColumn, int bottomRow, int rightColumn)
{
	if (selectionTopRow() != topRow || selectionBottomRow() != bottomRow || selectionLeftColumn() != leftColumn || selectionRightColumn() != rightColumn) {
		m_selectionSubPage = currentSubPage();
		m_selectionCornerRow = topRow;
		m_cursorRow = bottomRow;
		m_selectionCornerColumn = leftColumn;
		m_cursorColumn = rightColumn;

		emit selectionMoved();
		emit cursorMoved();
	}
}

void TeletextDocument::cancelSelection()
{
	if (m_selectionSubPage != nullptr) {
		m_selectionSubPage = nullptr;
		emit selectionMoved();
		m_selectionCornerRow = m_selectionCornerColumn = -1;
	}
}

int TeletextDocument::levelRequired() const
{
	int levelSeen = 0;

	for (auto &subPage : m_subPages) {
		levelSeen = qMax(levelSeen, subPage->levelRequired());
		if (levelSeen == 3)
			break;
	}

	return levelSeen;
}
