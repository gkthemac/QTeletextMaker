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

#include <QActionGroup>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QShortcut>
#include <QSpinBox>
#include <QStackedLayout>
#include <QStandardItemModel>
#include <QToolButton>
#include <QVBoxLayout>

#include "x26dockwidget.h"

X26DockWidget::X26DockWidget(TeletextWidget *parent): QDockWidget(parent)
{
	QVBoxLayout *x26Layout = new QVBoxLayout;
	QWidget *x26Widget = new QWidget;
	m_parentMainWidget = parent;

	this->setObjectName("x26DockWidget");
	this->setWindowTitle("X/26 triplets");

	// Table listing of local enhancements
	m_x26View = new QTableView;
	m_x26Model = new X26Model(m_parentMainWidget);
	m_x26View->setModel(m_x26Model);
	m_x26View->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_x26View->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_x26View->setSelectionMode(QAbstractItemView::SingleSelection);
	m_x26View->setColumnWidth(0, 50);
	m_x26View->setColumnWidth(1, 50);
	m_x26View->setColumnWidth(2, 200);
	m_x26View->setColumnWidth(3, 200);
	x26Layout->addWidget(m_x26View);

	connect(m_parentMainWidget->document(), &TeletextDocument::tripletCommandHighlight, this, &X26DockWidget::selectX26ListRow);

	// Triplet type and mode selection, with row and column spinboxes
	QHBoxLayout *tripletSelectLayout = new QHBoxLayout;

	// Checkbox to select user-friendly or raw-value widgets
	QCheckBox *rawOrCookedCheckBox = new QCheckBox(tr("Raw values"));
	tripletSelectLayout->addWidget(rawOrCookedCheckBox);

	// "Cooked" or user-friendly triplet type, row and column selection
	QHBoxLayout *cookedTripletLayout = new QHBoxLayout;

	m_cookedModeTypeComboBox = new QComboBox;
	m_cookedModeTypeComboBox->addItem("Set Active Position");
	m_cookedModeTypeComboBox->addItem("Row triplet");
	m_cookedModeTypeComboBox->addItem("Column triplet");
	m_cookedModeTypeComboBox->addItem("Object");
	m_cookedModeTypeComboBox->addItem("Terminator");
	m_cookedModeTypeComboBox->addItem("PDC/reserved");
	cookedTripletLayout->addWidget(m_cookedModeTypeComboBox);
	connect(m_cookedModeTypeComboBox, QOverload<int>::of(&QComboBox::activated), this, &X26DockWidget::updateCookedModeFromCookedType);

	// Cooked row spinbox
	QLabel *rowLabel = new QLabel(tr("Row"));
	rowLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
	cookedTripletLayout->addWidget(rowLabel);
	m_cookedRowSpinBox = new QSpinBox;
	m_cookedRowSpinBox->setMaximum(24); // FIXME maximum value reset when triplet is selected
	m_cookedRowSpinBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
	cookedTripletLayout->addWidget(m_cookedRowSpinBox);
	connect(m_cookedRowSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &X26DockWidget::cookedRowSpinBoxChanged);

	// Cooked column spinbox
	QLabel *columnLabel = new QLabel(tr("Column"));
	columnLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
	cookedTripletLayout->addWidget(columnLabel);
//	cookedTripletLayout->addWidget(new QLabel(tr("Column")));
	m_cookedColumnSpinBox = new QSpinBox;
	m_cookedColumnSpinBox->setMaximum(39); // FIXME maximum value reset when triplet is selected
	m_cookedColumnSpinBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
	cookedTripletLayout->addWidget(m_cookedColumnSpinBox);
	connect(m_cookedColumnSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &X26DockWidget::cookedColumnSpinBoxChanged);

	// Cooked triplet mode
	m_cookedModeComboBox = new QComboBox;
	m_cookedModeComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	cookedTripletLayout->addWidget(m_cookedModeComboBox);
	connect(m_cookedModeComboBox, QOverload<int>::of(&QComboBox::activated), this, &X26DockWidget::cookedModeComboBoxChanged);

	// Raw triplet values
	QHBoxLayout *rawTripletLayout = new QHBoxLayout;

	// Raw triplet address
	rawTripletLayout->addWidget(new QLabel(tr("Address")));
	m_rawTripletAddressSpinBox = new QSpinBox;
	m_rawTripletAddressSpinBox->setMaximum(63);
	rawTripletLayout->addWidget(m_rawTripletAddressSpinBox);

	// Raw triplet mode
	rawTripletLayout->addWidget(new QLabel(tr("Mode")));
	m_rawTripletModeSpinBox = new QSpinBox;
	m_rawTripletModeSpinBox->setMaximum(31);
	rawTripletLayout->addWidget(m_rawTripletModeSpinBox);

	// Raw triplet data
	rawTripletLayout->addWidget(new QLabel(tr("Data")));
	m_rawTripletDataSpinBox = new QSpinBox;
	m_rawTripletDataSpinBox->setMaximum(127);
	rawTripletLayout->addWidget(m_rawTripletDataSpinBox);

	connect(m_rawTripletAddressSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &X26DockWidget::rawTripletAddressSpinBoxChanged);
	connect(m_rawTripletModeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &X26DockWidget::rawTripletModeSpinBoxChanged);
	connect(m_rawTripletDataSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &X26DockWidget::rawTripletDataSpinBoxChanged);

	// Stack the raw and cooked layouts together
	m_rawOrCookedStackedLayout = new QStackedLayout;
	QWidget *cookedTripletWidget = new QWidget;
	cookedTripletWidget->setLayout(cookedTripletLayout);
	cookedTripletWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	m_rawOrCookedStackedLayout->addWidget(cookedTripletWidget);
	QWidget *rawTripletWidget = new QWidget;
	rawTripletWidget->setLayout(rawTripletLayout);
	rawTripletWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	m_rawOrCookedStackedLayout->addWidget(rawTripletWidget);
	// We could simply: tripletSelectLayout->addLayout(m_rawOrCookedStackedLayout);
	// but we need to keep it to the smallest size, only possible by putting it inside a QWidget
	QWidget *rawOrCookedWidget = new QWidget;
	rawOrCookedWidget->setLayout(m_rawOrCookedStackedLayout);
	rawOrCookedWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	tripletSelectLayout->addWidget(rawOrCookedWidget);

	x26Layout->addLayout(tripletSelectLayout);

	connect(rawOrCookedCheckBox, &QCheckBox::stateChanged,[=](const int value) { m_rawOrCookedStackedLayout->setCurrentIndex(value == 2); } );


	// Widgets that alter the parameters of triplets which will all be stacked
	// Index 0 is a blank label widget, we allocate it later when we start stacking

	// Index 1 - Colour and "this row only"/"down to bottom" selection
	QHBoxLayout *colourAndRowLayout = new QHBoxLayout;

	m_colourComboBox = new QComboBox;
	for (int c=0; c<=3; c++)
		for (int e=0; e<=7; e++)
			m_colourComboBox->addItem(tr("CLUT %1:%2").arg(c).arg(e));
	colourAndRowLayout->addWidget(m_colourComboBox);
	connect(m_colourComboBox, QOverload<int>::of(&QComboBox::activated), this, [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+1); } );

	m_fullRowColourThisRowOnlyRadioButton = new QRadioButton(tr("This row only"));
	m_fullRowColourDownToBottomRadioButton = new QRadioButton(tr("Down to bottom"));
	QButtonGroup *rowsButtonGroup = new QButtonGroup;
	rowsButtonGroup->addButton(m_fullRowColourThisRowOnlyRadioButton, 0);
	rowsButtonGroup->addButton(m_fullRowColourDownToBottomRadioButton, 1);
	colourAndRowLayout->addWidget(m_fullRowColourThisRowOnlyRadioButton);
	colourAndRowLayout->addWidget(m_fullRowColourDownToBottomRadioButton);
	connect(m_fullRowColourThisRowOnlyRadioButton, &QAbstractButton::clicked, this, [=] { updateModelFromCookedWidget(0, Qt::UserRole+2); } );
	connect(m_fullRowColourDownToBottomRadioButton, &QAbstractButton::clicked, this, [=] { updateModelFromCookedWidget(1, Qt::UserRole+2); } );

	// Index 2 - Character code
	QHBoxLayout *characterCodeLayout = new QHBoxLayout;

	m_characterCodeComboBox = new QComboBox;
	for (int i=32; i<128; i++)
		m_characterCodeComboBox->addItem(QString("0x%1").arg(i, 2, 16), i);
	characterCodeLayout->addWidget(m_characterCodeComboBox);
	connect(m_characterCodeComboBox, QOverload<int>::of(&QComboBox::activated), this, [=](const int value) { updateModelFromCookedWidget(value+32, Qt::UserRole+1); } );

	// Index 3 - Flash rate and phase
	QHBoxLayout *flashModeRateLayout = new QHBoxLayout;

	m_flashModeComboBox = new QComboBox;
	m_flashModeComboBox->addItem(tr("Steady"));
	m_flashModeComboBox->addItem(tr("Normal"));
	m_flashModeComboBox->addItem(tr("Invert"));
	m_flashModeComboBox->addItem(tr("Adjacent CLUT"));
	flashModeRateLayout->addWidget(m_flashModeComboBox);
	connect(m_flashModeComboBox, QOverload<int>::of(&QComboBox::activated), this, [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+1); } );

	m_flashRateComboBox = new QComboBox;
	m_flashRateComboBox->addItem(tr("Slow 1Hz"));
	m_flashRateComboBox->addItem(tr("Fast 2Hz phase 1"));
	m_flashRateComboBox->addItem(tr("Fast 2Hz phase 2"));
	m_flashRateComboBox->addItem(tr("Fast 2Hz phase 3"));
	m_flashRateComboBox->addItem(tr("Fast 2Hz inc/right"));
	m_flashRateComboBox->addItem(tr("Fast 2Hz dec/left"));
	flashModeRateLayout->addWidget(m_flashRateComboBox);
	connect(m_flashRateComboBox, QOverload<int>::of(&QComboBox::activated), this, [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+2); } );

	// Index 4 - Display attributes
	QHBoxLayout *displayAttributesLayout = new QHBoxLayout;

	m_textSizeComboBox = new QComboBox;
	m_textSizeComboBox->addItem(tr("Normal size"));
	m_textSizeComboBox->addItem(tr("Double height"));
	m_textSizeComboBox->addItem(tr("Double width"));
	m_textSizeComboBox->addItem(tr("Double size"));
	displayAttributesLayout->addWidget(m_textSizeComboBox);
	connect(m_textSizeComboBox, QOverload<int>::of(&QComboBox::activated), this, [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+1); } );

	m_displayAttributeBoxingCheckBox = new QCheckBox(tr("Box/Win"));
	m_displayAttributeConcealCheckBox = new QCheckBox(tr("Conceal"));
	m_displayAttributeInvertCheckBox = new QCheckBox(tr("Invert"));
	m_displayAttributeUnderlineCheckBox = new QCheckBox(tr("Und/Sep mosaics"));
	displayAttributesLayout->addWidget(m_displayAttributeBoxingCheckBox);
	displayAttributesLayout->addWidget(m_displayAttributeConcealCheckBox);
	displayAttributesLayout->addWidget(m_displayAttributeInvertCheckBox);
	displayAttributesLayout->addWidget(m_displayAttributeUnderlineCheckBox);
	connect(m_displayAttributeBoxingCheckBox, &QCheckBox::stateChanged, this, [=](const int value) {  updateModelFromCookedWidget((value == 2), Qt::UserRole+2); } );
	connect(m_displayAttributeConcealCheckBox, &QCheckBox::stateChanged, this, [=](const int value) {  updateModelFromCookedWidget((value == 2), Qt::UserRole+3); } );
	connect(m_displayAttributeInvertCheckBox, &QCheckBox::stateChanged, this, [=](const int value) {  updateModelFromCookedWidget((value == 2), Qt::UserRole+4); } );
	connect(m_displayAttributeUnderlineCheckBox, &QCheckBox::stateChanged, this, [=](const int value) {  updateModelFromCookedWidget((value == 2), Qt::UserRole+5); } );

	// Index 5 - Invoke Object
	QHBoxLayout *invokeObjectLayout = new QHBoxLayout;

	// From local or (G)POP page
	m_objectSourceComboBox = new QComboBox;
	m_objectSourceComboBox->addItem("Local");
	m_objectSourceComboBox->addItem("POP");
	m_objectSourceComboBox->addItem("GPOP");
	invokeObjectLayout->addWidget(m_objectSourceComboBox);
	connect(m_objectSourceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+1); updateCookedTripletParameters(m_x26View->currentIndex()); } );

	// Object required at which levels
	m_objectRequiredAtLevelsComboBox = new QComboBox;
	m_objectRequiredAtLevelsComboBox->addItem("L2.5 only");
	m_objectRequiredAtLevelsComboBox->addItem("L3.5 only");
	m_objectRequiredAtLevelsComboBox->addItem("L2.5 + 3.5");
	invokeObjectLayout->addWidget(m_objectRequiredAtLevelsComboBox);
	connect(m_objectRequiredAtLevelsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](const int value) {  updateModelFromCookedWidget(value, Qt::UserRole+1); } );

	// Invoke Local Objects
	QHBoxLayout *invokeLocalObjectLayout = new QHBoxLayout;

	invokeLocalObjectLayout->addWidget(new QLabel(tr("Designation")));
	m_invokeLocalObjectDesignationCodeSpinBox = new QSpinBox;
	m_invokeLocalObjectDesignationCodeSpinBox->setMaximum(15);
	invokeLocalObjectLayout->addWidget(m_invokeLocalObjectDesignationCodeSpinBox);
	connect(m_invokeLocalObjectDesignationCodeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+2); } );

	invokeLocalObjectLayout->addWidget(new QLabel(tr("Triplet")));
	m_invokeLocalObjectTripletNumberSpinBox = new QSpinBox;
	m_invokeLocalObjectTripletNumberSpinBox->setMaximum(12);
	invokeLocalObjectLayout->addWidget(m_invokeLocalObjectTripletNumberSpinBox);
	connect(m_invokeLocalObjectTripletNumberSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+3); } );

	// Invoke (G)POP Object
	QHBoxLayout *invokePOPLayout = new QHBoxLayout;

	// (G)POP subpage
	invokePOPLayout->addWidget(new QLabel(tr("Subpage")));
	m_invokePOPSubPageSpinBox = new QSpinBox;
	m_invokePOPSubPageSpinBox->setMaximum(15);
	m_invokePOPSubPageSpinBox->setWrapping(true);
	invokePOPLayout->addWidget(m_invokePOPSubPageSpinBox);
	connect(m_invokePOPSubPageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+2); } );

	// (G)POP packet to locate the pointer
	invokePOPLayout->addWidget(new QLabel(tr("Packet")));
	m_invokePOPPacketNumberSpinBox = new QSpinBox;
	m_invokePOPPacketNumberSpinBox->setRange(1, 4);
	m_invokePOPPacketNumberSpinBox->setWrapping(true);
	invokePOPLayout->addWidget(m_invokePOPPacketNumberSpinBox);
	connect(m_invokePOPPacketNumberSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+3); } );

	// (G)POP triplet with pointer
	m_invokePOPTripletNumberComboBox = new QComboBox;
	invokePOPLayout->addWidget(m_invokePOPTripletNumberComboBox);
	connect(m_invokePOPTripletNumberComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+4); } );

	// (G)POP which bits of triplet for the pointer
	m_invokePOPPointerBitsComboBox = new QComboBox;
	m_invokePOPPointerBitsComboBox->addItem("Bits 1-9");
	m_invokePOPPointerBitsComboBox->addItem("Bits 10-18");
	invokePOPLayout->addWidget(m_invokePOPPointerBitsComboBox);
	connect(m_invokePOPPointerBitsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+5); } );

	// Stack the Local and (G)POP layouts together
	m_invokeObjectSourceStackedLayout = new QStackedLayout;
	QWidget *invokeLocalObjectWidget = new QWidget;
	invokeLocalObjectWidget->setLayout(invokeLocalObjectLayout);
	invokeLocalObjectWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	m_invokeObjectSourceStackedLayout->addWidget(invokeLocalObjectWidget);
	QWidget *invokeGPOPWidget = new QWidget;
	invokeGPOPWidget->setLayout(invokePOPLayout);
	invokeGPOPWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	m_invokeObjectSourceStackedLayout->addWidget(invokeGPOPWidget);
