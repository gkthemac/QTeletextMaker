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

#ifndef DECODE_H
#define DECODE_H

#include <QList>
#include <QMap>
#include <QMultiMap>

#include "levelonepage.h"

class TeletextPageDecode : public QObject
{
	Q_OBJECT

public:
	enum CharacterFragment { NormalSize, DoubleHeightTopHalf, DoubleHeightBottomHalf, DoubleWidthLeftHalf, DoubleWidthRightHalf, DoubleSizeTopLeftQuarter, DoubleSizeTopRightQuarter, DoubleSizeBottomLeftQuarter, DoubleSizeBottomRightQuarter };
	enum RowHeight { NormalHeight, TopHalf, BottomHalf };

	TeletextPageDecode();
	~TeletextPageDecode();
	bool refresh(int r, int c) const { return m_refresh[r][c]; }
	void setRefresh(int r, int c, bool refresh);
	int level() const { return m_level; }
	void decodePage();
	LevelOnePage *teletextPage() const { return m_levelOnePage; };
	void setTeletextPage(LevelOnePage *newCurrentPage);
	void updateSidePanels();

	unsigned char cellCharacterCode(int r, int c) const { return m_cell[r][c].character.code; };
	int cellCharacterSet(int r, int c) const { return m_cell[r][c].character.set; };
	int cellCharacterDiacritical(int r, int c) const { return m_cell[r][c].character.diacritical; };
	int cellG0CharacterSet(int r, int c) const { return m_cell[r][c].g0Set; };
	int cellG2CharacterSet(int r, int c) const { return m_cell[r][c].g2Set; };
	int cellForegroundCLUT(int r, int c) const { return m_cell[r][c].attribute.foregroundCLUT; };
	int cellBackgroundCLUT(int r, int c) const { return m_cell[r][c].attribute.backgroundCLUT; };
	QColor cellForegroundQColor(int r, int c);
	QColor cellBackgroundQColor(int r, int c);
	QColor cellFlashForegroundQColor(int r, int c);
	int cellFlashMode(int r, int c) const { return m_cell[r][c].attribute.flash.mode; };
	int cellFlashRatePhase(int r, int c) const { return m_cell[r][c].attribute.flash.ratePhase; };
	int cellFlash2HzPhaseNumber(int r, int c) const { return m_cell[r][c].attribute.flash.phase2HzShown; };
	CharacterFragment cellCharacterFragment(int r, int c) const { return m_cell[r][c].fragment; };
	bool cellBoxed(int r, int c) const { return m_cell[r][c].attribute.display.boxingWindow; };
	bool cellConceal(int r, int c) const { return m_cell[r][c].attribute.display.conceal; };
	bool cellUnderlined(int r, int c) const { return cellCharacterSet(r, c) < 24 ? m_cell[r][c].attribute.display.underlineSeparated : false; };
	bool cellBold(int r, int c) const { return m_cell[r][c].attribute.style.bold; };
	bool cellItalic(int r, int c) const { return m_cell[r][c].attribute.style.italic; };
	bool cellProportional(int r, int c) const { return m_cell[r][c].attribute.style.proportional; };

	bool level1MosaicAttr(int r, int c) const { return m_cellLevel1MosaicAttr[r][c]; };
	bool level1MosaicChar(int r, int c) const { return m_cellLevel1MosaicChar[r][c]; };
	int level1CharSet(int r, int c) const { return m_cellLevel1CharSet[r][c]; };

	RowHeight rowHeight(int r) const { return m_rowHeight[r]; };

	QColor fullScreenQColor() const { return m_finalFullScreenQColor; };
	QColor fullRowQColor(int r) const { return m_fullRowQColor[r]; };
	int leftSidePanelColumns() const { return m_leftSidePanelColumns; };
	int rightSidePanelColumns() const { return m_rightSidePanelColumns; };

public slots:
	void setLevel(int level);

signals:
	void fullScreenColourChanged(QColor newColour);
	void fullRowColourChanged(int r, QColor newColour);
	void sidePanelsChanged();

protected:
	inline void setFullScreenColour(int newColour);
	inline void setFullRowColour(int row, int newColour);

