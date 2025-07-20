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

#ifndef X28COMMANDS_H
#define X28COMMANDS_H

#include <QUndoCommand>

#include "document.h"

class X28Command : public QUndoCommand
{
public:
	X28Command(TeletextDocument *teletextDocument, QUndoCommand *parent = 0);

protected:
	TeletextDocument *m_teletextDocument;
	int m_subPageIndex;
};

class SetFullScreenColourCommand : public X28Command
{
public:
	enum { Id = 301 };

	SetFullScreenColourCommand(TeletextDocument *teletextDocument, int newColour, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;
	bool mergeWith(const QUndoCommand *command) override;
	int id() const override { return Id; }

private:
	int m_oldColour, m_newColour;
};

class SetFullRowColourCommand : public X28Command
{
public:
	enum { Id = 302 };

	SetFullRowColourCommand(TeletextDocument *teletextDocument, int newColour, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;
	bool mergeWith(const QUndoCommand *command) override;
	int id() const override { return Id; }

private:
	int m_oldColour, m_newColour;
};

class SetCLUTRemapCommand : public X28Command
{
public:
	enum { Id = 303 };

	SetCLUTRemapCommand(TeletextDocument *teletextDocument, int newMap, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;
	bool mergeWith(const QUndoCommand *command) override;
	int id() const override { return Id; }

private:
	int m_oldMap, m_newMap;
};

class SetBlackBackgroundSubstCommand : public X28Command
{
public:
	enum { Id = 304 };

	SetBlackBackgroundSubstCommand(TeletextDocument *teletextDocument, bool newSub, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;
	bool mergeWith(const QUndoCommand *command) override;
	int id() const override { return Id; }

private:
	int m_oldSub, m_newSub;
};

class SetColourCommand : public X28Command
{
public:
	SetColourCommand(TeletextDocument *teletextDocument, int colourIndex, int newColour, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;

private:
	int m_colourIndex, m_oldColour, m_newColour;
};

class ResetCLUTCommand : public X28Command
{
public:
	ResetCLUTCommand(TeletextDocument *teletextDocument, int colourTable, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;

private:
	int m_colourTable;
	int m_oldColourEntry[8];
};

class SetDCLUTCommand : public X28Command
{
public:
	SetDCLUTCommand(TeletextDocument *teletextDocument, bool globalDrcs, int mode, int index, int colour, QUndoCommand *parent = 0);

	void redo() override;
	void undo() override;

private:
	bool m_globalDrcs;
	int m_mode, m_index, m_oldColour, m_newColour;
};

#endif
