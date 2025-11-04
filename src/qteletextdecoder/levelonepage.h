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
	LevelOnePage(const PageBase &other);

	bool isEmpty() const override;

	QByteArray packet(int y, int d) const override;
	bool setPacket(int y, int d, QByteArray pkt) override;
	bool packetExists(int y, int d) const override;

	bool setControlBit(int b, bool active) override;

	void clearPage();

	int maxEnhancements() const override;

/*	void setSubPageNumber(int); */
	int cycleValue() const;
	void setCycleValue(int newValue);
	CycleTypeEnum cycleType() const;
	void setCycleType(CycleTypeEnum newType);
	int defaultCharSet() const;
	void setDefaultCharSet(int newDefaultCharSet);
	int defaultNOS() const;
	void setDefaultNOS(int defaultNOS);
	int secondCharSet() const;
	void setSecondCharSet(int newSecondCharSet);
	int secondNOS() const;
	void setSecondNOS(int newSecondNOS);
	unsigned char character(int r, int c) const;
	void setCharacter(int r, int c, unsigned char newChar);
	int defaultScreenColour() const;
	void setDefaultScreenColour(int newDefaultScreenColour);
	int defaultRowColour() const;
	void setDefaultRowColour(int newDefaultRowColour);
	int colourTableRemap() const;
	void setColourTableRemap(int newColourTableRemap);
	bool blackBackgroundSubst() const;
	void setBlackBackgroundSubst(bool newBlackBackgroundSubst);
	int CLUT(int index, int renderLevel=3) const;
	void setCLUT(int index, int newColour);
	QColor CLUTtoQColor(int index, int renderlevel=3) const;
	bool isPaletteDefault(int colour) const;
	bool isPaletteDefault(int fromColour, int toColour) const;
	int dCLUT(bool globalDrcs, int mode, int index) const;
	void setDCLUT(bool globalDrcs, int mode, int index, int colour);
	int levelRequired() const;
	bool leftSidePanelDisplayed() const;
	void setLeftSidePanelDisplayed(bool newLeftSidePanelDisplayed);
	bool rightSidePanelDisplayed() const;
	void setRightSidePanelDisplayed(bool newRightSidePanelDisplayed);
	int sidePanelColumns() const;
	void setSidePanelColumns(int newSidePanelColumns);
	bool sidePanelStatusL25() const;
	void setSidePanelStatusL25(bool newSidePanelStatusL25);
	int fastTextLinkPageNumber(int linkNumber) const;
	void setFastTextLinkPageNumber(int linkNumber, int pageNumber);
	int composeLinkFunction(int linkNumber) const;
	void setComposeLinkFunction(int linkNumber, int newFunction);
	bool composeLinkLevel2p5(int linkNumber) const;
	void setComposeLinkLevel2p5(int linkNumber, bool newRequired);
	bool composeLinkLevel3p5(int linkNumber) const;
	void setComposeLinkLevel3p5(int linkNumber, bool newRequired);
	int composeLinkPageNumber(int linkNumber) const;
	void setComposeLinkPageNumber(int linkNumber, int newPageNumber);
	int composeLinkSubPageCodes(int linkNumber) const;
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
