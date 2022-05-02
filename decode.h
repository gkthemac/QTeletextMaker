/*
 * Copyright (C) 2020-2022 Gavin MacGregor
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

#ifndef DECODE_H
#define DECODE_H

#include <QMap>
#include <QMultiMap>
#include <QPair>
#include <vector>

#include "levelonepage.h"

struct textCharacter {
	unsigned char code=0x20;
	int set=0;
	int diacritical=0;
};

inline bool operator!=(const textCharacter &lhs, const textCharacter &rhs)
{
	return lhs.code        != rhs.code         ||
	       lhs.set         != rhs.set          ||
	       lhs.diacritical != rhs.diacritical;
}

struct displayAttributes {
	bool doubleHeight=false;
	bool doubleWidth=false;
	bool boxingWindow=false;
	bool conceal=false;
	bool invert=false;
	bool underlineSeparated=false;
	bool forceContiguous=false;
};

inline bool operator!=(const displayAttributes &lhs, const displayAttributes &rhs)
{
	return lhs.doubleHeight       != rhs.doubleHeight       ||
	       lhs.doubleWidth        != rhs.doubleWidth        ||
	       lhs.boxingWindow       != rhs.boxingWindow       ||
	       lhs.conceal            != rhs.conceal            ||
	       lhs.invert             != rhs.invert             ||
	       lhs.underlineSeparated != rhs.underlineSeparated ||
	       lhs.forceContiguous    != rhs.forceContiguous;
}

struct textAttributes {
	int foreColour=0x07;
	int backColour=0x00;
	struct flashFunctions {
		int mode=0;
		int ratePhase=0;
		int phaseNumber=0;
	} flash;
	displayAttributes display;
	/* font style */
};

inline bool operator!=(const textAttributes &lhs, const textAttributes &rhs)
{
	return lhs.foreColour        != rhs.foreColour        ||
	       lhs.backColour        != rhs.backColour        ||
	       lhs.flash.mode        != rhs.flash.mode        ||
	       lhs.flash.ratePhase   != rhs.flash.ratePhase   ||
	       lhs.flash.phaseNumber != rhs.flash.phaseNumber ||
	       lhs.display           != rhs.display;
}

struct textCell {
	textCharacter character;
	textAttributes attribute;
	bool bottomHalf=false;
	bool rightHalf=false;
	bool level1Mosaic=false;
	int level1CharSet=0;
};

inline bool operator!=(const textCell &lhs, const textCell &rhs)
{
	return lhs.character     != rhs.character    ||
	       lhs.attribute     != rhs.attribute    ||
	       lhs.bottomHalf    != rhs.bottomHalf   ||
	       lhs.rightHalf     != rhs.rightHalf    ||
	       lhs.level1Mosaic  != rhs.level1Mosaic ||
	       lhs.level1CharSet != rhs.level1CharSet;
}

struct applyAttributes {
	bool applyForeColour=false;
	bool applyBackColour=false;
	bool applyFlash=false;
	bool applyDisplayAttributes=false;
	bool applyTextSizeOnly=false;
	bool applyBoxingOnly=false;
	bool applyConcealOnly=false;
	bool applyContiguousOnly=false;
	bool copyAboveAttributes=false;
	textAttributes attribute;
};

class ActivePosition
{
public:
	ActivePosition();
	int row() const { return (m_row == -1) ? 0 : m_row; }
	int column() const { return (m_column == -1) ? 0 : m_column; }
	bool isDeployed() const { return m_row != -1; }
	bool setRow(int);
	bool setColumn(int);
//	bool setRowAndColumn(int, int);

private:
	int m_row, m_column;
};


