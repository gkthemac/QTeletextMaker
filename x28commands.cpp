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

#include "x28commands.h"

#include "document.h"

X28Command::X28Command(TeletextDocument *teletextDocument, QUndoCommand *parent) : QUndoCommand(parent)
{
	m_teletextDocument = teletextDocument;
	m_subPageIndex = teletextDocument->currentSubPageIndex();
}


SetFullScreenColourCommand::SetFullScreenColourCommand(TeletextDocument *teletextDocument, int newColour, QUndoCommand *parent) : X28Command(teletextDocument, parent)
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


SetFullRowColourCommand::SetFullRowColourCommand(TeletextDocument *teletextDocument, int newColour, QUndoCommand *parent) : X28Command(teletextDocument, parent)
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


SetCLUTRemapCommand::SetCLUTRemapCommand(TeletextDocument *teletextDocument, int newMap, QUndoCommand *parent) : X28Command(teletextDocument, parent)
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


SetBlackBackgroundSubstCommand::SetBlackBackgroundSubstCommand(TeletextDocument *teletextDocument, bool newSub, QUndoCommand *parent) : X28Command(teletextDocument, parent)
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


SetColourCommand::SetColourCommand(TeletextDocument *teletextDocument, int colourIndex, int newColour, QUndoCommand *parent) : X28Command(teletextDocument, parent)
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


ResetCLUTCommand::ResetCLUTCommand(TeletextDocument *teletextDocument, int colourTable, QUndoCommand *parent) : X28Command(teletextDocument, parent)
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
