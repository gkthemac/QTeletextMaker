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

#include <QApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QList>
#include <QMenuBar>
#include <QMessageBox>
#include <QRadioButton>
#include <QScreen>
#include <QSettings>
#include <QStatusBar>
#include <QToolBar>
#include <iostream>

#include "mainwindow.h"

#include "mainwidget.h"
#include "pageenhancementsdockwidget.h"
#include "pageoptionsdockwidget.h"
#include "palettedockwidget.h"
#include "x26dockwidget.h"

MainWindow::MainWindow()
{
	init();
	setCurrentFile(QString());
	m_textWidget->refreshPage();
}

MainWindow::MainWindow(const QString &fileName)
{
	init();
	loadFile(fileName);
	m_textWidget->refreshPage();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (maybeSave()) {
		writeSettings();
		event->accept();
	} else
		event->ignore();
}

void MainWindow::newFile()
{
	MainWindow *other = new MainWindow;
	other->tile(this);
	other->show();
}

void MainWindow::open()
{
	const QString fileName = QFileDialog::getOpenFileName(this);
	if (!fileName.isEmpty())
		openFile(fileName);
}

void MainWindow::openFile(const QString &fileName)
{
	MainWindow *existing = findMainWindow(fileName);
	if (existing) {
		existing->show();
		existing->raise();
		existing->activateWindow();
		return;
	}

	if (m_isUntitled && m_textWidget->document()->isEmpty() && !isWindowModified()) {
		loadFile(fileName);
		m_textWidget->refreshPage();
		return;
	}

	MainWindow *other = new MainWindow(fileName);
	if (other->m_isUntitled) {
		delete other;
		return;
	}
	other->tile(this);
	other->show();
}

bool MainWindow::save()
{
	return m_isUntitled ? saveAs() : saveFile(m_curFile);
}

bool MainWindow::saveAs()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), m_curFile);
	if (fileName.isEmpty())
		return false;

	return saveFile(fileName);
}

void MainWindow::exportURL()
{
	QDesktopServices::openUrl(QUrl(m_textWidget->document()->currentSubPage()->exportURLHash("http://www.zxnet.co.uk/teletext/editor/")));
}

void MainWindow::about()
{
	QMessageBox::about(this, tr("About QTeletextMaker"), tr("<b>QTeletextMaker</b><br><i>Pre-alpha version</i>.<br><br>Copyright (C) 2020 Gavin MacGregor<br><br>Released under the GNU General Public License version 3"));
}

void MainWindow::init()
{
	setAttribute(Qt::WA_DeleteOnClose);

	m_isUntitled = true;

	m_textWidget = new TeletextWidget;

	m_pageOptionsDockWidget = new PageOptionsDockWidget(m_textWidget);
	addDockWidget(Qt::RightDockWidgetArea, m_pageOptionsDockWidget);
	m_pageEnhancementsDockWidget = new PageEnhancementsDockWidget(m_textWidget);
	addDockWidget(Qt::RightDockWidgetArea, m_pageEnhancementsDockWidget);
	m_x26DockWidget = new X26DockWidget(m_textWidget);
	addDockWidget(Qt::RightDockWidgetArea, m_x26DockWidget);
	m_paletteDockWidget = new PaletteDockWidget(m_textWidget);
	addDockWidget(Qt::RightDockWidgetArea, m_paletteDockWidget);

	createActions();
	createStatusBar();

	readSettings();

	m_textScene = new QGraphicsScene(this);
	m_textScene->setSceneRect(0, 0, 600, 288);
	m_fullScreenTopRectItem = new QGraphicsRectItem(0, 0, 600, 19);
	m_fullScreenTopRectItem->setPen(Qt::NoPen);
	m_fullScreenTopRectItem->setBrush(QBrush(QColor(0, 0, 0)));
	m_textScene->addItem(m_fullScreenTopRectItem);
	m_fullScreenBottomRectItem = new QGraphicsRectItem(0, 269, 600, 19);
	m_fullScreenBottomRectItem->setPen(Qt::NoPen);
	m_fullScreenBottomRectItem->setBrush(QBrush(QColor(0, 0, 0)));
	m_textScene->addItem(m_fullScreenBottomRectItem);

	for (int r=0; r<25; r++) {
		m_fullRowLeftRectItem[r] = new QGraphicsRectItem(0, 19+r*10, 60, 10);
		m_fullRowLeftRectItem[r]->setPen(Qt::NoPen);
		m_fullRowLeftRectItem[r]->setBrush(QBrush(QColor(0, 0, 0)));
		m_textScene->addItem(m_fullRowLeftRectItem[r]);
		m_fullRowRightRectItem[r] = new QGraphicsRectItem(540, 19+r*10, 60, 10);
		m_fullRowRightRectItem[r]->setPen(Qt::NoPen);
		m_fullRowRightRectItem[r]->setBrush(QBrush(QColor(0, 0, 0)));
		m_textScene->addItem(m_fullRowRightRectItem[r]);
	}

	m_textProxyWidget = m_textScene->addWidget(m_textWidget);
	m_textProxyWidget->setPos(60, 19);
	m_textProxyWidget->setAutoFillBackground(false);

	m_textView = new QGraphicsView(this);
	m_textView->setScene(m_textScene);
	m_textView->setRenderHints(QPainter::SmoothPixmapTransform);
	m_textView->setBackgroundBrush(QBrush(QColor(32, 48, 96)));
	setSceneDimensions();
	setCentralWidget(m_textView);

	connect(m_textWidget->document(), &TeletextDocument::cursorMoved, this, &MainWindow::updateCursorPosition);
	connect(m_textWidget->document()->undoStack(), &QUndoStack::cleanChanged, this, [=]() { setWindowModified(!m_textWidget->document()->undoStack()->isClean()); } );
	connect(m_textWidget->document(), &TeletextDocument::aboutToChangeSubPage, m_x26DockWidget, &X26DockWidget::unloadX26List);
	connect(m_textWidget->document(), &TeletextDocument::subPageSelected, this, &MainWindow::updatePageWidgets);
	connect(m_textWidget, &TeletextWidget::sizeChanged, this, &MainWindow::setSceneDimensions);
	connect(m_textWidget->pageRender(), &TeletextPageRender::fullScreenColourChanged, this, &MainWindow::updateFullScreenRectItems);
	connect(m_textWidget->pageRender(), &TeletextPageRender::fullRowColourChanged, this, &MainWindow::updateFullRowRectItems);

	setUnifiedTitleAndToolBarOnMac(true);

	updatePageWidgets();
}

