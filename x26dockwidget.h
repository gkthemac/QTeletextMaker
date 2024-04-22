/*
 * Copyright (C) 2020-2024 Gavin MacGregor
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

#ifndef X26DOCKWIDGET_H
#define X26DOCKWIDGET_H

#include <QAbstractListModel>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QStackedLayout>
#include <QTableView>

#include "mainwidget.h"
#include "render.h"
#include "x26menus.h"
#include "x26model.h"

class CharacterListModel : public QAbstractListModel
{
	Q_OBJECT

public:
	CharacterListModel(QObject *parent = 0);

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	void setCharacterSet(int characterSet);

private:
	TeletextFontBitmap m_fontBitmap;
	int m_characterSet;
};

class X26DockWidget : public QDockWidget
{
	Q_OBJECT

public:
	X26DockWidget(TeletextWidget *parent);

public slots:
	void insertTriplet(int modeExt, bool after);
	void insertTripletCopy();
	void deleteTriplet();
	void customMenuRequested(QPoint pos);
	void loadX26List();
	void unloadX26List();
	void rowSelected(const QModelIndex &current, const QModelIndex &previous);
	void updateAllRawTripletSpinBoxes(const QModelIndex &index);
	void updateRawTripletDataSpinBox(const QModelIndex &index);
	void updateAllCookedTripletWidgets(const QModelIndex & index);
	void rawTripletAddressSpinBoxChanged(int value);
	void rawTripletModeSpinBoxChanged(int value);
	void rawTripletDataSpinBoxChanged(int value);
	void cookedRowSpinBoxChanged(const int value);
	void cookedColumnSpinBoxChanged(const int value);
	void cookedModeMenuSelected(const int value);
	void updateModelFromCookedWidget(const int value, const int role);
	void selectX26ListRow(int row);

protected:
	void keyPressEvent(QKeyEvent *event) override;
	CharacterListModel m_characterListModel;

private:
	QTableView *m_x26View;
	X26Model *m_x26Model;
	QSpinBox *m_cookedRowSpinBox, *m_cookedColumnSpinBox;
	QMenu *m_cookedModeMenu, *m_insertBeforeMenu, *m_insertAfterMenu;
	QPushButton *m_cookedModePushButton;
	QSpinBox *m_rawTripletAddressSpinBox, *m_rawTripletModeSpinBox, *m_rawTripletDataSpinBox;
	QStackedLayout *m_rawOrCookedStackedLayout;
	QComboBox *m_colourComboBox;
	QRadioButton *m_fullRowColourThisRowOnlyRadioButton, *m_fullRowColourDownToBottomRadioButton;
	QComboBox *m_characterCodeComboBox;
	QComboBox *m_flashModeComboBox, *m_flashRateComboBox;
	QComboBox *m_textSizeComboBox;
	QCheckBox *m_displayAttributeBoxingCheckBox, *m_displayAttributeConcealCheckBox, *m_displayAttributeInvertCheckBox, *m_displayAttributeUnderlineCheckBox;
	QComboBox *m_objectSourceComboBox, *m_objectRequiredAtLevelsComboBox;
	QLabel *m_invokeLocalObjectDesignationCodeLabel, *m_invokeLocalObjectTripletNumberLabel;
	QSpinBox *m_invokeLocalObjectDesignationCodeSpinBox, *m_invokeLocalObjectTripletNumberSpinBox;
	QSpinBox *m_invokePOPSubPageSpinBox, *m_invokePOPPacketNumberSpinBox;
	QComboBox *m_invokePOPTripletNumberComboBox, *m_invokePOPPointerBitsComboBox;
	QStackedLayout *m_invokeObjectSourceStackedLayout, *m_tripletParameterStackedLayout;
	QComboBox *m_DRCSModeRequiredAtLevelsComboBox;
	QRadioButton *m_DRCSModeGlobalRadioButton, *m_DRCSModeNormalRadioButton;
	QSpinBox *m_DRCSModeSubPageSpinBox;
	QRadioButton *m_DRCSCharacterGlobalRadioButton, *m_DRCSCharacterNormalRadioButton;
	QSpinBox *m_DRCSCharacterCodeSpinBox;
	QCheckBox *m_fontStyleProportionalCheckBox, *m_fontStyleBoldCheckBox, *m_fontStyleItalicCheckBox;
	QSpinBox *m_fontStyleRowsSpinBox;
	QSpinBox *m_reservedPDCSpinBox;
	QComboBox *m_terminationMarkerPageTypeComboBox;
	QCheckBox *m_terminationMarkerMoreFollowsCheckBox;
	QPushButton *m_insertBeforePushButton, *m_insertAfterPushButton, *m_insertCopyPushButton, *m_deletePushButton;

	ModeTripletNames m_modeTripletNames;

	TeletextWidget *m_parentMainWidget;

	void disableTripletWidgets();

};

#endif
