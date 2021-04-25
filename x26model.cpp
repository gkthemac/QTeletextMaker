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

#include "x26model.h"

#include <QList>

#include "x26commands.h"

X26Model::X26Model(TeletextWidget *parent): QAbstractListModel(parent)
{
	m_parentMainWidget = parent;
	m_listLoaded = true;
}

void X26Model::setX26ListLoaded(bool newListLoaded)
{
	beginResetModel();
	m_listLoaded = newListLoaded;
	endResetModel();
}

int X26Model::rowCount(const QModelIndex & /*parent*/) const
{
	return m_listLoaded ? m_parentMainWidget->document()->currentSubPage()->enhancements()->size() : 0;
}

int X26Model::columnCount(const QModelIndex & /*parent*/) const
{
	return 4;
}

QVariant X26Model::data(const QModelIndex &index, int role) const
{
	int mode = m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).mode();

	// Qt::UserRole will always return the raw values
	if (role == Qt::UserRole)
		switch (index.column()) {
			case 0:
				return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address();
			case 1:
				return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).mode();
			case 2:
				return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data();
			default:
				return QVariant();
		}

	if (role == Qt::DisplayRole || role == Qt::EditRole)
		switch (index.column()) {
			case 0:
				// Show row number only if address part of triplet actually represents a row
				// i.e. Full row colour, Set Active Position and Origin Modifier
				if (!m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).isRowTriplet())
					return QVariant();
				// For Origin Modifier address of 40 refers to same row, so show it as 0
				if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).mode() == 0x10) {
					if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() == 40)
						return 0;
					else
						return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).addressRow();
				}
				if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).mode() == 0x01 || m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).mode() == 0x04)
					return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).addressRow();
				else
					return QVariant();
			case 1:
				if (!m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).isRowTriplet())
					return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).addressColumn();
				// For Set Active Position and Origin Modifier, data is the column
				else if (mode == 0x04 || mode == 0x10)
					return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data();
				else
					return QVariant();
		}

	if (!m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).isRowTriplet())
		mode |= 0x20;

	QString result;

	if (role == Qt::DisplayRole) {
		if (index.column() == 2)
			return (modeTripletName[mode]);
		// Column 3 - describe effects of data/address triplet parameters in plain English
		switch (mode) {
			case 0x01: // Full row colour
			case 0x07: // Address row 0
				if ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x60) == 0x60)
					result = ", down to bottom";
				else if ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x60) == 0x00)
					result = ", this row only";
				// fall-through
			case 0x00: // Full screen colour
				if (!(result.isEmpty()) || (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x60) == 0x00) {
					result.prepend(QString("CLUT %1:%2").arg((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x18) >> 3).arg(m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x07));
					return result;
				}
				break;
			case 0x04: // Set Active Position
			case 0x10: // Origin Modifier
				// For Set Active Position and Origin Modifier, data is the column, so return blank
				return QVariant();
			case 0x11 ... 0x13: // Invoke object
				switch (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() & 0x18) {
					case 0x08:
						return QString("Local: d%1 t%2").arg((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() >> 4) | ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() & 0x01) << 3)).arg(m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x0f);
					case 0x10:
						result = "POP";
						break;
					case 0x18:
						result = "GPOP";
						break;
					// case 0x00: shouldn't happen since that would make a column triplet, not a row triplet
				}
				result.append(QString(": subpage %1 pkt %2 trplt %3 bits ").arg(m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x0f).arg((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() & 0x03) + 1).arg(((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x60) >> 5) * 3 + (mode & 0x03)));
				if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x10)
					result.append("10-18");
				else
					result.append("1-9");
				return result;
			case 0x15 ... 0x17: // Define object
				result = (QString("Local: d%1 t%2, ").arg((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() >> 4) | ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() & 1) << 3)).arg(m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x0f));
				switch (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() & 0x18) {
					case 0x08:
						result.append("L2.5 only");
						break;
					case 0x10:
						result.append("L3.5 only");
						break;
					case 0x18:
						result.append("L2.5 and 3.5");
						break;
					// case 0x00: shouldn't happen since that would make a column triplet, not a row triplet
				}
				return result;
			case 0x18: // DRCS mode
				result = (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x40) == 0x40 ? "Normal" : "Global";
				result.append(QString(": subpage %1, ").arg(m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x0f));
				switch (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x30) {
					case 0x10:
						result.append("L2.5 only");
						break;
					case 0x20:
						result.append("L3.5 only");
						break;
					case 0x30:
						result.append("L2.5 and 3.5");
						break;
					case 0x00:
						result.append("Reserved");
						break;
				}
				return result;
			case 0x1f: // Termination
				switch (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x07) {
					case 0x00:
						return "Intermed (G)POP subpage. End of object, more follows";
						break;
					case 0x01:
						return "Intermed (G)POP subpage. End of last object on page";
						break;
					case 0x02:
						return "Last (G)POP subpage. End of object, more follows";
						break;
					case 0x03:
						return "Last (G)POP subpage. End of last object on page";
						break;
					case 0x04:
						return "Local object definitions. End of object, more follows";
						break;
					case 0x05:
						return "Local object definitions. End of last object on page";
						break;
					case 0x06:
						return "End of local enhance data. Local objects follow";
						break;
					case 0x07:
						return "End of local enhance data. No local objects";
						break;
				}
				break;
			case 0x08 ... 0x0d: // PDC
				return QString("0x%1").arg(m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data(), 2, 16, QChar('0'));
			case 0x20: // Foreground colour
			case 0x23: // Background colour
				if (!(m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x60))
					return QString("CLUT %1:%2").arg((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x18) >> 3).arg(m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x07);
				break;
			case 0x21: // G1 mosaic character
			case 0x22: // G3 mosaic character at level 1.5
			case 0x29: // G0 character
			case 0x2b: // G3 mosaic character at level >=2.5
			case 0x2f ... 0x3f: // G2 character or G0 diacritical mark
				if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() >= 0x20)
					return QString("0x%1").arg(m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data(), 2, 16);
				break;
			case 0x27: // Flash functions
				if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() < 0x18) {
					switch (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x03) {
						case 0x00:
							result = "Steady";
							break;
						case 0x01:
							result = "Normal";
							break;
						case 0x02:
							result = "Invert";
							break;
						case 0x03:
							result = "Adj CLUT";
							break;
					}
					switch (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x1c) {
						case 0x00:
							result.append(", 1Hz");
							break;
						case 0x04:
							result.append(", 2Hz ph 1");
							break;
						case 0x08:
							result.append(", 2Hz ph 2");
							break;
						case 0x0c:
							result.append(", 2Hz ph 3");
							break;
						case 0x10:
							result.append(", 2Hz inc");
							break;
						case 0x14:
							result.append(", 2Hz dec");
							break;
					}
					return result;
				}
				break;
			//TODO case 0x28: // G0 and G2 designation
			case 0x2c: // Display attributes
				if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x02)
					result.append("Boxing ");
				if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x04)
					result.append("Conceal ");
				if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x10)
					result.append("Invert ");
				if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x20)
					result.append("Underline ");
				if (result.isEmpty())
					result = "None";
				else
					// Chop off the last space
					result.chop(1);
				switch (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x41) {
					case 0x00:
						result.append(", normal size");
						break;
					case 0x01:
						result.append(", double height");
						break;
					case 0x40:
						result.append(", double width");
						break;
					case 0x41:
						result.append(", double size");
						break;
				}
				return result;
			case 0x2d: // DRCS character
				result = (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x40) == 0x40 ? "Normal" : "Global";
				result.append(QString(": %1").arg(m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x3f));
				return result;
			case 0x2e: // Font style
				if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x01)
					result.append("Proportional ");
				if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x02)
					result.append("Bold ");
				if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x04)
					result.append("Italic ");
				if (result.isEmpty())
					result = "None";
				else
					// Chop off the last space
					result.chop(1);
				result.append(QString(", %1 row(s)").arg(m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() >> 4));
				return result;
			case 0x28: // Modified G0 and G2 character set
			case 0x26: // PDC
				return QString("0x%1").arg(m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data(), 2, 16, QChar('0'));
			default: // Reserved
				return QString("Reserved 0x%1").arg(m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data(), 2, 16, QChar('0'));
		}
		// Reserved mode or data
		return QString("Reserved 0x%1").arg(m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data(), 2, 16, QChar('0'));
	}

	if (role == Qt::EditRole && index.column() == 2) {
		if (!m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).isRowTriplet())
			mode |= 0x20;
		return mode;
	}

	if (role <= Qt::UserRole)
		return QVariant();

	switch (role) {
		case Qt::UserRole+1:
			switch (mode) {
				case 0x00: // Full screen colour
				case 0x01: // Full row colour
				case 0x07: // Address row 0
				case 0x20: // Foreground colour
				case 0x23: // Background colour
					// Colour index
					return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x1f;
				case 0x11 ... 0x13: // Invoke object
					// Object source: Local, POP or GPOP
					return ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() & 0x18) >> 3) - 1;
					break;
				case 0x15 ... 0x17: // Define object
					// Required at which levels
					return ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() & 0x18) >> 3) - 1;
				case 0x18: // DRCS mode
					// Required at which levels
					return ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x30) >> 4) - 1;
				case 0x1f: // Termination
					// Intermed POP subpage|Last POP subpage|Local Object|Local enhance
					return ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x06) >> 1);
				case 0x27: // Flash functions
					// Flash mode
					return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x03;
				case 0x2c: // Display attributes
					// Text size
					return (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x01) | ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x40) >> 5);
				case 0x2d: // DRCS character
					// Normal or Global
					return (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x40) == 0x40;
				case 0x2e: // Font style
					// Proportional
					return (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x01) == 0x01;
				default:
					return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data();
			}
			break;

		case Qt::UserRole+2:
			switch (mode) {
				case 0x01: // Full row colour
				case 0x07: // Address row 0
					// "this row only" or "down to bottom"
					return (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x60) == 0x60;
				case 0x11 ... 0x13: // Invoke object
					if ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() & 0x08) == 0x08)
						// Local object: Designation code
						return ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() & 0x01) << 3) | ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x70) >> 4);
					else
						// (G)POP object: Subpage
						return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x0f;
					break;
				case 0x15 ... 0x17: // Define object
					// Local object: Designation code
					return ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() & 0x01) << 3) | ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x70) >> 4);
				case 0x1f: // Termination
					// More follows/Last
					return (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x01) == 0x01;
				case 0x27: // Flash functions
					// Flash rate and phase
					return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() >> 2;
				case 0x2c: // Display attributes
					// Boxing/window
					return (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x02) == 0x02;
				case 0x2d: // DRCS character
					// Character number
					return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x3f;
				case 0x2e: // Font style
					// Bold
					return (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x02) == 0x02;
			}
			break;

		case Qt::UserRole+3:
			switch (mode) {
				case 0x11 ... 0x13: // Invoke object
					if ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() & 0x08) == 0x08)
						// Local object: Triplet number
						return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x0f;
					else
						// (G)POP object: Pointer location
						return (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() & 0x03) + 1;
					break;
				case 0x15 ... 0x17: // Define object
					// Local object: Triplet number
					return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x0f;
				case 0x18: // DRCS mode
					// Normal or Global
					return (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x40) == 0x40;
				case 0x2c: // Display attributes
					// Conceal
					return (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x04) == 0x04;
				case 0x2e: // Font style
					// Italics
					return (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x04) == 0x04;
			}
			break;

		case Qt::UserRole+4:
			switch (mode) {
				case 0x11 ... 0x13: // Invoke object
					// (G)POP object: Triplet number
					return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() >> 5;
				case 0x18: // DRCS mode
					// Subpage
					return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x0f;
				case 0x2c: // Display attributes
					// Invert
					return (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x10) == 0x10;
				case 0x2e: // Font style
					// Number of rows
					return m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() >> 4;
			}
			break;

		case Qt::UserRole+5:
			switch (mode) {
				case 0x11 ... 0x13: // Invoke object
					// (G)POP object: Pointer position
					return (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x10) >> 4;
				case 0x2c: // Display attributes
					// Underline/Separated
					return (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x20) == 0x20;
			}
			break;
	}

	return QVariant();
}

