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

#include <QBitmap>
#include <QFrame>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QMenu>
#include <QPainter>
#include <QPair>
#include <QUndoCommand>
#include <QWidget>
#include <vector>
#include <iostream>

#include "mainwidget.h"

#include "document.h"
#include "levelonecommands.h"
#include "levelonepage.h"
#include "render.h"
#include "x26triplets.h"

TeletextWidget::TeletextWidget(QFrame *parent) : QFrame(parent)
{
	this->resize(QSize(480, 250));
	this->setAttribute(Qt::WA_NoSystemBackground);
	m_teletextDocument = new TeletextDocument();
	m_levelOnePage = m_teletextDocument->currentSubPage();
	m_pageRender.setTeletextPage(m_levelOnePage);
	m_insertMode = false;
	m_selectionInProgress = false;
	m_grid = false;
	setFocusPolicy(Qt::StrongFocus);
	m_flashTiming = m_flashPhase = 0;
	connect(&m_pageRender, &TeletextPageRender::flashChanged, this, &TeletextWidget::updateFlashTimer);
	connect(&m_pageRender, &TeletextPageRender::sidePanelsChanged, this, &TeletextWidget::changeSize);
	connect(m_teletextDocument, &TeletextDocument::subPageSelected, this, &TeletextWidget::subPageSelected);
	connect(m_teletextDocument, &TeletextDocument::contentsChange, this, &TeletextWidget::refreshRow);
	connect(m_teletextDocument, &TeletextDocument::refreshNeeded, this, &TeletextWidget::refreshPage);
	connect(m_teletextDocument, &TeletextDocument::selectionMoved, this, QOverload<>::of(&TeletextWidget::update));
}

TeletextWidget::~TeletextWidget()
{
	if (m_flashTimer.isActive())
		m_flashTimer.stop();
	delete m_teletextDocument;
}

void TeletextWidget::subPageSelected()
{
	m_levelOnePage = m_teletextDocument->currentSubPage();
	m_pageRender.setTeletextPage(m_levelOnePage);
	refreshPage();
}

void TeletextWidget::refreshRow(int rowChanged)
{
	m_pageRender.renderPage(rowChanged);
	update();
}

void TeletextWidget::refreshPage()
{
	m_pageRender.decodePage();
	m_pageRender.renderPage();
	update();
}

void TeletextWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);
	QPainter widgetPainter(this);

	widgetPainter.drawPixmap(m_pageRender.leftSidePanelColumns()*12, 0, *m_pageRender.pagePixmap(m_flashPhase), 0, 0, 480, 250);
	if (m_pageRender.leftSidePanelColumns())
		widgetPainter.drawPixmap(0, 0, *m_pageRender.pagePixmap(m_flashPhase), 864-m_pageRender.leftSidePanelColumns()*12, 0, m_pageRender.leftSidePanelColumns()*12, 250);
	if (m_pageRender.rightSidePanelColumns())
		widgetPainter.drawPixmap(480+m_pageRender.leftSidePanelColumns()*12, 0, *m_pageRender.pagePixmap(m_flashPhase), 480, 0, m_pageRender.rightSidePanelColumns()*12, 250);
	if (this->hasFocus())
		widgetPainter.fillRect((m_teletextDocument->cursorColumn()+m_pageRender.leftSidePanelColumns())*12, m_teletextDocument->cursorRow()*10, 12, 10, QColor(128, 128, 128, 192));
	if (m_teletextDocument->selectionActive()) {
		widgetPainter.setPen(QPen(QColor(192, 192, 192, 224), 1, Qt::DashLine));
		widgetPainter.setBrush(QBrush(QColor(255, 255, 255, 64)));
		widgetPainter.drawRect((m_teletextDocument->selectionLeftColumn()+m_pageRender.leftSidePanelColumns())*12, m_teletextDocument->selectionTopRow()*10, m_teletextDocument->selectionWidth()*12-1, m_teletextDocument->selectionHeight()*10-1);
	}
}

void TeletextWidget::updateFlashTimer(int newFlashTimer)
{
	m_flashTiming = newFlashTimer;
	m_flashPhase = 0;
	if (newFlashTimer == 0) {
		m_flashTimer.stop();
		update();
		return;
	}
	m_flashTimer.start((newFlashTimer == 1) ? 500 : 167, this);
}

void TeletextWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_flashTimer.timerId()) {
		if (m_flashTiming == 1)
			m_flashPhase += 3;
		else
			m_flashPhase++;
		if (m_flashPhase == 6)
			m_flashPhase = 0;
		update();
	} else
		QWidget::timerEvent(event);
}

void TeletextWidget::toggleReveal(bool revealOn)
{
	m_pageRender.setReveal(revealOn);
	update();
}

void TeletextWidget::toggleMix(bool mixOn)
{
	m_pageRender.setMix(mixOn);
	update();
}

void TeletextWidget::toggleGrid(bool gridOn)
{
	m_grid = gridOn;
	m_pageRender.setGrid(gridOn);
	m_pageRender.renderPage();
}

void TeletextWidget::setControlBit(int bitNumber, bool active)
{
	m_levelOnePage->setControlBit(bitNumber, active);
	if (bitNumber == 1 || bitNumber == 2) {
		m_pageRender.decodePage();
		m_pageRender.renderPage();
	}
}

void TeletextWidget::setDefaultCharSet(int newDefaultCharSet)
{
	m_levelOnePage->setDefaultCharSet(newDefaultCharSet);
}

void TeletextWidget::setDefaultNOS(int newDefaultNOS)
{
	m_levelOnePage->setDefaultNOS(newDefaultNOS);
}

void TeletextWidget::setDefaultScreenColour(int newColour)
{
	m_levelOnePage->setDefaultScreenColour(newColour);
	m_pageRender.decodePage();
	m_pageRender.renderPage();
}

void TeletextWidget::setDefaultRowColour(int newColour)
{
	m_levelOnePage->setDefaultRowColour(newColour);
	m_pageRender.decodePage();
	m_pageRender.renderPage();
}

void TeletextWidget::setColourTableRemap(int newMap)
{
	m_levelOnePage->setColourTableRemap(newMap);
	m_pageRender.decodePage();
	m_pageRender.renderPage();
}

void TeletextWidget::setBlackBackgroundSubst(bool substOn)
{
	m_levelOnePage->setBlackBackgroundSubst(substOn);
	m_pageRender.decodePage();
	m_pageRender.renderPage();
}

void TeletextWidget::setSidePanelWidths(int newLeftSidePanelColumns, int newRightSidePanelColumns)
{
	m_levelOnePage->setLeftSidePanelDisplayed(newLeftSidePanelColumns != 0);
	m_levelOnePage->setRightSidePanelDisplayed(newRightSidePanelColumns != 0);
	if (newLeftSidePanelColumns)
		m_levelOnePage->setSidePanelColumns((newLeftSidePanelColumns == 16) ? 0 : newLeftSidePanelColumns);
	else
		m_levelOnePage->setSidePanelColumns((newRightSidePanelColumns == 0) ? 0 : 16-newRightSidePanelColumns);
	m_pageRender.updateSidePanels();
}

void TeletextWidget::setSidePanelAtL35Only(bool newSidePanelAtL35Only)
{
	m_levelOnePage->setSidePanelStatusL25(!newSidePanelAtL35Only);
	m_pageRender.updateSidePanels();
}

void TeletextWidget::changeSize()
{
	setFixedSize(QSize(480+(pageRender()->leftSidePanelColumns()+pageRender()->rightSidePanelColumns())*12, 250));
	emit sizeChanged();
}

void TeletextWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() < 0x01000000) {
		char keyPressed = *qPrintable(event->text());
//		if (attributes[cursorRow][cursorColumn].mosaics && (keyPressed < 0x40 || keyPressed > 0x5f) && (((keyPressed >= '1' && keyPressed <= '9') && (event->modifiers() & Qt::KeypadModifier)) || (keyPressed >= 'a' && keyPressed <= 'z'))) {
		if (m_pageRender.level1MosaicAttribute(m_teletextDocument->cursorRow(), m_teletextDocument->cursorColumn()) && (keyPressed < 0x40 || keyPressed > 0x5f) && (((keyPressed >= '1' && keyPressed <= '9') && (event->modifiers() & Qt::KeypadModifier)) || (keyPressed >= 'a' && keyPressed <= 'z'))) {
			if (!(m_levelOnePage->character(m_teletextDocument->cursorRow(), m_teletextDocument->cursorColumn()) & 0x20))
				setCharacter(0x20);
			switch (event->key()) {
				case Qt::Key_Q:
				case Qt::Key_7:
					toggleCharacterBit(0x01); // Top left
					break;
				case Qt::Key_W:
				case Qt::Key_8:
					toggleCharacterBit(0x02); // Top right
					break;
				case Qt::Key_A:
				case Qt::Key_4:
					toggleCharacterBit(0x04); // Middle left
					break;
				case Qt::Key_S:
				case Qt::Key_5:
					toggleCharacterBit(0x08); // Middle right
					break;
				case Qt::Key_Z:
				case Qt::Key_1:
					toggleCharacterBit(0x10); // Bottom left
					break;
				case Qt::Key_X:
				case Qt::Key_2:
					toggleCharacterBit(0x40); // Bottom right
					break;
				case Qt::Key_R:
				case Qt::Key_9:
					toggleCharacterBit(0x5f); // Invert
					break;
				case Qt::Key_F:
				case Qt::Key_6:
					toggleCharacterBit(0x7f); // Set all
					break;
				case Qt::Key_C:
				case Qt::Key_3:
					toggleCharacterBit(0x20); // Clear all
					break;
			}
			return;
		}
		if (keyPressed == 0x20)
			setCharacter((event->modifiers() & Qt::ShiftModifier) ? 0x7f : 0x20);
		else
			setCharacter(keyPressed);
		return;
	}
	switch (event->key()) {
		case Qt::Key_Backspace:
			backspaceEvent();
			break;
		case Qt::Key_Up:
			m_teletextDocument->cursorUp();
			update();
			break;
		case Qt::Key_Down:
			m_teletextDocument->cursorDown();
			update();
			break;
		case Qt::Key_Left:
			m_teletextDocument->cursorLeft();
			update();
			break;
		case Qt::Key_Right:
			m_teletextDocument->cursorRight();
			update();
			break;
		case Qt::Key_Return:
		case Qt::Key_Enter:
			m_teletextDocument->cursorDown();
			// fall through
		case Qt::Key_Home:
			m_teletextDocument->moveCursor(m_teletextDocument->cursorRow(), 0);
			update();
			break;
		case Qt::Key_End:
			m_teletextDocument->moveCursor(m_teletextDocument->cursorRow(), 39);
			update();
			break;

		case Qt::Key_PageUp:
			m_teletextDocument->selectSubPageNext();
			break;
		case Qt::Key_PageDown:
			m_teletextDocument->selectSubPagePrevious();
			break;
		case Qt::Key_F5:
			m_pageRender.decodePage();
			m_pageRender.renderPage();
			update();
			break;
		default:
			QFrame::keyPressEvent(event);
	}
}

void TeletextWidget::setCharacter(unsigned char newCharacter)
{
	QUndoCommand *overwriteCharacterCommand = new OverwriteCharacterCommand(m_teletextDocument, newCharacter);
	m_teletextDocument->undoStack()->push(overwriteCharacterCommand);
}

void TeletextWidget::toggleCharacterBit(unsigned char bitToToggle)
{
	QUndoCommand *toggleMosaicBitCommand = new ToggleMosaicBitCommand(m_teletextDocument, bitToToggle);
	m_teletextDocument->undoStack()->push(toggleMosaicBitCommand);
}

void TeletextWidget::backspaceEvent()
{
	QUndoCommand *backspaceCommand = new BackspaceCommand(m_teletextDocument);
	m_teletextDocument->undoStack()->push(backspaceCommand);
}

QPair<int, int> TeletextWidget::mouseToRowAndColumn(const QPoint &mousePosition)
{
	int row = mousePosition.y() / 10;
	int column = mousePosition.x() / 12 - m_pageRender.leftSidePanelColumns();
	if (row < 1)
		row = 1;
	if (row > 24)
		row = 24;
	if (column < 0)
		column = 0;
	if (column > 39)
		column = 39;
	return qMakePair(row, column);
}

void TeletextWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		m_teletextDocument->cancelSelection();
		QPair<int, int> position = mouseToRowAndColumn(event->pos());
		m_teletextDocument->moveCursor(position.first, position.second);
		update();
	}
}

void TeletextWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (event->buttons() & Qt::LeftButton) {
		QPair<int, int> position = mouseToRowAndColumn(event->pos());
		if (m_selectionInProgress || position.first != m_teletextDocument->cursorRow() || position.second != m_teletextDocument->cursorColumn()) {
			int topRow, bottomRow, leftColumn, rightColumn;

			m_selectionInProgress = true;
			if (m_teletextDocument->cursorRow() < position.first) {
				topRow = m_teletextDocument->cursorRow();
				bottomRow = position.first;
			} else {
				topRow = position.first;
				bottomRow = m_teletextDocument->cursorRow();
			}
			if (m_teletextDocument->cursorColumn() < position.second) {
				leftColumn = m_teletextDocument->cursorColumn();
				rightColumn = position.second;
			} else {
				leftColumn = position.second;
				rightColumn = m_teletextDocument->cursorColumn();
			}
			m_teletextDocument->setSelection(topRow, leftColumn, bottomRow, rightColumn);
		}
	}
}

void TeletextWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		m_selectionInProgress = false;
}

void TeletextWidget::focusInEvent(QFocusEvent *event)
{
	QFrame::focusInEvent(event);
}

void TeletextWidget::focusOutEvent(QFocusEvent *event)
{
	QFrame::focusOutEvent(event);
}


LevelOneScene::LevelOneScene(QWidget *levelOneWidget, QObject *parent) : QGraphicsScene(parent)
{
	setSceneRect(0, 0, 600, 288);
	m_fullScreenTopRectItem = new QGraphicsRectItem(0, 0, 600, 19);
	m_fullScreenTopRectItem->setPen(Qt::NoPen);
	m_fullScreenTopRectItem->setBrush(QBrush(QColor(0, 0, 0)));
	addItem(m_fullScreenTopRectItem);
	m_fullScreenBottomRectItem = new QGraphicsRectItem(0, 269, 600, 19);
	m_fullScreenBottomRectItem->setPen(Qt::NoPen);
	m_fullScreenBottomRectItem->setBrush(QBrush(QColor(0, 0, 0)));
	addItem(m_fullScreenBottomRectItem);

	for (int r=0; r<25; r++) {
		m_fullRowLeftRectItem[r] = new QGraphicsRectItem(0, 19+r*10, 60, 10);
		m_fullRowLeftRectItem[r]->setPen(Qt::NoPen);
		m_fullRowLeftRectItem[r]->setBrush(QBrush(QColor(0, 0, 0)));
		addItem(m_fullRowLeftRectItem[r]);
		m_fullRowRightRectItem[r] = new QGraphicsRectItem(540, 19+r*10, 60, 10);
		m_fullRowRightRectItem[r]->setPen(Qt::NoPen);
		m_fullRowRightRectItem[r]->setBrush(QBrush(QColor(0, 0, 0)));
		addItem(m_fullRowRightRectItem[r]);
	}

	m_levelOneProxyWidget = addWidget(levelOneWidget);
	m_levelOneProxyWidget->setPos(60, 19);
	m_levelOneProxyWidget->setAutoFillBackground(false);
}

void LevelOneScene::setDimensions(int sceneWidth, int sceneHeight, int widgetWidth)
{
	setSceneRect(0, 0, sceneWidth, sceneHeight);

	// Assume widget height is always 250
	int topBottomBorders = (sceneHeight-250) / 2;
	// Ideally we'd use m_levelOneProxyWidget->size() to discover the widget width ourselves
	// but this causes a stubborn segfault, so we have to receive the widgetWidth as a parameter
	int leftRightBorders = (sceneWidth-widgetWidth) / 2;

	m_levelOneProxyWidget->setPos(leftRightBorders, topBottomBorders);

	m_fullScreenTopRectItem->setRect(0, 0, sceneWidth, topBottomBorders);
	m_fullScreenBottomRectItem->setRect(0, 250+topBottomBorders, sceneWidth, topBottomBorders);

	for (int r=0; r<25; r++) {
		m_fullRowLeftRectItem[r]->setRect(0, topBottomBorders+r*10, leftRightBorders+1, 10);
		m_fullRowRightRectItem[r]->setRect(sceneWidth-leftRightBorders-1, topBottomBorders+r*10, leftRightBorders+1, 10);
	}
}

void LevelOneScene::setFullScreenColour(const QColor &newColor)
{
	m_fullScreenTopRectItem->setBrush(QBrush(newColor));
	m_fullScreenBottomRectItem->setBrush(QBrush(newColor));
}

void LevelOneScene::setFullRowColour(int row, const QColor &newColor)
{
	m_fullRowLeftRectItem[row]->setBrush(QBrush(newColor));
	m_fullRowRightRectItem[row]->setBrush(QBrush(newColor));
}