//	We could simply: invokeObjectLayout->addLayout(m_rawOrCookedStackedLayout);
//	but we need to keep it to the smallest size, only possible by putting it inside a QWidget
	QWidget *invokeObjectSourceWidget = new QWidget;
	invokeObjectSourceWidget->setLayout(m_invokeObjectSourceStackedLayout);
	invokeObjectSourceWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	invokeObjectLayout->addWidget(invokeObjectSourceWidget);

	// Index 6 - DRCS mode
	QHBoxLayout *DRCSModeLayout = new QHBoxLayout;

	m_DRCSModeRequiredAtL2p5CheckBox = new QCheckBox("L2.5");
	m_DRCSModeRequiredAtL3p5CheckBox = new QCheckBox("L3.5");
	DRCSModeLayout->addWidget(m_DRCSModeRequiredAtL2p5CheckBox);
	DRCSModeLayout->addWidget(m_DRCSModeRequiredAtL3p5CheckBox);
	connect(m_DRCSModeRequiredAtL2p5CheckBox, &QCheckBox::stateChanged, this, [=](const int value) {  updateModelFromCookedWidget(value, Qt::UserRole+1); } );
	connect(m_DRCSModeRequiredAtL3p5CheckBox, &QCheckBox::stateChanged, this, [=](const int value) {  updateModelFromCookedWidget(value, Qt::UserRole+2); } );
	m_DRCSModeGlobalRadioButton = new QRadioButton("Global");
	m_DRCSModeNormalRadioButton = new QRadioButton("Normal");
	QButtonGroup *DRCSModeButtonGroup = new QButtonGroup;
	DRCSModeButtonGroup->addButton(m_DRCSModeGlobalRadioButton, 0);
	DRCSModeButtonGroup->addButton(m_DRCSModeNormalRadioButton, 1);
	DRCSModeLayout->addWidget(m_DRCSModeGlobalRadioButton);
	DRCSModeLayout->addWidget(m_DRCSModeNormalRadioButton);
	connect(m_DRCSModeGlobalRadioButton, &QAbstractButton::clicked, this, [=] { updateModelFromCookedWidget(0, Qt::UserRole+3); } );
	connect(m_DRCSModeNormalRadioButton, &QAbstractButton::clicked, this, [=] { updateModelFromCookedWidget(1, Qt::UserRole+3); } );
	DRCSModeLayout->addWidget(new QLabel(tr("Subpage")));
	m_DRCSModeSubPageSpinBox = new QSpinBox;
	m_DRCSModeSubPageSpinBox->setMaximum(15);
	m_DRCSModeSubPageSpinBox->setWrapping(true);
	DRCSModeLayout->addWidget(m_DRCSModeSubPageSpinBox);
	connect(m_DRCSModeSubPageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+4); } );

	// Index 7 - DRCS character
	QHBoxLayout *DRCSCharacterLayout = new QHBoxLayout;

	m_DRCSCharacterGlobalRadioButton = new QRadioButton("Global");
	m_DRCSCharacterNormalRadioButton = new QRadioButton("Normal");
	QButtonGroup *DRCSCharacterButtonGroup = new QButtonGroup;
	DRCSCharacterButtonGroup->addButton(m_DRCSCharacterGlobalRadioButton, 0);
	DRCSCharacterButtonGroup->addButton(m_DRCSCharacterNormalRadioButton, 1);
	DRCSCharacterLayout->addWidget(m_DRCSCharacterGlobalRadioButton);
	DRCSCharacterLayout->addWidget(m_DRCSCharacterNormalRadioButton);
	connect(m_DRCSCharacterGlobalRadioButton, &QAbstractButton::clicked, this, [=] { updateModelFromCookedWidget(0, Qt::UserRole+1); } );
	connect(m_DRCSCharacterNormalRadioButton, &QAbstractButton::clicked, this, [=] { updateModelFromCookedWidget(1, Qt::UserRole+1); } );
	DRCSCharacterLayout->addWidget(new QLabel(tr("Character")));
	m_DRCSCharacterCodeSpinBox = new QSpinBox;
	m_DRCSCharacterCodeSpinBox->setMaximum(47);
	m_DRCSCharacterCodeSpinBox->setWrapping(true);
	DRCSCharacterLayout->addWidget(m_DRCSCharacterCodeSpinBox);
	connect(m_DRCSCharacterCodeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+2); } );

	// Index 8 - Font style
	QHBoxLayout *fontStyleLayout = new QHBoxLayout;

	m_fontStyleProportionalCheckBox = new QCheckBox(tr("Proportional"));
	m_fontStyleBoldCheckBox = new QCheckBox(tr("Bold"));
	m_fontStyleItalicCheckBox = new QCheckBox(tr("Italic"));
	fontStyleLayout->addWidget(m_fontStyleProportionalCheckBox);
	fontStyleLayout->addWidget(m_fontStyleBoldCheckBox);
	fontStyleLayout->addWidget(m_fontStyleItalicCheckBox);
	connect(m_fontStyleProportionalCheckBox, &QCheckBox::stateChanged, this, [=](const int value) {  updateModelFromCookedWidget((value == 2), Qt::UserRole+1); } );
	connect(m_fontStyleBoldCheckBox, &QCheckBox::stateChanged, this, [=](const int value) { updateModelFromCookedWidget((value == 2), Qt::UserRole+2);; } );
	connect(m_fontStyleItalicCheckBox, &QCheckBox::stateChanged, this, [=](const int value) { updateModelFromCookedWidget((value == 2), Qt::UserRole+3); } );
	m_fontStyleRowsSpinBox = new QSpinBox;
	m_fontStyleRowsSpinBox->setRange(0, 7);
	fontStyleLayout->addWidget(m_fontStyleRowsSpinBox);
	connect(m_fontStyleRowsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+4); } );

	// Index 9 - Reserved/PDC data
	QHBoxLayout *reservedPDCLayout = new QHBoxLayout;

	m_reservedPDCSpinBox = new QSpinBox;
	m_reservedPDCSpinBox->setMaximum(127);
	m_reservedPDCSpinBox->setDisplayIntegerBase(16);
	m_reservedPDCSpinBox->setPrefix("0x");
	m_reservedPDCSpinBox->setWrapping(true);
	reservedPDCLayout->addWidget(m_reservedPDCSpinBox);
	connect(m_reservedPDCSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+1); } );

	// Index 10 - Terminator
	QHBoxLayout *terminationMarkerLayout = new QHBoxLayout;

	m_terminationMarkerPageTypeComboBox = new QComboBox;
	m_terminationMarkerPageTypeComboBox->addItem("Interm (G)POP page");
	m_terminationMarkerPageTypeComboBox->addItem("Last (G)POP page");
	m_terminationMarkerPageTypeComboBox->addItem("Local Object defs");
	m_terminationMarkerPageTypeComboBox->addItem("Local enhancements");
	terminationMarkerLayout->addWidget(m_terminationMarkerPageTypeComboBox);
	connect(m_terminationMarkerPageTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+1); } );

	m_terminationMarkerMoreFollowsCheckBox = new QCheckBox(tr("Objects follow"));
	terminationMarkerLayout->addWidget(m_terminationMarkerMoreFollowsCheckBox);
	connect(m_terminationMarkerMoreFollowsCheckBox, &QCheckBox::stateChanged, this, [=](const int value) {  updateModelFromCookedWidget(value != 2, Qt::UserRole+2); } );


	// Stack all the triplet parameter layouts together
	m_tripletParameterStackedLayout = new QStackedLayout;

	// Index 0
	// Blank label
	QLabel *blankWidget = new QLabel(" ");
	m_tripletParameterStackedLayout->addWidget(blankWidget);

	// Index 1
	QWidget *colourAndRowWidget = new QWidget;
	colourAndRowWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	colourAndRowWidget->setLayout(colourAndRowLayout);
	m_tripletParameterStackedLayout->addWidget(colourAndRowWidget);

	// Index 2
	QWidget *characterCodeWidget = new QWidget;
	characterCodeWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	characterCodeWidget->setLayout(characterCodeLayout);
	m_tripletParameterStackedLayout->addWidget(characterCodeWidget);

	// Index 3
	QWidget *flashModeRateWidget = new QWidget;
	flashModeRateWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	flashModeRateWidget->setLayout(flashModeRateLayout);
	m_tripletParameterStackedLayout->addWidget(flashModeRateWidget);

	// Index 4
	QWidget *displayAttributesWidget = new QWidget;
	displayAttributesWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	displayAttributesWidget->setLayout(displayAttributesLayout);
	m_tripletParameterStackedLayout->addWidget(displayAttributesWidget);

	// Index 5
	QWidget *invokeObjectWidget = new QWidget;
	invokeObjectWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	invokeObjectWidget->setLayout(invokeObjectLayout);
	m_tripletParameterStackedLayout->addWidget(invokeObjectWidget);

	// Index 6
	QWidget *DRCSModeWidget = new QWidget;
	DRCSModeWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	DRCSModeWidget->setLayout(DRCSModeLayout);
	m_tripletParameterStackedLayout->addWidget(DRCSModeWidget);

	// Index 7
	QWidget *DRCSCharacterWidget = new QWidget;
	DRCSCharacterWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	DRCSCharacterWidget->setLayout(DRCSCharacterLayout);
	m_tripletParameterStackedLayout->addWidget(DRCSCharacterWidget);

	// Index 8
	QWidget *fontStyleWidget = new QWidget;
	fontStyleWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	fontStyleWidget->setLayout(fontStyleLayout);
	m_tripletParameterStackedLayout->addWidget(fontStyleWidget);

	// Index 9
	QWidget *reservedPDCWidget = new QWidget;
	reservedPDCWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	reservedPDCWidget->setLayout(reservedPDCLayout);
	m_tripletParameterStackedLayout->addWidget(reservedPDCWidget);

	// Index 10
	QWidget *terminationMarkerWidget = new QWidget;
	terminationMarkerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	terminationMarkerWidget->setLayout(terminationMarkerLayout);
	m_tripletParameterStackedLayout->addWidget(terminationMarkerWidget);

	// We could simply: x26Layout->addLayout(m_tripletParameterStackedLayout);
	// but we need to keep it to the smallest size, only possible by putting it inside a QWidget
	QWidget *tripletParameterWidget = new QWidget;
	tripletParameterWidget->setLayout(m_tripletParameterStackedLayout);
	tripletParameterWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	x26Layout->addWidget(tripletParameterWidget);

	// Insert and delete widgets
	QHBoxLayout *insertDeleteLayout = new QHBoxLayout;

	m_insertPushButton = new QPushButton(tr("Insert triplet"));
	insertDeleteLayout->addWidget(m_insertPushButton);
	m_deletePushButton = new QPushButton(tr("Delete triplet"));
	insertDeleteLayout->addWidget(m_deletePushButton);

	connect(m_insertPushButton, &QPushButton::clicked, this, &X26DockWidget::insertTriplet);
	connect(m_deletePushButton, &QPushButton::clicked, this, &X26DockWidget::deleteTriplet);

	x26Layout->addLayout(insertDeleteLayout);

	x26Widget->setLayout(x26Layout);
	this->setWidget(x26Widget);

	m_x26View->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_x26View, &QWidget::customContextMenuRequested, this, &X26DockWidget::customMenuRequested);

	connect(m_x26View, &QAbstractItemView::clicked, this, &X26DockWidget::rowClicked);
}

