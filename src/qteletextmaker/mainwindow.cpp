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

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileSystemWatcher>
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
#include <QSlider>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QVariant>
#include <iostream>

#include "mainwindow.h"

#include "drcspage.h"
#include "hashformats.h"
#include "levelonecommands.h"
#include "loadformats.h"
#include "mainwidget.h"
#include "pagecomposelinksdockwidget.h"
#include "pageenhancementsdockwidget.h"
#include "pageoptionsdockwidget.h"
#include "palettedockwidget.h"
#include "saveformats.h"
#include "x26dockwidget.h"

#include "gifimage/qgifimage.h"

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
	const QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QString(), m_loadFormats.filters());
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
	// If imported from a format we only export, force "Save As" so we don't clobber the original imported file
	if (m_isUntitled || m_saveFormats.isExportOnly(QFileInfo(m_curFile).suffix()))
		return saveAs();
	else
		return saveFile(m_curFile);
}

bool MainWindow::saveAs()
{
	QString suggestedName = m_curFile;

	// If imported from a format we only export, change suffix so we don't clobber the original imported file
	if (m_saveFormats.isExportOnly(QFileInfo(suggestedName).suffix())) {
		const int pos = suggestedName.lastIndexOf(QChar('.'));
		if (pos != -1)
			suggestedName.truncate(pos);

		suggestedName.append(".tti");
	}

	// Set the filter in the file dialog to the same as the current filename extension
	QString dialogFilter;

	SaveFormat *savingFormat = m_saveFormats.findExportFormat(QFileInfo(suggestedName).suffix());

	if (savingFormat != nullptr)
		dialogFilter = savingFormat->fileDialogFilter();

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), suggestedName, m_saveFormats.filters(), &dialogFilter);
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

void MainWindow::extractImages(QImage sceneImage[], bool smooth, bool flashExtract)
{
	// Prepare widget image for extraction
	m_textScene->hideGUIElements(true);
	// Disable exporting in Mix mode as it corrupts the background
	bool reMix = m_textWidget->pageRender()->renderMode() == TeletextPageRender::RenderMix;
	if (reMix)
		m_textScene->setRenderMode(TeletextPageRender::RenderNormal);

	const int flashTiming = flashExtract ? m_textWidget->flashTiming() : 0;

	// Allocate initial image, with additional images for flashing if necessary
	QImage interImage[6];

	interImage[0] = QImage(m_textScene->sceneRect().size().toSize(), QImage::Format_RGB32);
	if (flashTiming != 0) {
		interImage[3] = QImage(m_textScene->sceneRect().size().toSize(), QImage::Format_RGB32);
		if (flashTiming == 2) {
			interImage[1] = QImage(m_textScene->sceneRect().size().toSize(), QImage::Format_RGB32);
			interImage[2] = QImage(m_textScene->sceneRect().size().toSize(), QImage::Format_RGB32);
			interImage[4] = QImage(m_textScene->sceneRect().size().toSize(), QImage::Format_RGB32);
			interImage[5] = QImage(m_textScene->sceneRect().size().toSize(), QImage::Format_RGB32);
		}
	}

	// Now extract the image(s) from the scene
	for (int p=0; p<6; p++)
		if (!interImage[p].isNull()) {
			m_textWidget->pauseFlash(p);
			//	This ought to make the background transparent in Mix mode, but it doesn't
			//	if (m_textWidget->pageDecode()->mix())
			//		interImage.fill(QColor(0, 0, 0, 0));
			QPainter interPainter(&interImage[p]);
			m_textScene->render(&interPainter);
		}

	// Now we've extracted the image we can put the GUI things back
	m_textScene->hideGUIElements(false);
	if (reMix)
		m_textScene->setRenderMode(TeletextPageRender::RenderMix);
	m_textWidget->resumeFlash();

	// Now scale the extracted image(s) to the selected aspect ratio
	for (int p=0; p<6; p++)
		if (!interImage[p].isNull()) {
			if (m_viewAspectRatio == 3)
				// Aspect ratio is Pixel 1:2 so we only need to double the vertical height
				sceneImage[p] = interImage[p].scaled(interImage[p].width(), interImage[p].height()*2, Qt::IgnoreAspectRatio, Qt::FastTransformation);
			else {
				// Scale the image in two steps so that smoothing only occurs on vertical lines
				// Double the vertical height first
				const QImage doubleHeightImage = interImage[p].scaled(interImage[p].width(), interImage[p].height()*2, Qt::IgnoreAspectRatio, Qt::FastTransformation);
				// then scale it horizontally to the selected aspect ratio
				sceneImage[p] = doubleHeightImage.scaled((int)((float)doubleHeightImage.width() * aspectRatioHorizontalScaling[m_viewAspectRatio] * 2), doubleHeightImage.height(), Qt::IgnoreAspectRatio, (smooth) ? Qt::SmoothTransformation : Qt::FastTransformation);
			}
		}
}

