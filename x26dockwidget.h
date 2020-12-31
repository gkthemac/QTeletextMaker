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

#ifndef X26DOCKWIDGET_H
#define X26DOCKWIDGET_H

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QGroupBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QStackedLayout>
#include <QTableView>

#include "mainwidget.h"
#include "x26model.h"

class X26DockWidget : public QDockWidget
{
	Q_OBJECT

public:
	X26DockWidget(TeletextWidget *parent);

public slots:
	void insertTriplet();
	void deleteTriplet();
	void customMenuRequested(QPoint pos);
	void loadX26List();
	void unloadX26List();
	void rowClicked(const QModelIndex &);
	void updateAllRawTripletSpinBoxes(const QModelIndex &);
	void updateRawTripletDataSpinBox(const QModelIndex &);
	void updateAllCookedTripletWidgets(const QModelIndex &);
	void updateCookedModeFromCookedType(const int);
	void updateCookedTripletParameters(const QModelIndex &);
	void rawTripletAddressSpinBoxChanged(int);
	void rawTripletModeSpinBoxChanged(int);
	void rawTripletDataSpinBoxChanged(int);
	void cookedRowSpinBoxChanged(const int);
	void cookedColumnSpinBoxChanged(const int);
	void cookedModeComboBoxChanged(const int);
	void updateModelFromCookedWidget(const int, const int);
	void selectX26ListRow(int);

private:
	QTableView *m_x26View;
	X26Model *m_x26Model;
	QComboBox *m_cookedModeTypeComboBox;
	QSpinBox *m_cookedRowSpinBox, *m_cookedColumnSpinBox;
	QComboBox *m_cookedModeComboBox;
	QSpinBox *m_rawTripletAddressSpinBox, *m_rawTripletModeSpinBox, *m_rawTripletDataSpinBox;
	QStackedLayout *m_rawOrCookedStackedLayout;
	QComboBox *m_colourComboBox;
	QRadioButton *m_fullRowColourThisRowOnlyRadioButton, *m_fullRowColourDownToBottomRadioButton;
	QSpinBox *m_characterCodeSpinBox;
	QComboBox *m_flashModeComboBox, *m_flashRateComboBox;
	QComboBox *m_textSizeComboBox;
	QCheckBox *m_displayAttributeBoxingCheckBox, *m_displayAttributeConcealCheckBox, *m_displayAttributeInvertCheckBox, *m_displayAttributeUnderlineCheckBox;
	QComboBox *m_objectSourceComboBox;
	QCheckBox *m_objectRequiredAtL2p5CheckBox, *m_objectRequiredAtL3p5CheckBox;
	QSpinBox *m_invokeLocalObjectDesignationCodeSpinBox, *m_invokeLocalObjectTripletNumberSpinBox;
	QSpinBox *m_invokePOPSubPageSpinBox, *m_invokePOPPacketNumberSpinBox;
	QComboBox *m_invokePOPTripletNumberComboBox, *m_invokePOPPointerBitsComboBox;
	QStackedLayout *m_invokeObjectSourceStackedLayout, *m_tripletParameterStackedLayout;
	QCheckBox *m_DRCSModeRequiredAtL2p5CheckBox, *m_DRCSModeRequiredAtL3p5CheckBox;
	QRadioButton *m_DRCSModeGlobalRadioButton, *m_DRCSModeNormalRadioButton;
	QSpinBox *m_DRCSModeSubPageSpinBox;
	QRadioButton *m_DRCSCharacterGlobalRadioButton, *m_DRCSCharacterNormalRadioButton;
	QSpinBox *m_DRCSCharacterCodeSpinBox;
	QCheckBox *m_fontStyleProportionalCheckBox, *m_fontStyleBoldCheckBox, *m_fontStyleItalicCheckBox;
	QSpinBox *m_fontStyleRowsSpinBox;
	QComboBox *m_terminationMarkerPageTypeComboBox;
	QCheckBox *m_terminationMarkerMoreFollowsCheckBox;
	QPushButton *m_insertPushButton, *m_deletePushButton;

	TeletextWidget *m_parentMainWidget;
};

#endif