void MainWindow::updateFullScreenRectItems(QColor newColor)
{
	m_fullScreenTopRectItem->setBrush(QBrush(newColor));
	m_fullScreenBottomRectItem->setBrush(QBrush(newColor));
}

void MainWindow::updateFullRowRectItems(int row, QColor newColor)
{
	m_fullRowLeftRectItem[row]->setBrush(QBrush(newColor));
	m_fullRowRightRectItem[row]->setBrush(QBrush(newColor));
}

void MainWindow::tile(const QMainWindow *previous)
{
	//TODO sort out default tiling or positioning
	if (!previous)
		return;
	int topFrameWidth = previous->geometry().top() - previous->pos().y();
	if (!topFrameWidth)
		topFrameWidth = 40;
	const QPoint pos = previous->pos() + 2 * QPoint(topFrameWidth, topFrameWidth);
	if (QGuiApplication::primaryScreen()->availableGeometry().contains(rect().bottomRight() + pos))
		move(pos);
}

void MainWindow::createActions()
{
	QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
	QToolBar *fileToolBar = addToolBar(tr("File"));
	fileToolBar->setObjectName("fileToolBar");

	const QIcon newIcon = QIcon::fromTheme("document-new", QIcon(":/images/new.png"));
	QAction *newAct = new QAction(newIcon, tr("&New"), this);
	newAct->setShortcuts(QKeySequence::New);
	newAct->setStatusTip(tr("Create a new file"));
	connect(newAct, &QAction::triggered, this, &MainWindow::newFile);
	fileMenu->addAction(newAct);
	fileToolBar->addAction(newAct);

	const QIcon openIcon = QIcon::fromTheme("document-open", QIcon(":/images/open.png"));
	QAction *openAct = new QAction(openIcon, tr("&Open..."), this);
	openAct->setShortcuts(QKeySequence::Open);
	openAct->setStatusTip(tr("Open an existing file"));
	connect(openAct, &QAction::triggered, this, &MainWindow::open);
	fileMenu->addAction(openAct);
	fileToolBar->addAction(openAct);

	const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon(":/images/save.png"));
	QAction *saveAct = new QAction(saveIcon, tr("&Save"), this);
	saveAct->setShortcuts(QKeySequence::Save);
	saveAct->setStatusTip(tr("Save the document to disk"));
	connect(saveAct, &QAction::triggered, this, &MainWindow::save);
	fileMenu->addAction(saveAct);
	fileToolBar->addAction(saveAct);

	const QIcon saveAsIcon = QIcon::fromTheme("document-save-as");
	QAction *saveAsAct = fileMenu->addAction(saveAsIcon, tr("Save &As..."), this, &MainWindow::saveAs);
	saveAsAct->setShortcuts(QKeySequence::SaveAs);
	saveAsAct->setStatusTip(tr("Save the document under a new name"));

	fileMenu->addSeparator();

	QMenu *recentMenu = fileMenu->addMenu(tr("Recent"));
	connect(recentMenu, &QMenu::aboutToShow, this, &MainWindow::updateRecentFileActions);
	m_recentFileSubMenuAct = recentMenu->menuAction();

	for (int i = 0; i < m_MaxRecentFiles; ++i) {
		m_recentFileActs[i] = recentMenu->addAction(QString(), this, &MainWindow::openRecentFile);
		m_recentFileActs[i]->setVisible(false);
	}

	m_recentFileSeparator = fileMenu->addSeparator();

	setRecentFilesVisible(MainWindow::hasRecentFiles());

	QAction *exportAct = fileMenu->addAction(tr("Export to zxnet.co.uk"));
	exportAct->setStatusTip("Export URL to zxnet.co.uk online editor");
	connect(exportAct, &QAction::triggered, this, &MainWindow::exportURL);

	QAction *closeAct = fileMenu->addAction(tr("&Close"), this, &QWidget::close);
	closeAct->setShortcut(tr("Ctrl+W"));
	closeAct->setStatusTip(tr("Close this window"));

	const QIcon exitIcon = QIcon::fromTheme("application-exit");
	QAction *exitAct = fileMenu->addAction(exitIcon, tr("E&xit"), qApp, &QApplication::closeAllWindows);
	exitAct->setShortcuts(QKeySequence::Quit);
	exitAct->setStatusTip(tr("Exit the application"));

	QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
	QToolBar *editToolBar = addToolBar(tr("Edit"));
	editToolBar->setObjectName("editToolBar");

	QAction *undoAction = m_textWidget->document()->undoStack()->createUndoAction(this, tr("&Undo"));
	editMenu->addAction(undoAction);
	undoAction->setShortcuts(QKeySequence::Undo);
	QAction *redoAction = m_textWidget->document()->undoStack()->createRedoAction(this, tr("&Redo"));
	editMenu->addAction(redoAction);
	redoAction->setShortcuts(QKeySequence::Redo);

	editMenu->addSeparator();
