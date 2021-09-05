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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCheckBox>
#include <QComboBox>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>

#include "mainwidget.h"
#include "pageenhancementsdockwidget.h"
#include "pageoptionsdockwidget.h"
#include "palettedockwidget.h"
#include "x26dockwidget.h"

class QAction;
class QMenu;
class TeletextWidget;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();
	explicit MainWindow(const QString &fileName);

	void tile(const QMainWindow *previous);

protected:
	void closeEvent(QCloseEvent *event) override;

private slots:
	void newFile();
	void open();
	bool save();
	bool saveAs();
	void exportT42();
	void exportZXNet();
	void exportEditTF();
	void exportPNG();
	void updateRecentFileActions();
	void openRecentFile();
	void about();
	void updatePageWidgets();
	void updateCursorPosition();

	void insertRow(bool);
	void deleteRow();
	void insertSubPage(bool, bool);
	void deleteSubPage();

	void setSceneDimensions();
	void setBorder(int);
	void setAspectRatio(int);
	void setSmoothTransform(bool);
	void zoomIn();
	void zoomOut();
	void zoomReset();

	void toggleInsertMode();

private:
	enum { m_MaxRecentFiles = 10 };
	const float aspectRatioHorizontalScaling[4] = { 0.6, 0.6, 0.8, 0.5 };

	void init();
	void createActions();
	void createStatusBar();
	void readSettings();
	void writeSettings();
	bool maybeSave();
	void openFile(const QString &fileName);
	void loadFile(const QString &fileName);
	static bool hasRecentFiles();
	void prependToRecentFiles(const QString &fileName);
	void setRecentFilesVisible(bool visible);
	bool saveFile(const QString &fileName);
	void setCurrentFile(const QString &fileName);
	static QString strippedName(const QString &fullFileName);
	MainWindow *findMainWindow(const QString &fileName) const;

	TeletextWidget *m_textWidget;
	LevelOneScene *m_textScene;
	QGraphicsView *m_textView;

	int m_viewBorder, m_viewAspectRatio, m_viewZoom;
	bool m_viewSmoothTransform;
	PageOptionsDockWidget *m_pageOptionsDockWidget;
	PageEnhancementsDockWidget *m_pageEnhancementsDockWidget;
	X26DockWidget *m_x26DockWidget;
	PaletteDockWidget *m_paletteDockWidget;

	QAction *m_recentFileActs[m_MaxRecentFiles];
	QAction *m_recentFileSeparator;
	QAction *m_recentFileSubMenuAct;
	QAction *m_deleteSubPageAction;
	QAction *m_borderActs[3];
	QAction *m_aspectRatioActs[4];
	QAction *m_smoothTransformAction;

	QLabel *m_subPageLabel, *m_cursorPositionLabel;
	QToolButton *m_previousSubPageButton, *m_nextSubPageButton;
	QPushButton *m_insertModePushButton;
	QRadioButton *m_levelRadioButton[4];

	QString m_curFile;
	bool m_isUntitled;
};

#endif
