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

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QFile>
#include <QObject>
#include <QTextStream>
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
	bool isEmpty() const { return m_empty; }
	void setModified(bool);

	PageFunctionEnum pageFunction() const { return m_pageFunction; }
//	void setPageFunction(PageFunctionEnum);
	PacketCodingEnum packetCoding() const { return m_packetCoding; }
//	void setPacketCoding(PacketCodingEnum);

	void loadDocument(QFile *);
	void saveDocument(QTextStream *);
	int numberOfSubPages() const { return m_subPages.size(); }
	LevelOnePage* currentSubPage() const { return m_subPages[m_currentSubPageIndex]; }
	int currentSubPageIndex() const { return m_currentSubPageIndex; }
	void selectSubPageIndex(int, bool=false);
	void selectSubPageNext();
	void selectSubPagePrevious();
	void insertSubPage(int, bool);
	void deleteSubPage(int);
	int pageNumber() const { return m_pageNumber; }
	void setPageNumber(QString);
	QString description() const { return m_description; }
	void setDescription(QString);
	void setFastTextLinkPageNumberOnAllSubPages(int, int);
	QUndoStack *undoStack() const { return m_undoStack; }
	int cursorRow() const { return m_cursorRow; }
	int cursorColumn() const { return m_cursorColumn; }
	void cursorUp();
	void cursorDown();
	void cursorLeft();
	void cursorRight();
	void moveCursor(int, int);
	int selectionTopRow() const { return m_selectionTopRow; }
	int selectionBottomRow() const { return m_selectionBottomRow; }
	int selectionLeftColumn() const { return m_selectionLeftColumn; }
	int selectionRightColumn() const { return m_selectionRightColumn; }
	int selectionWidth() const { return m_selectionRightColumn - m_selectionLeftColumn + 1; }
	int selectionHeight() const { return m_selectionBottomRow - m_selectionTopRow + 1; }
	bool selectionActive() const { return m_selectionSubPage == currentSubPage(); }
	void setSelection(int, int, int, int);
	void cancelSelection();

signals:
	void cursorMoved();
	void selectionMoved();
	void colourChanged(int);
	void contentsChange(int);
	void aboutToChangeSubPage();
	void subPageSelected();
	void refreshNeeded();

private:
	QString m_description;
	bool m_empty;
	int m_pageNumber, m_currentSubPageIndex;
	PageFunctionEnum m_pageFunction;
	PacketCodingEnum m_packetCoding;
	std::vector<LevelOnePage *> m_subPages;
	QUndoStack *m_undoStack;
	int m_cursorRow, m_cursorColumn, m_selectionTopRow, m_selectionBottomRow, m_selectionLeftColumn, m_selectionRightColumn;
	LevelOnePage *m_selectionSubPage;
};

#endif
