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

#include "x26model.h"

#include <QIcon>
#include <QList>

#include "x26commands.h"
#include "x26menus.h"

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
	const X26Triplet triplet = m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row());

	// Qt::UserRole will always return the raw values
	if (role == Qt::UserRole)
		switch (index.column()) {
			case 0:
				return triplet.address();
			case 1:
				return triplet.mode();
			case 2:
				return triplet.data();
			default:
				return QVariant();
		}

	// Error colours from KDE Plasma Breeze (light) theme
	if (role == Qt::ForegroundRole) {
		if (triplet.error() != X26Triplet::NoError && index.column() == m_tripletErrors[triplet.error()].columnHighlight)
			return QColor(252, 252, 252);
		else if ((index.column() == 2 && triplet.reservedMode()) || (index.column() == 3 && triplet.reservedData()))
			return QColor(35, 38, 39);
	}

	if (role == Qt::BackgroundRole) {
		if (triplet.error() != X26Triplet::NoError && index.column() == m_tripletErrors[triplet.error()].columnHighlight)
			return QColor(218, 68, 63);
		else if ((index.column() == 2 && triplet.reservedMode()) || (index.column() == 3 && triplet.reservedData()))
			return QColor(246, 116, 0);
	}

	if (role == Qt::ToolTipRole && triplet.error() != X26Triplet::NoError)
		return m_tripletErrors[triplet.error()].message;

	if (role == Qt::DisplayRole || role == Qt::EditRole)
		switch (index.column()) {
			case 0:
				// Show row number only if address part of triplet actually represents a row:
				// Full row colour, Set Active Position and Origin Modifier
				// For Origin Modifier address of 40 refers to same row, so show it as 0
				if (triplet.modeExt() == 0x10) {
					if (triplet.address() == 40)
						return "+0";
					else
						return QString("+%1").arg(triplet.addressRow());
				}
				if (triplet.modeExt() == 0x01 || triplet.modeExt() == 0x04)
					return triplet.addressRow();
				else
					return QVariant();
			case 1:
				if (!triplet.isRowTriplet())
					return triplet.addressColumn();
				// For Set Active Position and Origin Modifier, data is the column
				else if (triplet.modeExt() == 0x04)
					return triplet.data();
				else if (triplet.modeExt() == 0x10)
					return QString("+%1").arg(triplet.data());
				else
					return QVariant();
		}

	QString result;

	if (role == Qt::DisplayRole) {
		if (index.column() == 2)
			return (m_modeTripletNames.modeName(triplet.modeExt()));
		// Column 3 - describe effects of data/address triplet parameters in plain English
		switch (triplet.modeExt()) {
			case 0x01: // Full row colour
			case 0x07: // Address row 0
				if ((triplet.data() & 0x60) == 0x60)
					result = ", down to bottom";
				else if ((triplet.data() & 0x60) == 0x00)
					result = ", this row only";
				// fall-through
			case 0x00: // Full screen colour
				if (!(result.isEmpty()) || (triplet.data() & 0x60) == 0x00) {
					result.prepend(QString("CLUT %1:%2").arg((triplet.data() & 0x18) >> 3).arg(triplet.data() & 0x07));
					return result;
				}
				break;
			case 0x04: // Set Active Position
			case 0x10: // Origin Modifier
				// For Set Active Position and Origin Modifier, data is the column, so return blank
				return QVariant();
			case 0x11: // Invoke Active Object
			case 0x12: // Invoke Adaptive Object
			case 0x13: // Invoke Passive Object
				switch (triplet.address() & 0x18) {
					case 0x08:
						return QString("Local: d%1 t%2").arg((triplet.data() >> 4) | ((triplet.address() & 0x01) << 3)).arg(triplet.data() & 0x0f);
					case 0x10:
						result = "POP";
						break;
					case 0x18:
						result = "GPOP";
						break;
					// case 0x00: shouldn't happen since that would make a column triplet, not a row triplet
				}
				result.append(QString(": subpage %1 pkt %2 trplt %3 bits ").arg(triplet.data() & 0x0f).arg((triplet.address() & 0x03) + 1).arg(((triplet.data() & 0x60) >> 5) * 3 + (triplet.modeExt() & 0x03)));
				if (triplet.data() & 0x10)
					result.append("10-18");
				else
					result.append("1-9");
				return result;
			case 0x15: // Define Active Object
			case 0x16: // Define Adaptive Object
			case 0x17: // Define Passive Object
				switch (triplet.address() & 0x18) {
					case 0x08:
						return "Local: L2.5 only";
						break;
					case 0x10:
						return "Local: L3.5 only";
						break;
					case 0x18:
						return "Local: L2.5 and 3.5";
						break;
					// case 0x00: shouldn't happen since that would make a column triplet, not a row triplet
				}
				break;
			case 0x18: // DRCS mode
				result = (triplet.data() & 0x40) == 0x40 ? "Normal" : "Global";
				result.append(QString(": subpage %1, ").arg(triplet.data() & 0x0f));
				switch (triplet.data() & 0x30) {
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
				switch (triplet.data() & 0x07) {
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
			case 0x08: // PDC country of origin & programme source
			case 0x09: // PDC month & day
			case 0x0a: // PDC cursor row & announced start hour
			case 0x0b: // PDC cursor row & announced finish hour
			case 0x0c: // PDC cursor row & local time offset
			case 0x0d: // PDC series ID & series code
				return QString("0x%1").arg(triplet.data(), 2, 16, QChar('0'));
			case 0x20: // Foreground colour
			case 0x23: // Background colour
				if (!(triplet.data() & 0x60))
					return QString("CLUT %1:%2").arg((triplet.data() & 0x18) >> 3).arg(triplet.data() & 0x07);
				break;
			case 0x21: // G1 mosaic character
			case 0x22: // G3 mosaic character at level 1.5
			case 0x29: // G0 character
			case 0x2b: // G3 mosaic character at level >=2.5
			case 0x2f: // G2 character
				if (triplet.data() >= 0x20)
					return QString("0x%1").arg(triplet.data(), 2, 16);
				break;
			case 0x27: // Flash functions
				if (triplet.data() < 0x18) {
					switch (triplet.data() & 0x03) {
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
					switch (triplet.data() & 0x1c) {
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
			case 0x28: // Modified G0 and G2 character set
				switch (triplet.data()) {
					case 0x20:
						return QString("0x20 Cyrillic 1 Serbian/Croatian");
					case 0x24:
						return QString("0x24 Cyrillic 2 Russian/Bulgarian");
					case 0x25:
						return QString("0x25 Cyrillic 3 Ukranian");
					case 0x36:
						return QString("0x36 Latin");
					case 0x37:
						return QString("0x37 Greek");
					case 0x40:
					case 0x44:
						return QString("0x%1 G0 Latin, G2 Arabic").arg(triplet.data(), 2, 16);
					case 0x47:
					case 0x57:
						return QString("0x%1 Arabic").arg(triplet.data(), 2, 16);
					case 0x55:
						return QString("0x55 G0 Hebrew, G2 Arabic");
				}
				if (triplet.data() < 0x27)
					return QString("0x%1 Latin").arg(triplet.data(), 2, 16, QChar('0'));
				break;
			case 0x2c: // Display attributes
				if (triplet.data() & 0x02)
					result.append("Boxing ");
				if (triplet.data() & 0x04)
					result.append("Conceal ");
				if (triplet.data() & 0x10)
					result.append("Invert ");
				if (triplet.data() & 0x20)
					result.append("Underline ");
				if (result.isEmpty())
					result = "None";
				else
					// Chop off the last space
					result.chop(1);
				switch (triplet.data() & 0x41) {
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
				result = (triplet.data() & 0x40) == 0x40 ? "Normal" : "Global";
				result.append(QString(": %1").arg(triplet.data() & 0x3f));
				return result;
			case 0x2e: // Font style
				if (triplet.data() & 0x01)
					result.append("Proportional ");
				if (triplet.data() & 0x02)
					result.append("Bold ");
				if (triplet.data() & 0x04)
					result.append("Italic ");
				if (result.isEmpty())
					result = "None";
				else
					// Chop off the last space
					result.chop(1);
				result.append(QString(", %1 row(s)").arg(triplet.data() >> 4));
				return result;
			case 0x26: // PDC
				return QString("0x%1").arg(triplet.data(), 2, 16, QChar('0'));
			default:
				if (triplet.modeExt() >= 0x30 && triplet.modeExt() <= 0x3f && triplet.data() >= 0x20)
					// G0 with diacritical
					return QString("0x%1").arg(triplet.data(), 2, 16);
				else
					// Reserved
					return QString("Reserved 0x%1").arg(triplet.data(), 2, 16, QChar('0'));
		}
		// Reserved mode or data
		return QString("Reserved 0x%1").arg(triplet.data(), 2, 16, QChar('0'));
	}

	if (role == Qt::DecorationRole && index.column() == 3)
		switch (triplet.modeExt()) {
			case 0x00: // Full screen colour
			case 0x20: // Foreground colour
			case 0x23: // Background colour
				if (!(triplet.data() & 0x60))
					return m_parentMainWidget->document()->currentSubPage()->CLUTtoQColor(triplet.data());
				break;
			case 0x01: // Full row colour
			case 0x07: // Address row 0
				if (((triplet.data() & 0x60) == 0x00) || ((triplet.data() & 0x60) == 0x60))
					return m_parentMainWidget->document()->currentSubPage()->CLUTtoQColor(triplet.data() & 0x1f);
				break;
			case 0x21: // G1 mosaic character
				if (triplet.data() & 0x20)
					// Returning the bitmap as-is doesn't seem to work here
					// but putting it inside a QIcon... does?
					return QIcon(m_fontBitmap.charBitmap(triplet.data(), 24));
				break;
			case 0x22: // G3 mosaic character at level 1.5
			case 0x2b: // G3 mosaic character at level >=2.5
				if (triplet.data() >= 0x20)
					return QIcon(m_fontBitmap.charBitmap(triplet.data(), 26));
				break;
			case 0x2f: // G2 character
				if (triplet.data() >= 0x20)
					return QIcon(m_fontBitmap.charBitmap(triplet.data(), m_parentMainWidget->pageDecode()->cellG2CharacterSet(triplet.activePositionRow(), triplet.activePositionColumn())));
				break;
			default:
				if (triplet.modeExt() == 0x29 || (triplet.modeExt() >= 0x30 && triplet.modeExt() <= 0x3f))
					// G0 character or G0 diacritical mark
					if (triplet.data() >= 0x20)
						return QIcon(m_fontBitmap.charBitmap(triplet.data(), m_parentMainWidget->pageDecode()->cellG0CharacterSet(triplet.activePositionRow(), triplet.activePositionColumn())));
		}

	if (role == Qt::EditRole && index.column() == 2)
		return triplet.modeExt();

	if (role < Qt::UserRole)
		return QVariant();

	switch (triplet.modeExt()) {
		case 0x01: // Full row colour
		case 0x07: // Address row 0
			if (role == Qt::UserRole+2) // "this row only" or "down to bottom"
				return (triplet.data() & 0x60) == 0x60;
			// fall-through
		case 0x00: // Full screen colour
		case 0x20: // Foreground colour
		case 0x23: // Background colour
			if (role == Qt::UserRole+1) // Colour index
				return triplet.data() & 0x1f;
			break;
		case 0x11: // Invoke Active Object
		case 0x12: // Invoke Adaptive Object
		case 0x13: // Invoke Passive Object
			switch (role) {
				case Qt::UserRole+1: // Object source: Local, POP or GPOP
					return ((triplet.address() & 0x18) >> 3) - 1;
				case Qt::UserRole+2:
					if ((triplet.address() & 0x18) == 0x08)
						// Local object: Designation code
						return ((triplet.address() & 0x01) << 3) | ((triplet.data() & 0x70) >> 4);
					else
						// (G)POP object: Subpage
						return triplet.data() & 0x0f;
				case Qt::UserRole+3:
					if ((triplet.address() & 0x08) == 0x08)
						// Local object: Triplet number
						return triplet.data() & 0x0f;
					else
						// (G)POP object: Pointer location
						return (triplet.address() & 0x03) + 1;
				case Qt::UserRole+4: // (G)POP object: Triplet number
					return triplet.data() >> 5;
				case Qt::UserRole+5: // (G)POP object: Pointer position
					return (triplet.data() & 0x10) >> 4;
			}
			break;
		case 0x15: // Define Active Object
		case 0x16: // Define Adaptive Object
		case 0x17: // Define Passive Object
			switch (role) {
				case Qt::UserRole+1: // Required at which levels
					return ((triplet.address() & 0x18) >> 3) - 1;
				case Qt::UserRole+2: // Local object: Designation code
					return ((triplet.address() & 0x01) << 3) | ((triplet.data() & 0x70) >> 4);
				case Qt::UserRole+3: // Local object: Triplet number
					return triplet.data() & 0x0f;
			}
			break;
		case 0x18: // DRCS mode
			switch (role) {
				case Qt::UserRole+1: // Required at which levels
					return ((triplet.data() & 0x30) >> 4) - 1;
				case Qt::UserRole+3: // Normal or Global
					return (triplet.data() & 0x40) == 0x40;
				case Qt::UserRole+4: // Subpage
					return triplet.data() & 0x0f;
			}
			break;
		case 0x1f: // Termination
			switch (role) {
				case Qt::UserRole+1: // Intermed POP subpage|Last POP subpage|Local Object|Local enhance
					return ((triplet.data() & 0x06) >> 1);
				case Qt::UserRole+2: // More follows/Last
					return (triplet.data() & 0x01) == 0x01;
			}
			break;
		case 0x27: // Flash functions
			switch (role) {
				case Qt::UserRole+1: // Flash mode
					return triplet.data() & 0x03;
				case Qt::UserRole+2: // Flash rate and phase
					return triplet.data() >> 2;
			}
			break;
		case 0x2c: // Display attributes
			switch (role) {
				case Qt::UserRole+1: // Text size
					return (triplet.data() & 0x01) | ((triplet.data() & 0x40) >> 5);
				case Qt::UserRole+2: // Boxing/window
					return (triplet.data() & 0x02) == 0x02;
				case Qt::UserRole+3: // Conceal
					return (triplet.data() & 0x04) == 0x04;
				case Qt::UserRole+4: // Invert
					return (triplet.data() & 0x10) == 0x10;
				case Qt::UserRole+5: // Underline/Separated
					return (triplet.data() & 0x20) == 0x20;
			}
			break;
		case 0x2d: // DRCS character
			switch (role) {
				case Qt::UserRole+1: // Normal or Global
					return (triplet.data() & 0x40) == 0x40;
				case Qt::UserRole+2: // Character number
					return triplet.data() & 0x3f;
			}
			break;
		case 0x2e: // Font style
			switch (role) {
				case Qt::UserRole+1: // Proportional
					return (triplet.data() & 0x01) == 0x01;
				case Qt::UserRole+2: // Bold
					return (triplet.data() & 0x02) == 0x02;
				case Qt::UserRole+3: // Italic
					return (triplet.data() & 0x04) == 0x04;
				case Qt::UserRole+4: // Number of rows
					return triplet.data() >> 4;
			}
			break;
		case 0x2f: // G2 character
			// Qt::UserRole+1 is character number, returned by default below
			if (role == Qt::UserRole+2) // Character set
				return m_parentMainWidget->pageDecode()->cellG2CharacterSet(triplet.activePositionRow(), triplet.activePositionColumn());
			break;
		default:
			if (triplet.modeExt() == 0x29 || (triplet.modeExt() >= 0x30 && triplet.modeExt() <= 0x3f))
				// G0 character or G0 diacritical mark
				// Qt::UserRole+1 is character number, returned by default below
				if (role == Qt::UserRole+2) // Character set
					return m_parentMainWidget->pageDecode()->cellG0CharacterSet(triplet.activePositionRow(), triplet.activePositionColumn());
	};

	// For characters and other triplet modes, return the complete data value
	if (role == Qt::UserRole+1)
		return triplet.data();

	return QVariant();
}

bool X26Model::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid())
		return false;

	if (!value.canConvert<int>())
		return false;

	const int intValue = value.toInt();

	if (intValue < 0)
		return false;

	// Raw address, mode and data values
	if (role == Qt::UserRole && index.column() <= 2) {
		m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), index.column(), 0x00, intValue, role));

		return true;
	}

	const X26Triplet triplet = m_parentMainWidget->document()->currentSubPage()->enhancements()->at(index.row());

	// Cooked row, column and triplet mode
	if (role == Qt::EditRole) {
		switch (index.column()) {
			case 0: // Cooked row
				// Maximum row is 24
				if (intValue > 24)
					return false;
				// Set Active Position and Full Row Colour can't select row 0
				if (((triplet.modeExt() == 0x04) || (triplet.modeExt() == 0x01)) && intValue == 0)
					return false;
				// For Origin Modifier address of 40 refers to same row
				if (triplet.modeExt() == 0x10 && intValue == 24) {
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x00, 40, role));
					return true;
				}
				// Others use address 40 for row 24
				m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x00, (intValue == 24) ? 40 : intValue+40, role));
				return true;

			case 1: // Cooked column
				// Origin modifier allows columns up to 71
				if (intValue > (triplet.modeExt() == 0x10 ? 71 : 39))
					return false;
				// For Set Active Position and Origin Modifier, data is the column
				if (triplet.modeExt() == 0x04 || triplet.modeExt() == 0x10)
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x00, intValue, role));
				else
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x00, intValue, role));
				return true;

			case 2: // Cooked triplet mode
				if (intValue < 0x20 && !triplet.isRowTriplet()) {
					// Changing mode from column triplet to row triplet
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x00, 41, role));
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x00, 0, role));
				}
				if (intValue >= 0x20 && triplet.isRowTriplet()) {
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
						if (triplet.data() & 0x60)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x1f, 0x00, role));
						break;
					case 0x07: // Address row 0
						// Address locked to 63
						if (triplet.address() != 63)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x00, 63, role));
						// fall-through
					case 0x01: // Full row colour
						// S1 and S0 bits need to be the same
						if ((triplet.data() & 0x60) != 0x00 && (triplet.data() & 0x60) != 0x60)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x1f, 0x00, role));
						break;
					case 0x04: // Set Active Position
						// Data range 0-39
						if (triplet.data() >= 40)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x00, 0, role));
						break;
					case 0x10: // Origin modifier
						// Data range 0-71
						if (triplet.data() >= 72)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x00, 0, role));
						break;
					case 0x11: // Invoke Active Object
					case 0x12: // Invoke Adaptive Object
					case 0x13: // Invoke Passive Object
					case 0x15: // Define Active Object
					case 0x16: // Define Adaptive Object
					case 0x17: // Define Passive Object
						// Bit 3 of Address is reserved
						if ((triplet.address() & 0x04) == 0x04)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x7b, 0x00, role));
						// BUG we're only dealing with Local Object Definitions at the moment!
						// If source is Local, triplet number must be in range 0-12
						if (((triplet.address() & 0x18) == 0x08) && ((triplet.data() & 0x0f) >= 12))
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x70, 0x00, role));
						break;
					case 0x18: // DRCS mode
						// At least one of the L1 and L0 bits must be set
						if ((triplet.data() & 0x30) == 0x00)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x4f, 0x30, role));
						break;
					case 0x1f: // Termination marker
						// Address locked to 63
						if (triplet.address() != 63)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x00, 63, role));
						// Clear reserved bits D6-D3
						if (triplet.data() & 0x78)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x07, 0x00, role));
						break;
					case 0x27: // Additional flash functions
						// D6 and D5 must be clear, D4 and D3 set is reserved phase
						if (triplet.data() >= 0x18)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x00, 0, role));
						break;
					case 0x2c: // Display attributes
					case 0x2e: // Font style
						// Clear reserved bit D3
						if (triplet.data() & 0x08)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x77, 0x00, role));
						break;
					case 0x2d: // DRCS character
						// D5-D0 range 0-47
						if ((triplet.data() & 0x3f) >= 48)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x40, 0x77, role));
						break;
					case 0x21: // G1 mosaic character
					case 0x22: // G3 mosaic character at level 1.5
					case 0x29: // G0 character
					case 0x2b: // G3 mosaic character at level >=2.5
					case 0x2f: // G2 character
						// Data range 0x20-0x7f
						if (triplet.data() < 0x20)
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x00, 0x20, role));
						break;
					default:
						if (intValue >= 0x30 && intValue <= 0x3f && triplet.data() < 0x20)
						// G0 diacritical mark
						// Data range 0x20-0x7f
							m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x00, 0x20, role));
				}
				return true;
		}
		return false;
	}

	if (role < Qt::UserRole)
		return false;

	switch (triplet.modeExt()) {
		case 0x01: // Full row colour
		case 0x07: // Address row 0
			switch (role) {
				case Qt::UserRole+1: // Colour index
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x60, intValue, role));
					return true;
				case Qt::UserRole+2: // "this row only" or "down to bottom"
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x1f, intValue * 0x60, role));
					break;
			}
			break;
		case 0x11: // Invoke Active Object
		case 0x12: // Invoke Adaptive Object
		case 0x13: // Invoke Passive Object
			switch (role) {
				case Qt::UserRole+1: // Object source: Local, POP or GPOP
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x27, (intValue+1) << 3, role));
					return true;
				case Qt::UserRole+2:
					if ((triplet.address() & 0x18) == 0x08) {
						// Local object: Designation code
						m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x0f, (intValue & 0x07) << 4, role));
						m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x38, intValue >> 3, role));
					} else
						// (G)POP object: Subpage
						m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x30, intValue, role));
					return true;
				case Qt::UserRole+3: // Invoke object
					if ((triplet.address() & 0x18) == 0x08)
						// Local object: triplet number
						m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x70, intValue, role));
					else
						// (G)POP object: Pointer location
						m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x7c, intValue - 1, role));
					return true;
				case Qt::UserRole+4: // (G)POP object: Triplet number
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x1f, intValue << 5, role));
					return true;
				case Qt::UserRole+5: // (G)POP object: Pointer position
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x6f, intValue << 4, role));
					return true;
			}
			break;
		case 0x15: // Define Active Object
		case 0x16: // Define Adaptive Object
		case 0x17: // Define Passive Object
			switch (role) {
				case Qt::UserRole+1: // Required at which levels
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x27, (intValue+1) << 3, role));
					return true;
				case Qt::UserRole+2: // Local object: Designation code
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x0f, (intValue & 0x07) << 4, role));
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETaddress, 0x38, intValue >> 3, role));
					return true;
				case Qt::UserRole+3: // Local object: triplet number
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x70, intValue, role));
					break;
			}
			break;
		case 0x18: // DRCS Mode
			switch (role) {
				case Qt::UserRole+1: // Required at which levels
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x4f, (intValue+1) << 4, role));
					return true;
				case Qt::UserRole+3: // Normal or Global
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x3f, intValue << 6, role));
					return true;
				case Qt::UserRole+4: // Subpage
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x70, intValue, role));
					return true;
			}
			break;
		case 0x1f: // Termination
			switch (role) {
				case Qt::UserRole+1: // Intermed POP subpage|Last POP subpage|Local Object|Local enhance
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x01, intValue << 1, role));
					return true;
				case Qt::UserRole+2: // More follows/Last
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x06, intValue, role));
					return true;
			}
			break;
		case 0x27: // Flash functions
			switch (role) {
				case Qt::UserRole+1: // Flash mode
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x1c, intValue, role));
					return true;
				case Qt::UserRole+2: // Flash rate and phase
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x03, intValue << 2, role));
					break;
			}
			break;
		case 0x2c: // Display attributes
			switch (role) {
				case Qt::UserRole+1: // Text size
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x36, ((intValue & 0x02) << 5) | (intValue & 0x01), role));
					return true;
				case Qt::UserRole+2: // Boxing/window
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x7d, intValue << 1, role));
					return true;
				case Qt::UserRole+3: // Conceal
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x7b, intValue << 2, role));
					return true;
				case Qt::UserRole+4: // Invert
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x6f, intValue << 4, role));
					return true;
				case Qt::UserRole+5: // Underline/Separated
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x5f, intValue << 5, role));
					return true;
			}
			break;
		case 0x2d: // DRCS character
			switch (role) {
				case Qt::UserRole+1: // Normal or Global
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x3f, intValue << 6, role));
					return true;
				case Qt::UserRole+2: // Character number
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x40, intValue, role));
					break;
			}
			break;
		case 0x2e: // Font style
			switch (role) {
				case Qt::UserRole+1: // Proportional
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x7e, intValue, role));
					return true;
				case Qt::UserRole+2: // Bold
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x7d, intValue << 1, role));
					return true;
				case Qt::UserRole+3: // Italic
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x7b, intValue << 2, role));
					return true;
				case Qt::UserRole+4: // Number of rows
					m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x07, intValue << 4, role));
					return true;
			}
			break;
	}

	// Fpr other triplet modes, set the complete data value
	if (role == Qt::UserRole+1) {
		m_parentMainWidget->document()->undoStack()->push(new EditTripletCommand(m_parentMainWidget->document(), this, index.row(), EditTripletCommand::ETdata, 0x00, intValue, role));
		return true;
	}

	return false;
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

bool X26Model::insertRows(int row, int count, const QModelIndex &parent)
{
	Q_UNUSED(parent);

	if (m_parentMainWidget->document()->currentSubPage()->enhancements()->size() + count > m_parentMainWidget->document()->currentSubPage()->maxEnhancements())
		return false;

	m_parentMainWidget->document()->undoStack()->push(new InsertTripletCommand(m_parentMainWidget->document(), this, row, count, m_parentMainWidget->document()->currentSubPage()->enhancements()->at(row)));
	return true;
}

bool X26Model::insertRows(int row, int count, const QModelIndex &parent, X26Triplet triplet)
{
	Q_UNUSED(parent);

	if (m_parentMainWidget->document()->currentSubPage()->enhancements()->size() + count > m_parentMainWidget->document()->currentSubPage()->maxEnhancements())
		return false;

	m_parentMainWidget->document()->undoStack()->push(new InsertTripletCommand(m_parentMainWidget->document(), this, row, count, triplet));
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