	int m_finalFullScreenColour, m_level;
	QColor m_finalFullScreenQColor;
	int m_leftSidePanelColumns, m_rightSidePanelColumns;
	const int m_foregroundRemap[8] = { 0,  0,  0,  8,  8, 16, 16, 16 };
	const int m_backgroundRemap[8] = { 0,  8, 16,  8, 16,  8, 16, 24 };

private:
	class Invocation;

	enum ColourPart { Foreground, Background, FlashForeground };

	struct textCharacter {
		unsigned char code=0x20;
		int set=0;
		int diacritical=0;
	};

	friend inline bool operator!=(const textCharacter &lhs, const textCharacter &rhs)
	{
		return lhs.code        != rhs.code ||
	           lhs.set         != rhs.set  ||
	           lhs.diacritical != rhs.diacritical;
	}

	struct flashFunctions {
		int mode=0;
		int ratePhase=0;
		int phase2HzShown=0;
	};

	struct displayAttributes {
		bool doubleHeight=false;
		bool doubleWidth=false;
		bool boxingWindow=false;
		bool conceal=false;
		bool invert=false;
		bool underlineSeparated=false;
	};

	struct fontStyle {
		bool proportional=false;
		bool bold=false;
		bool italic=false;
	};

	friend inline bool operator!=(const displayAttributes &lhs, const displayAttributes &rhs)
	{
		return lhs.doubleHeight       != rhs.doubleHeight       ||
		       lhs.doubleWidth        != rhs.doubleWidth        ||
		       lhs.boxingWindow       != rhs.boxingWindow       ||
		       lhs.conceal            != rhs.conceal            ||
		       lhs.invert             != rhs.invert             ||
		       lhs.underlineSeparated != rhs.underlineSeparated;
	}

	struct textAttributes {
		int foregroundCLUT=7;
		int backgroundCLUT=0;
		flashFunctions flash;
		displayAttributes display;
		fontStyle style;
	};

	friend inline bool operator!=(const textAttributes &lhs, const textAttributes &rhs)
	{
		return lhs.foregroundCLUT      != rhs.foregroundCLUT      ||
		       lhs.backgroundCLUT      != rhs.backgroundCLUT      ||
		       lhs.flash.mode          != rhs.flash.mode          ||
		       lhs.flash.ratePhase     != rhs.flash.ratePhase     ||
		       lhs.flash.phase2HzShown != rhs.flash.phase2HzShown ||
		       lhs.display             != rhs.display             ||
		       lhs.style.proportional  != rhs.style.proportional  ||
		       lhs.style.bold          != rhs.style.bold          ||
		       lhs.style.italic        != rhs.style.italic;
	}

	struct textCell {
		textCharacter character;
		textAttributes attribute;
		CharacterFragment fragment=NormalSize;
		int g0Set=0;
		int g2Set=7;
	};

	friend inline bool operator!=(const textCell &lhs, const textCell &rhs)
	{
		return lhs.character != rhs.character ||
		       lhs.attribute != rhs.attribute ||
		       lhs.fragment  != rhs.fragment;
	}

	struct textPainter {
		textAttributes attribute;
		textCell result;
		textCell rightHalfCell;
		textCell bottomHalfCell[72];

		int styleSpreadRows=0;
		int setProportionalRows[72], clearProportionalRows[72];
		int setBoldRows[72], clearBoldRows[72];
		int setItalicRows[72], clearItalicRows[72];
	};

