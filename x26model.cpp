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

#include <QList>

#include "x26model.h"

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
	return m_listLoaded ? m_parentMainWidget->document()->currentSubPage()->localEnhance.size() : 0;
}

int X26Model::columnCount(const QModelIndex & /*parent*/) const
{
	return 4;
}

QVariant X26Model::data(const QModelIndex &index, int role) const
{
	int mode = m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).mode();

	// Qt::UserRole will always return the raw values
	if (role == Qt::UserRole)
		switch (index.column()) {
			case 0:
				return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).address();
			case 1:
				return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).mode();
			case 2:
				return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data();
			default:
				return QVariant();
		}

	if (role == Qt::DisplayRole || role == Qt::EditRole)
		switch (index.column()) {
			case 0:
				// Show row number only if address part of triplet actually represents a row
				// i.e. Full row colour, Set Active Position and Origin Modifier
				if (!m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).isRowTriplet())
					return "";
				// For Origin Modifier address of 40 refers to same row, so show it as 0
				if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).mode() == 0x10) {
					if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).address() == 40)
						return 0;
					else
						return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).addressRow();
				}
				if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).mode() == 0x01 || m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).mode() == 0x04)
					return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).addressRow();
				else
					return "";
			case 1:
				if (!m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).isRowTriplet())
					return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).addressColumn();
				// For Set Active Position and Origin Modifier, data is the column
				else if (mode == 0x04 || mode == 0x10)
					return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data();
				else
					return "";
		}

	QString result;

	if (role == Qt::DisplayRole)
		switch (index.column()) {
			case 2:
				if (!m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).isRowTriplet())
					mode |= 0x20;
				return (modeTripletName[mode]);
			case 3:
				if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).isRowTriplet())
					// Row address group
					switch (mode) {
						case 0x01: // Full row colour
						case 0x07: // Address row 0
							if ((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x60) == 0x60)
								result = ", down to bottom";
							else if ((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x60) == 0x00)
								result = ", this row only";
							// fall-through
						case 0x00: // Full screen colour
							if (!(result.isEmpty()) || (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x60) == 0x00) {
								result.prepend(QString("CLUT %1:%2").arg((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x18) >> 3).arg(m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x07));
								return result;
							}
							break;
						case 0x04: // Set Active Position
						case 0x10: // Origin Modifier
							// For Set Active Position and Origin Modifier, data is the column, so return blank
							return "";
						case 0x11 ... 0x13: // Invoke object
							switch (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).address() & 0x18) {
								case 0x08:
									return QString("Local: d%1 t%2").arg((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() >> 4) | ((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).address() & 0x01) << 3)).arg(m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x0f);
								case 0x10:
									result = "POP";
									break;
								case 0x18:
									result = "GPOP";
									break;
								// case 0x00: won't happen since that would make a column triplet, not a row triplet
							}
							result.append(QString(": subpage %1 pkt %2 trplt %3 bits ").arg(m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x0f).arg((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).address() & 0x03) + 1).arg(((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x60) >> 5) * 3 + (mode & 0x03)));
							if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x10)
								result.append("10-18");
							else
								result.append("1-9");
							return result;
						case 0x15 ... 0x17: // Define object
							result = (QString("Local: d%1 t%2, ").arg((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() >> 4) | ((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).address() & 1) << 3)).arg(m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x0f));
							switch (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).address() & 0x18) {
								case 0x08:
									result.append("L2.5 only");
									break;
								case 0x10:
									result.append("L3.5 only");
									break;
								case 0x18:
									result.append("L2.5 and 3.5");
									break;
								// case 0x00: won't happen since that would make a column triplet, not a row triplet
							}
							return result;
						case 0x18: // DRCS mode
							result = (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x40) == 0x40 ? "Normal" : "Global";
							result.append(QString(": subpage %1, ").arg(m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x0f));
							switch (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x30) {
								case 0x10:
									result.append("L2.5 only");
									break;
								case 0x20:
									result.append("L3.5 only");
									break;
								case 0x30:
									result.append("L2.5 and 3.5");
									break;
								//case 0x00:
								//	result = "Reserved";
								//	break;
							}
							return result;
						case 0x1f: // Termination
							switch (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x07) {
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
							return QString("0x%1").arg(m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data(), 2, 16, QChar('0'));
					}
				else
					// Column address group
					switch (mode) {
						case 0x00: // Foreground colour
						case 0x03: // Background colour
							if (!(m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x60))
								return QString("CLUT %1:%2").arg((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x18) >> 3).arg(m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x07);
							break;
						case 0x01: // G1 mosaic character
						case 0x02: // G3 mosaic character at level 1.5
						case 0x09: // G0 character
						case 0x0b: // G3 mosaic character at level >=2.5
						case 0x0f ... 0x1f: // G2 character or G0 diacrtitical mark
							if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() >= 0x20)
								return QString("0x%1").arg(m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data(), 2, 16);
							break;
						case 0x07: // Flash functions
							if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() < 0x18) {
								switch (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x03) {
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
								switch (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x1c) {
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
						//TODO case 0x08: // G0 and G2 designation
						case 0x0c: // Display attributes
							if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x02)
								result.append("Boxing ");
							if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x04)
								result.append("Conceal ");
							if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x10)
								result.append("Invert ");
							if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x20)
								result.append("Underline ");
							if (result.isEmpty())
								result = "None";
							else
								// Chop off the last space
								result.chop(1);
							switch (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x41) {
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
						case 0x0d: // DRCS character
							result = (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x40) == 0x40 ? "Normal" : "Global";
							result.append(QString(": %1").arg(m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x3f));
							return result;
						case 0x0e: // Font style
							if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x01)
								result.append("Proportional ");
							if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x02)
								result.append("Bold ");
							if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x04)
								result.append("Italic ");
							if (result.isEmpty())
								result = "None";
							else
								// Chop off the last space
								result.chop(1);
							result.append(QString(", %1 row(s)").arg(m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() >> 4));
							return result;
						case 0x06: // PDC
							return QString("0x%1").arg(m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data(), 2, 16, QChar('0'));
						default: // Reserved
							return QString("Reserved 0x%1").arg(m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data(), 2, 16, QChar('0'));
					}
				// Reserved mode or data
				return QString("Reserved 0x%1").arg(m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data(), 2, 16, QChar('0'));
		}

	if (role == Qt::EditRole)
		switch (index.column()) {
			case 2:
				if (!m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).isRowTriplet())
					mode |= 0x20;
				return mode;
			case 3:
				if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).isRowTriplet())
					// Row address group
					switch (mode) {
						case 0x00: // Full screen colour
						case 0x01: // Full row colour
						case 0x07: // Address row 0
							// Colour index
							return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x1f;
						case 0x11 ... 0x13: // Invoke object
							// Local, POP or GPOP
							return ((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).address() & 0x18) >> 3) - 1;
						case 0x15 ... 0x17: // Define object
							// Required at level 2.5 and/or 3.5
							return ((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).address() & 0x18) >> 3) - 1;
						case 0x1f: // Termination
							// Intermed POP subpage|Last POP subpage|Local Object|Local enhance
							return ((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x06) >> 1);
						default:
							return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data();
					}
				else
					// Column address group
					switch (mode) {
						case 0x00: // Foreground colour
						case 0x03: // Background colour
							// Colour index
							return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x1f;
						case 0x07: // Flash functions
							// Flash mode
							return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x03;
						case 0x0c: // Display attributes
							// underline/separated, invert, conceal, boxing/window
							return ((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x06) >> 1) | ((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x30) >> 2);
						case 0x0e: // Font style
							// Proportional, Bold, Italics
							return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x07;
						default:
							return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data();
					}
			case 4:
				if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).isRowTriplet())
					// Row address group
					switch (mode) {
						case 0x01: // Full row colour
						case 0x07: // Address row 0
							// "this row only" or "down to bottom"
							return ((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x60) == 0x60) ? 1 : 0;
						case 0x11 ... 0x13: // Invoke object
							// Object source
							//TODO POP and GPOP, if ((x26Triplet.address() & 0x08) == 0x08)
							return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() | ((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).address() & 1) << 7);
						case 0x1f: // Termination
							// More follows/Last
							// return entire value as dropdown text changes depending on setting
							return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x07;
					}
				else
					// Column address group
					switch (mode) {
						case 0x07: // Flash functions
							// Flash rate and phase
							return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() >> 2;
						case 0x0c: // Display attributes
							// Text size
							return (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x01) | ((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x40) >> 5);
						case 0x0e: // Font style
							// Number of rows
							return m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() >> 4;
					}
		}

	return QVariant();
}

