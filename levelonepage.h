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

#ifndef LEVELONEPAGE_H
#define LEVELONEPAGE_H

#include <QByteArray>
#include <QColor>
#include <QList>
#include <QObject>
#include <QString>
#include <QTextStream>

#include "pagebase.h"
#include "x26triplets.h"

QColor CLUTtoQColor(int myColour);

// If we inherit from QObject then we can't copy construct, so "make a new subpage that's a copy of this one" wouldn't work
class LevelOnePage : public PageBase //: public QObject
{
	//Q_OBJECT

public:
	enum CycleTypeEnum { CTcycles, CTseconds };

	LevelOnePage();

	QByteArray packet(int, int=0);
	bool setPacket(int, QByteArray);
	bool setPacket(int, int, QByteArray);
	bool packetNeeded(int, int=0) const;

	void clearPage();
	void loadPagePacket(QByteArray &);
	void savePage(QTextStream *, int, int);
	QString exportURLHash(QString);

/*	void setSubPageNumber(int); */
	int cycleValue() const { return m_cycleValue; };
	void setCycleValue(int);
	CycleTypeEnum cycleType() const { return m_cycleType; };
	void setCycleType(CycleTypeEnum);
	int defaultCharSet() const { return m_defaultCharSet; }
	void setDefaultCharSet(int);
	int defaultNOS() const { return m_defaultNOS; }
	void setDefaultNOS(int);
	int secondCharSet() const { return m_secondCharSet; }
	void setSecondCharSet(int);
	int secondNOS() const { return m_secondNOS; }
	void setSecondNOS(int);
	unsigned char character(int row, int column) const { return m_level1Page[row][column]; }
	void setCharacter(int, int, unsigned char);
	int defaultScreenColour() const { return m_defaultScreenColour; }
	void setDefaultScreenColour(int);
	int defaultRowColour() const { return m_defaultRowColour; }
	void setDefaultRowColour(int);
	int colourTableRemap() const { return m_colourTableRemap; }
	void setColourTableRemap(int);
	bool blackBackgroundSubst() const { return m_blackBackgroundSubst; }
	void setBlackBackgroundSubst(bool);
	int CLUT(int index, int renderLevel=3) const;
	void setCLUT(int, int);
	bool leftSidePanelDisplayed() const { return m_leftSidePanelDisplayed; }
	void setLeftSidePanelDisplayed(bool);
	bool rightSidePanelDisplayed() const { return m_rightSidePanelDisplayed; }
	void setRightSidePanelDisplayed(bool);
	int sidePanelColumns() const { return m_sidePanelColumns; }
	void setSidePanelColumns(int);
	bool sidePanelStatusL25() const { return m_sidePanelStatusL25; }
	void setSidePanelStatusL25(bool);
	QString colourHash(int);
	QList<X26Triplet> localEnhance;

protected:
	int controlBitsToPS() const;

private:
	unsigned char m_level1Page[25][40];
/*	int m_subPageNumber; */
	int m_cycleValue;
	CycleTypeEnum m_cycleType;
	int m_defaultCharSet, m_defaultNOS, m_secondCharSet, m_secondNOS;
	int m_defaultScreenColour, m_defaultRowColour, m_colourTableRemap, m_sidePanelColumns;
	bool m_blackBackgroundSubst, m_leftSidePanelDisplayed, m_rightSidePanelDisplayed, m_sidePanelStatusL25;
	int m_CLUT[32];
	X26Triplet m_paddingX26Triplet;

	const int defaultCLUT[32] = {
	0x000, 0xf00, 0x0f0, 0xff0, 0x00f, 0xf0f, 0x0ff, 0xfff,
	0x000, 0x700, 0x070, 0x770, 0x007, 0x707, 0x077, 0x777,
	0xf05, 0xf70, 0x0f7, 0xffb, 0x0ca, 0x500, 0x652, 0xc77,
	0x333, 0xf77, 0x7f7, 0xff7, 0x77f, 0xf7f, 0x7ff, 0xddd
	};
};

#endif
