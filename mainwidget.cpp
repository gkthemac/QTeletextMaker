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

#include <QBitmap>
#include <QFrame>
#include <QGraphicsItem>
#include <QGraphicsItemGroup>
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
#include "keymap.h"
#include "levelonecommands.h"
#include "levelonepage.h"
#include "render.h"
#include "x26triplets.h"

TeletextWidget::TeletextWidget(QFrame *parent) : QFrame(parent)
{
	this->resize(QSize(480, 250));
	this->setAttribute(Qt::WA_NoSystemBackground);
	this->setAttribute(Qt::WA_InputMethodEnabled, true);
	m_teletextDocument = new TeletextDocument();
	m_levelOnePage = m_teletextDocument->currentSubPage();
	m_pageRender.setTeletextPage(m_levelOnePage);
	m_insertMode = false;
	m_selectionInProgress = false;
	setFocusPolicy(Qt::StrongFocus);
	m_flashTiming = m_flashPhase = 0;
	connect(&m_pageRender, &TeletextPageRender::flashChanged, this, &TeletextWidget::updateFlashTimer);
	connect(&m_pageRender, &TeletextPageRender::sidePanelsChanged, this, &TeletextWidget::changeSize);
	connect(m_teletextDocument, &TeletextDocument::subPageSelected, this, &TeletextWidget::subPageSelected);
	connect(m_teletextDocument, &TeletextDocument::contentsChange, this, &TeletextWidget::refreshRow);
	connect(m_teletextDocument, &TeletextDocument::refreshNeeded, this, &TeletextWidget::refreshPage);
}

TeletextWidget::~TeletextWidget()
{
	if (m_flashTimer.isActive())
		m_flashTimer.stop();
	delete m_teletextDocument;
}

// Re-implemented so compose/dead keys work properly
void TeletextWidget::inputMethodEvent(QInputMethodEvent* event)
{
	if (!event->commitString().isEmpty()) {
		QKeyEvent keyEvent(QEvent::KeyPress, 0, Qt::NoModifier, event->commitString());

		keyPressEvent(&keyEvent);
	}
	event->accept();
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

void TeletextWidget::setInsertMode(bool insertMode)
{
	m_insertMode = insertMode;
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
	update();
}

void TeletextWidget::setColourTableRemap(int newMap)
{
	m_levelOnePage->setColourTableRemap(newMap);
	m_pageRender.decodePage();
	m_pageRender.renderPage();
	update();
}

void TeletextWidget::setBlackBackgroundSubst(bool substOn)
{
	m_levelOnePage->setBlackBackgroundSubst(substOn);
	m_pageRender.decodePage();
	m_pageRender.renderPage();
	update();
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
		// A character-typing key was pressed
		// Try to keymap it, if not keymapped then plain ASCII code (may be) returned
		char mappedKeyPress = keymapping[m_pageRender.level1CharSet(m_teletextDocument->cursorRow(), m_teletextDocument->cursorColumn())].value(event->text().at(0), *qPrintable(event->text()));
		if (mappedKeyPress < 0x20)
			return;
		// If outside ASCII map then the character can't be represented by current Level 1 character set
		// Map it to block character so it doesn't need to be inserted-between later on
		if (mappedKeyPress & 0x80)
			mappedKeyPress = 0x7f;
		if (m_pageRender.level1MosaicAttribute(m_teletextDocument->cursorRow(), m_teletextDocument->cursorColumn()) && (mappedKeyPress < 0x40 || mappedKeyPress > 0x5f)) {
			// We're on a mosaic and a blast-through character was NOT pressed
			if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_9 && event->modifiers() & Qt::KeypadModifier) {
				switch (event->key()) {
					case Qt::Key_7:
						toggleCharacterBit(0x01); // Top left
						break;
					case Qt::Key_8:
						toggleCharacterBit(0x02); // Top right
						break;
					case Qt::Key_4:
						toggleCharacterBit(0x04); // Middle left
						break;
					case Qt::Key_5:
						toggleCharacterBit(0x08); // Middle right
						break;
					case Qt::Key_1:
						toggleCharacterBit(0x10); // Bottom left
						break;
					case Qt::Key_2:
						toggleCharacterBit(0x40); // Bottom right
						break;
					case Qt::Key_9:
						toggleCharacterBit(0x5f); // Invert
						break;
					case Qt::Key_6:
						toggleCharacterBit(0x7f); // Set all
						break;
					case Qt::Key_3:
						toggleCharacterBit(0x20); // Clear all
						break;
				}
				return;
			}
			if (event->key() == Qt::Key_Space) {
				setCharacter(0x20);
				return;
			}

			// This macro is defined in keymap.h if no native scan codes are defined
			// for the platform we are compiling on
#ifndef QTTM_NONATIVESCANCODES
			if (event->nativeScanCode() > 1) {
				switch (event->nativeScanCode()) {
					case mosaicNativeScanCodes[0]:
						toggleCharacterBit(0x01); // Top left
						break;
					case mosaicNativeScanCodes[1]:
						toggleCharacterBit(0x02); // Top right
						break;
					case mosaicNativeScanCodes[2]:
						toggleCharacterBit(0x04); // Middle left
						break;
					case mosaicNativeScanCodes[3]:
						toggleCharacterBit(0x08); // Middle right
						break;
					case mosaicNativeScanCodes[4]:
						toggleCharacterBit(0x10); // Bottom left
						break;
					case mosaicNativeScanCodes[5]:
						toggleCharacterBit(0x40); // Bottom right
						break;
					case mosaicNativeScanCodes[6]:
						toggleCharacterBit(0x5f); // Invert
						break;
					case mosaicNativeScanCodes[7]:
						toggleCharacterBit(0x7f); // Set all
						break;
					case mosaicNativeScanCodes[8]:
						toggleCharacterBit(0x20); // Clear all
						break;
				}
				return;
			} else
				qDebug("nativeScanCode not usable! Please use numeric keypad to toggle mosaic bits.");
#else
			qDebug("nativeScanCode was not compiled in! Please use numeric keypad to toggle mosaic bits.");
#endif

			// TODO some contingency plan if nativeScanCode didn't work?
			return;
		}
		setCharacter(mappedKeyPress);
		return;
	}
	switch (event->key()) {
		case Qt::Key_Backspace:
			m_teletextDocument->undoStack()->push(new BackspaceKeyCommand(m_teletextDocument, m_insertMode));
			break;
		case Qt::Key_Tab:
			m_teletextDocument->undoStack()->push(new TypeCharacterCommand(m_teletextDocument, 0x20, true));
			break;
		case Qt::Key_Delete:
			m_teletextDocument->undoStack()->push(new DeleteKeyCommand(m_teletextDocument));
			break;
		case Qt::Key_Insert:
			emit insertKeyPressed();
			break;

		case Qt::Key_Up:
			m_teletextDocument->cursorUp();
			break;
		case Qt::Key_Down:
			m_teletextDocument->cursorDown();
			break;
		case Qt::Key_Left:
			m_teletextDocument->cursorLeft();
			break;
		case Qt::Key_Right:
			m_teletextDocument->cursorRight();
			break;
		case Qt::Key_Return:
		case Qt::Key_Enter:
			m_teletextDocument->cursorDown();
			// fall through
		case Qt::Key_Home:
			m_teletextDocument->moveCursor(m_teletextDocument->cursorRow(), 0);
			break;
		case Qt::Key_End:
			m_teletextDocument->moveCursor(m_teletextDocument->cursorRow(), 39);
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
	m_teletextDocument->undoStack()->push(new TypeCharacterCommand(m_teletextDocument, newCharacter, m_insertMode));
}

