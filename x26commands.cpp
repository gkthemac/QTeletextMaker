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

#include "x26commands.h"

#include <QUndoCommand>

#include "document.h"
#include "x26model.h"
#include "x26triplets.h"

InsertTripletCommand::InsertTripletCommand(TeletextDocument *teletextDocument, X26Model *x26Model, int row, int count, X26Triplet newTriplet, QUndoCommand *parent) : QUndoCommand(parent)
{
	m_teletextDocument = teletextDocument;
	m_subPageIndex = teletextDocument->currentSubPageIndex();
	m_x26Model = x26Model;
	m_row = row;
	m_count = count;
	m_insertedTriplet = newTriplet;
	setText(QString("insert triplet d%1 t%2").arg(m_row / 13).arg(m_row % 13));;
	m_firstDo = true;
}

void InsertTripletCommand::redo()
{
	bool changingSubPage = (m_teletextDocument->currentSubPageIndex() != m_subPageIndex);

	if (changingSubPage) {
		m_teletextDocument->emit aboutToChangeSubPage();
		m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	} else
		m_x26Model->beginInsertRows(QModelIndex(), m_row, m_row+m_count-1);

	for (int i=0; i<m_count; i++)
		m_teletextDocument->currentSubPage()->enhancements()->insert(m_row+i, m_insertedTriplet);

	if (changingSubPage)
		m_teletextDocument->emit subPageSelected();
	else {
		m_x26Model->endInsertRows();
		m_teletextDocument->emit refreshNeeded();
	}

	if (m_firstDo)
		m_firstDo = false;
	else
		m_teletextDocument->emit tripletCommandHighlight(m_row+1);
}

void InsertTripletCommand::undo()
{
	bool changingSubPage = (m_teletextDocument->currentSubPageIndex() != m_subPageIndex);

	if (changingSubPage) {
		m_teletextDocument->emit aboutToChangeSubPage();
		m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	} else
		m_x26Model->beginRemoveRows(QModelIndex(), m_row, m_row+m_count-1);

	for (int i=0; i<m_count; i++)
		m_teletextDocument->currentSubPage()->enhancements()->removeAt(m_row);

	if (changingSubPage)
		m_teletextDocument->emit subPageSelected();
	else {
		m_x26Model->endRemoveRows();
		m_teletextDocument->emit refreshNeeded();
	}
}


DeleteTripletCommand::DeleteTripletCommand(TeletextDocument *teletextDocument, X26Model *x26Model, int row, int count, QUndoCommand *parent) : QUndoCommand(parent)
{
	m_teletextDocument = teletextDocument;
	m_subPageIndex = teletextDocument->currentSubPageIndex();
	m_x26Model = x26Model;
	m_row = row;
	m_count = count;
	m_deletedTriplet = m_teletextDocument->subPage(m_subPageIndex)->enhancements()->at(m_row);
	setText(QString("delete triplet d%1 t%2").arg(m_row / 13).arg(m_row % 13));;
	// BUG we only store one of the deleted triplets
	if (m_count > 1)
		qDebug("More than one triplet row deleted, undo won't put the lost triplets back!");
}

void DeleteTripletCommand::redo()
{
	m_teletextDocument->emit aboutToChangeSubPage();
	m_teletextDocument->selectSubPageIndex(m_subPageIndex);

	m_x26Model->beginRemoveRows(QModelIndex(), m_row, m_row+m_count-1);
	for (int i=0; i<m_count; i++)
		m_teletextDocument->currentSubPage()->enhancements()->removeAt(m_row);
	m_x26Model->endRemoveRows();

	m_teletextDocument->emit subPageSelected();
}

void DeleteTripletCommand::undo()
{
	bool changingSubPage = (m_teletextDocument->currentSubPageIndex() != m_subPageIndex);

	if (changingSubPage) {
		m_teletextDocument->emit aboutToChangeSubPage();
		m_teletextDocument->selectSubPageIndex(m_subPageIndex);
	} else
		m_x26Model->beginInsertRows(QModelIndex(), m_row, m_row+m_count-1);

	for (int i=0; i<m_count; i++)
		m_teletextDocument->currentSubPage()->enhancements()->insert(m_row+i, m_deletedTriplet);

	if (changingSubPage)
		m_teletextDocument->emit subPageSelected();
	else {
		m_x26Model->endInsertRows();
		m_teletextDocument->emit refreshNeeded();
	}

	m_teletextDocument->emit tripletCommandHighlight(m_row);
}


EditTripletCommand::EditTripletCommand(TeletextDocument *teletextDocument, X26Model *x26Model, int row, int tripletPart, int bitsToKeep, int newValue, int role, QUndoCommand *parent) : QUndoCommand(parent)
{
	m_teletextDocument = teletextDocument;
	m_subPageIndex = teletextDocument->currentSubPageIndex();
	m_x26Model = x26Model;
	m_row = row;
	m_role = role;
	m_oldTriplet = m_newTriplet = teletextDocument->currentSubPage()->enhancements()->at(m_row);
	setText(QString("edit triplet d%1 t%2").arg(m_row / 13).arg(m_row % 13));;
	switch (tripletPart) {
		case ETaddress:
			m_newTriplet.setAddress((m_newTriplet.address() & bitsToKeep) | newValue);
			break;
		case ETmode:
			m_newTriplet.setMode((m_newTriplet.mode() & bitsToKeep) | newValue);
			break;
		case ETdata:
			m_newTriplet.setData((m_newTriplet.data() & bitsToKeep) | newValue);
			break;
	};
	m_firstDo = true;
}

void EditTripletCommand::redo()
{
	if (m_teletextDocument->currentSubPageIndex() != m_subPageIndex)
		m_teletextDocument->selectSubPageIndex(m_subPageIndex, true);

	m_teletextDocument->currentSubPage()->enhancements()->replace(m_row, m_newTriplet);
	m_x26Model->emit dataChanged(m_x26Model->createIndex(m_row, 0), m_x26Model->createIndex(m_row, 3), {m_role});
	m_teletextDocument->emit refreshNeeded();

	if (m_firstDo)
		m_firstDo = false;
	else
		m_teletextDocument->emit tripletCommandHighlight(m_row);
}

void EditTripletCommand::undo()
{
	if (m_teletextDocument->currentSubPageIndex() != m_subPageIndex)
		m_teletextDocument->selectSubPageIndex(m_subPageIndex, true);

	m_teletextDocument->currentSubPage()->enhancements()->replace(m_row, m_oldTriplet);
	m_x26Model->emit dataChanged(m_x26Model->createIndex(m_row, 0), m_x26Model->createIndex(m_row, 3), {m_role});
	m_teletextDocument->emit refreshNeeded();
	m_teletextDocument->emit tripletCommandHighlight(m_row);
}

bool EditTripletCommand::mergeWith(const QUndoCommand *command)
{
	const EditTripletCommand *newerCommand = static_cast<const EditTripletCommand *>(command);

	if (m_subPageIndex != newerCommand->m_subPageIndex || m_row != newerCommand->m_row)
		return false;

	m_newTriplet = newerCommand->m_newTriplet;
	return true;
}