bool X26Model::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid())
		return false;

	// Raw address, mode and data values
	if (role == Qt::UserRole && value.canConvert<int>() && index.column() <= 2) {
		m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), index.column(), 0x00, value.toInt(), role));

		return true;
	}

	int mode = m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).modeExt();

	// Cooked row, column and triplet mode
	if (role == Qt::EditRole && value.canConvert<int>()) {
		int intValue = value.toInt();
		if (intValue < 0)
			return false;

		switch (index.column()) {
			case 0: // Cooked row
				// Maximum row is 24
				if (intValue > 24)
					return false;
				// Set Active Position and Full Row Colour can't select row 0
				if (((mode == 0x04) || (mode == 0x01)) && intValue == 0)
					return false;
				// For Origin Modifier address of 40 refers to same row
				if (mode == 0x10 && intValue == 24) {
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x00, 40, role));
					return true;
				}
				// Others use address 40 for row 24
				m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x00, (intValue == 24) ? 40 : intValue+40, role));
				return true;

			case 1: // Cooked column
				// Origin modifier allows columns up to 71
				if (intValue > (mode == 0x10 ? 71 : 39))
					return false;
				// For Set Active Position and Origin Modifier, data is the column
				if (mode == 0x04 || mode == 0x10)
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x00, intValue, role));
				else
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x00, intValue, role));
				return true;

			case 2: // Cooked triplet mode
				if (intValue < 0x20 && !m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).isRowTriplet()) {
					// Changing mode from column triplet to row triplet
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x00, 41, role));
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x00, 0, role));
				}
				if (intValue >= 0x20 && m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).isRowTriplet()) {
					// Changing mode from row triplet to column triplet
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x00, 0, role));
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x00, 0, role));
				}
				m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETmode, 0x00, intValue & 0x1f, role));

				// Now set data values to avoid reserved bits if we need to
				// FIXME this can rather messily push multiple EditTripletCommands
				// that rely on mergeWith to tidy them up afterwards
				// Also this just flips bits, where we could use default values
				switch (intValue) {
					case 0x00: // Full screen colour
					case 0x20: // Foreground colour
					case 0x23: // Background colour
						// Both S1 and S0 reserved bits must be clear
						if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x60)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x1f, 0x00, role));
						break;
					case 0x07: // Address row 0
						// Address locked to 63
						if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() != 63)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x00, 63, role));
						// fall-through
					case 0x01: // Full row colour
						// S1 and S0 bits need to be the same
						if ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x60) != 0x00 && (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x60) != 0x60)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x1f, 0x00, role));
						break;
					case 0x04: // Set Active Position
						// Data range 0-39
						if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() >= 40)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x00, 0, role));
						break;
					case 0x10: // Origin modifier
						// Data range 0-71
						if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() >= 72)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x00, 0, role));
						break;
					case 0x11: // Invoke Active Object
					case 0x12: // Invoke Adaptive Object
					case 0x13: // Invoke Passive Object
					case 0x15: // Define Active Object
					case 0x16: // Define Adaptive Object
					case 0x17: // Define Passive Object
						// Bit 3 of Address is reserved
						if ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() & 0x04) == 0x04)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x7b, 0x00, role));
						// BUG we're only dealing with Local Object Definitions at the moment!
						// If source is Local, triplet number must be in range 0-12
						if (((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() & 0x18) == 0x08) && ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x0f) >= 12))
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x70, 0x00, role));
						break;
					case 0x18: // DRCS mode
						// At least one of the L1 and L0 bits must be set
						if ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x30) == 0x00)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x4f, 0x30, role));
						break;
					case 0x1f: // Termination marker
						// Address locked to 63
						if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() != 63)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x00, 63, role));
						// Clear reserved bits D6-D3
						if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x78)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x07, 0x00, role));
						break;
					case 0x21: // G1 mosaic character
					case 0x22: // G3 mosaic character at level 1.5
					case 0x29: // G0 character
					case 0x2b: // G3 mosaic character at level >=2.5
					case 0x2f ... 0x3f: // G2 character or G0 diacritical mark
						// Data range 0x20-0x7f
						if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() < 0x20)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x00, 0x20, role));
						break;
					case 0x27: // Additional flash functions
						// D6 and D5 must be clear, D4 and D3 set is reserved phase
						if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() >= 0x18)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x00, 0, role));
						break;
					case 0x28: // Display attributes
					case 0x2e: // Font style
						// Clear reserved bit D3
						if (m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x08)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x77, 0x00, role));
						break;
					case 0x2d: // DRCS character
						// D5-D0 range 0-47
						if ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).data() & 0x3f) >= 48)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x40, 0x77, role));
						break;
				};
				return true;
		}
		return false;
	}

	if (role <= Qt::UserRole || !value.canConvert<int>())
		return false;

	int intValue = value.toInt();
	if (intValue < 0)
		return false;

	// Triplet data
	switch (role) {
		case Qt::UserRole+1:
			switch (mode) {
				case 0x01: // Full row colour
				case 0x07: // Address row 0
					// Colour index
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x60, value.toInt(), role));
					break;
				case 0x11 ... 0x13: // Invoke object
					// Object source: Local, POP or GPOP
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x27, (value.toInt()+1) << 3, role));
					break;
				case 0x15 ... 0x17: // Define object
					// Required at which levels
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x27, (value.toInt()+1) << 3, role));
					break;
				case 0x18: // DRCS Mode
					// Required at which levels
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x4f, (value.toInt()+1) << 4, role));
					break;
				case 0x1f: // Termination
					// Intermed POP subpage|Last POP subpage|Local Object|Local enhance
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x01, value.toInt() << 1, role));
					break;
				case 0x27: // Flash functions
					// Flash mode
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x1c, value.toInt(), role));
					break;
				case 0x2c: // Display attributes
					// Text size
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x36, ((value.toInt() & 0x02) << 5) | (value.toInt() & 0x01), role));
					break;
				case 0x2d: // DRCS character
					// Normal or Global
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x3f, value.toInt() << 6, role));
					break;
				case 0x2e: // Font style
					// Proportional
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x7e, value.toInt(), role));
					break;
				default: // Others set the complete data value
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x00, value.toInt(), role));
			}
			break;

		case Qt::UserRole+2:
			switch (mode) {
				case 0x00: // Full screen colour
				case 0x01: // Full row colour
				case 0x07: // Address row 0
					// "this row only" or "down to bottom"
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x1f, value.toInt() * 0x60, role));
					break;
				case 0x11 ... 0x13: // Invoke object
					if ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() & 0x08) == 0x08) {
						// Local object: Designation code
						m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x0f, (value.toInt() & 0x07) << 4, role));
						m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x38, value.toInt() >> 3, role));
					} else
						// (G)POP object: Subpage
						m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x30, value.toInt(), role));
					break;
				case 0x15 ... 0x17: // Define object
					// Local object: Designation code
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x0f, (value.toInt() & 0x07) << 4, role));
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x38, value.toInt() >> 3, role));
					break;
				case 0x1f: // Termination
					// More follows/Last
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x06, value.toInt(), role));
					break;
				case 0x27: // Flash functions
					// Flash rate and phase
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x03, value.toInt() << 2, role));
					break;
				case 0x2c: // Display attributes
					// Boxing/window
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x7d, value.toInt() << 1, role));
					break;
				case 0x2d: // DRCS character
					// Character number
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x40, value.toInt(), role));
					break;
				case 0x2e: // Font style
					// Bold
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x7d, value.toInt() << 1, role));
					break;
			}
			break;

		case Qt::UserRole+3:
			switch (mode) {
				case 0x11 ... 0x13: // Invoke object
					if ((m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row()).address() & 0x08) == 0x08)
						// Local object: triplet number
						m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x70, value.toInt(), role));
					else
						// (G)POP object: Pointer location
						m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x7c, value.toInt() - 1, role));
					break;
				case 0x15 ... 0x17: // Define object
					// Local object: triplet number
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x70, value.toInt(), role));
					break;
				case 0x18: // DRCS Mode
					// Normal or Global
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x3f, value.toInt() << 6, role));
					break;
				case 0x2c: // Display attributes
					// Conceal
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x7b, value.toInt() << 2, role));
					break;
				case 0x2e: // Font style
					// Italics
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x7b, value.toInt() << 2, role));
					break;
			}
			break;

		case Qt::UserRole+4:
			switch (mode) {
				case 0x11 ... 0x13: // Invoke object
					// (G)POP object: Triplet number
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x1f, value.toInt() << 5, role));
					break;
				case 0x18: // DRCS Mode
					// Subpage
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x70, value.toInt(), role));
					break;
				case 0x2c: // Display attributes
					// Invert
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x6f, value.toInt() << 4, role));
					break;
				case 0x2e: // Font style
					// Number of rows
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x07, value.toInt() << 4, role));
					break;
			}
			break;

		case Qt::UserRole+5:
			switch (mode) {
				case 0x11 ... 0x13: // Invoke object
					// (G)POP object: Pointer position
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x6f, value.toInt() << 4, role));
					break;
				case 0x2c: // Display attributes
					// Underline/Separated
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x5f, value.toInt() << 5, role));
					break;
			}
			break;

	}
	return true;
}