void X26DockWidget::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
		case Qt::Key_Insert:
			insertTriplet();
			break;
		case Qt::Key_Delete:
			deleteTriplet();
			break;
		default:
			QWidget::keyPressEvent(event);
	}
}

void X26DockWidget::selectX26ListRow(int row)
{
	if (m_x26Model->rowCount() <= 0)
		return;
	if (row >= m_x26Model->rowCount())
		row = m_x26Model->rowCount() - 1;

	m_x26View->selectRow(row);
	rowClicked(m_x26View->currentIndex());
}

void X26DockWidget::loadX26List()
{
	m_x26Model->setX26ListLoaded(true);
}

void X26DockWidget::unloadX26List()
{
	m_x26Model->setX26ListLoaded(false);
	m_rawTripletAddressSpinBox->setEnabled(false);
	m_rawTripletDataSpinBox->setEnabled(false);
	m_rawTripletModeSpinBox->setEnabled(false);
}

void X26DockWidget::rowClicked(const QModelIndex &index)
{
	updateAllRawTripletSpinBoxes(index);
	updateAllCookedTripletWidgets(index);
}

void X26DockWidget::updateAllCookedTripletWidgets(const QModelIndex &index)
{
	const int modeExt = index.model()->data(index.model()->index(index.row(), 2), Qt::EditRole).toInt();

	// Find which triplettype the triplet is
	const int oldCookedModeType = m_cookedModeTypeComboBox->currentIndex();
	switch (modeExt) {
		case 0x04:
			m_cookedModeTypeComboBox->setCurrentIndex(0);
			break;
		case 0x00:
		case 0x01:
		case 0x07:
		case 0x18:
			m_cookedModeTypeComboBox->setCurrentIndex(1);
			break;
		case 0x20 ... 0x23:
		case 0x27 ... 0x29:
		case 0x2b ... 0x3f:
			m_cookedModeTypeComboBox->setCurrentIndex(2);
			break;
		case 0x10 ... 0x13:
		case 0x15 ... 0x17:
			m_cookedModeTypeComboBox->setCurrentIndex(3);
			break;
		case 0x1f:
			m_cookedModeTypeComboBox->setCurrentIndex(4);
			break;
		default:
			m_cookedModeTypeComboBox->setCurrentIndex(5);
	}

	// If the triplettype has changed, update the triplet mode combobox
	if (oldCookedModeType != m_cookedModeTypeComboBox->currentIndex())
		updateCookedModeFromCookedType(-1);
	for (int i=0; i<m_cookedModeComboBox->count(); i++)
		if (m_cookedModeComboBox->itemData(i) == modeExt) {
			m_cookedModeComboBox->blockSignals(true);
			m_cookedModeComboBox->setCurrentIndex(i);
			m_cookedModeComboBox->blockSignals(false);
			break;
		}

	updateCookedTripletParameters(index);
}