void MainWindow::exportImage()
{
	QString exportFileName, selectedFilter, gifFilter;

	if (!m_exportImageFileName.isEmpty())
		exportFileName = m_exportImageFileName;
	else {
		// Image not exported before: suggest a filename with image extension
		// If page has flashing, suggest GIF
		exportFileName = m_curFile.left(m_curFile.lastIndexOf('.'));
		exportFileName.append(m_textWidget->flashTiming() != 0 ? ".gif" : ".png");
	}

	if (m_textWidget->flashTiming() != 0)
		gifFilter = "Animated GIF image (*.gif)";
	else
		gifFilter = "GIF image (*.gif)";

	// Set the filter in the file dialog to the same as the current filename extension
	if (QFileInfo(exportFileName).suffix().toLower() == "gif")
		selectedFilter = gifFilter;
	else
		selectedFilter = "PNG image (*.png)";

	exportFileName = QFileDialog::getSaveFileName(this, tr("Export image"), exportFileName, "PNG image (*.png);;" + gifFilter, &selectedFilter);
	if (exportFileName.isEmpty())
		return;

	const QString suffix = QFileInfo(exportFileName).suffix().toLower();

	if (suffix.isEmpty()) {
		QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("No filename extension specified."));
		return;
	} else if (suffix != "png" && suffix != "gif") {
		QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("Cannot export image of format %1.").arg(suffix));
		return;
	}

	QImage scaledImage[6];
	extractImages(scaledImage, suffix != "gif", suffix == "gif");

	if (suffix == "png") {
		if (scaledImage[0].save(exportFileName, "PNG"))
			m_exportImageFileName = exportFileName;
		else
			QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("Cannot export image %1.").arg(QDir::toNativeSeparators(exportFileName)));
	}

	if (suffix == "gif") {
		QGifImage gif(scaledImage[0].size());

		if (scaledImage[3].isNull())
			// No flashing
			gif.addFrame(scaledImage[0], 0);
		else if (scaledImage[1].isNull()) {
			// 1Hz flashing
			gif.addFrame(scaledImage[0], 500);
			gif.addFrame(scaledImage[3], 500);
		} else
			// 2Hz flashing
			for (int p=0; p<6; p++)
				gif.addFrame(scaledImage[p], (p % 3 == 0) ? 166 : 167);

		if (gif.save(exportFileName))
			m_exportImageFileName = exportFileName;
		else
			QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("Cannot export image %1.").arg(QDir::toNativeSeparators(exportFileName)));
	}
}

#ifndef QT_NO_CLIPBOARD
void MainWindow::imageToClipboard()
{
	QImage scaledImage[1];
	QClipboard *clipboard = QApplication::clipboard();

	extractImages(scaledImage, true);
	clipboard->setImage(scaledImage[0]);
}
#endif // !QT_NO_CLIPBOARD

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
	"Copyright (C) 2020-2025 Gavin MacGregor<br><br>"
	"Released under the GNU General Public License version 3<br>"
	"<a href=\"https://github.com/gkthemac/qteletextmaker\">https://github.com/gkthemac/qteletextmaker</a>").arg(QApplication::applicationDisplayName()).arg(QApplication::applicationVersion()));
}

