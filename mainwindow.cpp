/*
 * Copyright (C) 2020-2023 Gavin MacGregor
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

#include <QActionGroup>
#include <QApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QImage>
#include <QList>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScreen>
#include <QSettings>
#include <QShortcut>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <iostream>

#include "mainwindow.h"

#include "levelonecommands.h"
#include "loadsave.h"
#include "mainwidget.h"
#include "pagecomposelinksdockwidget.h"
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

static inline bool hasTTISuffix(const QString &filename)
{
	return filename.endsWith(".tti", Qt::CaseInsensitive) || filename.endsWith(".ttix", Qt::CaseInsensitive);
}

static inline void changeSuffixFromTTI(QString &filename, const QString &newSuffix)
{
	if (filename.endsWith(".tti", Qt::CaseInsensitive)) {
		filename.chop(4);
		filename.append("." + newSuffix);
	} else if (filename.endsWith(".ttix", Qt::CaseInsensitive)) {
		filename.chop(5);
		filename.append("." + newSuffix);
	}
}

bool MainWindow::save()
{
	// If imported from non-.tti, force "Save As" so we don't clobber the original imported file
	return m_isUntitled || !hasTTISuffix(m_curFile) ? saveAs() : saveFile(m_curFile);
}

bool MainWindow::saveAs()
{
	QString suggestedName = m_curFile;

	// If imported from non-.tti, change extension so we don't clobber the original imported file
	if (suggestedName.endsWith(".t42", Qt::CaseInsensitive)) {
		suggestedName.chop(4);
		suggestedName.append(".tti");
	}

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), suggestedName, "TTI teletext page (*.tti *.ttix)");
	if (fileName.isEmpty())
		return false;

	return saveFile(fileName);
}

void MainWindow::reload()
{
	if (m_isUntitled)
		return;

	if (!m_textWidget->document()->undoStack()->isClean()) {
		const QMessageBox::StandardButton ret = QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("The document \"%1\" has been modified.\nDo you want to discard your changes?").arg(QFileInfo(m_curFile).fileName()), QMessageBox::Discard | QMessageBox::Cancel);

		if (ret != QMessageBox::Discard)
			return;
	} else {
		const QMessageBox::StandardButton ret = QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("Do you want to reload the document \"%1\" from disk?").arg(QFileInfo(m_curFile).fileName()), QMessageBox::Yes | QMessageBox::No);

		if (ret != QMessageBox::Yes)
			return;
	}

	int subPageIndex = m_textWidget->document()->currentSubPageIndex();

	m_textWidget->document()->clear();
	loadFile(m_curFile);

	if (subPageIndex >= m_textWidget->document()->numberOfSubPages())
		subPageIndex = m_textWidget->document()->numberOfSubPages()-1;

	m_textWidget->document()->selectSubPageIndex(subPageIndex, true);
}

void MainWindow::exportPNG()
{
	QString exportFileName = QFileDialog::getSaveFileName(this, tr("Export PNG"), QString(), "PNG image (*.png)");
	if (exportFileName.isEmpty())
		return;

	// Prepare widget image for extraction
	m_textWidget->pauseFlash(true);
	m_textScene->hideGUIElements(true);
	// Disable exporting in Mix mode as it corrupts the background
	bool reMix = m_textWidget->pageRender()->mix();
	if (reMix) {
		m_textWidget->setMix(false);
		m_textScene->setMix(false);
	}

	// Extract the image from the scene
	QImage interImage = QImage(m_textScene->sceneRect().size().toSize(), QImage::Format_RGB32);
//	This ought to make the background transparent in Mix mode, but it doesn't
//	if (m_textWidget->pageDecode()->mix())
//		interImage.fill(QColor(0, 0, 0, 0));
	QPainter interPainter(&interImage);
	m_textScene->render(&interPainter);

	// Now we've extracted the image we can put the GUI things back
	m_textScene->hideGUIElements(false);
	if (reMix) {
		m_textWidget->setMix(true);
		m_textScene->setMix(true);
	}
	m_textWidget->pauseFlash(false);

	// Now scale the extracted image to the selected aspect ratio
	// We do this in two steps so that anti-aliasing only occurs on vertical lines

	// Double the vertical height first
	const QImage doubleHeightImage = interImage.scaled(interImage.width(), interImage.height()*2, Qt::IgnoreAspectRatio, Qt::FastTransformation);

	// If aspect ratio is Pixel 1:2 we're already at the correct scale
	if (m_viewAspectRatio != 3) {
		// Scale it horizontally to the selected aspect ratio
		const QImage scaledImage = doubleHeightImage.scaled((int)((float)doubleHeightImage.width() * aspectRatioHorizontalScaling[m_viewAspectRatio] * 2), doubleHeightImage.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

		if (!scaledImage.save(exportFileName, "PNG"))
			QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("Cannot export file %1.").arg(QDir::toNativeSeparators(exportFileName)));
	} else if (!doubleHeightImage.save(exportFileName, "PNG"))
		QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("Cannot export file %1.").arg(QDir::toNativeSeparators(exportFileName)));
}

void MainWindow::exportZXNet()
{
	QDesktopServices::openUrl(QUrl("http://zxnet.co.uk/teletext/editor/" + exportHashStringPage(m_textWidget->document()->currentSubPage()) + exportHashStringPackets(m_textWidget->document()->currentSubPage())));
}

void MainWindow::exportEditTF()
{
	QDesktopServices::openUrl(QUrl("http://edit.tf/" + exportHashStringPage(m_textWidget->document()->currentSubPage())));
}

void MainWindow::about()
{
	QMessageBox::about(this, tr("About"), QString("<b>%1</b><br>"
	"An open source Level 2.5 teletext page editor.<br>"
	"<i>Version %2</i><br><br>"
	"Copyright (C) 2020-2023 Gavin MacGregor<br><br>"
	"Released under the GNU General Public License version 3<br>"
	"<a href=\"https://github.com/gkthemac/qteletextmaker\">https://github.com/gkthemac/qteletextmaker</a>").arg(QApplication::applicationDisplayName()).arg(QApplication::applicationVersion()));
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
	m_pageComposeLinksDockWidget = new PageComposeLinksDockWidget(m_textWidget);
	addDockWidget(Qt::RightDockWidgetArea, m_pageComposeLinksDockWidget);

	m_textScene = new LevelOneScene(m_textWidget, this);

	createActions();
	createStatusBar();

	readSettings();

	m_textView = new QGraphicsView(this);
	m_textView->setScene(m_textScene);
	if (m_viewSmoothTransform)
		m_textView->setRenderHints(QPainter::SmoothPixmapTransform);
	m_textView->setBackgroundBrush(QBrush(QColor(32, 48, 96)));
	setSceneDimensions();
	setCentralWidget(m_textView);

	connect(m_textWidget->document(), &TeletextDocument::cursorMoved, this, &MainWindow::updateCursorPosition);
	connect(m_textWidget->document(), &TeletextDocument::selectionMoved, m_textScene, &LevelOneScene::updateSelection);
	connect(m_textWidget->document()->undoStack(), &QUndoStack::cleanChanged, this, [=]() { setWindowModified(!m_textWidget->document()->undoStack()->isClean()); } );
	connect(m_textWidget->document(), &TeletextDocument::aboutToChangeSubPage, m_x26DockWidget, &X26DockWidget::unloadX26List);
	connect(m_textWidget->document(), &TeletextDocument::subPageSelected, this, &MainWindow::updatePageWidgets);
	connect(m_textWidget->document(), &TeletextDocument::pageOptionsChanged, this, &MainWindow::updatePageWidgets);
	connect(m_textWidget, &TeletextWidget::sizeChanged, this, &MainWindow::setSceneDimensions);
	connect(m_textWidget->pageDecode(), &TeletextPageDecode::fullScreenColourChanged, m_textScene, &LevelOneScene::setFullScreenColour);
	connect(m_textWidget->pageDecode(), &TeletextPageDecode::fullRowColourChanged, m_textScene, &LevelOneScene::setFullRowColour);
	connect(m_textWidget, &TeletextWidget::insertKeyPressed, this, &MainWindow::toggleInsertMode);

	connect(m_textScene, &LevelOneScene::mouseZoomIn, this, &MainWindow::zoomIn);
	connect(m_textScene, &LevelOneScene::mouseZoomOut, this, &MainWindow::zoomOut);

	QShortcut *blockShortCut = new QShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_J), m_textView);
	connect(blockShortCut, &QShortcut::activated, [=]() { m_textWidget->setCharacter(0x7f); });

	setUnifiedTitleAndToolBarOnMac(true);

	updatePageWidgets();

	m_textView->setFocus();
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

	const QIcon reloadIcon = QIcon::fromTheme("document-revert");
	QAction *reloadAct = fileMenu->addAction(reloadIcon, tr("Reload"), this, &MainWindow::reload);
	reloadAct->setShortcut(QKeySequence(Qt::Key_F5));
	reloadAct->setStatusTip(tr("Reload the document from disk"));

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

	m_exportAutoAct = fileMenu->addAction(tr("Export subpage..."));
	m_exportAutoAct->setEnabled(false);
	m_exportAutoAct->setShortcut(tr("Ctrl+E"));
	m_exportAutoAct->setStatusTip("Export this subpage back to the imported file");
	connect(fileMenu, &QMenu::aboutToShow, this, &MainWindow::updateExportAutoAction);
	connect(m_exportAutoAct, &QAction::triggered, this, &MainWindow::exportAuto);

	QAction *exportT42Act = fileMenu->addAction(tr("Export subpage as t42..."));
	exportT42Act->setStatusTip("Export this subpage as a t42 file");
	connect(exportT42Act, &QAction::triggered, this, [=]() { exportT42(false); });

	QMenu *exportHashStringSubMenu = fileMenu->addMenu(tr("Export subpage to online editor"));

	QAction *exportZXNetAct = exportHashStringSubMenu->addAction(tr("Open in zxnet.co.uk"));
	exportZXNetAct->setStatusTip("Export and open this subpage in the zxnet.co.uk online editor");
	connect(exportZXNetAct, &QAction::triggered, this, &MainWindow::exportZXNet);

	QAction *exportEditTFAct = exportHashStringSubMenu->addAction(tr("Open in edit.tf"));
	exportEditTFAct->setStatusTip("Export and open this subpage in the edit.tf online editor");
	connect(exportEditTFAct, &QAction::triggered, this, &MainWindow::exportEditTF);

	QAction *exportPNGAct = fileMenu->addAction(tr("Export subpage as PNG..."));
	exportPNGAct->setStatusTip("Export a PNG image of this subpage");
	connect(exportPNGAct, &QAction::triggered, this, &MainWindow::exportPNG);

	QAction *exportM29Act = fileMenu->addAction(tr("Export subpage X/28 as M/29..."));
	exportM29Act->setStatusTip("Export this subpage's X/28 packets as a tti file with M/29 packets");
	connect(exportM29Act, &QAction::triggered, this, &MainWindow::exportM29);

	fileMenu->addSeparator();

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

	const QIcon undoIcon = QIcon::fromTheme("edit-undo", QIcon(":/images/undo.png"));
	QAction *undoAction = m_textWidget->document()->undoStack()->createUndoAction(this, tr("&Undo"));
	undoAction->setIcon(undoIcon);
	editMenu->addAction(undoAction);
	editToolBar->addAction(undoAction);
	undoAction->setShortcuts(QKeySequence::Undo);

	const QIcon redoIcon = QIcon::fromTheme("edit-redo", QIcon(":/images/redo.png"));
	QAction *redoAction = m_textWidget->document()->undoStack()->createRedoAction(this, tr("&Redo"));
	redoAction->setIcon(redoIcon);
	editMenu->addAction(redoAction);
	editToolBar->addAction(redoAction);
	redoAction->setShortcuts(QKeySequence::Redo);

	editMenu->addSeparator();

#ifndef QT_NO_CLIPBOARD
	const QIcon cutIcon = QIcon::fromTheme("edit-cut", QIcon(":/images/cut.png"));
	QAction *cutAct = new QAction(cutIcon, tr("Cu&t"), this);
	cutAct->setShortcuts(QKeySequence::Cut);
	cutAct->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
	connect(cutAct, &QAction::triggered, m_textWidget, &TeletextWidget::cut);
	editMenu->addAction(cutAct);
	editToolBar->addAction(cutAct);

	const QIcon copyIcon = QIcon::fromTheme("edit-copy", QIcon(":/images/copy.png"));
	QAction *copyAct = new QAction(copyIcon, tr("&Copy"), this);
	copyAct->setShortcuts(QKeySequence::Copy);
	copyAct->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
	connect(copyAct, &QAction::triggered, m_textWidget, &TeletextWidget::copy);
	editMenu->addAction(copyAct);
	editToolBar->addAction(copyAct);

	const QIcon pasteIcon = QIcon::fromTheme("edit-paste", QIcon(":/images/paste.png"));
	QAction *pasteAct = new QAction(pasteIcon, tr("&Paste"), this);
	pasteAct->setShortcuts(QKeySequence::Paste);
	pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
	connect(pasteAct, &QAction::triggered, m_textWidget, &TeletextWidget::paste);
	editMenu->addAction(pasteAct);
	editToolBar->addAction(pasteAct);

	editMenu->addSeparator();
#endif // !QT_NO_CLIPBOARD

	QAction *insertBlankRowAct = editMenu->addAction(tr("Insert blank row"));
	insertBlankRowAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
	insertBlankRowAct->setStatusTip(tr("Insert a blank row at the cursor position"));
	connect(insertBlankRowAct, &QAction::triggered, [=]() { insertRow(false); } );

	QAction *insertCopyRowAct = editMenu->addAction(tr("Insert copy row"));
	insertCopyRowAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I));
	insertCopyRowAct->setStatusTip(tr("Insert a row that's a copy of the row at the cursor position"));
	connect(insertCopyRowAct, &QAction::triggered, [=]() { insertRow(true); } );

	QAction *deleteRowAct = editMenu->addAction(tr("Delete row"));
	deleteRowAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
	deleteRowAct->setStatusTip(tr("Delete the row at the cursor position"));
	connect(deleteRowAct, &QAction::triggered, this, &MainWindow::deleteRow);

	editMenu->addSeparator();

	QAction *insertBeforeAct = editMenu->addAction(tr("&Insert subpage before"));
	insertBeforeAct->setStatusTip(tr("Insert a blank subpage before this subpage"));
	connect(insertBeforeAct, &QAction::triggered, [=]() { insertSubPage(false, false); });

	QAction *insertAfterAct = editMenu->addAction(tr("Insert subpage after"));
	insertAfterAct->setStatusTip(tr("Insert a blank subpage after this subpage"));
	connect(insertAfterAct, &QAction::triggered, [=]() { insertSubPage(true, false); });

	QAction *insertCopyAct = editMenu->addAction(tr("Insert subpage copy"));
	insertCopyAct->setStatusTip(tr("Insert a subpage that's a copy of this subpage"));
	connect(insertCopyAct, &QAction::triggered, [=]() { insertSubPage(false, true); });

	m_deleteSubPageAction = editMenu->addAction(tr("Delete subpage"));
	m_deleteSubPageAction->setStatusTip(tr("Delete this subpage"));
	connect(m_deleteSubPageAction, &QAction::triggered, this, &MainWindow::deleteSubPage);

	QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

	QAction *revealAct = viewMenu->addAction(tr("&Reveal"));
	revealAct->setCheckable(true);
	revealAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
	revealAct->setStatusTip(tr("Toggle reveal"));
	connect(revealAct, &QAction::toggled, m_textWidget, &TeletextWidget::setReveal);

	QAction *mixAct = viewMenu->addAction(tr("&Mix"));
	mixAct->setCheckable(true);
	mixAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
	mixAct->setStatusTip(tr("Toggle mix"));
	connect(mixAct, &QAction::toggled, m_textWidget, &TeletextWidget::setMix);
	connect(mixAct, &QAction::toggled, m_textScene, &LevelOneScene::setMix);

	QAction *gridAct = viewMenu->addAction(tr("&Grid"));
	gridAct->setCheckable(true);
	gridAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
	gridAct->setStatusTip(tr("Toggle the text grid"));
	connect(gridAct, &QAction::toggled, m_textScene, &LevelOneScene::toggleGrid);

	QAction *showControlCodesAct = viewMenu->addAction(tr("Show control codes"));
	showControlCodesAct->setCheckable(true);
	showControlCodesAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
	showControlCodesAct->setStatusTip(tr("Toggle showing of control codes"));
	connect(showControlCodesAct, &QAction::toggled, m_textWidget, &TeletextWidget::setShowControlCodes);

	viewMenu->addSeparator();

	QMenu *borderSubMenu = viewMenu->addMenu(tr("Border"));
	m_borderActs[0] = borderSubMenu->addAction(tr("None"));
	m_borderActs[0]->setStatusTip(tr("View with no border"));
	m_borderActs[1] = borderSubMenu->addAction(tr("Minimal"));
	m_borderActs[1]->setStatusTip(tr("View with minimal border"));
	m_borderActs[2] = borderSubMenu->addAction(tr("Full TV"));
	m_borderActs[2]->setStatusTip(tr("View with full TV overscan border"));

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

	viewMenu->addSeparator();

	m_smoothTransformAction = viewMenu->addAction(tr("Smooth font scaling"));
	m_smoothTransformAction->setCheckable(true);
	m_smoothTransformAction->setStatusTip(tr("Toggle smooth font scaling"));
	connect(m_smoothTransformAction, &QAction::toggled, this, &MainWindow::setSmoothTransform);

	QAction *zoomInAct = viewMenu->addAction(tr("Zoom In"));
	zoomInAct->setShortcuts(QKeySequence::ZoomIn);
	zoomInAct->setStatusTip(tr("Zoom in"));
	connect(zoomInAct, &QAction::triggered, this, &MainWindow::zoomIn);

	QAction *zoomOutAct = viewMenu->addAction(tr("Zoom Out"));
	zoomOutAct->setShortcuts(QKeySequence::ZoomOut);
	zoomOutAct->setStatusTip(tr("Zoom out"));
	connect(zoomOutAct, &QAction::triggered, this, &MainWindow::zoomOut);

	QAction *zoomResetAct = viewMenu->addAction(tr("Reset zoom"));
	zoomResetAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
	zoomResetAct->setStatusTip(tr("Reset zoom level"));
	connect(zoomResetAct, &QAction::triggered, this, &MainWindow::zoomReset);

	QMenu *insertMenu = menuBar()->addMenu(tr("&Insert"));

	QMenu *alphaColourSubMenu = insertMenu->addMenu(tr("Alphanumeric colour"));
	QMenu *mosaicColourSubMenu = insertMenu->addMenu(tr("Mosaic colour"));
	for (int i=0; i<=7; i++) {
		const char *colours[] = { "Black", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White" };

		QAction *alphaColour = alphaColourSubMenu->addAction(tr(colours[i]));
		alphaColour->setShortcut(QKeySequence(QString("Esc, %1").arg(i)));
		alphaColour->setStatusTip(QString("Insert alphanumeric %1 attribute").arg(QString(colours[i]).toLower()));
		connect(alphaColour, &QAction::triggered, [=]() { m_textWidget->setCharacter(i); });

		QAction *mosaicColour = mosaicColourSubMenu->addAction(tr(colours[i]));
		mosaicColour->setShortcut(QKeySequence(QString("Esc, Shift+%1").arg(i)));
		mosaicColour->setStatusTip(QString("Insert mosaic %1 attribute").arg(QString(colours[i]).toLower()));
		connect(mosaicColour, &QAction::triggered, [=]() { m_textWidget->setCharacter(i+0x10); });
	}

	QMenu *mosaicsStyleSubMenu = insertMenu->addMenu(tr("Mosaics style"));
	QAction *mosaicsSeparatedAct = mosaicsStyleSubMenu->addAction(tr("Separated mosaics"));
	mosaicsSeparatedAct->setShortcut(QKeySequence(Qt::NoModifier | Qt::Key_Escape, Qt::SHIFT | Qt::Key_S));
	mosaicsSeparatedAct->setStatusTip(tr("Insert separated mosaics attribute"));
	connect(mosaicsSeparatedAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x1a); });
	QAction *mosaicsContiguousAct = mosaicsStyleSubMenu->addAction(tr("Contiguous mosaics"));
	mosaicsContiguousAct->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_S));
	mosaicsContiguousAct->setStatusTip(tr("Insert contiguous mosaics attribute"));
	connect(mosaicsContiguousAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x19); });

	QMenu *mosaicsHoldSubMenu = insertMenu->addMenu(tr("Mosaics hold"));
	QAction *mosaicsHoldAct = mosaicsHoldSubMenu->addAction(tr("Hold mosaics"));
	mosaicsHoldAct->setShortcut(QKeySequence(Qt::NoModifier | Qt::Key_Escape, Qt::SHIFT | Qt::Key_H));
	mosaicsHoldAct->setStatusTip(tr("Insert hold mosaics attribute"));
	connect(mosaicsHoldAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x1e); });
	QAction *mosaicsReleaseAct = mosaicsHoldSubMenu->addAction(tr("Release mosaics"));
	mosaicsReleaseAct->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_H));
	mosaicsReleaseAct->setStatusTip(tr("Insert release mosaics attribute"));
	connect(mosaicsReleaseAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x1f); });

	QMenu *backgroundColourSubMenu = insertMenu->addMenu(tr("Background colour"));
	QAction *backgroundNewAct = backgroundColourSubMenu->addAction(tr("New background"));
	backgroundNewAct->setShortcut(QKeySequence(Qt::NoModifier | Qt::Key_Escape, Qt::SHIFT | Qt::Key_N));
	backgroundNewAct->setStatusTip(tr("Insert new background attribute"));
	connect(backgroundNewAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x1d); });
	QAction *backgroundBlackAct = backgroundColourSubMenu->addAction(tr("Black background"));
	backgroundBlackAct->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_N));
	backgroundBlackAct->setStatusTip(tr("Insert black background attribute"));
	connect(backgroundBlackAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x1c); });

	QMenu *textSizeSubMenu = insertMenu->addMenu(tr("Text size"));
	QAction *textSizeNormalAct = textSizeSubMenu->addAction(tr("Normal size"));
	textSizeNormalAct->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_D));
	textSizeNormalAct->setStatusTip(tr("Insert normal size attribute"));
	connect(textSizeNormalAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x0c); });
	QAction *textSizeDoubleHeightAct = textSizeSubMenu->addAction(tr("Double height"));
	textSizeDoubleHeightAct->setShortcut(QKeySequence(Qt::NoModifier | Qt::Key_Escape, Qt::SHIFT | Qt::Key_D));
	textSizeDoubleHeightAct->setStatusTip(tr("Insert double height attribute"));
	connect(textSizeDoubleHeightAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x0d); });
	QAction *textSizeDoubleWidthAct = textSizeSubMenu->addAction(tr("Double width"));
	textSizeDoubleWidthAct->setShortcut(QKeySequence(Qt::NoModifier | Qt::Key_Escape, Qt::CTRL | Qt::Key_D));
	textSizeDoubleWidthAct->setStatusTip(tr("Insert double width attribute"));
	connect(textSizeDoubleWidthAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x0e); });
	QAction *textSizeDoubleSizeAct = textSizeSubMenu->addAction(tr("Double size"));
	textSizeDoubleSizeAct->setShortcut(QKeySequence(Qt::NoModifier | Qt::Key_Escape, Qt::CTRL | Qt::SHIFT | Qt::Key_D));
	textSizeDoubleSizeAct->setStatusTip(tr("Insert double size attribute"));
	connect(textSizeDoubleSizeAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x0f); });

	QAction *concealAct = insertMenu->addAction(tr("Conceal"));
	concealAct->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_O));
	concealAct->setStatusTip(tr("Insert conceal attribute"));
	connect(concealAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x18); });

	QMenu *flashSubMenu = insertMenu->addMenu(tr("Flash"));
	QAction *flashFlashingAct = flashSubMenu->addAction(tr("Flashing"));
	flashFlashingAct->setShortcut(QKeySequence(Qt::NoModifier | Qt::Key_Escape, Qt::SHIFT | Qt::Key_F));
	flashFlashingAct->setStatusTip(tr("Insert flashing attribute"));
	connect(flashFlashingAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x08); });
	QAction *flashSteadyAct = flashSubMenu->addAction(tr("Steady"));
	flashSteadyAct->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_F));
	flashSteadyAct->setStatusTip(tr("Insert steady attribute"));
	connect(flashSteadyAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x09); });

	QMenu *boxingSubMenu = insertMenu->addMenu(tr("Box"));
	QAction *boxingStartAct = boxingSubMenu->addAction(tr("Start box"));
	boxingStartAct->setShortcut(QKeySequence(Qt::NoModifier | Qt::Key_Escape, Qt::SHIFT | Qt::Key_X));
	boxingStartAct->setStatusTip(tr("Insert start box attribute"));
	connect(boxingStartAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x0b); });
	QAction *boxingEndAct = boxingSubMenu->addAction(tr("End box"));
	boxingEndAct->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_X));
	boxingEndAct->setStatusTip(tr("Insert end box attribute"));
	connect(boxingEndAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x0a); });

	QAction *escSwitchAct = insertMenu->addAction(tr("ESC/switch"));
	escSwitchAct->setShortcut(QKeySequence(Qt::NoModifier | Qt::Key_Escape, Qt::CTRL | Qt::Key_S));
	escSwitchAct->setStatusTip(tr("Insert ESC/switch character set attribute"));
	connect(escSwitchAct, &QAction::triggered, [=]() { m_textWidget->setCharacter(0x1b); });

	QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));

	toolsMenu->addAction(m_pageOptionsDockWidget->toggleViewAction());
	toolsMenu->addAction(m_x26DockWidget->toggleViewAction());
	toolsMenu->addAction(m_pageEnhancementsDockWidget->toggleViewAction());
	toolsMenu->addAction(m_paletteDockWidget->toggleViewAction());
	toolsMenu->addAction(m_pageComposeLinksDockWidget->toggleViewAction());

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
	const int topBottomBorders[3] = { 0, 10, 19 };
	const int pillarBoxSizes[3] = { 672, 720, 854 };
	const int leftRightBorders[3] = { 0, 24, 77 };
	int newSceneWidth;

	if (m_viewAspectRatio == 1)
		// 16:9 pillar box aspect ratio, fixed horizontal size whatever the widget width is
		newSceneWidth = pillarBoxSizes[m_viewBorder];
	else if (m_viewBorder == 2)
		// "Full TV" border, semi-fixed horizontal size to TV width
		// Side panel width over 13 columns will cause this to widen a little
		newSceneWidth = qMax(640, m_textWidget->width());
	else
		newSceneWidth = m_textWidget->width() + leftRightBorders[m_viewBorder]*2;

	m_textScene->setBorderDimensions(newSceneWidth, 250+topBottomBorders[m_viewBorder]*2, m_textWidget->width(), m_textWidget->pageDecode()->leftSidePanelColumns(), m_textWidget->pageDecode()->rightSidePanelColumns());
	m_textView->setTransform(QTransform((1+(float)m_viewZoom/2)*aspectRatioHorizontalScaling[m_viewAspectRatio], 0, 0, 1+(float)m_viewZoom/2, 0, 0));
}

void MainWindow::insertRow(bool copyRow)
{
	if (m_textWidget->document()->cursorRow() == 24)
		return;
	m_textWidget->document()->undoStack()->push(new InsertRowCommand(m_textWidget->document(), copyRow));
}

void MainWindow::deleteRow()
{
	m_textWidget->document()->undoStack()->push(new DeleteRowCommand(m_textWidget->document()));
}

void MainWindow::insertSubPage(bool afterCurrentSubPage, bool copyCurrentSubPage)
{
	m_textWidget->document()->undoStack()->push(new InsertSubPageCommand(m_textWidget->document(), afterCurrentSubPage, copyCurrentSubPage));
}

void MainWindow::deleteSubPage()
{
	if (m_textWidget->document()->numberOfSubPages() == 1)
		return;

	m_textWidget->document()->undoStack()->push(new DeleteSubPageCommand(m_textWidget->document()));
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

void MainWindow::setSmoothTransform(bool smoothTransform)
{
	m_viewSmoothTransform = smoothTransform;
	if (smoothTransform)
		m_textView->setRenderHints(QPainter::SmoothPixmapTransform);
	else
		m_textView->setRenderHints({ });
}

void MainWindow::zoomIn()
{
	if (m_viewZoom < 4)
		m_viewZoom++;
	else if (m_viewZoom < 12)
		m_viewZoom += 2;
	setSceneDimensions();
}

void MainWindow::zoomOut()
{
	if (m_viewZoom > 4)
		m_viewZoom -= 2;
	else if (m_viewZoom > 0)
		m_viewZoom--;
	setSceneDimensions();
}

void MainWindow::zoomReset()
{
	m_viewZoom = 2;
	setSceneDimensions();
}

void MainWindow::toggleInsertMode()
{
	m_textWidget->setInsertMode(!m_textWidget->insertMode());
	if (m_textWidget->insertMode())
		m_insertModePushButton->setText("INSERT");
	else
		m_insertModePushButton->setText("OVERWRITE");
}

void MainWindow::createStatusBar()
{
	m_previousSubPageButton = new QToolButton;
	m_previousSubPageButton->setAutoRaise(true);
	m_previousSubPageButton->setArrowType(Qt::LeftArrow);
	statusBar()->insertWidget(0, m_previousSubPageButton);
	connect(m_previousSubPageButton, &QToolButton::clicked, m_textWidget->document(), &TeletextDocument::selectSubPagePrevious);

	m_subPageLabel = new QLabel("1/1");
	statusBar()->insertWidget(1, m_subPageLabel);

	m_nextSubPageButton = new QToolButton;
	m_previousSubPageButton->setMinimumSize(m_subPageLabel->height(), m_subPageLabel->height());
	m_previousSubPageButton->setMaximumSize(m_subPageLabel->height(), m_subPageLabel->height());
	m_nextSubPageButton->setMinimumSize(m_subPageLabel->height(), m_subPageLabel->height());
	m_nextSubPageButton->setMaximumSize(m_subPageLabel->height(), m_subPageLabel->height());
	m_nextSubPageButton->setAutoRaise(true);
	m_nextSubPageButton->setArrowType(Qt::RightArrow);
	statusBar()->insertWidget(2, m_nextSubPageButton);
	connect(m_nextSubPageButton, &QToolButton::clicked, m_textWidget->document(), &TeletextDocument::selectSubPageNext);

	m_cursorPositionLabel = new QLabel("1, 1");
	statusBar()->insertWidget(3, m_cursorPositionLabel);

	m_insertModePushButton = new QPushButton("OVERWRITE");
	m_insertModePushButton->setFlat(true);
	m_insertModePushButton->setFocusProxy(m_textWidget);
	statusBar()->addPermanentWidget(m_insertModePushButton);
	connect(m_insertModePushButton, &QPushButton::clicked, this, &MainWindow::toggleInsertMode);

	statusBar()->addPermanentWidget(new QLabel("Level"));

	m_levelRadioButton[0] = new QRadioButton("1");
	m_levelRadioButton[1] = new QRadioButton("1.5");
	m_levelRadioButton[2] = new QRadioButton("2.5");
	m_levelRadioButton[3] = new QRadioButton("3.5");
	for (int i=0; i<4; i++) {
		m_levelRadioButton[i]->setFocusPolicy(Qt::NoFocus);
		statusBar()->addPermanentWidget(m_levelRadioButton[i]);
	}
	m_levelRadioButton[0]->toggle();
	connect(m_levelRadioButton[0], &QAbstractButton::clicked, [=]() { m_textWidget->pageDecode()->setLevel(0); m_textWidget->update(); m_paletteDockWidget->setLevel3p5Accepted(false); });
	connect(m_levelRadioButton[1], &QAbstractButton::clicked, [=]() { m_textWidget->pageDecode()->setLevel(1); m_textWidget->update(); m_paletteDockWidget->setLevel3p5Accepted(false);});
	connect(m_levelRadioButton[2], &QAbstractButton::clicked, [=]() { m_textWidget->pageDecode()->setLevel(2); m_textWidget->update(); m_paletteDockWidget->setLevel3p5Accepted(false);});
	connect(m_levelRadioButton[3], &QAbstractButton::clicked, [=]() { m_textWidget->pageDecode()->setLevel(3); m_textWidget->update(); m_paletteDockWidget->setLevel3p5Accepted(true);});
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
	m_viewSmoothTransform = settings.value("smoothTransform", 0).toBool();
	m_smoothTransformAction->blockSignals(true);
	m_smoothTransformAction->setChecked(m_viewSmoothTransform);
	m_smoothTransformAction->blockSignals(false);
	m_viewZoom = settings.value("zoom", 2).toInt();
	m_viewZoom = (m_viewZoom < 0 || m_viewZoom > 12) ? 2 : m_viewZoom;

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
		m_pageComposeLinksDockWidget->hide();
		m_pageComposeLinksDockWidget->setFloating(true);
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
	settings.setValue("smoothTransform", m_viewSmoothTransform);
	settings.setValue("zoom", m_viewZoom);
}

bool MainWindow::maybeSave()
{
	if (m_textWidget->document()->undoStack()->isClean())
		return true;
	const QMessageBox::StandardButton ret = QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("The document \"%1\" has been modified.\nDo you want to save your changes or discard them?").arg(QFileInfo(m_curFile).fileName()), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
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
	int levelSeen;

	QFile file(fileName);
	const QFileInfo fileInfo(file);
	QIODevice::OpenMode fileOpenMode;

	if (fileInfo.suffix() == "t42")
		fileOpenMode = QFile::ReadOnly;
	else
		fileOpenMode = QFile::ReadOnly | QFile::Text;

	if (!file.open(fileOpenMode)) {
		QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("Cannot read file %1:\n%2.").arg(QDir::toNativeSeparators(fileName), file.errorString()));
		setCurrentFile(QString());
		return;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);

	if (fileInfo.suffix() == "t42") {
		importT42(&file, m_textWidget->document());
		m_exportAutoFileName = fileName;
	} else {
		loadTTI(&file, m_textWidget->document());
		m_exportAutoFileName.clear();
	}

	levelSeen = m_textWidget->document()->levelRequired();
	m_levelRadioButton[levelSeen]->toggle();
	m_textWidget->pageDecode()->setLevel(levelSeen);
	if (levelSeen == 3)
		m_paletteDockWidget->setLevel3p5Accepted(true);
	updatePageWidgets();

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

void MainWindow::updateExportAutoAction()
{
	if (m_exportAutoFileName.isEmpty()) {
		m_exportAutoAct->setText(tr("Export subpage..."));
		m_exportAutoAct->setEnabled(false);
		return;
	}

	m_exportAutoAct->setText(tr("Overwrite &%1").arg(MainWindow::strippedName(m_exportAutoFileName)));
	m_exportAutoAct->setEnabled(true);
}

void MainWindow::openRecentFile()
{
	if (const QAction *action = qobject_cast<const QAction *>(sender()))
		openFile(action->data().toString());
}

bool MainWindow::saveFile(const QString &fileName)
{
	QString errorMessage;

	QApplication::setOverrideCursor(Qt::WaitCursor);
	QSaveFile file(fileName);
	if (file.open(QFile::WriteOnly | QFile::Text)) {
		saveTTI(file, *m_textWidget->document());
		if (!file.commit())
			errorMessage = tr("Cannot write file %1:\n%2.").arg(QDir::toNativeSeparators(fileName), file.errorString());
	} else
		errorMessage = tr("Cannot open file %1 for writing:\n%2.").arg(QDir::toNativeSeparators(fileName), file.errorString());
	QApplication::restoreOverrideCursor();

	if (!errorMessage.isEmpty()) {
		QMessageBox::warning(this, QApplication::applicationDisplayName(), errorMessage);
		return false;
	}

	setCurrentFile(fileName);
	statusBar()->showMessage(tr("File saved"), 2000);
	return true;
}

void MainWindow::exportAuto()
{
	// Menu should be disabled if m_exportAutoFileName is empty, but just in case...
	if (m_exportAutoFileName.isEmpty())
		return;

	exportT42(true);
}

void MainWindow::exportT42(bool fromAuto)
{
	QString errorMessage;
	QString exportFileName;

	if (fromAuto)
		exportFileName = m_exportAutoFileName;
	else {
		exportFileName = m_curFile;
		changeSuffixFromTTI(exportFileName, "t42");

		exportFileName = QFileDialog::getSaveFileName(this, tr("Export t42"), exportFileName, "t42 stream (*.t42)");
		if (exportFileName.isEmpty())
			return;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);
	QSaveFile file(exportFileName);
	if (file.open(QFile::WriteOnly)) {
		exportT42File(file, *m_textWidget->document());
		if (!file.commit())
			errorMessage = tr("Cannot write file %1:\n%2.").arg(QDir::toNativeSeparators(exportFileName), file.errorString());
	} else
		errorMessage = tr("Cannot open file %1 for writing:\n%2.").arg(QDir::toNativeSeparators(exportFileName), file.errorString());
	QApplication::restoreOverrideCursor();

	if (!errorMessage.isEmpty()) {
		QMessageBox::warning(this, QApplication::applicationDisplayName(), errorMessage);
		return;
	}

	MainWindow::prependToRecentFiles(exportFileName);

	m_exportAutoFileName = exportFileName;
	statusBar()->showMessage(tr("File exported"), 2000);
}

void MainWindow::exportM29()
{
	QString errorMessage;
	QString exportFileName = m_curFile;

	if (m_isUntitled || !QFileInfo(m_curFile).exists())
		exportFileName = QString("P%1FF.tti").arg(m_textWidget->document()->pageNumber() >> 8, 1, 16);
	else {
		exportFileName = QFileInfo(m_curFile).fileName();
		// Suggest a new filename to avoid clobbering the original file
		if (QRegularExpression(("^[Pp]?[1-8][0-9A-Fa-f][0-9A-Fa-f]")).match(exportFileName).hasMatch()) {
			// Page number forms start of file name, change it to xFF
			if (exportFileName.at(0) == 'P' || exportFileName.at(0) == 'p') {
				exportFileName[2] = 'F';
				exportFileName[3] = 'F';
			} else {
				exportFileName[1] = 'F';
				exportFileName[2] = 'F';
			}
		// No page number at start of file name. Try to insert "-m29" while preserving .tti(x) suffix
		} else if (exportFileName.endsWith(".tti", Qt::CaseInsensitive)) {
			exportFileName.chop(4);
			exportFileName.append("-m29.tti");
		} else if (exportFileName.endsWith(".ttix", Qt::CaseInsensitive)) {
			exportFileName.chop(5);
			exportFileName.append("-m29.ttix");
		} else
			// Shouldn't get here, bit of a messy escape but still better than clobbering the original file
			exportFileName.append("-m29.tti");

		exportFileName = QDir(QFileInfo(m_curFile).absoluteDir()).filePath(exportFileName);
	}

	exportFileName = QFileDialog::getSaveFileName(this, tr("Export M/29 tti"), exportFileName, "TTI teletext page (*.tti *.ttix)");
	if (exportFileName.isEmpty())
		return;

	QApplication::setOverrideCursor(Qt::WaitCursor);
	QSaveFile file(exportFileName);
	if (file.open(QFile::WriteOnly | QFile::Text)) {
		exportM29File(file, *m_textWidget->document());
		if (!file.commit())
			errorMessage = tr("Cannot write file %1:\n%2.").arg(QDir::toNativeSeparators(exportFileName), file.errorString());
	} else
		errorMessage = tr("Cannot open file %1 for writing:\n%2.").arg(QDir::toNativeSeparators(exportFileName), file.errorString());
	QApplication::restoreOverrideCursor();

	if (!errorMessage.isEmpty())
		QMessageBox::warning(this, QApplication::applicationDisplayName(), errorMessage);
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
	QString result;
	result.reserve(19);

	if (m_textWidget->document()->cursorRow() == 0)
		result = QString("Header, %1").arg(m_textWidget->document()->cursorColumn());
	else if (m_textWidget->document()->cursorRow() == 24)
		result = QString("FLOF, %1").arg(m_textWidget->document()->cursorColumn());
	else
		result = QString("%1, %2").arg(m_textWidget->document()->cursorRow()).arg(m_textWidget->document()->cursorColumn());

	if (m_textWidget->document()->selectionActive())
		result.append(QString(" (%1, %2)").arg(m_textWidget->document()->selectionHeight()).arg(m_textWidget->document()->selectionWidth()));

	m_cursorPositionLabel->setText(result);
	m_textScene->updateCursor();
	m_textView->ensureVisible(m_textScene->cursorRectItem(), 16, 24);
}

void MainWindow::updatePageWidgets()
{
	m_subPageLabel->setText(QString("%1/%2").arg(m_textWidget->document()->currentSubPageIndex()+1).arg(m_textWidget->document()->numberOfSubPages()));
	m_previousSubPageButton->setEnabled(!(m_textWidget->document()->currentSubPageIndex() == 0));
	m_nextSubPageButton->setEnabled(!(m_textWidget->document()->currentSubPageIndex() == (m_textWidget->document()->numberOfSubPages()) - 1));
	updateCursorPosition();
	m_deleteSubPageAction->setEnabled(m_textWidget->document()->numberOfSubPages() > 1);
	m_pageOptionsDockWidget->updateWidgets();
	m_pageEnhancementsDockWidget->updateWidgets();
	m_x26DockWidget->loadX26List();
	m_paletteDockWidget->updateAllColourButtons();
	m_pageComposeLinksDockWidget->updateWidgets();
}