void X26DockWidget::updateCookedModeFromCookedType(const int value)
{
	while (m_cookedModeComboBox->count() > 0)
		m_cookedModeComboBox->removeItem(0);
	// When called as a slot, "value" parameter would be the same as this currentIndex
	switch (m_cookedModeTypeComboBox->currentIndex()) {
		case 1:
			m_cookedModeComboBox->addItem("select...", -1);
			m_cookedModeComboBox->addItem("Full screen colour", 0x00);
			m_cookedModeComboBox->addItem("Full row colour", 0x01);
			m_cookedModeComboBox->addItem("Address row 0", 0x07);
			m_cookedModeComboBox->addItem("DRCS mode", 0x18);
			break;
		case 2:
			m_cookedModeComboBox->addItem("select...", -1);
			m_cookedModeComboBox->addItem("Foreground colour", 0x20);
			m_cookedModeComboBox->addItem("Background colour", 0x23);
			m_cookedModeComboBox->addItem("Flash functions", 0x27);
			m_cookedModeComboBox->addItem("Display attrs", 0x2c);
			m_cookedModeComboBox->addItem("Font style L 3.5", 0x2e);
			m_cookedModeComboBox->addItem("Mod G0 and G2", 0x28);
			m_cookedModeComboBox->addItem("G0 character", 0x29);
			m_cookedModeComboBox->addItem("G2 character", 0x2f);
			m_cookedModeComboBox->addItem("G1 block mosaic", 0x21);
			m_cookedModeComboBox->addItem("G3 at L 1.5", 0x22);
			m_cookedModeComboBox->addItem("G3 at L 2.5", 0x2b);
			m_cookedModeComboBox->addItem("DRCS character", 0x2d);
			for (int i=0; i<16; i++)
				m_cookedModeComboBox->addItem(QString("G0 diactricial ")+QString("%1").arg(i, 1, 16).toUpper(), 0x30 | i);
			break;
		case 3:
			m_cookedModeComboBox->addItem("select...", -1);
			m_cookedModeComboBox->addItem("Origin modifier", 0x10);
			m_cookedModeComboBox->addItem("Invoke active obj", 0x11);
			m_cookedModeComboBox->addItem("Invoke adaptive obj", 0x12);
			m_cookedModeComboBox->addItem("Invoke passive obj", 0x13);
			m_cookedModeComboBox->addItem("Define active obj", 0x15);
			m_cookedModeComboBox->addItem("Define adaptive obj", 0x16);
			m_cookedModeComboBox->addItem("Define passive obj", 0x17);
			break;
		case 5:
			m_cookedModeComboBox->addItem("select...", -1);
			m_cookedModeComboBox->addItem("Origin and Source", 0x08);
			m_cookedModeComboBox->addItem("Month and day", 0x09);
			m_cookedModeComboBox->addItem("Row + start hours", 0x0a);
			m_cookedModeComboBox->addItem("Row + end hours", 0x0b);
			m_cookedModeComboBox->addItem("Row + time offset", 0x0c);
			m_cookedModeComboBox->addItem("Series ID and code", 0x0d);
			m_cookedModeComboBox->addItem("Col + start/end mins", 0x26);
			m_cookedModeComboBox->addItem("Reserved row 0x02", 0x02);
			m_cookedModeComboBox->addItem("Reserved row 0x03", 0x03);
			m_cookedModeComboBox->addItem("Reserved row 0x05", 0x05);
			m_cookedModeComboBox->addItem("Reserved row 0x06", 0x06);
			m_cookedModeComboBox->addItem("Reserved row 0x0e", 0x0e);
			m_cookedModeComboBox->addItem("Reserved row 0x0f", 0x0f);
			m_cookedModeComboBox->addItem("Reserved row 0x14", 0x14);
			m_cookedModeComboBox->addItem("Reserved row 0x19", 0x19);
			m_cookedModeComboBox->addItem("Reserved row 0x1a", 0x1a);
			m_cookedModeComboBox->addItem("Reserved row 0x1b", 0x1b);
			m_cookedModeComboBox->addItem("Reserved row 0x1c", 0x1c);
			m_cookedModeComboBox->addItem("Reserved row 0x1d", 0x1d);
			m_cookedModeComboBox->addItem("Reserved row 0x1e", 0x1e);
			m_cookedModeComboBox->addItem("Reserved col 0x04", 0x04);
			m_cookedModeComboBox->addItem("Reserved col 0x05", 0x05);
			m_cookedModeComboBox->addItem("Reserved col 0x0a", 0x0a);
			break;
		case 0:
			// When called as a slot the user set the combobox themself, so set the triplet mode immediately
			if (value != -1) {
				m_x26Model->setData(m_x26Model->index(m_x26View->currentIndex().row(), 2), 4, Qt::EditRole);
				updateAllRawTripletSpinBoxes(m_x26View->currentIndex());
				updateCookedTripletParameters(m_x26View->currentIndex());
			}
			break;
		case 4:
			if (value != -1) {
				m_x26Model->setData(m_x26Model->index(m_x26View->currentIndex().row(), 2), 31, Qt::EditRole);
				updateAllRawTripletSpinBoxes(m_x26View->currentIndex());
				updateCookedTripletParameters(m_x26View->currentIndex());
			}
			break;
	}
}

