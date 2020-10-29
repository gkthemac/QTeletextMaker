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
#include <QUndoCommand>
#include <QUndoStack>
#include <vector>
#include "levelonepage.h"

class TeletextDocument : public QObject
{
	Q_OBJECT

public:
	TeletextDocument();
	~TeletextDocument();
	bool isEmpty() const { return m_empty; }
	void setModified(bool);
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
	std::vector<LevelOnePage *> m_subPages;
	QUndoStack *m_undoStack;
	int m_cursorRow, m_cursorColumn, m_selectionTopRow, m_selectionBottomRow, m_selectionLeftColumn, m_selectionRightColumn;
	LevelOnePage *m_selectionSubPage;
};


class OverwriteCharacterCommand : public QUndoCommand
{
public:
	OverwriteCharacterCommand(TeletextDocument *, unsigned char, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;

private:
	TeletextDocument *m_teletextDocument;
	unsigned char m_oldCharacter, m_newCharacter;
	int m_subPageIndex, m_row, m_column;
};

class ToggleMosaicBitCommand : public QUndoCommand
{
public:
	enum { Id = 2 };

	ToggleMosaicBitCommand(TeletextDocument *, unsigned char, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;
	bool mergeWith(const QUndoCommand *) override;
	int id() const override { return Id; }

private:
	TeletextDocument *m_teletextDocument;
	unsigned char m_oldCharacter, m_newCharacter;
	int m_subPageIndex, m_row, m_column;
};

class BackspaceCommand : public QUndoCommand
{
public:
	BackspaceCommand(TeletextDocument *, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;

private:
	TeletextDocument *m_teletextDocument;
	unsigned char m_deletedCharacter;
	int m_subPageIndex, m_row, m_column;
};

class InsertSubPageCommand : public QUndoCommand
{
public:
	InsertSubPageCommand(TeletextDocument *, bool, bool, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;

private:
	TeletextDocument *m_teletextDocument;
	int m_newSubPageIndex;
	bool m_copySubPage;
};

class InsertRowCommand : public QUndoCommand
{
public:
	InsertRowCommand(TeletextDocument *, bool, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;

private:
	TeletextDocument *m_teletextDocument;
	int m_subPageIndex, m_row;
	bool m_copyRow;
	unsigned char m_deletedBottomRow[40];
};

class DeleteRowCommand : public QUndoCommand
{
public:
	DeleteRowCommand(TeletextDocument *, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;

private:
	TeletextDocument *m_teletextDocument;
	int m_subPageIndex, m_row;
	unsigned char m_deletedRow[40];
};

class SetColourCommand : public QUndoCommand
{
public:
	SetColourCommand(TeletextDocument *, int, int, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;

private:
	TeletextDocument *m_teletextDocument;
	int m_subPageIndex, m_colourIndex, m_oldColour, m_newColour;
};

class ResetCLUTCommand : public QUndoCommand
{
public:
	ResetCLUTCommand(TeletextDocument *, int, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;

private:
	TeletextDocument *m_teletextDocument;
	int m_subPageIndex, m_colourTable;
	int m_oldColourEntry[8];
};
#endif