class TextLayer
{
public:
//	TextLayer(TeletextPage* thePage) : currentPage(thePage) { };
	virtual ~TextLayer() = default;
	void setTeletextPage(LevelOnePage *);
	virtual textCharacter character(int, int) =0;
	virtual void attributes(int, int, applyAttributes *) =0;
	virtual int fullScreenColour() const =0;
	virtual int fullRowColour(int) const =0;
	virtual bool fullRowDownwards(int) const =0;
	virtual int objectType() const =0;
	virtual int originR() const { return 0; };
	virtual int originC() const { return 0; };
	void setFullScreenColour(int);
	void setFullRowColour(int, int, bool);

	// Key QPair is row and column, value QPair is triplet mode and data
	QMultiMap<QPair<int, int>, QPair<int, int>> enhanceMap;

protected:
	LevelOnePage* m_levelOnePage;
	int m_layerFullScreenColour=-1;
	int m_layerFullRowColour[25];
	bool m_layerFullRowDownwards[25];
	applyAttributes m_applyAttributes;
};

class EnhanceLayer: public TextLayer
{
public:
	EnhanceLayer();
	textCharacter character(int, int);
	void attributes(int, int, applyAttributes *);
	int fullScreenColour() const { return m_layerFullScreenColour; };
	int fullRowColour(int r) const { return m_layerFullRowColour[r]; };
	bool fullRowDownwards(int r) const { return m_layerFullRowDownwards[r]; };
	int objectType() const { return m_objectType; };
	int originR() const { return m_originR; };
	int originC() const { return m_originC; };
	void setObjectType(int);
	void setOrigin(int, int);

protected:
	int m_objectType=0;
	int m_originR=0;
	int m_originC=0;
	int m_rowCached=-1;
	int m_rightMostColumn[25];
};

class Level1Layer: public TextLayer
{
public:
	enum RowHeight { Normal=-1, TopHalf, BottomHalf };

//	Level1Layer(TeletextPage *thePage) : TextLayer(thePage) { };
	Level1Layer();
	textCharacter character(int, int);
	void attributes(int, int, applyAttributes *);
	int fullScreenColour() const { return -1; };
	int fullRowColour(int) const { return -1; };
	bool fullRowDownwards(int) const { return false; };
	int objectType() const { return 0; }
	RowHeight rowHeight(int r) const { return m_rowHeight[r]; };

private:
	void updateRowCache(int);

	struct level1CacheAttributes {
		int foreColour=0x07;
		int backColour=0x00;
		unsigned char sizeCode=0x0c;
		bool mosaics=false;
		bool separated=false;
		bool held=false;
		bool escSwitch=false;
		unsigned char holdChar=0x20;
		bool holdSeparated=false;
	};
	level1CacheAttributes m_attributeCache[40];
	int m_rowCached=-1;
	bool m_rowHasDoubleHeightAttr[25];
	RowHeight m_rowHeight[25];
};

class TeletextPageDecode : public QObject
{
	Q_OBJECT

public:
	enum CharacterFragment { NormalSize, DoubleHeightTopHalf, DoubleHeightBottomHalf, DoubleWidthLeftHalf, DoubleWidthRightHalf, DoubleSizeTopLeftQuarter, DoubleSizeTopRightQuarter, DoubleSizeBottomLeftQuarter, DoubleSizeBottomRightQuarter };

	TeletextPageDecode();
	~TeletextPageDecode();
	bool refresh(int r, int c) const { return m_refresh[r][c]; }
	void setRefresh(int, int, bool);
	void decodePage();
	void decodeRow(int r);
	LevelOnePage *teletextPage() const { return m_levelOnePage; };
	void setTeletextPage(LevelOnePage *);
	void updateSidePanels();
	void buildEnhanceMap(TextLayer *, int=0);

