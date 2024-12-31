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

#include <QApplication>
#include <QBitmap>
#include <QClipboard>
#include <QFrame>
#include <QGraphicsItem>
#include <QGraphicsItemGroup>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneEvent>
#include <QImage>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QPair>
#include <QSet>
#include <QUndoCommand>
#include <QWidget>
#include <vector>
#include <iostream>

#include "mainwidget.h"

#include "decode.h"
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
	m_pageDecode.setTeletextPage(m_levelOnePage);
	m_pageRender.setDecoder(&m_pageDecode);
	m_insertMode = false;
	m_selectionInProgress = false;
	setFocusPolicy(Qt::StrongFocus);
	m_flashTiming = m_flashPhase = 0;
	connect(&m_pageRender, &TeletextPageRender::flashChanged, this, &TeletextWidget::updateFlashTimer);
	connect(&m_pageDecode, &TeletextPageDecode::sidePanelsChanged, this, &TeletextWidget::changeSize);
	connect(m_teletextDocument, &TeletextDocument::subPageSelected, this, &TeletextWidget::subPageSelected);
	connect(m_teletextDocument, &TeletextDocument::contentsChanged, this, &TeletextWidget::refreshPage);
	connect(m_teletextDocument, &TeletextDocument::colourChanged, &m_pageRender, &TeletextPageRender::colourChanged);
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
	m_pageDecode.setTeletextPage(m_levelOnePage);
	m_pageDecode.decodePage();
	m_pageRender.renderPage(true);
	update();
}

void TeletextWidget::refreshPage()
{
	m_pageDecode.decodePage();
	update();
}

void TeletextWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);
	QPainter widgetPainter(this);

	m_pageRender.renderPage();
	widgetPainter.drawImage(m_pageDecode.leftSidePanelColumns()*12, 0, *m_pageRender.image(m_flashPhase), 0, 0, 480, 250);
	if (m_pageDecode.leftSidePanelColumns())
		widgetPainter.drawImage(0, 0, *m_pageRender.image(m_flashPhase), 864-m_pageDecode.leftSidePanelColumns()*12, 0, m_pageDecode.leftSidePanelColumns()*12, 250);
	if (m_pageDecode.rightSidePanelColumns())
		widgetPainter.drawImage(480+m_pageDecode.leftSidePanelColumns()*12, 0, *m_pageRender.image(m_flashPhase), 480, 0, m_pageDecode.rightSidePanelColumns()*12, 250);
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

void TeletextWidget::pauseFlash(bool pauseNow)
{
	if (pauseNow && m_flashTiming != 0) {
		m_flashTimer.stop();
		m_flashPhase = 0;
		update();
	} else if (m_flashTiming != 0)
		m_flashTimer.start((m_flashTiming == 1) ? 500 : 167, this);
}

void TeletextWidget::setInsertMode(bool insertMode)
{
	m_insertMode = insertMode;
}

void TeletextWidget::setReveal(bool reveal)
{
	m_pageRender.setReveal(reveal);
	update();
}

void TeletextWidget::setShowControlCodes(bool showControlCodes)
{
	m_pageRender.setShowControlCodes(showControlCodes);
	update();
}

void TeletextWidget::setRenderMode(TeletextPageRender::RenderMode renderMode)
{
	m_pageRender.setRenderMode(renderMode);
	update();
}

