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

#include <QAbstractListModel>
#include <QList>
#include <QVariant>

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
		emit dataChanged(createIndex(0, 0), createIndex(31, 0), QList<int>(Qt::DecorationRole));
	}
}


TeletextDocument::TeletextDocument()
{
	m_pageNumber = 0x199;
	m_description.clear();
	m_subPages.append(new LevelOnePage);
	m_currentSubPageIndex = 0;
	m_undoStack = new QUndoStack(this);
	m_cursorRow = 1;
	m_cursorColumn = 0;
	m_rowZeroAllowed = false;
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

void TeletextDocument::clear()
{
	m_subPages.prepend(new LevelOnePage);

	emit aboutToChangeSubPage();
	m_currentSubPageIndex = 0;
	m_clutModel->setSubPage(m_subPages[0]);
	emit subPageSelected();
	cancelSelection();
	m_undoStack->clear();

	for (int i=m_subPages.size()-1; i>0; i--) {
		delete(m_subPages[i]);
		m_subPages.remove(i);
	}
}

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
		m_subPages.append(insertedSubPage);
	else
		m_subPages.insert(beforeSubPageIndex, insertedSubPage);
}

void TeletextDocument::deleteSubPage(int subPageToDelete)
{
	m_clutModel->setSubPage(nullptr);

	delete(m_subPages[subPageToDelete]);
	m_subPages.remove(subPageToDelete);
}

void TeletextDocument::deleteSubPageToRecycle(int subPageToRecycle)
{
	m_recycleSubPages.append(m_subPages[subPageToRecycle]);
	m_subPages.remove(subPageToRecycle);
}

void TeletextDocument::unDeleteSubPageFromRecycle(int subPage)
{
	m_subPages.insert(subPage, m_recycleSubPages.last());
	m_recycleSubPages.removeLast();
}

void TeletextDocument::loadFromList(QList<PageBase> const &subPageList)
{
	*m_subPages[0] = subPageList.at(0);

	for (int i=1; i<subPageList.size(); i++)
		m_subPages.append(new LevelOnePage(subPageList.at(i)));
}

void TeletextDocument::loadMetaData(QVariantHash const &metadata)
{
	bool valueOk;

	if (const QString description = metadata.value("description").toString(); !description.isEmpty())
		m_description = description;

	if (const int pageNumber = metadata.value("pageNumber").toInt(&valueOk); valueOk)
		m_pageNumber = pageNumber;

	if (metadata.value("fastextAbsolute").toBool()) {
		const int magazineFlip = m_pageNumber & 0x700;

		for (auto &subPage : m_subPages)
			for (int i=0; i<6; i++)
				subPage->setFastTextLinkPageNumber(i, subPage->fastTextLinkPageNumber(i) ^ magazineFlip);
	}

	for (int i=0; i<numberOfSubPages(); i++) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
		const QString subPageStr = QString("%1").arg(i, 3, '0');
#else
		const QString subPageStr = QString("%1").arg(i, 3, QChar('0'));
#endif

		if (int region = metadata.value("region" + subPageStr).toInt(&valueOk); valueOk)
			subPage(i)->setDefaultCharSet(region);
		if (int cycleValue = metadata.value("cycleValue" + subPageStr).toInt(&valueOk); valueOk)
			subPage(i)->setCycleValue(cycleValue);
		QChar cycleType = metadata.value("cycleType" + subPageStr).toChar();
		if (cycleType == 'C')
			subPage(i)->setCycleType(LevelOnePage::CTcycles);
		else if (cycleType == 'T')
			subPage(i)->setCycleType(LevelOnePage::CTseconds);
	}
}

void TeletextDocument::setPageNumber(int pageNumber)
{
	// If the magazine number was changed, we need to update the relative magazine numbers in FastText
	// and page enhancement links
	int oldMagazine = (m_pageNumber & 0xf00);
	int newMagazine = (pageNumber & 0xf00);
	// Fix magazine 0 to 8
	if (oldMagazine == 0x800)
		oldMagazine = 0x000;
	if (newMagazine == 0x800)
		newMagazine = 0x000;
	int magazineFlip = oldMagazine ^ newMagazine;

	m_pageNumber = pageNumber;

	for (auto &subPage : m_subPages)
		if (magazineFlip) {
			for (int i=0; i<6; i++)
				subPage->setFastTextLinkPageNumber(i, subPage->fastTextLinkPageNumber(i) ^ magazineFlip);
			for (int i=0; i<8; i++)
				subPage->setComposeLinkPageNumber(i, subPage->composeLinkPageNumber(i) ^ magazineFlip);
	}
}

void TeletextDocument::setPageNumberFromString(QString pageNumberString)
{
	bool pageNumberOk;
	int pageNumberRead = pageNumberString.toInt(&pageNumberOk, 16);

	if ((!pageNumberOk) || pageNumberRead < 0x100 || pageNumberRead > 0x8ff)
		return;

	setPageNumber(pageNumberRead);
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

	if (--m_cursorRow == 0 - (int)m_rowZeroAllowed)
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
		m_cursorRow = (int)!m_rowZeroAllowed;

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

void TeletextDocument::setRowZeroAllowed(bool allowed)
{
	m_rowZeroAllowed = allowed;
	if (m_cursorRow == 0 && !allowed)
		cursorDown();
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