bool X26Model::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid())
		return false;

	int mode = m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).mode();

	if (role == Qt::UserRole && value.canConvert<int>() && index.column() <= 2) {
		int intValue = value.toInt();
		switch (index.column()) {
			case 0:
				m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setAddress(intValue);
				break;
			case 1:
				m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setMode(intValue);
				break;
			case 2:
				m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData(intValue);
				break;
		}
		emit dataChanged(createIndex(index.row(), 0), createIndex(index.row(), 4), {role});
		m_parentMainWidget->refreshPage();
		return true;
	}
	if (role == Qt::EditRole && value.canConvert<int>()) {
		int intValue = value.toInt();
		if (intValue < 0)
			return false;
		switch (index.column()) {
			case 0:
				if (intValue > 24)
					return false;
				m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setAddressRow(intValue);
				break;
			case 1:
				if (intValue > 39)
					return false;
				// For Set Active Position and Origin Modifier, data is the column
				if (mode == 0x04 || mode == 0x10)
					m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData(intValue);
				else
					m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setAddressColumn(intValue);
				break;
			case 2:
				if (intValue < 0x20 && !m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).isRowTriplet()) {
					// Changing mode from column triplet to row triplet
					m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setAddressRow(1);
					m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData(0);
				}
				if (intValue >= 0x20 && m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).isRowTriplet()) {
					// Changing mode from row triplet to column triplet
					m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setAddressColumn(0);
					m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData(0);
				}
				m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setMode(intValue & 0x1f);
				break;
			case 3:
				if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).isRowTriplet())
					// Row address group
					switch (mode) {
						case 0x00: // Full screen colour
						case 0x01: // Full row colour
						case 0x07: // Address row 0
							// Colour index
							m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x60) | intValue);
							break;
						case 0x04: // Set Active Position
							m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData(intValue);
							break;
						case 0x11 ... 0x13: // Invoke object
							// Local, POP or GPOP
							m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setAddress((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).address() & 0x67) | ((intValue+1) << 3));
							break;
						case 0x15 ... 0x17: // Define object
							// required at level 2.5 and/or 3.5
							m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setAddress((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).address() & 0x67) | ((intValue+1) << 3));
							break;
						case 0x1f: // Termination
							// Intermed POP subpage|Last POP subpage|Local Object|Local enhance
							m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x01) | (intValue << 1));
							break;
					}
				else
					// Column address group
					switch (mode) {
						case 0x07: // Flash functions
							// Flash mode
							m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x1c) | intValue);
							break;
						case 0x0c: // Display attributes
							// underline/separated, invert, conceal, boxing/window
							m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x41) | ((intValue & 0x0c) << 2) | ((intValue & 0x03) << 1));
							break;
						case 0x0e: // Font style
							// Proportional, Bold, Italics
							m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x70) | intValue);
							break;
						default:
							m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData(intValue);
					}
				break;
			case 4:
				if (m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).isRowTriplet())
					// Row address group
					switch (mode) {
						case 0x00: // Full screen colour
						case 0x01: // Full row colour
						case 0x07: // Address row 0
							// "this row only" or "down to bottom"
							m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x1f) | (intValue * 0x60));
							break;
						case 0x11 ... 0x13: // Invoke object
							// Object source
							//TODO POP and GPOP, if ((x26Triplet.address() & 0x08) == 0x08)
							m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData(intValue & 0x7f);
							m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setAddress((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).address() & 0x3e) | (intValue >> 7));
							break;
						case 0x1f: // Termination
							// More follows/Last
							m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x06) | intValue);
							break;
					}
				else
					// Column address group
					switch (mode) {
						case 0x07: // Flash functions
							// Flash rate and phase
							m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x03) | (intValue << 2));
							break;
						case 0x0c: // Display attributes
							// Text size
							m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x36) | ((intValue & 0x02) << 5) | (intValue & 0x01));
							break;
						case 0x0e: // Font style
							// Number of rows
							m_parentMainWidget->document()->currentSubPage()->localEnhance[index.row()].setData((m_parentMainWidget->document()->currentSubPage()->localEnhance.at(index.row()).data() & 0x07) | (intValue << 4));
							break;
					}
		}
		emit dataChanged(index, index, {role});
		m_parentMainWidget->refreshPage();
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
					return QString("");
			}
		} else if (orientation == Qt::Vertical)
			return QString("d%1 t%2").arg(section / 13).arg(section % 13);
	}
	return QVariant();
}