void TeletextWidget::setControlBit(int bitNumber, bool active)
{
	m_levelOnePage->setControlBit(bitNumber, active);
	if (bitNumber == 1 || bitNumber == 2) {
		m_pageDecode.decodePage();
		m_pageRender.renderPage(true);
		update();
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

void TeletextWidget::setSidePanelWidths(int newLeftSidePanelColumns, int newRightSidePanelColumns)
{
	m_levelOnePage->setLeftSidePanelDisplayed(newLeftSidePanelColumns != 0);
	m_levelOnePage->setRightSidePanelDisplayed(newRightSidePanelColumns != 0);
	if (newLeftSidePanelColumns)
		m_levelOnePage->setSidePanelColumns((newLeftSidePanelColumns == 16) ? 0 : newLeftSidePanelColumns);
	else
		m_levelOnePage->setSidePanelColumns((newRightSidePanelColumns == 0) ? 0 : 16-newRightSidePanelColumns);
	m_pageDecode.updateSidePanels();
}

void TeletextWidget::setSidePanelAtL35Only(bool newSidePanelAtL35Only)
{
	m_levelOnePage->setSidePanelStatusL25(!newSidePanelAtL35Only);
	m_pageDecode.updateSidePanels();
}

void TeletextWidget::changeSize()
{
	setFixedSize(QSize(480+(pageDecode()->leftSidePanelColumns()+pageDecode()->rightSidePanelColumns())*12, 250));
	emit sizeChanged();
}

void TeletextWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() < 0x01000000) {
		// A character-typing key was pressed
		// Try to keymap it, if not keymapped then plain ASCII code (may be) returned
		char mappedKeyPress = keymapping[m_pageDecode.level1CharSet(m_teletextDocument->cursorRow(), m_teletextDocument->cursorColumn())].value(event->text().at(0), *qPrintable(event->text().at(0)));
		if (mappedKeyPress >= 0x00 && mappedKeyPress <= 0x1f)
			return;
		// If outside ASCII map then the character can't be represented by current Level 1 character set
		// Map it to block character so it doesn't need to be inserted-between later on
		if (mappedKeyPress & 0x80)
			mappedKeyPress = 0x7f;
		if ((m_pageDecode.level1MosaicAttr(m_teletextDocument->cursorRow(), m_teletextDocument->cursorColumn()) || m_teletextDocument->selectionActive()) && (mappedKeyPress < 0x40 || mappedKeyPress > 0x5f)) {
			// A blast-through character was NOT pressed
			// and we're either on a mosaic or a selection is active
			if (event->key() >= Qt::Key_0 && event->key() <= Qt::Key_9 && event->modifiers() & Qt::KeypadModifier) {
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
					case Qt::Key_0:
						toggleCharacterBit(0x66); // Dither
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
					case mosaicNativeScanCodes[9]:
						toggleCharacterBit(0x66); // Dither
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
			if (event->modifiers() & Qt::ControlModifier)
				shiftMosaics(event->key());
			else
				m_teletextDocument->cursorUp(event->modifiers() & Qt::ShiftModifier);
			break;
		case Qt::Key_Down:
			if (event->modifiers() & Qt::ControlModifier)
				shiftMosaics(event->key());
			else
				m_teletextDocument->cursorDown(event->modifiers() & Qt::ShiftModifier);
			break;
		case Qt::Key_Left:
			if (event->modifiers() & Qt::ControlModifier)
				shiftMosaics(event->key());
			else
				m_teletextDocument->cursorLeft(event->modifiers() & Qt::ShiftModifier);
			break;
		case Qt::Key_Right:
			if (event->modifiers() & Qt::ControlModifier)
				shiftMosaics(event->key());
			else
				m_teletextDocument->cursorRight(event->modifiers() & Qt::ShiftModifier);
			break;
		case Qt::Key_Return:
		case Qt::Key_Enter:
			m_teletextDocument->cursorDown();
			m_teletextDocument->moveCursor(m_teletextDocument->cursorRow(), 0);
			break;
		case Qt::Key_Home:
			m_teletextDocument->moveCursor(m_teletextDocument->cursorRow(), 0, event->modifiers() & Qt::ShiftModifier);
			break;
		case Qt::Key_End:
			m_teletextDocument->moveCursor(m_teletextDocument->cursorRow(), 39, event->modifiers() & Qt::ShiftModifier);
			break;

		case Qt::Key_PageUp:
			m_teletextDocument->selectSubPageNext();
			break;
		case Qt::Key_PageDown:
			m_teletextDocument->selectSubPagePrevious();
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
	if (!m_teletextDocument->selectionActive())
		m_teletextDocument->undoStack()->push(new ToggleMosaicBitCommand(m_teletextDocument, bitToToggle));
	else {
		const QSet<QPair<int, int>> mosaicList = findMosaics();

		if (!mosaicList.isEmpty())
			switch (bitToToggle) {
				case 0x7f:
					m_teletextDocument->undoStack()->push(new FillMosaicsCommand(m_teletextDocument, mosaicList));
					break;
				case 0x20:
					m_teletextDocument->undoStack()->push(new ClearMosaicsCommand(m_teletextDocument, mosaicList));
					break;
				case 0x5f:
					m_teletextDocument->undoStack()->push(new InvertMosaicsCommand(m_teletextDocument, mosaicList));
					break;
				case 0x66:
					m_teletextDocument->undoStack()->push(new DitherMosaicsCommand(m_teletextDocument, mosaicList));
					break;
			}
	}
}

QSet<QPair<int, int>> TeletextWidget::findMosaics()
{
	QSet<QPair<int, int>> result;

	if (!m_teletextDocument->selectionActive())
		return result;

	for (int r=m_teletextDocument->selectionTopRow(); r<=m_teletextDocument->selectionBottomRow(); r++)
		for (int c=m_teletextDocument->selectionLeftColumn(); c<=m_teletextDocument->selectionRightColumn(); c++)
			if (m_pageDecode.level1MosaicChar(r, c))
				result.insert(qMakePair(r, c));

	return result;
}

void TeletextWidget::shiftMosaics(int key)
{
	const QSet<QPair<int, int>> mosaicList = findMosaics();

	if (!mosaicList.isEmpty())
		switch (key) {
			case Qt::Key_Up:
				m_teletextDocument->undoStack()->push(new ShiftMosaicsUpCommand(m_teletextDocument, mosaicList));
				break;
			case Qt::Key_Down:
				m_teletextDocument->undoStack()->push(new ShiftMosaicsDownCommand(m_teletextDocument, mosaicList));
				break;
			case Qt::Key_Left:
				m_teletextDocument->undoStack()->push(new ShiftMosaicsLeftCommand(m_teletextDocument, mosaicList));
				break;
			case Qt::Key_Right:
				m_teletextDocument->undoStack()->push(new ShiftMosaicsRightCommand(m_teletextDocument, mosaicList));
				break;
		}
}

void TeletextWidget::selectionToClipboard()
{
	QByteArray nativeData;
	QString plainTextData;
	QImage *imageData = nullptr;
	QClipboard *clipboard = QApplication::clipboard();

	nativeData.resize(2 + m_teletextDocument->selectionWidth() * m_teletextDocument->selectionHeight());
	nativeData[0] = m_teletextDocument->selectionHeight();
	nativeData[1] = m_teletextDocument->selectionWidth();

	plainTextData.reserve((m_teletextDocument->selectionWidth()+1) * m_teletextDocument->selectionHeight() - 1);

	int i = 2;

	for (int r=m_teletextDocument->selectionTopRow(); r<=m_teletextDocument->selectionBottomRow(); r++) {
		for (int c=m_teletextDocument->selectionLeftColumn(); c<=m_teletextDocument->selectionRightColumn(); c++) {
			nativeData[i++] = m_teletextDocument->currentSubPage()->character(r, c);

			if (m_teletextDocument->currentSubPage()->character(r, c) >= 0x20)
				plainTextData.append(keymapping[m_pageDecode.level1CharSet(r, c)].key(m_teletextDocument->currentSubPage()->character(r, c), QChar(m_teletextDocument->currentSubPage()->character(r, c))));
			else
				plainTextData.append(' ');

			if (m_pageDecode.level1MosaicChar(r, c)) {
				// A first mosaic character was found so create the image "just in time"
				if (imageData == nullptr) {
					imageData = new QImage(m_teletextDocument->selectionWidth() * 2, m_teletextDocument->selectionHeight() * 3, QImage::Format_Mono);

					imageData->fill(0);
				}

				const int ix = (c - m_teletextDocument->selectionLeftColumn()) * 2;
				const int iy = (r - m_teletextDocument->selectionTopRow()) * 3;

				if (m_teletextDocument->currentSubPage()->character(r, c) & 0x01)
					imageData->setPixel(ix, iy, 1);
				if (m_teletextDocument->currentSubPage()->character(r, c) & 0x02)
					imageData->setPixel(ix+1, iy, 1);
				if (m_teletextDocument->currentSubPage()->character(r, c) & 0x04)
					imageData->setPixel(ix, iy+1, 1);
				if (m_teletextDocument->currentSubPage()->character(r, c) & 0x08)
					imageData->setPixel(ix+1, iy+1, 1);
				if (m_teletextDocument->currentSubPage()->character(r, c) & 0x10)
					imageData->setPixel(ix, iy+2, 1);
				if (m_teletextDocument->currentSubPage()->character(r, c) & 0x40)
					imageData->setPixel(ix+1, iy+2, 1);
			}
		}

		plainTextData.append('\n');
	}

	QMimeData *mimeData = new QMimeData();
	mimeData->setData("application/x-teletext", nativeData);
	mimeData->setText(plainTextData);
	if (imageData != nullptr) {
		mimeData->setImageData(*imageData);
		delete imageData;
	}

	clipboard->setMimeData(mimeData);
}

void TeletextWidget::cut()
{
	if (!m_teletextDocument->selectionActive())
		return;

	selectionToClipboard();
	m_teletextDocument->undoStack()->push(new CutCommand(m_teletextDocument));
}

void TeletextWidget::copy()
{
	if (!m_teletextDocument->selectionActive())
		return;

	selectionToClipboard();
}

void TeletextWidget::paste()
{
	m_teletextDocument->undoStack()->push(new PasteCommand(m_teletextDocument, m_pageDecode.level1CharSet(m_teletextDocument->cursorRow(), m_teletextDocument->cursorColumn())));
}

QPair<int, int> TeletextWidget::mouseToRowAndColumn(const QPoint &mousePosition)
{
	int row = mousePosition.y() / 10;
	int column = mousePosition.x() / 12 - m_pageDecode.leftSidePanelColumns();
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
		if (position.first != m_teletextDocument->cursorRow() || position.second != m_teletextDocument->cursorColumn()) {
			if (!m_selectionInProgress) {
				m_selectionInProgress = true;
				m_teletextDocument->setSelectionCorner(m_teletextDocument->cursorRow(), m_teletextDocument->cursorColumn());
			}
			m_teletextDocument->moveCursor(position.first, position.second, true);
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
	m_levelOneProxyWidget->setFocus();

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

	installEventFilter(this);
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

void LevelOneScene::setRenderMode(TeletextPageRender::RenderMode renderMode)
{
	static_cast<TeletextWidget *>(m_levelOneProxyWidget->widget())->setRenderMode(renderMode);

	QColor fullColour;

	switch (renderMode) {
		case TeletextPageRender::RenderNormal:
			setFullScreenColour(static_cast<TeletextWidget *>(m_levelOneProxyWidget->widget())->pageDecode()->fullScreenQColor());
			for (int r=0; r<25; r++)
				setFullRowColour(r, static_cast<TeletextWidget *>(m_levelOneProxyWidget->widget())->pageDecode()->fullRowQColor(r));
			return;
		case TeletextPageRender::RenderMix:
			fullColour = Qt::transparent;
			break;
		case TeletextPageRender::RenderWhiteOnBlack:
			fullColour = Qt::black;
			break;
		case TeletextPageRender::RenderBlackOnWhite:
			fullColour = Qt::white;
			break;
	}

	m_fullScreenTopRectItem->setBrush(fullColour);
	m_fullScreenBottomRectItem->setBrush(fullColour);
	for (int r=0; r<25; r++) {
		m_fullRowLeftRectItem[r]->setBrush(fullColour);
		m_fullRowRightRectItem[r]->setBrush(fullColour);
	}
}

void LevelOneScene::toggleGrid(bool gridOn)
{
	m_grid = gridOn;
	m_mainGridItemGroup->setVisible(gridOn);
	for (int i=0; i<32; i++)
		if (m_sidePanelGridNeeded[i])
			m_sidePanelGridItemGroup[i]->setVisible(gridOn);
}

void LevelOneScene::hideGUIElements(bool hidden)
{
	if (hidden) {
		m_cursorRectItem->setVisible(false);
		m_selectionRectItem->setVisible(false);
	} else {
		if (static_cast<TeletextWidget *>(m_levelOneProxyWidget->widget())->document()->selectionActive())
			m_selectionRectItem->setVisible(true);

		m_cursorRectItem->setVisible(true);
	}
}

// Implements Ctrl+mousewheel zoom
bool LevelOneScene::eventFilter(QObject *object, QEvent *event)
{
	Q_UNUSED(object);

	if (event->type() == QEvent::GraphicsSceneWheel && static_cast<QGraphicsSceneWheelEvent *>(event)->modifiers() == Qt::ControlModifier) {
		if (static_cast<QGraphicsSceneWheelEvent *>(event)->delta() > 0)
			emit mouseZoomIn();
		else if (static_cast<QGraphicsSceneWheelEvent *>(event)->delta() < 0)
			emit mouseZoomOut();

		event->accept();
		return true;
	}
	return false;
}

// Clicking outside the main text widget but still within the scene would
// cause keyboard focus loss.
// So on every keypress within the scene, wrench the focus back to the widget
// if necessary.
void LevelOneScene::keyPressEvent(QKeyEvent *keyEvent)
{
	if (focusItem() != m_levelOneProxyWidget)
		setFocusItem(m_levelOneProxyWidget);

	QGraphicsScene::keyPressEvent(keyEvent);
}

void LevelOneScene::keyReleaseEvent(QKeyEvent *keyEvent)
{
	if (focusItem() != m_levelOneProxyWidget)
		setFocusItem(m_levelOneProxyWidget);

	QGraphicsScene::keyReleaseEvent(keyEvent);
}

void LevelOneScene::setFullScreenColour(const QColor &newColor)
{
	if (static_cast<TeletextWidget *>(m_levelOneProxyWidget->widget())->pageRender()->renderMode() == TeletextPageRender::RenderNormal) {
		m_fullScreenTopRectItem->setBrush(newColor);
		m_fullScreenBottomRectItem->setBrush(newColor);
	}
}

void LevelOneScene::setFullRowColour(int row, const QColor &newColor)
{
	if (static_cast<TeletextWidget *>(m_levelOneProxyWidget->widget())->pageRender()->renderMode() == TeletextPageRender::RenderNormal) {
		m_fullRowLeftRectItem[row]->setBrush(newColor);
		m_fullRowRightRectItem[row]->setBrush(newColor);
	}
}
