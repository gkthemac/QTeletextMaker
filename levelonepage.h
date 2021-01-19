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

#ifndef LEVELONEPAGE_H
#define LEVELONEPAGE_H

#include <QByteArray>
#include <QColor>
#include <QObject>
#include <QString>

#include "pagex26base.h"
#include "x26triplets.h"

class LevelOnePage : public PageX26Base //: public QObject
{
	//Q_OBJECT

public:
	enum CycleTypeEnum { CTcycles, CTseconds };

	LevelOnePage();
	LevelOnePage(const PageBase &);

	bool isEmpty() const override;

	QByteArray packet(int) const override;
	QByteArray packet(int, int) const override;
	bool packetNeeded(int) const override;
	bool packetNeeded(int, int) const override;
	bool setPacket(int, QByteArray) override;
	bool setPacket(int, int, QByteArray) override;

	bool controlBit(int bitNumber) const override;
	bool setControlBit(int, bool) override;

	void clearPage();

	QList<X26Triplet> *enhancements() { return &m_enhancements; };

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
	QColor CLUTtoQColor(int index, int renderlevel=3) const;
	bool isPaletteDefault(int) const;
	bool isPaletteDefault(int, int) const;
	int levelRequired() const;
	bool leftSidePanelDisplayed() const { return m_leftSidePanelDisplayed; }
	void setLeftSidePanelDisplayed(bool);
	bool rightSidePanelDisplayed() const { return m_rightSidePanelDisplayed; }
	void setRightSidePanelDisplayed(bool);
	int sidePanelColumns() const { return m_sidePanelColumns; }
	void setSidePanelColumns(int);
	bool sidePanelStatusL25() const { return m_sidePanelStatusL25; }
	void setSidePanelStatusL25(bool);
	int fastTextLinkPageNumber(int linkNumber) const { return m_fastTextLink[linkNumber].pageNumber; }
	void setFastTextLinkPageNumber(int, int);
	int composeLinkFunction(int linkNumber) const { return m_composeLink[linkNumber].function; }
	void setComposeLinkFunction(int, int);
	bool composeLinkLevel2p5(int linkNumber) const { return m_composeLink[linkNumber].level2p5; }
	void setComposeLinkLevel2p5(int, bool);
	bool composeLinkLevel3p5(int linkNumber) const { return m_composeLink[linkNumber].level3p5; }
	void setComposeLinkLevel3p5(int, bool);
	int composeLinkPageNumber(int linkNumber) const { return m_composeLink[linkNumber].pageNumber; }
	void setComposeLinkPageNumber(int, int);
	int composeLinkSubPageCodes(int linkNumber) const { return m_composeLink[linkNumber].subPageCodes; }
	void setComposeLinkSubPageCodes(int, int);

private:
	unsigned char m_level1Page[25][40];
/*	int m_subPageNumber; */
	int m_cycleValue;
	CycleTypeEnum m_cycleType;
	int m_defaultCharSet, m_defaultNOS, m_secondCharSet, m_secondNOS;
	int m_defaultScreenColour, m_defaultRowColour, m_colourTableRemap, m_sidePanelColumns;
	bool m_blackBackgroundSubst, m_leftSidePanelDisplayed, m_rightSidePanelDisplayed, m_sidePanelStatusL25;
	int m_CLUT[32];
	struct fastTextLink {
		int pageNumber;
		int subPageNumber;
	} m_fastTextLink[6];
	struct composeLink {
		int function;
		bool level2p5, level3p5;
		int pageNumber, subPageCodes;
	} m_composeLink[8];

	const int m_defaultCLUT[32] = {
		0x000, 0xf00, 0x0f0, 0xff0, 0x00f, 0xf0f, 0x0ff, 0xfff,
		0x000, 0x700, 0x070, 0x770, 0x007, 0x707, 0x077, 0x777,
		0xf05, 0xf70, 0x0f7, 0xffb, 0x0ca, 0x500, 0x652, 0xc77,
		0x333, 0xf77, 0x7f7, 0xff7, 0x77f, 0xf7f, 0x7ff, 0xddd
	};
};

#endif