void TeletextWidget::toggleCharacterBit(unsigned char bitToToggle)
{
	QUndoCommand *toggleMosaicBitCommand = new ToggleMosaicBitCommand(m_teletextDocument, bitToToggle);
	m_teletextDocument->undoStack()->push(toggleMosaicBitCommand);
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
	m_grid = false;

	// These dimensions are scratch, setBorderDimensions will get called straight away to adjust them
	setSceneRect(0, 0, 600, 288);

	// Full screen colours
	m_fullScreenTopRectItem = new QGraphicsRectItem(0, 0, 600, 19);
	m_fullScreenTopRectItem->setPen(Qt::NoPen);
	m_fullScreenTopRectItem->setBrush(QBrush(QColor(0, 0, 0)));
	addItem(m_fullScreenTopRectItem);
	m_fullScreenBottomRectItem = new QGraphicsRectItem(0, 269, 600, 19);
	m_fullScreenBottomRectItem->setPen(Qt::NoPen);
	m_fullScreenBottomRectItem->setBrush(QBrush(QColor(0, 0, 0)));
	addItem(m_fullScreenBottomRectItem);

	// Full row colours
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

	// Main text widget
	m_levelOneProxyWidget = addWidget(levelOneWidget);
	m_levelOneProxyWidget->setPos(60, 19);
	m_levelOneProxyWidget->setAutoFillBackground(false);

	// Selection
	m_selectionRectItem = new QGraphicsRectItem(0, 0, 12, 10);
	m_selectionRectItem->setVisible(false);
	m_selectionRectItem->setPen(QPen(QColor(192, 192, 192), 1, Qt::DashLine));
	m_selectionRectItem->setBrush(QBrush(QColor(255, 255, 255, 64)));
	addItem(m_selectionRectItem);

	// Cursor
	m_cursorRectItem = new QGraphicsRectItem(0, 0, 12, 10);
	m_cursorRectItem->setPen(Qt::NoPen);
	m_cursorRectItem->setBrush(QBrush(QColor(128, 128, 128, 192)));
	addItem(m_cursorRectItem);

	// Optional grid overlay for text widget
	m_mainGridItemGroup = new QGraphicsItemGroup;
	m_mainGridItemGroup->setVisible(false);
	addItem(m_mainGridItemGroup);
	// Additional vertical pieces of grid for side panels
	for (int i=0; i<32; i++) {
		m_sidePanelGridNeeded[i] = false;
		m_sidePanelGridItemGroup[i] = new QGraphicsItemGroup;
		m_sidePanelGridItemGroup[i]->setVisible(false);
		addItem(m_sidePanelGridItemGroup[i]);
	}
	for (int r=1; r<25; r++) {
		for (int c=0; c<40; c++) {
			QGraphicsRectItem *gridPiece = new QGraphicsRectItem(c*12, r*10, 12, 10);
			gridPiece->setPen(QPen(QBrush(QColor(128, 128, 128, r<24 ? 192 : 128)), 0));
			m_mainGridItemGroup->addToGroup(gridPiece);
		}

		if (r < 24)
			for (int c=0; c<32; c++) {
				QGraphicsRectItem *gridPiece = new QGraphicsRectItem(0, r*10, 12, 10);
				gridPiece->setPen(QPen(QBrush(QColor(128, 128, 128, 64)), 0));
				m_sidePanelGridItemGroup[c]->addToGroup(gridPiece);
			}
	}
}

