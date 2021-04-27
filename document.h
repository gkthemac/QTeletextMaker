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

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QObject>
#include <QUndoStack>
#include <vector>
#include "levelonepage.h"

class TeletextDocument : public QObject
{
	Q_OBJECT

public:
	// Available Page Functions according to 9.4.2.1 of the spec
	enum PageFunctionEnum { PFLevelOnePage, PFDataBroadcasting, PFGlobalPOP, PFNormalPOP, PFGlobalDRCS, PFNormalDRCS, PFMOT, PFMIP, PFBasicTOPTable, PFAdditionalInformationTable, PFMultiPageTable, PFMultiPageExtensionTable, PFTriggerMessages };
	// Available Page Codings of X/1 to X/25 according to 9.4.2.1 of the spec
	enum PacketCodingEnum { Coding7bit, Coding8bit, Coding18bit, Coding4bit, Coding4bitThen7bit, CodingPerPacket };

	TeletextDocument();
	~TeletextDocument();

	bool isEmpty() const;

	PageFunctionEnum pageFunction() const { return m_pageFunction; }
//	void setPageFunction(PageFunctionEnum);
	PacketCodingEnum packetCoding() const { return m_packetCoding; }
//	void setPacketCoding(PacketCodingEnum);

	int numberOfSubPages() const { return m_subPages.size(); }
	LevelOnePage* subPage(int p) const { return m_subPages[p]; }
	LevelOnePage* currentSubPage() const { return m_subPages[m_currentSubPageIndex]; }
	int currentSubPageIndex() const { return m_currentSubPageIndex; }
	void selectSubPageIndex(int, bool=false);
	void selectSubPageNext();
	void selectSubPagePrevious();
	void insertSubPage(int, bool);
	void deleteSubPage(int);
	void deleteSubPageToRecycle(int);
	void unDeleteSubPageFromRecycle(int);
	int pageNumber() const { return m_pageNumber; }
	void setPageNumber(QString);
	QString description() const { return m_description; }
	void setDescription(QString);
	void setFastTextLinkPageNumberOnAllSubPages(int, int);
	QUndoStack *undoStack() const { return m_undoStack; }
	int cursorRow() const { return m_cursorRow; }
	int cursorColumn() const { return m_cursorColumn; }
	void cursorUp(bool shiftKey=false);
	void cursorDown(bool shiftKey=false);
	void cursorLeft(bool shiftKey=false);
	void cursorRight(bool shiftKey=false);
	void moveCursor(int, int, bool selectionInProgress=false);
	int selectionTopRow() const { return qMin(m_selectionCornerRow, m_cursorRow); }
	int selectionBottomRow() const { return qMax(m_selectionCornerRow, m_cursorRow); }
	int selectionLeftColumn() const { return qMin(m_selectionCornerColumn, m_cursorColumn); }
	int selectionRightColumn() const { return qMax(m_selectionCornerColumn, m_cursorColumn); }
	int selectionWidth() const { return selectionRightColumn() - selectionLeftColumn() + 1; }
	int selectionHeight() const { return selectionBottomRow() - selectionTopRow() + 1; }
	bool selectionActive() const { return m_selectionSubPage == currentSubPage(); }
	void setSelectionCorner(int, int);
	void setSelection(int, int, int, int);
	void cancelSelection();
	int levelRequired() const;

signals:
	void cursorMoved();
	void selectionMoved();
	void colourChanged(int);
	void contentsChange(int);
	void aboutToChangeSubPage();
	void subPageSelected();
	void refreshNeeded();

	void tripletCommandHighlight(int);

private:
	QString m_description;
	int m_pageNumber, m_currentSubPageIndex;
	PageFunctionEnum m_pageFunction;
	PacketCodingEnum m_packetCoding;
	std::vector<LevelOnePage *> m_subPages;
	std::vector<LevelOnePage *> m_recycleSubPages;
	QUndoStack *m_undoStack;
	int m_cursorRow, m_cursorColumn, m_selectionCornerRow, m_selectionCornerColumn;
	LevelOnePage *m_selectionSubPage;
};

#endif