void X26DockWidget::updateCookedTripletParameters(const QModelIndex &index)
{
	const int modeExt = index.model()->data(index.model()->index(index.row(), 2), Qt::EditRole).toInt();

	switch (modeExt) {
		case 0x04: // Set active position
		case 0x10: // Origin modifier
			m_tripletParameterStackedLayout->setCurrentIndex(0);
			break;
		case 0x01: // Full row colour
		case 0x07: // Address row 0
			m_fullRowColourThisRowOnlyRadioButton->blockSignals(true);
			m_fullRowColourDownToBottomRadioButton->blockSignals(true);
			m_fullRowColourThisRowOnlyRadioButton->setChecked(!index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+2).toBool());
			m_fullRowColourDownToBottomRadioButton->setChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+2).toBool());
			m_fullRowColourThisRowOnlyRadioButton->blockSignals(false);
			m_fullRowColourDownToBottomRadioButton->blockSignals(false);
			// fall-through
		case 0x00: // Full screen colour
		case 0x20: // Foreground colour
		case 0x23: // Background colour
			m_fullRowColourThisRowOnlyRadioButton->setVisible((modeExt == 0x01) || (modeExt == 0x07));
			m_fullRowColourDownToBottomRadioButton->setVisible((modeExt == 0x01) || (modeExt == 0x07));
			m_colourComboBox->blockSignals(true);
			m_colourComboBox->setCurrentIndex(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+1).toInt());
			m_colourComboBox->blockSignals(false);
			m_tripletParameterStackedLayout->setCurrentIndex(1);
			break;
		case 0x21: // G1 character
		case 0x22: // G3 character at Level 1.5
		case 0x29: // G0 character
		case 0x2b: // G3 character at Level 2.5
		case 0x2f ... 0x3f: // G2 character, G0 character with diacritical
			m_characterCodeComboBox->blockSignals(true);
			m_characterCodeComboBox->setCurrentIndex(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+1).toInt()-32);
			m_characterCodeComboBox->blockSignals(false);
			m_tripletParameterStackedLayout->setCurrentIndex(2);
			break;
		// TODO case 0x28: Modified G0 and G2 designation
		case 0x27: // Flash functions
			m_flashModeComboBox->blockSignals(true);
			m_flashRateComboBox->blockSignals(true);
			m_flashModeComboBox->setCurrentIndex(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+1).toInt());
			m_flashRateComboBox->setCurrentIndex(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+2).toInt());
			m_flashModeComboBox->blockSignals(false);
			m_flashRateComboBox->blockSignals(false);
			m_tripletParameterStackedLayout->setCurrentIndex(3);
			break;
		case 0x2c: // Display attributes
			m_textSizeComboBox->blockSignals(true);
			m_displayAttributeBoxingCheckBox->blockSignals(true);
			m_displayAttributeConcealCheckBox->blockSignals(true);
			m_displayAttributeInvertCheckBox->blockSignals(true);
			m_displayAttributeUnderlineCheckBox->blockSignals(true);
			m_textSizeComboBox->setCurrentIndex(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+1).toInt());
			m_displayAttributeBoxingCheckBox->setChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+2).toBool());
			m_displayAttributeConcealCheckBox->setChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+3).toBool());
			m_displayAttributeInvertCheckBox->setChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+4).toBool());
			m_displayAttributeUnderlineCheckBox->setChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+5).toBool());
			m_textSizeComboBox->blockSignals(false);
			m_displayAttributeBoxingCheckBox->blockSignals(false);
			m_displayAttributeConcealCheckBox->blockSignals(false);
			m_displayAttributeInvertCheckBox->blockSignals(false);
			m_displayAttributeUnderlineCheckBox->blockSignals(false);
			m_tripletParameterStackedLayout->setCurrentIndex(4);
			break;
		case 0x11 ... 0x13: // Invoke object
		case 0x15 ... 0x17: // Define object
			if (index.model()->data(index.model()->index(index.row(), 1), Qt::UserRole).toInt() & 0x04) {
				// Define object
				m_objectSourceComboBox->setVisible(false);
				m_objectRequiredAtLevelsComboBox->setVisible(true);
				m_objectRequiredAtLevelsComboBox->blockSignals(true);
				m_objectRequiredAtLevelsComboBox->setCurrentIndex(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+1).toInt());
				m_objectRequiredAtLevelsComboBox->blockSignals(false);
			} else {
				// Invoke object
				m_objectRequiredAtLevelsComboBox->setVisible(false);
				m_objectSourceComboBox->setVisible(true);
			}
			m_objectSourceComboBox->blockSignals(true);
			// BUG we're only dealing with Local Object Definitions at the moment!
			if (index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+1).toInt() == 0 || (index.model()->data(index.model()->index(index.row(), 1), Qt::UserRole).toInt() & 0x04)) {
//			if (triplet.objectSource() == X26Triplet::LocalObjectSource) {
				m_objectSourceComboBox->setCurrentIndex(0);
				m_invokeObjectSourceStackedLayout->setCurrentIndex(0);
				m_invokeLocalObjectDesignationCodeSpinBox->blockSignals(true);
				m_invokeLocalObjectTripletNumberSpinBox->blockSignals(true);
				m_invokeLocalObjectDesignationCodeSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+2).toInt());
				m_invokeLocalObjectTripletNumberSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+3).toInt());
				m_invokeLocalObjectDesignationCodeSpinBox->blockSignals(false);
				m_invokeLocalObjectTripletNumberSpinBox->blockSignals(false);
			} else { // if (triplet.objectSource() != X26Triplet::IllegalObjectSource) {
				m_objectSourceComboBox->setCurrentIndex(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+1).toInt());
				m_invokeObjectSourceStackedLayout->setCurrentIndex(1);
				m_invokePOPSubPageSpinBox->blockSignals(true);
				m_invokePOPPacketNumberSpinBox->blockSignals(true);
				m_invokePOPTripletNumberComboBox->blockSignals(true);
				m_invokePOPPointerBitsComboBox->blockSignals(true);
				m_invokePOPSubPageSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+2).toInt());
				m_invokePOPPacketNumberSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+3).toInt());
				while (m_invokePOPTripletNumberComboBox->count() > 0)
					m_invokePOPTripletNumberComboBox->removeItem(0);
				for (int i=0; i<4; i++)
					m_invokePOPTripletNumberComboBox->addItem(QString("Triplet %1").arg(i*3+(modeExt & 0x03)));
				m_invokePOPTripletNumberComboBox->setCurrentIndex(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+4).toInt());
				m_invokePOPPointerBitsComboBox->setCurrentIndex(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+5).toInt());
				m_invokePOPSubPageSpinBox->blockSignals(false);
				m_invokePOPPacketNumberSpinBox->blockSignals(false);
				m_invokePOPTripletNumberComboBox->blockSignals(false);
				m_invokePOPPointerBitsComboBox->blockSignals(false);
			}
			m_objectSourceComboBox->blockSignals(false);
			m_tripletParameterStackedLayout->setCurrentIndex(5);
			break;
		case 0x18: // DRCS mode
			m_DRCSModeRequiredAtL2p5CheckBox->blockSignals(true);
			m_DRCSModeRequiredAtL3p5CheckBox->blockSignals(true);
			m_DRCSModeGlobalRadioButton->blockSignals(true);
			m_DRCSModeNormalRadioButton->blockSignals(true);
			m_DRCSModeSubPageSpinBox->blockSignals(true);
			m_DRCSModeRequiredAtL2p5CheckBox->setChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+1).toBool());
			m_DRCSModeRequiredAtL3p5CheckBox->setChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+2).toBool());
			m_DRCSModeGlobalRadioButton->setChecked(!index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+3).toBool());
			m_DRCSModeNormalRadioButton->setChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+3).toBool());
			m_DRCSModeSubPageSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+4).toInt());
			m_DRCSModeRequiredAtL2p5CheckBox->blockSignals(false);
			m_DRCSModeRequiredAtL3p5CheckBox->blockSignals(false);
			m_DRCSModeGlobalRadioButton->blockSignals(false);
			m_DRCSModeNormalRadioButton->blockSignals(false);
			m_DRCSModeSubPageSpinBox->blockSignals(false);
			m_tripletParameterStackedLayout->setCurrentIndex(6);
			break;
		case 0x2d: // DRCS character
			m_DRCSCharacterGlobalRadioButton->blockSignals(true);
			m_DRCSCharacterNormalRadioButton->blockSignals(true);
			m_DRCSCharacterCodeSpinBox->blockSignals(true);
			m_DRCSCharacterGlobalRadioButton->setChecked(!index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+1).toBool());
			m_DRCSCharacterNormalRadioButton->setChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+1).toBool());
			m_DRCSCharacterCodeSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+2).toInt());
			m_DRCSCharacterGlobalRadioButton->blockSignals(false);
			m_DRCSCharacterNormalRadioButton->blockSignals(false);
			m_DRCSCharacterCodeSpinBox->blockSignals(false);
			m_tripletParameterStackedLayout->setCurrentIndex(7);
			break;
		case 0x2e: // Font style
			m_fontStyleProportionalCheckBox->blockSignals(true);
			m_fontStyleBoldCheckBox->blockSignals(true);
			m_fontStyleItalicCheckBox->blockSignals(true);
			m_fontStyleRowsSpinBox->blockSignals(true);
			m_fontStyleProportionalCheckBox->setChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+1).toBool());
			m_fontStyleBoldCheckBox->setChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+2).toBool());
			m_fontStyleItalicCheckBox->setChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+3).toBool());
			m_fontStyleRowsSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+4).toInt());
			m_fontStyleProportionalCheckBox->blockSignals(false);
			m_fontStyleBoldCheckBox->blockSignals(false);
			m_fontStyleItalicCheckBox->blockSignals(false);
			m_fontStyleRowsSpinBox->blockSignals(false);
			m_tripletParameterStackedLayout->setCurrentIndex(8);
			break;
		case 0x1f: // Termination marker
			m_terminationMarkerPageTypeComboBox->blockSignals(true);
			m_terminationMarkerMoreFollowsCheckBox->blockSignals(true);
			m_terminationMarkerPageTypeComboBox->setCurrentIndex(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+1).toInt());
			m_terminationMarkerMoreFollowsCheckBox->setChecked(!index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+2).toBool());
			m_terminationMarkerPageTypeComboBox->blockSignals(false);
			m_terminationMarkerMoreFollowsCheckBox->blockSignals(false);
			m_tripletParameterStackedLayout->setCurrentIndex(10);
			break;
		default: // PDC and reserved triplet
			m_reservedPDCSpinBox->blockSignals(true);
			m_reservedPDCSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+1).toInt());
			m_reservedPDCSpinBox->blockSignals(false);
			m_tripletParameterStackedLayout->setCurrentIndex(9);
			break;
	}

	// Now deal with cooked row and column spinboxes
	m_cookedRowSpinBox->blockSignals(true);
	m_cookedColumnSpinBox->blockSignals(true);
	QVariant rowVariant = index.model()->data(index.model()->index(index.row(), 0), Qt::EditRole);
	if (rowVariant.isNull()) {
		m_cookedRowSpinBox->setEnabled(false);
		m_cookedRowSpinBox->setValue(0);
	} else {
		m_cookedRowSpinBox->setEnabled(true);
		if (index.model()->data(index.model()->index(index.row(), 2), Qt::EditRole) == 0x10)
			m_cookedRowSpinBox->setRange(0, 23);
		else
			m_cookedRowSpinBox->setRange(1, 24);
		m_cookedRowSpinBox->setValue(rowVariant.toInt());
	}
	QVariant columnVariant = index.model()->data(index.model()->index(index.row(), 1), Qt::EditRole);
	if (columnVariant.isNull()) {
		m_cookedColumnSpinBox->setEnabled(false);
		m_cookedColumnSpinBox->setValue(0);
	} else {
		m_cookedColumnSpinBox->setEnabled(true);
		if (index.model()->data(index.model()->index(index.row(), 2), Qt::EditRole) == 0x10)
			m_cookedColumnSpinBox->setMaximum(71);
		else
			m_cookedColumnSpinBox->setMaximum(39);
		m_cookedColumnSpinBox->setValue(columnVariant.toInt());
	}
	m_cookedRowSpinBox->blockSignals(false);
	m_cookedColumnSpinBox->blockSignals(false);
}

