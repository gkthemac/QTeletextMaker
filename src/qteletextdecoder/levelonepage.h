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
	using PageX26Base::packet;
	using PageX26Base::setPacket;
	using PageX26Base::packetExists;

	enum CycleTypeEnum { CTcycles, CTseconds };

	LevelOnePage();

	bool isEmpty() const override;

	QByteArray packet(int y, int d) const override;
	bool setPacket(int y, int d, QByteArray pkt) override;
	bool packetExists(int y, int d) const override;

	bool setControlBit(int b, bool active) override;

	void clearPage();

	int maxEnhancements() const override { return 208; };

/*	void setSubPageNumber(int); */
	int cycleValue() const { return m_cycleValue; };
	void setCycleValue(int newValue);
	CycleTypeEnum cycleType() const { return m_cycleType; };
	void setCycleType(CycleTypeEnum newType);
	int defaultCharSet() const { return m_defaultCharSet; }
	void setDefaultCharSet(int newDefaultCharSet);
	int defaultNOS() const { return m_defaultNOS; }
	void setDefaultNOS(int defaultNOS);
	int secondCharSet() const { return m_secondCharSet; }
	void setSecondCharSet(int newSecondCharSet);
	int secondNOS() const { return m_secondNOS; }
	void setSecondNOS(int newSecondNOS);
	unsigned char character(int r, int c) const { return PageX26Base::packetExists(r) ? PageX26Base::packet(r).at(c) : 0x20; }
	void setCharacter(int r, int c, unsigned char newChar);
	int defaultScreenColour() const { return m_defaultScreenColour; }
	void setDefaultScreenColour(int newDefaultScreenColour);
	int defaultRowColour() const { return m_defaultRowColour; }
	void setDefaultRowColour(int newDefaultRowColour);
	int colourTableRemap() const { return m_colourTableRemap; }
	void setColourTableRemap(int newColourTableRemap);
	bool blackBackgroundSubst() const { return m_blackBackgroundSubst; }
	void setBlackBackgroundSubst(bool newBlackBackgroundSubst);
	int CLUT(int index, int renderLevel=3) const;
	void setCLUT(int index, int newColour);
	QColor CLUTtoQColor(int index, int renderlevel=3) const;
	bool isPaletteDefault(int colour) const;
	bool isPaletteDefault(int fromColour, int toColour) const;
	int levelRequired() const;
	bool leftSidePanelDisplayed() const { return m_leftSidePanelDisplayed; }
	void setLeftSidePanelDisplayed(bool newLeftSidePanelDisplayed);
	bool rightSidePanelDisplayed() const { return m_rightSidePanelDisplayed; }
	void setRightSidePanelDisplayed(bool newRightSidePanelDisplayed);
	int sidePanelColumns() const { return m_sidePanelColumns; }
	void setSidePanelColumns(int newSidePanelColumns);
	bool sidePanelStatusL25() const { return m_sidePanelStatusL25; }
	void setSidePanelStatusL25(bool newSidePanelStatusL25);
	int fastTextLinkPageNumber(int linkNumber) const { return m_fastTextLink[linkNumber].pageNumber; }
	void setFastTextLinkPageNumber(int linkNumber, int pageNumber);
	int composeLinkFunction(int linkNumber) const { return m_composeLink[linkNumber].function; }
	void setComposeLinkFunction(int linkNumber, int newFunction);
	bool composeLinkLevel2p5(int linkNumber) const { return m_composeLink[linkNumber].level2p5; }
	void setComposeLinkLevel2p5(int linkNumber, bool newRequired);
	bool composeLinkLevel3p5(int linkNumber) const { return m_composeLink[linkNumber].level3p5; }
	void setComposeLinkLevel3p5(int linkNumber, bool newRequired);
	int composeLinkPageNumber(int linkNumber) const { return m_composeLink[linkNumber].pageNumber; }
	void setComposeLinkPageNumber(int linkNumber, int newPageNumber);
	int composeLinkSubPageCodes(int linkNumber) const { return m_composeLink[linkNumber].subPageCodes; }
	void setComposeLinkSubPageCodes(int linkNumber, int newSubPageCodes);

private:
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

	static constexpr int m_defaultCLUT[32] = {
		0x000, 0xf00, 0x0f0, 0xff0, 0x00f, 0xf0f, 0x0ff, 0xfff,
		0x000, 0x700, 0x070, 0x770, 0x007, 0x707, 0x077, 0x777,
		0xf05, 0xf70, 0x0f7, 0xffb, 0x0ca, 0x500, 0x652, 0xc77,
		0x333, 0xf77, 0x7f7, 0xff7, 0x77f, 0xf7f, 0x7ff, 0xddd
	};
};

#endif