void MainWindow::init()
{
	setAttribute(Qt::WA_DeleteOnClose);

	m_isUntitled = true;
	m_reExportWarning = false;

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
	m_zoomSlider->setValue(m_viewZoom);
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

	connect(&m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::updateWatchedFile);

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

	QAction *exportFileAct = fileMenu->addAction(tr("Export subpage as..."));
	exportFileAct->setStatusTip("Export this subpage to various formats");
	connect(exportFileAct, &QAction::triggered, this, [=]() { exportFile(false); });

	QMenu *exportHashStringSubMenu = fileMenu->addMenu(tr("Export subpage to online editor"));

	QAction *exportZXNetAct = exportHashStringSubMenu->addAction(tr("Open in zxnet.co.uk"));
	exportZXNetAct->setStatusTip("Export and open this subpage in the zxnet.co.uk online editor");
	connect(exportZXNetAct, &QAction::triggered, this, &MainWindow::exportZXNet);

	QAction *exportEditTFAct = exportHashStringSubMenu->addAction(tr("Open in edit.tf"));
	exportEditTFAct->setStatusTip("Export and open this subpage in the edit.tf online editor");
	connect(exportEditTFAct, &QAction::triggered, this, &MainWindow::exportEditTF);

	QAction *exportImageAct = fileMenu->addAction(tr("Export subpage as image..."));
	exportImageAct->setStatusTip("Export an image of this subpage");
	connect(exportImageAct, &QAction::triggered, this, &MainWindow::exportImage);

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

	QAction *copyImageAct = editMenu->addAction(tr("Copy as image"));
	copyImageAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
	copyImageAct->setStatusTip(tr("Copy this subpage as an image to the clipboard"));
	connect(copyImageAct, &QAction::triggered, this, &MainWindow::imageToClipboard);

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

	editMenu->addSeparator();

	m_rowZeroAct = editMenu->addAction(tr("Edit header row"));
	m_rowZeroAct->setCheckable(true);
	m_rowZeroAct->setStatusTip(tr("Allow editing of header row"));
	connect(m_rowZeroAct, &QAction::toggled, m_textScene, &LevelOneScene::toggleRowZeroAllowed);

	QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

	QAction *revealAct = viewMenu->addAction(tr("&Reveal"));
	revealAct->setCheckable(true);
	revealAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
	revealAct->setStatusTip(tr("Toggle reveal"));
	connect(revealAct, &QAction::toggled, m_textWidget, &TeletextWidget::setReveal);

	QAction *gridAct = viewMenu->addAction(tr("&Grid"));
	gridAct->setCheckable(true);
	gridAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
	gridAct->setStatusTip(tr("Toggle the text grid"));
	connect(gridAct, &QAction::toggled, m_textScene, &LevelOneScene::toggleGrid);

	QAction *showControlCodesAct = viewMenu->addAction(tr("Control codes"));
	showControlCodesAct->setCheckable(true);
	showControlCodesAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
	showControlCodesAct->setStatusTip(tr("Toggle showing of control codes"));
	connect(showControlCodesAct, &QAction::toggled, m_textWidget, &TeletextWidget::setShowControlCodes);

	viewMenu->addSeparator();

	QMenu *renderModeSubMenu = viewMenu->addMenu(tr("Render mode"));
	QAction *renderModeActs[4];
	renderModeActs[0] = renderModeSubMenu->addAction(tr("Normal"));
	renderModeActs[0]->setStatusTip(tr("Render page normally"));
	renderModeActs[1] = renderModeSubMenu->addAction(tr("Mix"));
	renderModeActs[1]->setStatusTip(tr("Render page in mix mode"));
	renderModeActs[2] = renderModeSubMenu->addAction(tr("White on black"));
	renderModeActs[2]->setStatusTip(tr("Render page with white foreground on black background"));
	renderModeActs[3] = renderModeSubMenu->addAction(tr("Black on white"));
	renderModeActs[3]->setStatusTip(tr("Render page with black foreground on white background"));

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

	QActionGroup *renderModeGroup = new QActionGroup(this);
	QActionGroup *borderGroup = new QActionGroup(this);
	QActionGroup *aspectRatioGroup = new QActionGroup(this);
	for (int i=0; i<=3; i++) {
		renderModeActs[i]->setCheckable(true);
		connect(renderModeActs[i], &QAction::triggered, [=]() { m_textScene->setRenderMode(static_cast<TeletextPageRender::RenderMode>(i)); });
		renderModeGroup->addAction(renderModeActs[i]);

		m_aspectRatioActs[i]->setCheckable(true);
		connect(m_aspectRatioActs[i], &QAction::triggered, [=]() { setAspectRatio(i); });
		aspectRatioGroup->addAction(m_aspectRatioActs[i]);

		if (i == 3)
			break;

		m_borderActs[i]->setCheckable(true);
		connect(m_borderActs[i], &QAction::triggered, [=]() { setBorder(i); });
		borderGroup->addAction(m_borderActs[i]);
	}
	renderModeActs[0]->setChecked(true);

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

	viewMenu->addSeparator();

	QMenu *drcsSubMenu = viewMenu->addMenu(tr("DRCS pages"));
	// Apparently Qt on non-Unix GUIs won't show text on separators or sections
	// so use a disabled menu entry instead
#ifndef Q_OS_UNIX
	m_drcsSection[1] = drcsSubMenu->addAction("Global DRCS");
	m_drcsSection[1]->setEnabled(false);
#else
	m_drcsSection[1] = drcsSubMenu->addSection("Global DRCS");
#endif
	QAction *gDrcsFileSelect = drcsSubMenu->addAction(tr("Load file..."));
	gDrcsFileSelect->setStatusTip(tr("Load a file to use for Global DRCS definitions"));
	connect(gDrcsFileSelect, &QAction::triggered, [=]() { loadDRCSFile(1); });
	m_drcsClear[1] = drcsSubMenu->addAction(tr("Clear"));
	m_drcsClear[1]->setStatusTip(tr("Clear Global DRCS definitions"));
	m_drcsClear[1]->setEnabled(false);
	connect(m_drcsClear[1], &QAction::triggered, [=]() { clearDRCSFile(1); });

#ifndef Q_OS_UNIX
	QAction *separator = drcsSubMenu->addSeparator();
	m_drcsSection[0] = drcsSubMenu->addAction("Normal DRCS");
	m_drcsSection[0]->setEnabled(false);
#else
	m_drcsSection[0] = drcsSubMenu->addSection("Normal DRCS");
#endif
	QAction *nDrcsFileSelect = drcsSubMenu->addAction(tr("Load file..."));
	nDrcsFileSelect->setStatusTip(tr("Load a file to use for Normal DRCS definitions"));
	connect(nDrcsFileSelect, &QAction::triggered, [=]() { loadDRCSFile(0); });
	m_drcsClear[0] = drcsSubMenu->addAction(tr("Clear"));
	m_drcsClear[0]->setStatusTip(tr("Clear Normal DRCS definitions"));
	m_drcsClear[0]->setEnabled(false);
	connect(m_drcsClear[0], &QAction::triggered, [=]() { clearDRCSFile(0); });

	drcsSubMenu->addSeparator();
	m_drcsSwap = drcsSubMenu->addAction(tr("Swap Global and Normal"));
	m_drcsSwap->setStatusTip(tr("Swap the files used for Global and Normal DRCS definitions"));
	m_drcsSwap->setEnabled(false);
	connect(m_drcsSwap, &QAction::triggered, this, &MainWindow::swapDRCS);

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
	if (m_viewZoom < 4) {
		m_viewZoom++;
		m_zoomSlider->setValue(m_viewZoom);
	} else if (m_viewZoom < 12) {
		m_viewZoom += 2;
		m_zoomSlider->setValue(m_viewZoom / 2 + 2);
	}
}