void X26DockWidget::updateAllRawTripletSpinBoxes(const QModelIndex &index)
{
	m_rawTripletAddressSpinBox->setEnabled(true);
	m_rawTripletDataSpinBox->setEnabled(true);
	m_rawTripletModeSpinBox->setEnabled(true);
	m_rawTripletAddressSpinBox->blockSignals(true);
	m_rawTripletModeSpinBox->blockSignals(true);
	m_rawTripletDataSpinBox->blockSignals(true);
	m_rawTripletAddressSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole).toInt());
	m_rawTripletModeSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 1), Qt::UserRole).toInt());
	m_rawTripletDataSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 2), Qt::UserRole).toInt());
	m_rawTripletAddressSpinBox->blockSignals(false);
	m_rawTripletModeSpinBox->blockSignals(false);
	m_rawTripletDataSpinBox->blockSignals(false);
}

void X26DockWidget::updateRawTripletDataSpinBox(const QModelIndex &index)
{
	m_rawTripletDataSpinBox->blockSignals(true);
	m_rawTripletDataSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 2), Qt::UserRole).toInt());
	m_rawTripletDataSpinBox->blockSignals(false);
}

void X26DockWidget::rawTripletAddressSpinBoxChanged(int value)
{
	if (!m_x26View->currentIndex().isValid())
		return;

	m_x26Model->setData(m_x26Model->index(m_x26View->currentIndex().row(), 0), value, Qt::UserRole);
	updateAllCookedTripletWidgets(m_x26View->currentIndex());
}