	unsigned char cellCharacterCode(int r, int c) { return cellAtCharacterOrigin(r, c).character.code; };
	int cellCharacterSet(int r, int c) { return cellAtCharacterOrigin(r, c).character.set; };
	int cellCharacterDiacritical(int r, int c) { return cellAtCharacterOrigin(r, c).character.diacritical; };
	int cellForegroundCLUT(int r, int c) { return cellAtCharacterOrigin(r, c).attribute.foreColour; };
	int cellBackgroundCLUT(int r, int c) { return cellAtCharacterOrigin(r, c).attribute.backColour; };
	QColor cellForegroundQColor(int, int);
	QColor cellBackgroundQColor(int, int);
	QColor cellFlashForegroundQColor(int, int);
	int cellFlashMode(int r, int c) { return cellAtCharacterOrigin(r, c).attribute.flash.mode; };
	int cellFlashRatePhase(int r, int c) { return cellAtCharacterOrigin(r, c).attribute.flash.ratePhase; };
	int cellFlash2HzPhaseNumber(int r, int c) { return cellAtCharacterOrigin(r, c).attribute.flash.phaseNumber; };
	CharacterFragment cellCharacterFragment(int, int) const;
	bool cellBoxed(int r, int c) { return cellAtCharacterOrigin(r, c).attribute.display.boxingWindow; };
	bool cellConceal(int r, int c) { return cellAtCharacterOrigin(r, c).attribute.display.conceal; };
	bool cellUnderlined(int r, int c) { return cellAtCharacterOrigin(r, c).attribute.display.underlineSeparated && cellAtCharacterOrigin(r, c).character.set < 24; };

	bool level1MosaicAttribute(int r, int c) const { return m_cell[r][c].level1Mosaic; };
	int level1CharSet(int r, int c) const { return m_cell[r][c].level1CharSet; };

	QColor fullScreenQColor() const { return m_finalFullScreenQColor; };
	QColor fullRowQColor(int r) const { return m_fullRowQColor[r]; };
	int leftSidePanelColumns() const { return m_leftSidePanelColumns; };
	int rightSidePanelColumns() const { return m_rightSidePanelColumns; };

public slots:
	void setLevel(int);

signals:
	void fullScreenColourChanged(QColor);
	void fullRowColourChanged(int, QColor);
	void sidePanelsChanged();

protected:
	inline void setFullScreenColour(int);
	inline void setFullRowColour(int, int);
	textCell& cellAtCharacterOrigin(int, int);

	int m_finalFullScreenColour, m_level;
	QColor m_finalFullScreenQColor;
	int m_leftSidePanelColumns, m_rightSidePanelColumns;
	Level1Layer m_level1Layer;
	std::vector<TextLayer *> m_textLayer;
	const int m_foregroundRemap[8] = { 0,  0,  0,  8,  8, 16, 16, 16 };
	const int m_backgroundRemap[8] = { 0,  8, 16,  8, 16,  8, 16, 24 };

private:
	enum ColourPart { Foreground, Background, FlashForeground };

	QColor cellQColor(int, int, ColourPart);

	textCell m_cell[25][72];
	bool m_refresh[25][72];
	LevelOnePage* m_levelOnePage;
	int m_fullRowColour[25];
	QColor m_fullRowQColor[25];
};

static const QMap<int, int> g0CharacterMap {
	{ 0x00, 12 }, { 0x01, 15 }, { 0x02, 22 }, { 0x03, 16 }, { 0x04, 14 }, { 0x05, 19 }, { 0x06, 11 },
	{ 0x08, 18 }, { 0x09, 15 }, { 0x0a, 22 }, { 0x0b, 16 }, { 0x0c, 14 }, { 0x0e, 11 },
	{ 0x10, 12 }, { 0x11, 15 }, { 0x12, 22 }, { 0x13, 16 }, { 0x14, 14 }, { 0x15, 19 }, { 0x16, 23 },
	{ 0x1d, 21 }, { 0x1f, 20 },
	{ 0x20,  1 }, { 0x21, 15 }, { 0x22, 13 }, { 0x23, 17 }, { 0x24,  2 }, { 0x25,  3 }, { 0x26, 11 },
	{ 0x36, 23 }, { 0x37,  4 },
	{ 0x40, 12 }, { 0x44, 14 }, { 0x47,  5 },
	{ 0x55,  6 }, { 0x57,  5 }
};

#endif