#ifndef QT_NO_CLIPBOARD
/*	const QIcon cutIcon = QIcon::fromTheme("edit-cut", QIcon(":/images/cut.png"));
	QAction *cutAct = new QAction(cutIcon, tr("Cu&t"), this);
	cutAct->setShortcuts(QKeySequence::Cut);
	cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));
	connect(cutAct, &QAction::triggered, textWidget, &QTextEdit::cut);
	editMenu->addAction(cutAct);
	editToolBar->addAction(cutAct);

	const QIcon copyIcon = QIcon::fromTheme("edit-copy", QIcon(":/images/copy.png"));
	QAction *copyAct = new QAction(copyIcon, tr("&Copy"), this);
	copyAct->setShortcuts(QKeySequence::Copy);
	copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
	connect(copyAct, &QAction::triggered, textWidget, &QTextEdit::copy);
	editMenu->addAction(copyAct);
	editToolBar->addAction(copyAct);

	const QIcon pasteIcon = QIcon::fromTheme("edit-paste", QIcon(":/images/paste.png"));
	QAction *pasteAct = new QAction(pasteIcon, tr("&Paste"), this);
	pasteAct->setShortcuts(QKeySequence::Paste);
	pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
	connect(pasteAct, &QAction::triggered, textWidget, &QTextEdit::paste);
	editMenu->addAction(pasteAct);
	editToolBar->addAction(pasteAct);

	editMenu->addSeparator();
*/
#endif // !QT_NO_CLIPBOARD

	QAction *insertBeforeAct = editMenu->addAction(tr("&Insert subpage before"));
	insertBeforeAct->setStatusTip(tr("Insert a blank subpage before this subpage"));
	connect(insertBeforeAct, &QAction::triggered, [=]() { insertSubPage(false, false); });

	QAction *insertAfterAct = editMenu->addAction(tr("Insert subpage after"));
	insertAfterAct->setStatusTip(tr("Insert a blank subpage after this subpage"));
	connect(insertAfterAct, &QAction::triggered, [=]() { insertSubPage(true, false); });

	QAction *insertCopyAct = editMenu->addAction(tr("Insert subpage copy"));
	insertCopyAct->setStatusTip(tr("Insert a subpage that's a copy of this subpage"));
	connect(insertCopyAct, &QAction::triggered, [=]() { insertSubPage(false, true); });

	QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

	QAction *revealAct = viewMenu->addAction(tr("&Reveal"));
	revealAct->setCheckable(true);
	revealAct->setStatusTip(tr("Toggle reveal"));
	connect(revealAct, &QAction::toggled, m_textWidget, &TeletextWidget::toggleReveal);

	QAction *mixAct = viewMenu->addAction(tr("&Mix"));
	mixAct->setCheckable(true);
	mixAct->setStatusTip(tr("Toggle mix"));
	connect(mixAct, &QAction::toggled, m_textWidget, &TeletextWidget::toggleMix);

	QAction *gridAct = viewMenu->addAction(tr("&Grid"));
	gridAct->setCheckable(true);
	gridAct->setStatusTip(tr("Toggle the text grid"));
	connect(gridAct, &QAction::toggled, m_textWidget, &TeletextWidget::toggleGrid);

	QAction *showCodesAct = viewMenu->addAction(tr("Show codes"));
	showCodesAct->setCheckable(true);
	showCodesAct->setStatusTip(tr("Toggle showing of control codes"));
	connect(showCodesAct, &QAction::toggled, m_textWidget->pageRender(), &TeletextPageRender::setShowCodes);

	viewMenu->addSeparator();

	QMenu *borderSubMenu = viewMenu->addMenu(tr("Border"));
	m_borderActs[0] = borderSubMenu->addAction(tr("None"));
	m_borderActs[0]->setStatusTip(tr("View with no border"));
	m_borderActs[1] = borderSubMenu->addAction(tr("Minimal"));
	m_borderActs[1]->setStatusTip(tr("View with minimal border"));
	m_borderActs[2] = borderSubMenu->addAction(tr("Full"));
	m_borderActs[2]->setStatusTip(tr("View with full overscan border"));

	QMenu *aspectRatioSubMenu = viewMenu->addMenu(tr("Aspect ratio"));
	m_aspectRatioActs[0] = aspectRatioSubMenu->addAction(tr("4:3"));
	m_aspectRatioActs[0]->setStatusTip(tr("View in 4:3 aspect ratio"));
	m_aspectRatioActs[1] = aspectRatioSubMenu->addAction(tr("16:9 pillarbox"));
	m_aspectRatioActs[1]->setStatusTip(tr("View in 16:9 with space on either side"));
	m_aspectRatioActs[2] = aspectRatioSubMenu->addAction(tr("16:9 stretch"));
	m_aspectRatioActs[2]->setStatusTip(tr("View in 16:9 with horizontally stretched text"));
	m_aspectRatioActs[3] = aspectRatioSubMenu->addAction(tr("Pixel 1:2"));
	m_aspectRatioActs[3]->setStatusTip(tr("View with 1:2 pixel mapping"));

	QActionGroup *borderGroup = new QActionGroup(this);
	QActionGroup *aspectRatioGroup = new QActionGroup(this);
	for (int i=0; i<=3; i++) {
		m_aspectRatioActs[i]->setCheckable(true);
		connect(m_aspectRatioActs[i], &QAction::triggered, [=]() { setAspectRatio(i); });
		aspectRatioGroup->addAction(m_aspectRatioActs[i]);
		if (i == 3)
			break;
		m_borderActs[i]->setCheckable(true);
		connect(m_borderActs[i], &QAction::triggered, [=]() { setBorder(i); });
		borderGroup->addAction(m_borderActs[i]);
	}

	QAction *zoomInAct = viewMenu->addAction(tr("Zoom In"));
	zoomInAct->setShortcuts(QKeySequence::ZoomIn);
	zoomInAct->setStatusTip(tr("Zoom in"));
	connect(zoomInAct, &QAction::triggered, this, &MainWindow::zoomIn);

	QAction *zoomOutAct = viewMenu->addAction(tr("Zoom Out"));
	zoomOutAct->setShortcuts(QKeySequence::ZoomOut);
	zoomOutAct->setStatusTip(tr("Zoom out"));
	connect(zoomOutAct, &QAction::triggered, this, &MainWindow::zoomOut);

	QAction *zoomResetAct = viewMenu->addAction(tr("Reset zoom"));
	zoomResetAct->setStatusTip(tr("Reset zoom level"));
	connect(zoomResetAct, &QAction::triggered, this, &MainWindow::zoomReset);

	QMenu *insertMenu = menuBar()->addMenu(tr("&Insert"));

	QMenu *alphaColourSubMenu = insertMenu->addMenu(tr("Alphanumeric colour"));
	QMenu *mosaicColourSubMenu = insertMenu->addMenu(tr("Mosaic colour"));
	for (int i=0; i<=7; i++) {
		const char *colours[] = { "Black", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White" };

		QAction *alphaColour = alphaColourSubMenu->addAction(tr(colours[i]));
		alphaColour->setStatusTip(QString("Insert alphanumeric %1 attribute").arg(QString(colours[i]).toLower()));
		connect(alphaColour, &QAction::triggered, [=]() { m_textWidget->setCharacter(i); });

		QAction *mosaicColour = mosaicColourSubMenu->addAction(tr(colours[i]));
		mosaicColour->setStatusTip(QString("Insert mosaic %1 attribute").arg(QString(colours[i]).toLower()));
		connect(mosaicColour, &QAction::triggered, [=]() { m_textWidget->setCharacter(i+0x10); });
	}

	QMenu *mosaicsStyleSubMenu = insertMenu->addMenu(tr("Mosaics style"));
	QAction *mosaicsContiguousAct = mosaicsStyleSubMenu->addAction(tr("Contiguous mosaics"));
	mosaicsContiguousAct->setStatusTip(tr("Insert contiguous mosaics attribute"));
	connect(mosaicsContiguousAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x19); });
	QAction *mosaicsSeparatedAct = mosaicsStyleSubMenu->addAction(tr("Separated mosaics"));
	mosaicsSeparatedAct->setStatusTip(tr("Insert separated mosaics attribute"));
	connect(mosaicsSeparatedAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x1a); });

	QMenu *mosaicsHoldSubMenu = insertMenu->addMenu(tr("Mosaics hold"));
	QAction *mosaicsHoldAct = mosaicsHoldSubMenu->addAction(tr("Hold mosaics"));
	mosaicsHoldAct->setStatusTip(tr("Insert hold mosaics attribute"));
	connect(mosaicsHoldAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x1e); });
	QAction *mosaicsReleaseAct = mosaicsHoldSubMenu->addAction(tr("Release mosaics"));
	mosaicsReleaseAct->setStatusTip(tr("Insert release mosaics attribute"));
	connect(mosaicsReleaseAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x1f); });

	QMenu *backgroundColourSubMenu = insertMenu->addMenu(tr("Background colour"));
	QAction *backgroundNewAct = backgroundColourSubMenu->addAction(tr("New background"));
	backgroundNewAct->setStatusTip(tr("Insert new background attribute"));
	connect(backgroundNewAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x1d); });
	QAction *backgroundBlackAct = backgroundColourSubMenu->addAction(tr("Black background"));
	backgroundBlackAct->setStatusTip(tr("Insert black background attribute"));
	connect(backgroundBlackAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x1c); });

	QMenu *textSizeSubMenu = insertMenu->addMenu(tr("Text size"));
	QAction *textSizeNormalAct = textSizeSubMenu->addAction(tr("Normal size"));
	textSizeNormalAct->setStatusTip(tr("Insert normal size attribute"));
	connect(textSizeNormalAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x0c); });
	QAction *textSizeHeightAct = textSizeSubMenu->addAction(tr("Double height"));
	textSizeHeightAct->setStatusTip(tr("Insert double height attribute"));
	connect(textSizeHeightAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x0d); });
	QAction *textSizeWidthAct = textSizeSubMenu->addAction(tr("Double width"));
	textSizeWidthAct->setStatusTip(tr("Insert double width attribute"));
	connect(textSizeWidthAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x0e); });
	QAction *textSizeDoubleAct = textSizeSubMenu->addAction(tr("Double size"));
	textSizeDoubleAct->setStatusTip(tr("Insert double size attribute"));
	connect(textSizeDoubleAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x0f); });

	QAction *concealAct = insertMenu->addAction(tr("Conceal"));
	concealAct->setStatusTip(tr("Insert conceal attribute"));
	connect(concealAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x18); });

	QMenu *flashSubMenu = insertMenu->addMenu(tr("Flash"));
	QAction *flashFlashingAct = flashSubMenu->addAction(tr("Flashing"));
	flashFlashingAct->setStatusTip(tr("Insert flashing attribute"));
	connect(flashFlashingAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x08); });
	QAction *flashSteadyAct = flashSubMenu->addAction(tr("Steady"));
	flashSteadyAct->setStatusTip(tr("Insert steady attribute"));
	connect(flashSteadyAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x09); });

	QMenu *boxingSubMenu = insertMenu->addMenu(tr("Box"));
	QAction *boxingStartAct = boxingSubMenu->addAction(tr("Start box"));
	boxingStartAct->setStatusTip(tr("Insert start box attribute"));
	connect(boxingStartAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x0b); });
	QAction *boxingEndAct = boxingSubMenu->addAction(tr("End box"));
	boxingEndAct->setStatusTip(tr("Insert end box attribute"));
	connect(boxingEndAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x0a); });

	QAction *escSwitchAct = insertMenu->addAction(tr("ESC/switch"));
	escSwitchAct->setStatusTip(tr("Insert ESC/switch character set attribute"));
	connect(escSwitchAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x1b); });

	QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));

	toolsMenu->addAction(m_pageOptionsDockWidget->toggleViewAction());
	toolsMenu->addAction(m_x26DockWidget->toggleViewAction());
	toolsMenu->addAction(m_pageEnhancementsDockWidget->toggleViewAction());
	toolsMenu->addAction(m_paletteDockWidget->toggleViewAction());

	//FIXME is this main menubar separator to put help menu towards the right?
	menuBar()->addSeparator();

	QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
	QAction *aboutAct = helpMenu->addAction(tr("&About"), this, &MainWindow::about);
	aboutAct->setStatusTip(tr("Show the application's About box"));