void MainWindow::zoomOut()
{
	if (m_viewZoom > 4) {
		m_viewZoom -= 2;
		m_zoomSlider->setValue(m_viewZoom == 4 ? 4 : m_viewZoom / 2 + 2);
	} else if (m_viewZoom > 0) {
		m_viewZoom--;
		m_zoomSlider->setValue(m_viewZoom);
	}
}

void MainWindow::zoomSet(int viewZoom)
{
	m_viewZoom = (viewZoom < 5) ? viewZoom : (viewZoom - 2) * 2;
	setSceneDimensions();
}

void MainWindow::zoomReset()
{
	m_viewZoom = 2;
	m_zoomSlider->setValue(2);
}

void MainWindow::loadDRCSFile(int drcsType, QString fileName)
{
	const QString drcsTypeName = drcsType == 1 ? "Global DRCS" : "Normal DRCS";

	const bool updatingWatched = !fileName.isEmpty();

	if (!updatingWatched)
		fileName = QFileDialog::getOpenFileName(this, tr("Select %1 file").arg(drcsTypeName), m_drcsFileName[drcsType], m_loadFormats.filters());

	if (!fileName.isEmpty()) {
		QFile file(fileName);

		LoadFormat *loadingFormat = m_loadFormats.findFormat(QFileInfo(fileName).suffix());
		if (loadingFormat == nullptr) {
			if (updatingWatched)
				clearDRCSFile(drcsType);
			else
				QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("Cannot load file %1:\nUnknown file format or extension").arg(QDir::toNativeSeparators(fileName)));

			return;
		}

		if (!file.open(QFile::ReadOnly)) {
			if (updatingWatched)
				clearDRCSFile(drcsType);
			else
				QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("Cannot read file %1:\n%2.").arg(QDir::toNativeSeparators(fileName), file.errorString()));

			return;
		}

		QList<PageBase> loadedPages;

		if (loadingFormat->load(&file, loadedPages, nullptr)) {
			if (!m_drcsFileName[drcsType].isEmpty())
				m_fileWatcher.removePath(m_drcsFileName[drcsType]);

			m_textWidget->pageDecode()->clearDRCSPage((TeletextPageDecode::DRCSPageType)drcsType);
			m_drcsPage[drcsType].clear();

			for (int i=0; i<loadedPages.size(); i++)
				m_drcsPage[drcsType].append(loadedPages.at(i));

			m_textWidget->pageDecode()->setDRCSPage((TeletextPageDecode::DRCSPageType)drcsType, &m_drcsPage[drcsType]);
			m_textWidget->refreshPage();

			m_fileWatcher.addPath(fileName);
			m_drcsFileName[drcsType] = fileName;
			m_drcsSection[drcsType]->setText(QString("%1: %2").arg(drcsTypeName).arg(QFileInfo(fileName).fileName()));
			m_drcsClear[drcsType]->setEnabled(true);
			m_drcsSwap->setEnabled(true);
		} else {
			if (updatingWatched)
				clearDRCSFile(drcsType);
			else
				QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("Cannot load file %1\n%2").arg(QDir::toNativeSeparators(fileName), loadingFormat->errorString()));

			return;
		}
	}
}

