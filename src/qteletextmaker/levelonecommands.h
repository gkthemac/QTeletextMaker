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

#ifndef LEVELONECOMMANDS_H
#define LEVELONECOMMANDS_H

#include <QByteArrayList>
#include <QSet>
#include <QUndoCommand>

#include "document.h"

class LevelOneCommand : public QUndoCommand
{
public:
	LevelOneCommand(TeletextDocument *teletextDocument, QUndoCommand *parent = 0);

protected:
	QByteArrayList storeCharacters(int topRow, int leftColumn, int bottomRow, int rightColumn);
	void retrieveCharacters(int topRow, int leftColumn, const QByteArrayList &oldChars);

	TeletextDocument *m_teletextDocument;
	int m_subPageIndex, m_row, m_column;
	bool m_firstDo;
};

class TypeCharacterCommand : public LevelOneCommand
{
public:
	enum { Id = 101 };

	TypeCharacterCommand(TeletextDocument *teletextDocument, unsigned char newCharacter, bool insertMode, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;
	bool mergeWith(const QUndoCommand *command) override;
	int id() const override { return Id; }

private:
	unsigned char m_newCharacter, m_oldRowContents[40], m_newRowContents[40];
	int m_columnStart, m_columnEnd;
	bool m_insertMode;
};

class ToggleMosaicBitCommand : public LevelOneCommand
{
public:
	enum { Id = 102 };

	ToggleMosaicBitCommand(TeletextDocument *teletextDocument, unsigned char bitToToggle, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;
	bool mergeWith(const QUndoCommand *command) override;
	int id() const override { return Id; }

private:
	unsigned char m_oldCharacter, m_newCharacter;
};

class BackspaceKeyCommand : public LevelOneCommand
{
public:
	enum { Id = 103 };

	BackspaceKeyCommand(TeletextDocument *teletextDocument, bool insertMode, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;
	bool mergeWith(const QUndoCommand *command) override;
	int id() const override { return Id; }

private:
	unsigned char m_oldRowContents[40], m_newRowContents[40];
	int m_columnStart, m_columnEnd;
	bool m_insertMode;
};

class DeleteKeyCommand : public LevelOneCommand
{
public:
	enum { Id = 104 };

	DeleteKeyCommand(TeletextDocument *teletextDocument, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;
	bool mergeWith(const QUndoCommand *command) override;
	int id() const override { return Id; }

private:
	unsigned char m_oldRowContents[40], m_newRowContents[40];
};

class ShiftMosaicsCommand : public LevelOneCommand
{
public:
	ShiftMosaicsCommand(TeletextDocument *teletextDocument, const QSet<QPair<int, int>> &mosaicList, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;
	bool mergeWith(const QUndoCommand *command) override;

protected:
	QByteArrayList m_oldCharacters, m_newCharacters;
	QSet<QPair<int, int>> m_mosaicList;
	int m_selectionTopRow, m_selectionBottomRow, m_selectionLeftColumn, m_selectionRightColumn;
	int m_selectionCornerRow, m_selectionCornerColumn;
};

class ShiftMosaicsUpCommand : public ShiftMosaicsCommand
{
public:
	enum { Id = 110 };

	ShiftMosaicsUpCommand(TeletextDocument *teletextDocument, const QSet<QPair<int, int>> &mosaicList, QUndoCommand *parent = 0);

	int id() const override { return Id; }
};

class ShiftMosaicsDownCommand : public ShiftMosaicsCommand
{
public:
	enum { Id = 111 };

	ShiftMosaicsDownCommand(TeletextDocument *teletextDocument, const QSet<QPair<int, int>> &mosaicList, QUndoCommand *parent = 0);

	int id() const override { return Id; }
};

class ShiftMosaicsLeftCommand : public ShiftMosaicsCommand
{
public:
	enum { Id = 112 };

	ShiftMosaicsLeftCommand(TeletextDocument *teletextDocument, const QSet<QPair<int, int>> &mosaicList, QUndoCommand *parent = 0);

	int id() const override { return Id; }
};

class ShiftMosaicsRightCommand : public ShiftMosaicsCommand
{
public:
	enum { Id = 113 };

	ShiftMosaicsRightCommand(TeletextDocument *teletextDocument, const QSet<QPair<int, int>> &mosaicList, QUndoCommand *parent = 0);

	int id() const override { return Id; }
};

class FillMosaicsCommand : public ShiftMosaicsCommand
{
public:
	enum { Id = 120 };

	FillMosaicsCommand(TeletextDocument *teletextDocument, const QSet<QPair<int, int>> &mosaicList, QUndoCommand *parent = 0);

	int id() const override { return Id; }
};

class ClearMosaicsCommand : public ShiftMosaicsCommand
{
public:
	enum { Id = 121 };

	ClearMosaicsCommand(TeletextDocument *teletextDocument, const QSet<QPair<int, int>> &mosaicList, QUndoCommand *parent = 0);

	int id() const override { return Id; }
};

class InvertMosaicsCommand : public ShiftMosaicsCommand
{
public:
	enum { Id = 122 };

	InvertMosaicsCommand(TeletextDocument *teletextDocument, const QSet<QPair<int, int>> &mosaicList, QUndoCommand *parent = 0);

	bool mergeWith(const QUndoCommand *command) override;
	int id() const override { return Id; }
};

class DitherMosaicsCommand : public ShiftMosaicsCommand
{
public:
	enum { Id = 123 };

	DitherMosaicsCommand(TeletextDocument *teletextDocument, const QSet<QPair<int, int>> &mosaicList, QUndoCommand *parent = 0);

	int id() const override { return Id; }
};

class InsertSubPageCommand : public LevelOneCommand
{
public:
	InsertSubPageCommand(TeletextDocument *teletextDocument, bool afterCurrentSubPage, bool copySubPage, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;

private:
	int m_newSubPageIndex;
	bool m_copySubPage;
};

class DeleteSubPageCommand : public LevelOneCommand
{
public:
	DeleteSubPageCommand(TeletextDocument *teletextDocument, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;
};

class InsertRowCommand : public LevelOneCommand
{
public:
	InsertRowCommand(TeletextDocument *teletextDocument, bool copyRow, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;

private:
	bool m_copyRow;
	unsigned char m_deletedBottomRow[40];
};

class DeleteRowCommand : public LevelOneCommand
{
public:
	DeleteRowCommand(TeletextDocument *teletextDocument, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;

private:
	unsigned char m_deletedRow[40];
};

#ifndef QT_NO_CLIPBOARD
class CutCommand : public LevelOneCommand
{
public:
	CutCommand(TeletextDocument *teletextDocument, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;

private:
	QByteArrayList m_oldCharacters;
	int m_selectionTopRow, m_selectionBottomRow, m_selectionLeftColumn, m_selectionRightColumn;
	int m_selectionCornerRow, m_selectionCornerColumn;
};

class PasteCommand : public LevelOneCommand
{
public:
	PasteCommand(TeletextDocument *teletextDocument, int pageCharSet, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;

private:
	QByteArrayList m_oldCharacters, m_pastingCharacters;
	int m_pasteTopRow, m_pasteBottomRow, m_pasteLeftColumn, m_pasteRightColumn;
	int m_clipboardDataHeight, m_clipboardDataWidth;
	int m_selectionCornerRow, m_selectionCornerColumn;
	bool m_selectionActive, m_plainText;
};
#endif // !QT_NO_CLIPBOARD

#endif
