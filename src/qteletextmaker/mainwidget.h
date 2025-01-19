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

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QBasicTimer>
#include <QFrame>
#include <QGraphicsItemGroup>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QPair>
#include <QTextStream>
#include <vector>

#include "decode.h"
#include "document.h"
#include "levelonepage.h"
#include "render.h"

class QPaintEvent;

class TeletextWidget : public QFrame
{
	Q_OBJECT

public:
	TeletextWidget(QFrame *parent = 0);
	~TeletextWidget();
	void setCharacter(unsigned char newCharacter);
	void toggleCharacterBit(unsigned char bitToToggle);
	bool insertMode() const { return m_insertMode; };
	void setInsertMode(bool insertMode);
	bool showControlCodes() const { return m_pageRender.showControlCodes(); };
	int flashTiming() const { return m_flashTiming; };

	QSize sizeHint() { return QSize(480+(pageDecode()->leftSidePanelColumns()+pageDecode()->rightSidePanelColumns())*12, 250); }

	void inputMethodEvent(QInputMethodEvent *event) override;

	TeletextDocument* document() const { return m_teletextDocument; }
	TeletextPageDecode *pageDecode() { return &m_pageDecode; }
	TeletextPageRender *pageRender() { return &m_pageRender; }

signals:
	void sizeChanged();
	void insertKeyPressed();

public slots:
	void subPageSelected();
	void refreshPage();
	void setReveal(bool reveal);
	void setShowControlCodes(bool showControlCodes);
	void setRenderMode(TeletextPageRender::RenderMode renderMode);
	void updateFlashTimer(int newFlashTimer);
	void pauseFlash(int p);
	void resumeFlash();

	void setControlBit(int bitNumber, bool active);
	void setDefaultCharSet(int newDefaultCharSet);
	void setDefaultNOS(int newDefaultNOS);
	void setSidePanelWidths(int newLeftSidePanelColumns, int newRightSidePanelColumns);
	void setSidePanelAtL35Only(bool newSidePanelAtL35Only);

	void cut();
	void copy();
	void paste();

	void changeSize();

protected:
	void paintEvent(QPaintEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void focusOutEvent(QFocusEvent *event) override;

	TeletextPageDecode m_pageDecode;
	TeletextPageRender m_pageRender;

private:
	TeletextDocument* m_teletextDocument;
	LevelOnePage* m_levelOnePage;
	bool m_insertMode, m_selectionInProgress;
	QBasicTimer m_flashTimer;
	int m_flashTiming, m_flashPhase;

	void timerEvent(QTimerEvent *event) override;
	QSet<QPair<int, int>> findMosaics();
	void shiftMosaics(int key);
	void selectionToClipboard();

	QPair<int, int> mouseToRowAndColumn(const QPoint &mousePosition);
};

class LevelOneScene : public QGraphicsScene
{
	Q_OBJECT

public:
	LevelOneScene(QWidget *levelOneWidget, QObject *parent = nullptr);
	void setBorderDimensions(int sceneWidth, int sceneHeight, int widgetWidth, int leftSidePanelColumns, int rightSidePanelColumns);
	QGraphicsRectItem *cursorRectItem() const { return m_cursorRectItem; }

public slots:
	void updateCursor();
	void updateSelection();
	void setRenderMode(TeletextPageRender::RenderMode renderMode);
	void toggleGrid(bool gridOn);
	void hideGUIElements(bool hidden);
	void setFullScreenColour(const QColor &newColor);
	void setFullRowColour(int row, const QColor &newColor);

signals:
	void mouseZoomIn();
	void mouseZoomOut();

protected:
	bool eventFilter(QObject *object, QEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *keyEvent);

private:
	QGraphicsRectItem *m_fullScreenTopRectItem, *m_fullScreenBottomRectItem;
	QGraphicsRectItem *m_fullRowLeftRectItem[25], *m_fullRowRightRectItem[25];
	QGraphicsProxyWidget *m_levelOneProxyWidget;
	QGraphicsRectItem *m_cursorRectItem, *m_selectionRectItem;
	QGraphicsItemGroup *m_mainGridItemGroup, *m_sidePanelGridItemGroup[32];
	bool m_grid, m_sidePanelGridNeeded[32];
};

#endif