void MainWindow::clearDRCSFile(int drcsType)
{
	m_fileWatcher.removePath(m_drcsFileName[drcsType]);

	m_textWidget->pageDecode()->clearDRCSPage((TeletextPageDecode::DRCSPageType)drcsType);
	m_drcsPage[drcsType].clear();

	m_textWidget->refreshPage();

	m_drcsFileName[drcsType].clear();
	m_drcsSection[drcsType]->setText(drcsType == 1 ? "Global DRCS" : "Normal DRCS");
	m_drcsClear[drcsType]->setEnabled(false);
	m_drcsSwap->setEnabled(m_drcsClear[0]->isEnabled() || m_drcsClear[1]->isEnabled());
}

void MainWindow::swapDRCS()
{
	m_drcsPage[0].swap(m_drcsPage[1]);
	m_drcsFileName[0].swap(m_drcsFileName[1]);

	for (int i=0; i<2; i++) {
		const QString drcsTypeName = i == 1 ? "Global DRCS" : "Normal DRCS";

		if (m_drcsPage[i].isEmpty()) {
			m_textWidget->pageDecode()->clearDRCSPage((TeletextPageDecode::DRCSPageType)i);
			m_drcsSection[i]->setText(drcsTypeName);
		} else {
			m_textWidget->pageDecode()->setDRCSPage((TeletextPageDecode::DRCSPageType)i, &m_drcsPage[i]);
			m_drcsSection[i]->setText(QString("%1: %2").arg(drcsTypeName).arg(QFileInfo(m_drcsFileName[i]).fileName()));
		}

		m_drcsClear[i]->setEnabled(!m_drcsPage[i].isEmpty());
	}

	m_textWidget->refreshPage();
}