void X26DockWidget::rawTripletModeSpinBoxChanged(int value)
{
	if (!m_x26View->currentIndex().isValid())
		return;

	m_x26Model->setData(m_x26Model->index(m_x26View->currentIndex().row(), 1), value, Qt::UserRole);
	updateAllCookedTripletWidgets(m_x26View->currentIndex());
}

void X26DockWidget::rawTripletDataSpinBoxChanged(int value)
{
	if (!m_x26View->currentIndex().isValid())
		return;

	m_x26Model->setData(m_x26Model->index(m_x26View->currentIndex().row(), 2), value, Qt::UserRole);
	updateAllCookedTripletWidgets(m_x26View->currentIndex());
}

void X26DockWidget::cookedRowSpinBoxChanged(const int value)
{
	if (!m_x26View->currentIndex().isValid())
		return;

	m_x26Model->setData(m_x26Model->index(m_x26View->currentIndex().row(), 0), value, Qt::EditRole);
	updateAllRawTripletSpinBoxes(m_x26View->currentIndex());
}

void X26DockWidget::cookedColumnSpinBoxChanged(const int value)
{
	if (!m_x26View->currentIndex().isValid())
		return;

	m_x26Model->setData(m_x26Model->index(m_x26View->currentIndex().row(), 1), value, Qt::EditRole);
	updateAllRawTripletSpinBoxes(m_x26View->currentIndex());
}