void LevelOneScene::setBorderDimensions(int sceneWidth, int sceneHeight, int widgetWidth, int leftSidePanelColumns, int rightSidePanelColumns)
{
	setSceneRect(0, 0, sceneWidth, sceneHeight);

	// Assume text widget height is always 250
	int topBottomBorders = (sceneHeight-250) / 2;
	// Ideally we'd use m_levelOneProxyWidget->size() to discover the widget width ourselves
	// but this causes a stubborn segfault, so we have to receive the widgetWidth as a parameter
	int leftRightBorders = (sceneWidth-widgetWidth) / 2;

	m_levelOneProxyWidget->setPos(leftRightBorders, topBottomBorders);

	// Position grid to cover central 40 columns
	m_mainGridItemGroup->setPos(leftRightBorders + leftSidePanelColumns*12, topBottomBorders);

	updateCursor();
	updateSelection();

	// Grid for right side panel
	for (int c=0; c<16; c++)
		if (rightSidePanelColumns > c) {
			m_sidePanelGridItemGroup[c]->setPos(leftRightBorders + leftSidePanelColumns*12 + 480 + c*12, topBottomBorders);
			m_sidePanelGridItemGroup[c]->setVisible(m_grid);
			m_sidePanelGridNeeded[c] = true;
		} else {
			m_sidePanelGridItemGroup[c]->setVisible(false);
			m_sidePanelGridNeeded[c] = false;
		}
	// Grid for left side panel
	for (int c=0; c<16; c++)
		if (c < leftSidePanelColumns) {
			m_sidePanelGridItemGroup[31-c]->setPos(leftRightBorders + (leftSidePanelColumns-c-1)*12, topBottomBorders);
			m_sidePanelGridItemGroup[31-c]->setVisible(m_grid);
			m_sidePanelGridNeeded[31-c] = true;
		} else {
			m_sidePanelGridItemGroup[31-c]->setVisible(false);
			m_sidePanelGridNeeded[31-c] = false;
		}

	// Full screen colours
	m_fullScreenTopRectItem->setRect(0, 0, sceneWidth, topBottomBorders);
	m_fullScreenBottomRectItem->setRect(0, 250+topBottomBorders, sceneWidth, topBottomBorders);
	// Full row colours
	for (int r=0; r<25; r++) {
		m_fullRowLeftRectItem[r]->setRect(0, topBottomBorders+r*10, leftRightBorders+1, 10);
		m_fullRowRightRectItem[r]->setRect(sceneWidth-leftRightBorders-1, topBottomBorders+r*10, leftRightBorders+1, 10);
	}
}

void LevelOneScene::updateCursor()
{
	m_cursorRectItem->setPos(m_mainGridItemGroup->pos().x() + static_cast<TeletextWidget *>(m_levelOneProxyWidget->widget())->document()->cursorColumn()*12, m_mainGridItemGroup->pos().y() + static_cast<TeletextWidget *>(m_levelOneProxyWidget->widget())->document()->cursorRow()*10);
}

void LevelOneScene::updateSelection()
{
	if (!static_cast<TeletextWidget *>(m_levelOneProxyWidget->widget())->document()->selectionActive()) {
		m_selectionRectItem->setVisible(false);
		return;
	}

	m_selectionRectItem->setRect(m_mainGridItemGroup->pos().x() + static_cast<TeletextWidget *>(m_levelOneProxyWidget->widget())->document()->selectionLeftColumn()*12, m_mainGridItemGroup->pos().y() + static_cast<TeletextWidget *>(m_levelOneProxyWidget->widget())->document()->selectionTopRow()*10, static_cast<TeletextWidget *>(m_levelOneProxyWidget->widget())->document()->selectionWidth()*12-1, static_cast<TeletextWidget *>(m_levelOneProxyWidget->widget())->document()->selectionHeight()*10-1);

	m_selectionRectItem->setVisible(true);
}

void LevelOneScene::toggleGrid(bool gridOn)
{
	m_grid = gridOn;
	m_mainGridItemGroup->setVisible(gridOn);
	for (int i=0; i<32; i++)
		if (m_sidePanelGridNeeded[i])
			m_sidePanelGridItemGroup[i]->setVisible(gridOn);
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