void MainWindow::updateWatchedFile(const QString &path)
{
	int drcsType;

	if (path == m_drcsFileName[1])
		drcsType = 1;
	else if (path == m_drcsFileName[0])
		drcsType = 0;
	else
		return;

	loadDRCSFile(drcsType, path);
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

	m_zoomSlider = new QSlider;
	m_zoomSlider->setOrientation(Qt::Horizontal);
	m_zoomSlider->setMinimumHeight(m_subPageLabel->height());
	m_zoomSlider->setMaximumHeight(m_subPageLabel->height());
	m_zoomSlider->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
	m_zoomSlider->setRange(0, 8);
	m_zoomSlider->setPageStep(1);
	m_zoomSlider->setFocusPolicy(Qt::NoFocus);
	statusBar()->insertWidget(4, m_zoomSlider);
	connect(m_zoomSlider, &QSlider::valueChanged, this, &MainWindow::zoomSet);

	m_insertModePushButton = new QPushButton("OVERWRITE");
	m_insertModePushButton->setFlat(true);
	m_insertModePushButton->setMinimumHeight(m_subPageLabel->height());
	m_insertModePushButton->setMaximumHeight(m_subPageLabel->height());
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
}

void MainWindow::readSettings()
{
	//TODO window sizing
	QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
	const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
	const QByteArray windowState = settings.value("windowState", QByteArray()).toByteArray();

	m_viewBorder = settings.value("border", 1).toInt();
	m_viewBorder = (m_viewBorder < 0 || m_viewBorder > 2) ? 1 : m_viewBorder;
	m_borderActs[m_viewBorder]->setChecked(true);
	m_viewAspectRatio = settings.value("aspectratio", 0).toInt();
	m_viewAspectRatio = (m_viewAspectRatio < 0 || m_viewAspectRatio > 3) ? 0 : m_viewAspectRatio;
	m_aspectRatioActs[m_viewAspectRatio]->setChecked(true);
	m_viewSmoothTransform = settings.value("smoothTransform", 0).toBool();
	m_smoothTransformAction->blockSignals(true);
	m_smoothTransformAction->setChecked(m_viewSmoothTransform);
	m_smoothTransformAction->blockSignals(false);
	m_viewZoom = settings.value("zoom", 2).toInt();
	m_viewZoom = (m_viewZoom < 0 || m_viewZoom > 12) ? 2 : m_viewZoom;

	// zoom 0 = 430,385px, 1 = 500,530px, 2 = 650,670px
	if (geometry.isEmpty()) {
		const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
		if (availableGeometry.width() < 500 || availableGeometry.height() < 530) {
			resize(430, 385);
			m_viewZoom = 0;
		} else if (availableGeometry.width() < 650 || availableGeometry.height() < 670) {
			resize(500, 530);
			m_viewZoom = 1;
		} else
			resize(650, 670);
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

	LoadFormat *loadingFormat = m_loadFormats.findFormat(QFileInfo(fileName).suffix());
	if (loadingFormat == nullptr) {
		QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("Cannot load file %1:\nUnknown file format or extension").arg(QDir::toNativeSeparators(fileName)));
		setCurrentFile(QString());

		return;
	}

	if (!file.open(QFile::ReadOnly)) {
		QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("Cannot read file %1:\n%2.").arg(QDir::toNativeSeparators(fileName), file.errorString()));
		setCurrentFile(QString());

		return;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);

	QList<PageBase> subPages;
	QVariantHash metadata;

	if (loadingFormat->load(&file, subPages, &metadata)) {
		m_textWidget->document()->loadFromList(subPages);
		m_textWidget->document()->loadMetaData(metadata);

		if (m_saveFormats.isExportOnly(QFileInfo(file).suffix()))
			m_exportAutoFileName = fileName;
		else
			m_exportAutoFileName.clear();
	} else {
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("Cannot load file %1\n%2").arg(QDir::toNativeSeparators(fileName), loadingFormat->errorString()));
		setCurrentFile(QString());

		return;
	}

	levelSeen = m_textWidget->document()->levelRequired();
	m_levelRadioButton[levelSeen]->toggle();
	m_textWidget->pageDecode()->setLevel(levelSeen);
	if (levelSeen == 3)
		m_paletteDockWidget->setLevel3p5Accepted(true);
	updatePageWidgets();

	QApplication::restoreOverrideCursor();

	if (!loadingFormat->warningStrings().isEmpty())
		QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("The following issues were encountered when loading<br>%1:<ul><li>%2</li></ul>").arg(QDir::toNativeSeparators(fileName), loadingFormat->warningStrings().join("</li><li>")));

	m_reExportWarning = loadingFormat->reExportWarning();

	for (int i=0; i<m_textWidget->document()->numberOfSubPages(); i++)
		if (m_textWidget->document()->subPage(i)->packetExists(0)) {
			m_rowZeroAct->setChecked(true);
			break;
		}

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

	SaveFormat *savingFormat = m_saveFormats.findFormat(QFileInfo(fileName).suffix());
	if (savingFormat == nullptr) {
		QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("Cannot save file %1:\nUnknown file format or extension").arg(QDir::toNativeSeparators(fileName)));
		return false;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);

	QSaveFile file(fileName);
	if (file.open(QFile::WriteOnly)) {
		savingFormat->saveAllPages(file, *m_textWidget->document());

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

	exportFile(true);
}