	const QMap<int, int> m_level1CharacterMap {
		{ 0x00, 12 }, { 0x01, 15 }, { 0x02, 22 }, { 0x03, 16 }, { 0x04, 14 }, { 0x05, 19 }, { 0x06, 11 },
		{ 0x08, 18 }, { 0x09, 15 }, { 0x0a, 22 }, { 0x0b, 16 }, { 0x0c, 14 }, { 0x0e, 11 },
		{ 0x10, 12 }, { 0x11, 15 }, { 0x12, 22 }, { 0x13, 16 }, { 0x14, 14 }, { 0x15, 19 }, { 0x16, 23 },
		{ 0x1d, 21 }, { 0x1f, 20 },
		{ 0x20,  1 }, { 0x21, 15 }, { 0x22, 13 }, { 0x23, 17 }, { 0x24,  2 }, { 0x25,  3 }, { 0x26, 11 },
		{ 0x36, 23 }, { 0x37,  4 },
		{ 0x40, 12 }, { 0x44, 14 }, { 0x47,  5 },
		{ 0x55,  6 }, { 0x57,  5 }
	};
	const QMap<int, int> m_g0CharacterMap {
		{ 0x20,  1 }, { 0x24,  2 }, { 0x25,  3 },
		{ 0x37,  4 },
		{ 0x47,  5 },
		{ 0x55,  6 }, { 0x57,  5 }
	};
	const QMap<int, int> m_g2CharacterMap {
		{ 0x20,  8 }, { 0x24,  8 }, { 0x25,  8 },
		{ 0x37,  9 },
		{ 0x40, 10 }, { 0x44, 10 }, { 0x47, 10 },
		{ 0x55, 10 }, { 0x57, 10 }
	};

	class Invocation
	{
	public:
		Invocation();

		X26TripletList *tripletList() const { return m_tripletList; };
		void clear();
		void setTripletList(X26TripletList *tripletList);
		int startTripletNumber() const { return m_startTripletNumber; };
		void setStartTripletNumber(int n);
		int endTripletNumber() const { return m_endTripletNumber; };
		void setEndTripletNumber(int n);
		int originRow() const { return m_originRow; };
		int originColumn() const { return m_originColumn; };
		void setOrigin(int row, int column);
		void buildMap(int level);

		QList<QPair<int, int>> charPositions() const { return m_characterMap.uniqueKeys(); };
		QList<QPair<int, int>> attrPositions() const { return m_attributeMap.uniqueKeys(); };
		QList<X26Triplet> charactersMappedAt(int r, int c) const { return m_characterMap.values(qMakePair(r, c)); };
		QList<X26Triplet> attributesMappedAt(int r, int c) const { return m_attributeMap.values(qMakePair(r, c)); };
		int rightMostColumn(int r) const { return m_rightMostColumn.value(r, -1); };
		int fullScreenColour() const { return m_fullScreenCLUT; };
		QList<X26Triplet> fullRowColoursMappedAt(int r) const { return m_fullRowCLUTMap.values(r); };

	private:
		X26TripletList *m_tripletList;
		int m_startTripletNumber, m_endTripletNumber;
		int m_originRow, m_originColumn;
		// QPair is row and column
		QMultiMap<QPair<int, int>, X26Triplet> m_characterMap;
		QMultiMap<QPair<int, int>, X26Triplet> m_attributeMap;
		QMap<int, int> m_rightMostColumn;
		int m_fullScreenCLUT;
		QMultiMap<int, X26Triplet> m_fullRowCLUTMap;
	};

	static int s_instances;
	static textPainter s_blankPainter;

	void decodeRow(int r);
	QColor cellQColor(int r, int c, ColourPart colourPart);
	textCell& cellAtCharacterOrigin(int r, int c);
	void buildInvocationList(Invocation &invocation, int objectType);
	textCharacter characterFromTriplets(const QList<X26Triplet> triplets);
	inline void rotateFlashMovement(flashFunctions &flash);

	bool m_refresh[25][72];
	textCell m_cell[25][72];
	bool m_cellLevel1MosaicAttr[25][40];
	bool m_cellLevel1MosaicChar[25][40];
	int m_cellLevel1CharSet[25][40];
	LevelOnePage* m_levelOnePage;
	int m_fullRowColour[25];
	QColor m_fullRowQColor[25];
	QList<Invocation> m_invocations[3];
	Invocation m_localEnhancements;
	textPainter m_level1ActivePainter;
	QList<textPainter> m_adapPassPainter[2];
	int m_level1DefaultCharSet, m_level1SecondCharSet;
	int m_defaultG0andG2, m_secondG0andG2;

	RowHeight m_rowHeight[25];
};

#endif