void X26DockWidget::cookedModeComboBoxChanged(const int value)
{
	if (!m_x26View->currentIndex().isValid())
		return;

	// Avoid "select..."
	if (m_cookedModeComboBox->itemData(value) == -1)
		return;

	m_x26Model->setData(m_x26Model->index(m_x26View->currentIndex().row(), 2), m_cookedModeComboBox->itemData(value).toInt(), Qt::EditRole);

	updateAllRawTripletSpinBoxes(m_x26View->currentIndex());
	updateCookedTripletParameters(m_x26View->currentIndex());
}

void X26DockWidget::updateModelFromCookedWidget(const int value, const int role)
{
	m_x26Model->setData(m_x26Model->index(m_x26View->currentIndex().row(), 0), value, role);
	updateAllRawTripletSpinBoxes(m_x26View->currentIndex());
}

void X26DockWidget::insertTriplet()
{
	QModelIndex index = m_x26View->currentIndex();

	if (index.isValid())
		m_x26Model->insertRow(index.row(), QModelIndex());
	else
		m_x26Model->insertFirstRow();
}

void X26DockWidget::deleteTriplet()
{
	QModelIndex index = m_x26View->currentIndex();
	if (index.isValid())
		m_x26Model->removeRow(index.row(), index.parent());
}

void X26DockWidget::customMenuRequested(QPoint pos)
{
	QModelIndex index = m_x26View->indexAt(pos);

	QMenu *menu = new QMenu(this);

	QAction *insertAct = new QAction("Insert triplet", this);
	menu->addAction(insertAct);
	connect(insertAct, &QAction::triggered, this, &X26DockWidget::insertTriplet);
	if (index.isValid()) {
		QAction *deleteAct = new QAction("Delete triplet", this);
		menu->addAction(deleteAct);
		connect(deleteAct, &QAction::triggered, this, &X26DockWidget::deleteTriplet);
	}
	menu->popup(m_x26View->viewport()->mapToGlobal(pos));
}