void MainWindow::exportFile(bool fromAuto)
{
	QString errorMessage;
	QString exportFileName;
	SaveFormat *exportFormat = nullptr;

	if (fromAuto) {
		if (m_reExportWarning) {
			QMessageBox::StandardButton ret = QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("Will not overwrite imported file %1:\nAll other subpages in that file would be erased.\nPlease save or export to a different file.").arg(strippedName(m_exportAutoFileName)));

			return;
		}
		exportFileName = m_exportAutoFileName;
	} else {
		if (m_exportAutoFileName.isEmpty())
			exportFileName = m_curFile;
		else
			exportFileName = m_exportAutoFileName;

		// Set the filter in the file dialog to the same as the current filename extension
		QString dialogFilter;

		exportFormat = m_saveFormats.findExportFormat(QFileInfo(exportFileName).suffix());

		if (exportFormat != nullptr)
			dialogFilter = exportFormat->fileDialogFilter();

		exportFileName = QFileDialog::getSaveFileName(this, tr("Export subpage"), exportFileName, m_saveFormats.exportFilters(), &dialogFilter);
		if (exportFileName.isEmpty())
			return;
	}

	exportFormat = m_saveFormats.findExportFormat(QFileInfo(exportFileName).suffix());
	if (exportFormat == nullptr) {
		QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("Cannot export file %1:\nUnknown file format or extension").arg(QDir::toNativeSeparators(exportFileName)));
		return;
	}

	if (!fromAuto && exportFormat->getWarnings(*m_textWidget->document()->currentSubPage())) {
		const QMessageBox::StandardButton ret = QMessageBox::warning(this, QApplication::applicationDisplayName(), tr("The following issues will be encountered when exporting<br>%1:<ul><li>%2</li></ul>Do you want to export?").arg(strippedName(exportFileName)).arg(exportFormat->warningStrings().join("</li><li>")), QMessageBox::Yes | QMessageBox::No);

		if (ret != QMessageBox::Yes)
			return;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);

	QSaveFile file(exportFileName);

	if (file.open(QFile::WriteOnly)) {
		exportFormat->saveCurrentSubPage(file, *m_textWidget->document());

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
	m_reExportWarning = false;
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

	exportFileName = QFileDialog::getSaveFileName(this, tr("Export M/29 tti"), exportFileName, "MRG Systems TTI (*.tti *.ttix)");
	if (exportFileName.isEmpty())
		return;

	QApplication::setOverrideCursor(Qt::WaitCursor);

	QSaveFile file(exportFileName);
	if (file.open(QFile::WriteOnly)) {
		SaveM29Format saveM29Format;

		saveM29Format.saveCurrentSubPage(file, *m_textWidget->document());

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
