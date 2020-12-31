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

#ifndef RENDER_H
#define RENDER_H

#include <QBitmap>
#include <QMap>
#include <QMultiMap>
#include <vector>

#include "levelonepage.h"


struct textCharacter {
	unsigned char code=0x20;
	int set=0;
	int diacritical=0;
};

struct displayAttributes {
	bool doubleHeight=false;
	bool doubleWidth=false;
	bool boxingWindow=false;
	bool conceal=false;
	bool invert=false;
	bool underlineSeparated=false;
	bool forceContiguous=false;
};

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

struct textCell {
	textCharacter character;
	textAttributes attribute;
	bool bottomHalf=false;
	bool rightHalf=false;
	bool level1Mosaic=false;
	int level1CharSet=0;
};

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
	void setFullScreenColour(int);
	void setFullRowColour(int, int, bool);

	// TODO replace ints with proper classes
	QMultiMap<int, int> enhanceMap;

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
//	Level1Layer(TeletextPage *thePage) : TextLayer(thePage) { };
	Level1Layer();
	textCharacter character(int, int);
	void attributes(int, int, applyAttributes *);
	int fullScreenColour() const { return -1; };
	int fullRowColour(int) const { return -1; };
	bool fullRowDownwards(int) const { return false; };
	int objectType() const { return 0; }
	bool isRowBottomHalf(int r) const { return m_rowHeight[r]==RHbottomhalf; }

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
	enum rowHeightEnum { RHnormal=-1, RHtophalf, RHbottomhalf } m_rowHeight[25];
};

class TeletextPageRender : public QObject
{
	Q_OBJECT

public:
	TeletextPageRender();
	~TeletextPageRender();
	void decodePage();
	void renderPage();
	void renderPage(int r);
	void setTeletextPage(LevelOnePage *);
	void updateSidePanels();
	void buildEnhanceMap(TextLayer *, int=0);
	QPixmap* pagePixmap(int i) const { return m_pagePixmap[i]; };
	bool level1MosaicAttribute(int r, int c) const { return m_cell[r][c].level1Mosaic; };
	int level1CharSet(int r, int c) const { return m_cell[r][c].level1CharSet; };
	int leftSidePanelColumns() const { return m_leftSidePanelColumns; };
	int rightSidePanelColumns() const { return m_rightSidePanelColumns; };
	void setGrid(bool);

public slots:
	void setReveal(bool);
	void setMix(bool);
	void setShowCodes(bool);
	void setRenderLevel(int);

signals:
	void fullScreenColourChanged(QColor);
	void fullRowColourChanged(int, QColor);
	void flashChanged(int);
	void sidePanelsChanged();

protected:
	void updateFlashRequired(int);
	inline void setFullScreenColour(int);
	inline void setFullRowColour(int, int);

	QBitmap* m_fontBitmap;
	QPixmap* m_pagePixmap[6];
	int m_finalFullScreenColour, m_renderLevel;
	QColor m_finalFullScreenQColor;
	int m_leftSidePanelColumns, m_rightSidePanelColumns;
	bool m_reveal, m_mix, m_grid, m_showCodes;
	Level1Layer m_level1Layer;
	std::vector<TextLayer *> m_textLayer;
	const int m_foregroundRemap[8] = { 0,  0,  0,  8,  8, 16, 16, 16 };
	const int m_backgroundRemap[8] = { 0,  8, 16,  8, 16,  8, 16, 24 };

private:
	textCell m_cell[25][72];
	LevelOnePage* m_levelOnePage;
	int m_flashRequired;
	int m_fullRowColour[25];
	QColor m_fullRowQColor[25];
	int m_flashRow[25];
	bool m_concealRow[25];
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