#ifndef QT_NO_CLIPBOARD
/*
	cutAct->setEnabled(false);
	copyAct->setEnabled(false);
	connect(textWidget, &QTextEdit::copyAvailable, cutAct, &QAction::setEnabled);
	connect(textWidget, &QTextEdit::copyAvailable, copyAct, &QAction::setEnabled);
*/
#endif // !QT_NO_CLIPBOARD
}

void MainWindow::setSceneDimensions()
{
	const float aspectRatioHorizontalScaling[4] = { 0.6, 0.6, 0.8, 0.5 };
	const int topBottomBorders[3] = { 0, 10, 19 };
	const int leftRightBorders[3][2] = { { 0, 0 }, { 24, 72 }, { 77, 183 } };
	int newSceneWidth;

	int newViewAspectRatio = m_viewAspectRatio; // In case we need to narrow the characters to fit wide side panels in 4:3
	if (m_viewBorder == 0)
		newSceneWidth = m_textWidget->width();
	else
		newSceneWidth = 480+leftRightBorders[m_viewBorder][newViewAspectRatio == 1]*2;

	//FIXME find a better way of narrowing characters to squeeze in side panels
	if (m_viewAspectRatio != 1 && m_textWidget->width() > 576)
		newViewAspectRatio = 3;

	m_textScene->setSceneRect(0, 0, newSceneWidth, 250+topBottomBorders[m_viewBorder]*2);
	m_fullScreenTopRectItem->setRect(0, 0, newSceneWidth, topBottomBorders[m_viewBorder]);
	m_fullScreenBottomRectItem->setRect(0, 250+topBottomBorders[m_viewBorder], newSceneWidth, topBottomBorders[m_viewBorder]);
	m_textView->setTransform(QTransform((1+(float)m_viewZoom/2)*aspectRatioHorizontalScaling[newViewAspectRatio], 0, 0, 1+(float)m_viewZoom/2, 0, 0));
	if (m_viewBorder == 0)
		m_textProxyWidget->setPos(0, 0);
	else
		m_textProxyWidget->setPos(leftRightBorders[m_viewBorder][newViewAspectRatio == 1]-(m_textWidget->width()-480)/2, topBottomBorders[m_viewBorder]);
	for (int r=0; r<25; r++) {
		m_fullRowLeftRectItem[r]->setRect(0, topBottomBorders[m_viewBorder]+r*10, m_textWidget->x()+1, 10);
		m_fullRowRightRectItem[r]->setRect(m_textWidget->x()+m_textWidget->width()-1, topBottomBorders[m_viewBorder]+r*10, m_textWidget->x()+1, 10);
	}
}

