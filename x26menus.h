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

#ifndef X26MENUS_H
#define X26MENUS_H

#include <QActionGroup>
#include <QColor>
#include <QMenu>
#include <QString>

#include "render.h"

class ModeTripletNames
{
public:
	static const QString modeName(int i) { return m_modeTripletName[i]; }

private:
	static inline const QString m_modeTripletName[64] {
		// Row triplet modes
		"Full screen colour",
		"Full row colour",
		"Reserved 0x02",
		"Reserved 0x03",

		"Set Active Position",
		"Reserved 0x05",
		"Reserved 0x06",
		"Address row 0",

		"PDC origin, source",
		"PDC month and day",
		"PDC cursor and start hour",
		"PDC cursor and end hour",

		"PDC cursor local offset",
		"PDC series ID and code",
		"Reserved 0x0e",
		"Reserved 0x0f",

		"Origin modifier",
		"Invoke active object",
		"Invoke adaptive object",
		"Invoke passive object",

		"Reserved 0x14",
		"Define active object",
		"Define adaptive object",
		"Define passive object",

		"DRCS mode",
		"Reserved 0x19",
		"Reserved 0x1a",
		"Reserved 0x1b",

		"Reserved 0x1c",
		"Reserved 0x1d",
		"Reserved 0x1e",
		"Termination marker",

		// Column triplet modes
		"Foreground colour",
		"G1 block mosaic",
		"G3 smooth mosaic, level 1.5",
		"Background colour",

		"Reserved 0x04",
		"Reserved 0x05",
		"PDC cursor, start end min",
		"Additional flash functions",

		"Modified G0/G2 character set",
		"G0 character",
		"Reserved 0x0a",
		"G3 smooth mosaic, level 2.5",

		"Display attributes",
		"DRCS character",
		"Font style, level 3.5",
		"G2 supplementary character",

		"G0 character no diacritical",
		"G0 character diacritical 1",
		"G0 character diacritical 2",
		"G0 character diacritical 3",
		"G0 character diacritical 4",
		"G0 character diacritical 5",
		"G0 character diacritical 6",
		"G0 character diacritical 7",
		"G0 character diacritical 8",
		"G0 character diacritical 9",
		"G0 character diacritical A",
		"G0 character diacritical B",
		"G0 character diacritical C",
		"G0 character diacritical D",
		"G0 character diacritical E",
		"G0 character diacritical F"
	};
};

class TripletModeQMenu : public QMenu
{
	Q_OBJECT

public:
	TripletModeQMenu(QWidget *parent = nullptr);
	QAction *action(int n) const { return m_actions[n]; };

private:
	void addModeAction(QMenu *menu, int mode);

	QAction *m_actions[64];
	ModeTripletNames m_modeTripletNames;
};

class TripletCLUTQMenu : public QMenu
{
	Q_OBJECT

public:
	TripletCLUTQMenu(bool rows, QWidget *parent = nullptr);
	QAction *action(int n) const { return m_actions[n]; };
	void setColour(int i, QColor c);

private:
	QAction *m_actions[34];
};

class TripletCharacterQMenu : public QMenu
{
	Q_OBJECT

public:
	TripletCharacterQMenu(int charSet, bool mosaic, QWidget *parent = nullptr);
	QAction *action(int n) const { return m_actions[n]; };

private:
	QAction *m_actions[96];
	TeletextFontBitmap m_fontBitmap;
};

class TripletFlashQMenu : public QMenu
{
	Q_OBJECT

public:
	TripletFlashQMenu(QWidget *parent = nullptr);
	QAction *modeAction(int n) const { return m_actions[n]; };
	QAction *ratePhaseAction(int n) const { return m_actions[n+4]; };
	void setModeChecked(int n);
	void setRatePhaseChecked(int n);

private:
	QAction *m_actions[10];
	QActionGroup *m_modeActionGroup, *m_ratePhaseActionGroup;
};

class TripletDisplayAttrsQMenu : public QMenu
{
	Q_OBJECT

public:
	TripletDisplayAttrsQMenu(QWidget *parent = nullptr);
	QAction *textSizeAction(int n) const { return m_actions[n]; };
	QAction *otherAttrAction(int n) const { return m_actions[n+4]; };
	void setTextSizeChecked(int n);
	void setOtherAttrChecked(int n, bool b);

private:
	QAction *m_actions[8];
	QActionGroup *m_sizeActionGroup, *m_otherActionGroup;
};

class TripletFontStyleQMenu : public QMenu
{
	Q_OBJECT

public:
	TripletFontStyleQMenu(QWidget *parent = nullptr);
	QAction *styleAction(int n) const { return m_actions[n]; };
	QAction *rowsAction(int n) const { return m_actions[n+3]; };
	void setStyleChecked(int n, bool b);
	void setRowsChecked(int n);

private:
	QAction *m_actions[11];
	QActionGroup *m_styleActionGroup, *m_rowsActionGroup;
};

#endif