QVariant X26Model::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal) {
			switch (section) {
				case 0:
					return tr("Row");
				case 1:
					return tr("Col");
				case 2:
					return tr("Mode");
				case 3:
					return tr("Data");
				default:
					return QVariant();
			}
		} else if (orientation == Qt::Vertical)
			return QString("d%1 t%2").arg(section / 13).arg(section % 13);
	}
	return QVariant();
}

bool X26Model::insertFirstRow()
{
	m_parentMainWidget->document()->undoStack()->push(new InsertTripletCommand(m_parentMainWidget->document(), this, 0, 1, X26Triplet(63, 31, 7)));

	return true;
}

bool X26Model::insertRows(int row, int count, const QModelIndex &parent)
{
	Q_UNUSED(parent);

	if (m_parentMainWidget->document()->currentSubPage()->enhancements()->size() + count > m_parentMainWidget->document()->currentSubPage()->maxEnhancements())
		return false;

	m_parentMainWidget->document()->undoStack()->push(new InsertTripletCommand(m_parentMainWidget->document(), this, row, count, m_parentMainWidget->document()->currentSubPage()->enhancements()->at(row)));
	return true;
}

bool X26Model::removeRows(int row, int count, const QModelIndex &index)
{
	Q_UNUSED(index);

	m_parentMainWidget->document()->undoStack()->push(new DeleteTripletCommand(m_parentMainWidget->document(), this, row, count));
	return true;
}

/*
Qt::ItemFlags X26Model::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return QAbstractItemModel::flags(index);

	if (index.column() <= 1)
		return QAbstractItemModel::flags(index);
	if (index.column() >= 2)
		return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;

	return QAbstractItemModel::flags(index);
}
*/