void MainWindow::insertSubPage(bool afterCurrentSubPage, bool copyCurrentSubPage)
{
	QUndoCommand *insertSubPageCommand = new InsertSubPageCommand(m_textWidget->document(), afterCurrentSubPage, copyCurrentSubPage);
	m_textWidget->document()->undoStack()->push(insertSubPageCommand);
}

void MainWindow::setBorder(int newViewBorder)
{
	m_viewBorder = newViewBorder;
	setSceneDimensions();
}

void MainWindow::setAspectRatio(int newViewAspectRatio)
{
	m_viewAspectRatio = newViewAspectRatio;
	setSceneDimensions();
}

void MainWindow::zoomIn()
{
	if (m_viewZoom < 4)
		m_viewZoom++;
	setSceneDimensions();
}

void MainWindow::zoomOut()
{
	if (m_viewZoom > 0)
		m_viewZoom--;
	setSceneDimensions();
}

void MainWindow::zoomReset()
{
	m_viewZoom = 2;
	setSceneDimensions();
}

void MainWindow::createStatusBar()
{
	m_cursorStatus = new QLabel("Subpage 1/1 row 1 column 1");
	statusBar()->insertWidget(0, m_cursorStatus);
	m_levelSelect = new QLabel("Level");
	statusBar()->addPermanentWidget(m_levelSelect);

	QRadioButton *level1 = new QRadioButton("1");
	QRadioButton *level15 = new QRadioButton("1.5");
	QRadioButton *level25 = new QRadioButton("2.5");
	QRadioButton *level35 = new QRadioButton("3.5");
	statusBar()->addPermanentWidget(level1);
	statusBar()->addPermanentWidget(level15);
	statusBar()->addPermanentWidget(level25);
	statusBar()->addPermanentWidget(level35);
	level1->toggle();
	connect(level1, &QAbstractButton::clicked, [=]() { m_textWidget->pageRender()->setRenderLevel(0); m_textWidget->update(); });
	connect(level15, &QAbstractButton::clicked, [=]() { m_textWidget->pageRender()->setRenderLevel(1); m_textWidget->update(); });
	connect(level25, &QAbstractButton::clicked, [=]() { m_textWidget->pageRender()->setRenderLevel(2); m_textWidget->update(); });
	connect(level35, &QAbstractButton::clicked, [=]() { m_textWidget->pageRender()->setRenderLevel(3); m_textWidget->update(); });
	statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()
{
	//TODO window sizing
	QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
	const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
	const QByteArray windowState = settings.value("windowState", QByteArray()).toByteArray();

	m_viewBorder = settings.value("border", 2).toInt();
	m_viewBorder = (m_viewBorder < 0 || m_viewBorder > 2) ? 2 : m_viewBorder;
	m_borderActs[m_viewBorder]->setChecked(true);
	m_viewAspectRatio = settings.value("aspectratio", 0).toInt();
	m_viewAspectRatio = (m_viewAspectRatio < 0 || m_viewAspectRatio > 2) ? 0 : m_viewAspectRatio;
	m_aspectRatioActs[m_viewAspectRatio]->setChecked(true);
	m_viewZoom = settings.value("zoom", 2).toInt();
	m_viewZoom = (m_viewZoom < 0 || m_viewZoom > 4) ? 2 : m_viewZoom;

	// zoom 0 = 420,426px, 1 = 620,570px, 2 = 780,720px
	if (geometry.isEmpty()) {
		const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
		if (availableGeometry.width() < 620 || availableGeometry.height() < 570) {
			resize(430, 430);
			m_viewZoom = 0;
		} else if (availableGeometry.width() < 780 || availableGeometry.height() < 720) {
			resize(620, 570);
			m_viewZoom = 1;
		} else
			resize(780, 720);
//			m_viewZoom = 2;
		move((availableGeometry.width() - width()) / 2, (availableGeometry.height() - height()) / 2);
	} else
		restoreGeometry(geometry);
	if (windowState.isEmpty()) {
		m_pageOptionsDockWidget->hide();
		m_pageOptionsDockWidget->setFloating(true);
		m_pageEnhancementsDockWidget->hide();
		m_pageEnhancementsDockWidget->setFloating(true);
		m_x26DockWidget->hide();
		m_x26DockWidget->setFloating(true);
		m_paletteDockWidget->hide();
		m_paletteDockWidget->setFloating(true);
	} else
		restoreState(windowState);
}

void MainWindow::writeSettings()
{
	QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
	settings.setValue("geometry", saveGeometry());
	settings.setValue("windowState", saveState());
	settings.setValue("border", m_viewBorder);
	settings.setValue("aspectratio", m_viewAspectRatio);
	settings.setValue("zoom", m_viewZoom);
}

bool MainWindow::maybeSave()
{
	if (m_textWidget->document()->undoStack()->isClean())
		return true;
	const QMessageBox::StandardButton ret = QMessageBox::warning(this, tr("QTeletextMaker"), tr("The document has been modified.\nDo you want to save your changes?"), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
	switch (ret) {
		case QMessageBox::Save:
			return save();
		case QMessageBox::Cancel:
			return false;
		default:
			break;
	}
	return true;
}

void MainWindow::loadFile(const QString &fileName)
{
	QFile file(fileName);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		QMessageBox::warning(this, tr("QTeletextMaker"), tr("Cannot read file %1:\n%2.").arg(QDir::toNativeSeparators(fileName), file.errorString()));
		setCurrentFile(QString());
		return;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);
	m_textWidget->document()->loadDocument(&file);

	QApplication::restoreOverrideCursor();

	setCurrentFile(fileName);
	statusBar()->showMessage(tr("File loaded"), 2000);
}

void MainWindow::setRecentFilesVisible(bool visible)
{
	m_recentFileSubMenuAct->setVisible(visible);
	m_recentFileSeparator->setVisible(visible);
}

static inline QString recentFilesKey() { return QStringLiteral("recentFileList"); }
static inline QString fileKey() { return QStringLiteral("file"); }

static QStringList readRecentFiles(QSettings &settings)
{
	QStringList result;
	const int count = settings.beginReadArray(recentFilesKey());
	for (int i = 0; i < count; ++i) {
		settings.setArrayIndex(i);
		result.append(settings.value(fileKey()).toString());
	}
	settings.endArray();
	return result;
}

static void writeRecentFiles(const QStringList &files, QSettings &settings)
{
	const int count = files.size();
	settings.beginWriteArray(recentFilesKey());
	for (int i = 0; i < count; ++i) {
		settings.setArrayIndex(i);
		settings.setValue(fileKey(), files.at(i));
	}
	settings.endArray();
}

bool MainWindow::hasRecentFiles()
{
	QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
	const int count = settings.beginReadArray(recentFilesKey());
	settings.endArray();
	return count > 0;
}

void MainWindow::prependToRecentFiles(const QString &fileName)
{
	QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

	const QStringList oldRecentFiles = readRecentFiles(settings);
	QStringList recentFiles = oldRecentFiles;
	recentFiles.removeAll(fileName);
	recentFiles.prepend(fileName);
	if (oldRecentFiles != recentFiles)
		writeRecentFiles(recentFiles, settings);

	setRecentFilesVisible(!recentFiles.isEmpty());
}

void MainWindow::updateRecentFileActions()
{
	QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

	const QStringList recentFiles = readRecentFiles(settings);
	const int count = qMin(int(m_MaxRecentFiles), recentFiles.size());
	int i = 0;
	for ( ; i < count; ++i) {
		const QString fileName = MainWindow::strippedName(recentFiles.at(i));
		m_recentFileActs[i]->setText(tr("&%1 %2").arg(i + 1).arg(fileName));
		m_recentFileActs[i]->setData(recentFiles.at(i));
		m_recentFileActs[i]->setVisible(true);
	}
	for ( ; i < m_MaxRecentFiles; ++i)
		m_recentFileActs[i]->setVisible(false);
}

void MainWindow::openRecentFile()
{
	if (const QAction *action = qobject_cast<const QAction *>(sender()))
		openFile(action->data().toString());
}

bool MainWindow::saveFile(const QString &fileName)
{
	QFile file(fileName);
	if (!file.open(QFile::WriteOnly | QFile::Text)) {
		QMessageBox::warning(this, tr("QTeletextMaker"), tr("Cannot write file %1:\n%2.").arg(QDir::toNativeSeparators(fileName), file.errorString()));
		return false;
	}

	QTextStream out(&file);
	out.setCodec("ISO-8859-1");
	QApplication::setOverrideCursor(Qt::WaitCursor);
	m_textWidget->document()->saveDocument(&out);
	QApplication::restoreOverrideCursor();

	setCurrentFile(fileName);
	statusBar()->showMessage(tr("File saved"), 2000);
	return true;
}

void MainWindow::setCurrentFile(const QString &fileName)
{
	static int sequenceNumber = 1;

	m_isUntitled = fileName.isEmpty();
	if (m_isUntitled)
		m_curFile = tr("untitled%1.tti").arg(sequenceNumber++);
	else
		m_curFile = QFileInfo(fileName).canonicalFilePath();

	m_textWidget->document()->undoStack()->setClean();

	if (!m_isUntitled && windowFilePath() != m_curFile)
		MainWindow::prependToRecentFiles(m_curFile);

	setWindowFilePath(m_curFile);
}

QString MainWindow::strippedName(const QString &fullFileName)
{
	return QFileInfo(fullFileName).fileName();
}

MainWindow *MainWindow::findMainWindow(const QString &fileName) const
{
	QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

	foreach (QWidget *widget, QApplication::topLevelWidgets()) {
		MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
		if (mainWin && mainWin->m_curFile == canonicalFilePath)
			return mainWin;
	}

	return 0;
}

void MainWindow::updateCursorPosition()
{
	m_cursorStatus->setText(QString("Subpage %1/%2 row %3 column %4").arg(m_textWidget->document()->currentSubPageIndex()+1).arg(m_textWidget->document()->numberOfSubPages()).arg(m_textWidget->document()->cursorRow()).arg(m_textWidget->document()->cursorColumn()));
}

void MainWindow::updatePageWidgets()
{
	updateCursorPosition();
	m_pageOptionsDockWidget->updateWidgets();
	m_pageEnhancementsDockWidget->updateWidgets();
	m_x26DockWidget->loadX26List();
	m_paletteDockWidget->updateAllColourButtons();
}