bool X26Model::insertFirstRow()
{
	X26Triplet firstTriplet;

	firstTriplet.setAddress(63);
	firstTriplet.setMode(31);
	firstTriplet.setData(7);

	beginInsertRows(QModelIndex(), 0, 0);
	m_parentMainWidget->document()->currentSubPage()->localEnhance.insert(0, firstTriplet);
	endInsertRows();

	// Since we always insert a Termination Marker there's no need to refresh the page
	return true;
}

bool X26Model::insertRows(int position, int rows, const QModelIndex &parent)
{
	Q_UNUSED(parent);
	X26Triplet copyTriplet = m_parentMainWidget->document()->currentSubPage()->localEnhance.at(position);

	beginInsertRows(QModelIndex(), position, position+rows-1);
	for (int row=0; row<rows; ++row)
		m_parentMainWidget->document()->currentSubPage()->localEnhance.insert(position+row, copyTriplet);
	endInsertRows();

	// Since we always insert duplicates of the selected triplet there's no need to refresh the page
	return true;
}

bool X26Model::removeRows(int position, int rows, const QModelIndex &index)
{
	Q_UNUSED(index);
	beginRemoveRows(QModelIndex(), position, position+rows-1);

	for (int row=0; row<rows; ++row)
		m_parentMainWidget->document()->currentSubPage()->localEnhance.removeAt(position+row);

	endRemoveRows();

	m_parentMainWidget->refreshPage();
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
