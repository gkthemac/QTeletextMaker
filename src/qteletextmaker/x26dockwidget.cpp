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

#include "x26dockwidget.h"

#include <QAbstractListModel>
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

#include "render.h"
#include "x26menus.h"

CharacterListModel::CharacterListModel(QObject *parent): QAbstractListModel(parent)
{
	m_characterSet = 0;
	m_mosaic = true;
}

int CharacterListModel::rowCount(const QModelIndex & /*parent*/) const
{
	return 96;
}

QVariant CharacterListModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role == Qt::DisplayRole)
		return QString("0x%1").arg(index.row()+0x20, 2, 16);

	if (role == Qt::DecorationRole) {
		if (m_mosaic && (index.row()+32) & 0x20)
			return m_fontBitmap.charBitmap(index.row()+32, 24);
		else
			return m_fontBitmap.charBitmap(index.row()+32, m_characterSet);
	}

	return QVariant();
}

void CharacterListModel::setCharacterSet(int characterSet)
{
	if (characterSet != m_characterSet || m_mosaic) {
		m_characterSet = characterSet;
		m_mosaic = false;
		emit dataChanged(createIndex(0, 0), createIndex(95, 0), QVector<int>(Qt::DecorationRole));
	}
}

void CharacterListModel::setG1AndBlastCharacterSet(int characterSet)
{
	if (characterSet != m_characterSet || !m_mosaic) {
		m_characterSet = characterSet;
		m_mosaic = true;
		emit dataChanged(createIndex(0, 0), createIndex(95, 0), QVector<int>(Qt::DecorationRole));
	}
}


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
	QLabel *modeLabel = new QLabel(tr("Mode"));
	modeLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
	cookedTripletLayout->addWidget(modeLabel);
	m_cookedModePushButton = new QPushButton;
	cookedTripletLayout->addWidget(m_cookedModePushButton);

	// Cooked triplet menu
	// We connect the menus for both "Insert" buttons at the same time
	m_cookedModeMenu = new TripletModeQMenu(this);
	m_insertBeforeMenu = new TripletModeQMenu(this);
	m_insertAfterMenu = new TripletModeQMenu(this);

	for (int m=0; m<64; m++) {
		connect(static_cast<TripletModeQMenu *>(m_cookedModeMenu)->action(m), &QAction::triggered, [=]() { cookedModeMenuSelected(m); });
		connect(static_cast<TripletModeQMenu *>(m_insertBeforeMenu)->action(m), &QAction::triggered, [=]() { insertTriplet(m, false); });
		connect(static_cast<TripletModeQMenu *>(m_insertAfterMenu)->action(m), &QAction::triggered, [=]() { insertTriplet(m, true); });
	}

	m_cookedModePushButton->setMenu(m_cookedModeMenu);

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

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	connect(rawOrCookedCheckBox, &QCheckBox::checkStateChanged,[=](const int value) { m_rawOrCookedStackedLayout->setCurrentIndex(value == 2); } );
#else
	connect(rawOrCookedCheckBox, &QCheckBox::stateChanged,[=](const int value) { m_rawOrCookedStackedLayout->setCurrentIndex(value == 2); } );
#endif


	// Widgets that alter the parameters of triplets which will all be stacked
	// Index 0 is a blank label widget, we allocate it later when we start stacking

	// Index 1 - Colour and "this row only"/"down to bottom" selection
	QHBoxLayout *colourAndRowLayout = new QHBoxLayout;

	m_colourComboBox = new QComboBox;
	m_colourComboBox->setModel(m_parentMainWidget->document()->clutModel());
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
	m_characterCodeComboBox->setModel(&m_characterListModel);
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	connect(m_displayAttributeBoxingCheckBox, &QCheckBox::checkStateChanged, this, [=](const int value) {  updateModelFromCookedWidget((value == 2), Qt::UserRole+2); } );
	connect(m_displayAttributeConcealCheckBox, &QCheckBox::checkStateChanged, this, [=](const int value) {  updateModelFromCookedWidget((value == 2), Qt::UserRole+3); } );
	connect(m_displayAttributeInvertCheckBox, &QCheckBox::checkStateChanged, this, [=](const int value) {  updateModelFromCookedWidget((value == 2), Qt::UserRole+4); } );
	connect(m_displayAttributeUnderlineCheckBox, &QCheckBox::checkStateChanged, this, [=](const int value) {  updateModelFromCookedWidget((value == 2), Qt::UserRole+5); } );
#else
	connect(m_displayAttributeBoxingCheckBox, &QCheckBox::stateChanged, this, [=](const int value) {  updateModelFromCookedWidget((value == 2), Qt::UserRole+2); } );
	connect(m_displayAttributeConcealCheckBox, &QCheckBox::stateChanged, this, [=](const int value) {  updateModelFromCookedWidget((value == 2), Qt::UserRole+3); } );
	connect(m_displayAttributeInvertCheckBox, &QCheckBox::stateChanged, this, [=](const int value) {  updateModelFromCookedWidget((value == 2), Qt::UserRole+4); } );
	connect(m_displayAttributeUnderlineCheckBox, &QCheckBox::stateChanged, this, [=](const int value) {  updateModelFromCookedWidget((value == 2), Qt::UserRole+5); } );
#endif

	// Index 5 - Invoke Object
	QHBoxLayout *invokeObjectLayout = new QHBoxLayout;

	// From local or (G)POP page
	m_objectSourceComboBox = new QComboBox;
	m_objectSourceComboBox->addItem("Local");
	m_objectSourceComboBox->addItem("POP");
	m_objectSourceComboBox->addItem("GPOP");
	invokeObjectLayout->addWidget(m_objectSourceComboBox);
	connect(m_objectSourceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+1); updateAllCookedTripletWidgets(m_x26View->currentIndex()); } );

	// Object required at which levels
	m_objectRequiredAtLevelsComboBox = new QComboBox;
	m_objectRequiredAtLevelsComboBox->addItem("L2.5 only");
	m_objectRequiredAtLevelsComboBox->addItem("L3.5 only");
	m_objectRequiredAtLevelsComboBox->addItem("L2.5 + 3.5");
	invokeObjectLayout->addWidget(m_objectRequiredAtLevelsComboBox);
	connect(m_objectRequiredAtLevelsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](const int value) {  updateModelFromCookedWidget(value, Qt::UserRole+1); } );

	// Invoke Local Objects
	QHBoxLayout *invokeLocalObjectLayout = new QHBoxLayout;

	m_invokeLocalObjectDesignationCodeLabel = new QLabel(tr("Designation"));
	invokeLocalObjectLayout->addWidget(m_invokeLocalObjectDesignationCodeLabel);
	m_invokeLocalObjectDesignationCodeSpinBox = new QSpinBox;
	m_invokeLocalObjectDesignationCodeSpinBox->setMaximum(15);
	invokeLocalObjectLayout->addWidget(m_invokeLocalObjectDesignationCodeSpinBox);
	connect(m_invokeLocalObjectDesignationCodeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](const int value) { updateModelFromCookedWidget(value, Qt::UserRole+2); } );

	m_invokeLocalObjectTripletNumberLabel = new QLabel(tr("Triplet"));
	invokeLocalObjectLayout->addWidget(m_invokeLocalObjectTripletNumberLabel);
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

	m_DRCSModeRequiredAtLevelsComboBox = new QComboBox;
	m_DRCSModeRequiredAtLevelsComboBox->addItem("L2.5 only");
	m_DRCSModeRequiredAtLevelsComboBox->addItem("L3.5 only");
	m_DRCSModeRequiredAtLevelsComboBox->addItem("L2.5 + 3.5");
	DRCSModeLayout->addWidget(m_DRCSModeRequiredAtLevelsComboBox);
	connect(m_DRCSModeRequiredAtLevelsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](const int value) {  updateModelFromCookedWidget(value, Qt::UserRole+1); } );
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	connect(m_fontStyleProportionalCheckBox, &QCheckBox::checkStateChanged, this, [=](const int value) {  updateModelFromCookedWidget((value == 2), Qt::UserRole+1); } );
	connect(m_fontStyleBoldCheckBox, &QCheckBox::checkStateChanged, this, [=](const int value) { updateModelFromCookedWidget((value == 2), Qt::UserRole+2);; } );
	connect(m_fontStyleItalicCheckBox, &QCheckBox::checkStateChanged, this, [=](const int value) { updateModelFromCookedWidget((value == 2), Qt::UserRole+3); } );
#else
	connect(m_fontStyleProportionalCheckBox, &QCheckBox::stateChanged, this, [=](const int value) {  updateModelFromCookedWidget((value == 2), Qt::UserRole+1); } );
	connect(m_fontStyleBoldCheckBox, &QCheckBox::stateChanged, this, [=](const int value) { updateModelFromCookedWidget((value == 2), Qt::UserRole+2);; } );
	connect(m_fontStyleItalicCheckBox, &QCheckBox::stateChanged, this, [=](const int value) { updateModelFromCookedWidget((value == 2), Qt::UserRole+3); } );
#endif
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	connect(m_terminationMarkerMoreFollowsCheckBox, &QCheckBox::checkStateChanged, this, [=](const int value) {  updateModelFromCookedWidget(value != 2, Qt::UserRole+2); } );
#else
	connect(m_terminationMarkerMoreFollowsCheckBox, &QCheckBox::stateChanged, this, [=](const int value) {  updateModelFromCookedWidget(value != 2, Qt::UserRole+2); } );
#endif


	// Stack all the triplet parameter layouts together
	m_tripletParameterStackedLayout = new QStackedLayout;

	// Index 0
	// Blank label
	QLabel *blankWidget = new QLabel(" ");
	m_tripletParameterStackedLayout->addWidget(blankWidget);

	// Index 1
	colourAndRowLayout->setContentsMargins(0, 0, 0, 0);
	QWidget *colourAndRowWidget = new QWidget;
	colourAndRowWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	colourAndRowWidget->setLayout(colourAndRowLayout);
	m_tripletParameterStackedLayout->addWidget(colourAndRowWidget);

	// Index 2
	characterCodeLayout->setContentsMargins(0, 0, 0, 0);
	QWidget *characterCodeWidget = new QWidget;
	characterCodeWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	characterCodeWidget->setLayout(characterCodeLayout);
	m_tripletParameterStackedLayout->addWidget(characterCodeWidget);

	// Index 3
	flashModeRateLayout->setContentsMargins(0, 0, 0, 0);
	QWidget *flashModeRateWidget = new QWidget;
	flashModeRateWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	flashModeRateWidget->setLayout(flashModeRateLayout);
	m_tripletParameterStackedLayout->addWidget(flashModeRateWidget);

	// Index 4
	displayAttributesLayout->setContentsMargins(0, 0, 0, 0);
	QWidget *displayAttributesWidget = new QWidget;
	displayAttributesWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	displayAttributesWidget->setLayout(displayAttributesLayout);
	m_tripletParameterStackedLayout->addWidget(displayAttributesWidget);

	// Index 5
	invokeObjectLayout->setContentsMargins(0, 0, 0, 0);
	QWidget *invokeObjectWidget = new QWidget;
	invokeObjectWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	invokeObjectWidget->setLayout(invokeObjectLayout);
	m_tripletParameterStackedLayout->addWidget(invokeObjectWidget);

	// Index 6
	DRCSModeLayout->setContentsMargins(0, 0, 0, 0);
	QWidget *DRCSModeWidget = new QWidget;
	DRCSModeWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	DRCSModeWidget->setLayout(DRCSModeLayout);
	m_tripletParameterStackedLayout->addWidget(DRCSModeWidget);

	// Index 7
	DRCSCharacterLayout->setContentsMargins(0, 0, 0, 0);
	QWidget *DRCSCharacterWidget = new QWidget;
	DRCSCharacterWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	DRCSCharacterWidget->setLayout(DRCSCharacterLayout);
	m_tripletParameterStackedLayout->addWidget(DRCSCharacterWidget);

	// Index 8
	fontStyleLayout->setContentsMargins(0, 0, 0, 0);
	QWidget *fontStyleWidget = new QWidget;
	fontStyleWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	fontStyleWidget->setLayout(fontStyleLayout);
	m_tripletParameterStackedLayout->addWidget(fontStyleWidget);

	// Index 9
	reservedPDCLayout->setContentsMargins(0, 0, 0, 0);
	QWidget *reservedPDCWidget = new QWidget;
	reservedPDCWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	reservedPDCWidget->setLayout(reservedPDCLayout);
	m_tripletParameterStackedLayout->addWidget(reservedPDCWidget);

	// Index 10
	terminationMarkerLayout->setContentsMargins(0, 0, 0, 0);
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

	m_insertBeforePushButton = new QPushButton(tr("Insert before"));
	insertDeleteLayout->addWidget(m_insertBeforePushButton);
	m_insertBeforePushButton->setMenu(m_insertBeforeMenu);

	m_insertAfterPushButton = new QPushButton(tr("Insert after"));
	insertDeleteLayout->addWidget(m_insertAfterPushButton);
	m_insertAfterPushButton->setMenu(m_insertAfterMenu);

	m_insertCopyPushButton = new QPushButton(tr("Insert copy"));
	insertDeleteLayout->addWidget(m_insertCopyPushButton);

	m_deletePushButton = new QPushButton(tr("Delete"));
	insertDeleteLayout->addWidget(m_deletePushButton);

	connect(m_insertCopyPushButton, &QPushButton::clicked, this, &X26DockWidget::insertTripletCopy);
	connect(m_deletePushButton, &QPushButton::clicked, this, &X26DockWidget::deleteTriplet);

	x26Layout->addLayout(insertDeleteLayout);

	disableTripletWidgets();

	x26Widget->setLayout(x26Layout);
	this->setWidget(x26Widget);

	m_x26View->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_x26View, &QWidget::customContextMenuRequested, this, &X26DockWidget::customMenuRequested);

	connect(m_x26View->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &X26DockWidget::rowSelected);
}

void X26DockWidget::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
		case Qt::Key_Insert:
			insertTripletCopy();
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
	rowSelected(m_x26View->currentIndex(), QModelIndex());
}

void X26DockWidget::loadX26List()
{
	disableTripletWidgets();
	m_x26Model->setX26ListLoaded(true);
}

void X26DockWidget::unloadX26List()
{
	disableTripletWidgets();
	m_x26Model->setX26ListLoaded(false);
}

void X26DockWidget::rowSelected(const QModelIndex &current, const QModelIndex &previous)
{
	Q_UNUSED(previous);

	if (current.isValid()) {
		updateAllRawTripletSpinBoxes(current);
		updateAllCookedTripletWidgets(current);
	} else
		disableTripletWidgets();
}

void X26DockWidget::disableTripletWidgets()
{
	m_rawTripletAddressSpinBox->setEnabled(false);
	m_rawTripletDataSpinBox->setEnabled(false);
	m_rawTripletModeSpinBox->setEnabled(false);
	m_rawTripletAddressSpinBox->blockSignals(true);
	m_rawTripletModeSpinBox->blockSignals(true);
	m_rawTripletDataSpinBox->blockSignals(true);
	m_rawTripletAddressSpinBox->setValue(0);
	m_rawTripletModeSpinBox->setValue(0);
	m_rawTripletDataSpinBox->setValue(0);
	m_rawTripletAddressSpinBox->blockSignals(false);
	m_rawTripletModeSpinBox->blockSignals(false);
	m_rawTripletDataSpinBox->blockSignals(false);

	m_cookedRowSpinBox->setEnabled(false);
	m_cookedColumnSpinBox->setEnabled(false);
	m_cookedRowSpinBox->blockSignals(true);
	m_cookedColumnSpinBox->blockSignals(true);
	m_cookedRowSpinBox->setValue(1);
	m_cookedColumnSpinBox->setValue(0);
	m_cookedRowSpinBox->blockSignals(false);
	m_cookedColumnSpinBox->blockSignals(false);

	m_cookedModePushButton->setEnabled(false);
	m_cookedModePushButton->setText(QString());

	m_tripletParameterStackedLayout->setCurrentIndex(0);
}

void X26DockWidget::updateAllCookedTripletWidgets(const QModelIndex &index)
{
	const int modeExt = index.model()->data(index.model()->index(index.row(), 2), Qt::EditRole).toInt();

	m_cookedModePushButton->setEnabled(true);
	m_cookedModePushButton->setText(m_modeTripletNames.modeName(modeExt));

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
		case 0x2f: // G2 character
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
		case 0x38:
		case 0x39:
		case 0x3a:
		case 0x3b:
		case 0x3c:
		case 0x3d:
		case 0x3e:
		case 0x3f: // G0 character with diacritical
			m_characterCodeComboBox->blockSignals(true);
			if (modeExt == 0x21)
				m_characterListModel.setG1AndBlastCharacterSet(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+3).toInt());
			else if (modeExt == 0x22 || modeExt == 0x2b)
				m_characterListModel.setCharacterSet(26);
			else
				m_characterListModel.setCharacterSet(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+2).toInt());
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
		case 0x11: // Invoke Active Object
		case 0x12: // Invoke Adaptive Object
		case 0x13: // Invoke Passive Object
		case 0x15: // Define Active Object
		case 0x16: // Define Adaptive Object
		case 0x17: // Define Passive Object
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
				const bool tripletLocationWidgetsVisible = (modeExt & 0x04) != 0x04;

				m_invokeLocalObjectDesignationCodeLabel->setVisible(tripletLocationWidgetsVisible);
				m_invokeLocalObjectDesignationCodeSpinBox->setVisible(tripletLocationWidgetsVisible);
				m_invokeLocalObjectTripletNumberLabel->setVisible(tripletLocationWidgetsVisible);
				m_invokeLocalObjectTripletNumberSpinBox->setVisible(tripletLocationWidgetsVisible);
				m_objectSourceComboBox->setCurrentIndex(0);
				m_invokeObjectSourceStackedLayout->setCurrentIndex(0);
				if (tripletLocationWidgetsVisible) {
					m_invokeLocalObjectDesignationCodeSpinBox->blockSignals(true);
					m_invokeLocalObjectTripletNumberSpinBox->blockSignals(true);
					m_invokeLocalObjectDesignationCodeSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+2).toInt());
					m_invokeLocalObjectTripletNumberSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+3).toInt());
					m_invokeLocalObjectDesignationCodeSpinBox->blockSignals(false);
					m_invokeLocalObjectTripletNumberSpinBox->blockSignals(false);
				}
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
			m_DRCSModeRequiredAtLevelsComboBox->blockSignals(true);
			m_DRCSModeGlobalRadioButton->blockSignals(true);
			m_DRCSModeNormalRadioButton->blockSignals(true);
			m_DRCSModeSubPageSpinBox->blockSignals(true);
			m_DRCSModeRequiredAtLevelsComboBox->setCurrentIndex(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+1).toInt());
			m_DRCSModeGlobalRadioButton->setChecked(!index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+3).toBool());
			m_DRCSModeNormalRadioButton->setChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+3).toBool());
			m_DRCSModeSubPageSpinBox->setValue(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+4).toInt());
			m_DRCSModeRequiredAtLevelsComboBox->blockSignals(false);
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
	const QVariant rowVariant = index.model()->data(index.model()->index(index.row(), 0), Qt::EditRole);
	if (rowVariant.isNull()) {
		m_cookedRowSpinBox->setEnabled(false);
		m_cookedRowSpinBox->setValue(0);
		m_cookedRowSpinBox->setPrefix("");
	} else {
		m_cookedRowSpinBox->setEnabled(true);
		if (modeExt == 0x10) {
			m_cookedRowSpinBox->setRange(0, 23);
			m_cookedRowSpinBox->setPrefix("+");
		} else {
			m_cookedRowSpinBox->setRange(1, 24);
			m_cookedRowSpinBox->setPrefix("");
		}
		m_cookedRowSpinBox->setValue(rowVariant.toInt());
	}
	const QVariant columnVariant = index.model()->data(index.model()->index(index.row(), 1), Qt::EditRole);
	if (columnVariant.isNull()) {
		m_cookedColumnSpinBox->setEnabled(false);
		m_cookedColumnSpinBox->setValue(0);
		m_cookedColumnSpinBox->setPrefix("");
	} else {
		m_cookedColumnSpinBox->setEnabled(true);
		if (modeExt == 0x10) {
			m_cookedColumnSpinBox->setMaximum(71);
			m_cookedColumnSpinBox->setPrefix("+");
		} else {
			m_cookedColumnSpinBox->setMaximum(39);
			m_cookedColumnSpinBox->setPrefix("");
		}
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

void X26DockWidget::cookedModeMenuSelected(const int value)
{
	if (!m_x26View->currentIndex().isValid())
		return;

	m_x26Model->setData(m_x26Model->index(m_x26View->currentIndex().row(), 2), value, Qt::EditRole);

	updateAllRawTripletSpinBoxes(m_x26View->currentIndex());
	updateAllCookedTripletWidgets(m_x26View->currentIndex());
}


void X26DockWidget::updateModelFromCookedWidget(const int value, const int role)
{
	m_x26Model->setData(m_x26Model->index(m_x26View->currentIndex().row(), 0), value, role);
	updateAllRawTripletSpinBoxes(m_x26View->currentIndex());
}

void X26DockWidget::insertTriplet(int modeExt, bool after)
{
	QModelIndex index = m_x26View->currentIndex();

	if (index.isValid())
		insertTriplet(modeExt, index.row()+after);
	else
		insertTriplet(modeExt);
}

void X26DockWidget::insertTriplet(int modeExt, int row)
{
	X26Triplet newTriplet(modeExt < 0x20 ? 41 : 0, modeExt & 0x1f, 0);

	if (row != -1) {
		QModelIndex index = m_x26View->currentIndex();

		if (index.isValid()) {
			// If we're inserting a column triplet next to another column triplet,
			// duplicate the column number
			// Avoid the PDC and reserved mode triplets
			if (modeExt >= 0x20 && modeExt != 0x24 && modeExt != 0x25 && modeExt != 0x26 && modeExt != 0x2a) {
				const int existingTripletModeExt = index.model()->data(index.model()->index(index.row(), 2), Qt::EditRole).toInt();

				if (existingTripletModeExt >= 0x20 && existingTripletModeExt != 0x24 && existingTripletModeExt != 0x25 && existingTripletModeExt != 0x26 && existingTripletModeExt != 0x2a)
					newTriplet.setAddress(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole).toInt());
			}
			// If we're inserting a Set Active Position or Full Row Colour triplet,
			// look for a previous row setting triplet and set this one to the row after
			if (modeExt == 0x04 || modeExt == 0x01) {
				for (int i=row-1; i>=0; i--) {
					const int scanTripletModeExt = index.model()->data(index.model()->index(i, 2), Qt::EditRole).toInt();

					if (scanTripletModeExt == 0x04 || scanTripletModeExt == 0x01) {
						const int scanActivePositionRow = index.model()->data(index.model()->index(i, 0), Qt::EditRole).toInt()+1;

						if (scanActivePositionRow < 25)
							newTriplet.setAddressRow(scanActivePositionRow);
						else
							newTriplet.setAddressRow(24);

						break;
					}
				}
			}
		}
	} else
		row = 0;

	// For character triplets, ensure Data is not reserved
	if (modeExt == 0x21 || modeExt == 0x22 || modeExt == 0x29 || modeExt == 0x2b || modeExt >= 0x2f)
		newTriplet.setData(0x20);
	// For Address Row 0, set Address
	if (modeExt == 0x07)
		newTriplet.setAddress(63);
	// For Termination Marker, set Address and Mode
	if (modeExt == 0x1f) {
		newTriplet.setAddress(63);
		newTriplet.setData(7);
	}

	m_x26Model->insertRows(row, 1, QModelIndex(), newTriplet);
}

void X26DockWidget::insertTripletCopy()
{
	QModelIndex index = m_x26View->currentIndex();

	if (index.isValid())
		m_x26Model->insertRow(index.row(), QModelIndex());
	else
		// No existing triplet to copy, so insert a Termination Marker
		m_x26Model->insertRows(0, 1, QModelIndex(), X26Triplet(63, 31, 7));
}

void X26DockWidget::deleteTriplet()
{
	QModelIndex index = m_x26View->currentIndex();
	if (index.isValid())
		m_x26Model->removeRow(index.row(), index.parent());
}

void X26DockWidget::customMenuRequested(QPoint pos)
{
	QMenu *customMenu = nullptr;

	QModelIndex index = m_x26View->indexAt(pos);

	if (index.isValid()) {
		const int modeExt = index.model()->data(index.model()->index(index.row(), 2), Qt::EditRole).toInt();

		switch (modeExt) {
			case 0x11:
			case 0x12:
			case 0x13: {  // Invoke Object
				QMenu *localDefMenu;
				const QList objectList = m_parentMainWidget->document()->currentSubPage()->enhancements()->objects(modeExt - 0x11);

				for (int i=0; i<objectList.size(); i++) {
					// Messy way to create submenu only if definitions exist
					if (i == 0) {
						customMenu = new QMenu(this);
						localDefMenu = customMenu->addMenu(tr("Local definition"));
					}

					const int d = objectList.at(i) / 13;
					const int t = objectList.at(i) % 13;
					QAction *action = localDefMenu->addAction(QString("d%1 t%2").arg(d).arg(t));
					connect(action, &QAction::triggered, [=]() {
						updateModelFromCookedWidget(0, Qt::UserRole+1);
						updateModelFromCookedWidget(d, Qt::UserRole+2);
						updateModelFromCookedWidget(t, Qt::UserRole+3);
						updateAllCookedTripletWidgets(index);
					} );
				}
			} break;
			case 0x01: // Full Row colour
			case 0x07: // Address row 0
				customMenu = new TripletCLUTQMenu(true, this);

				connect(static_cast<TripletCLUTQMenu *>(customMenu)->action(32), &QAction::triggered, [=]() { updateModelFromCookedWidget(0, Qt::UserRole+2); updateAllCookedTripletWidgets(index); });
				connect(static_cast<TripletCLUTQMenu *>(customMenu)->action(33), &QAction::triggered, [=]() { updateModelFromCookedWidget(1, Qt::UserRole+2); updateAllCookedTripletWidgets(index); });
				// fall-through
			case 0x00: // Full Screen colour
			case 0x20: // Foreground colour
			case 0x23: // Background colour
				if (!customMenu)
					customMenu = new TripletCLUTQMenu(false, this);

				for (int i=0; i<32; i++) {
					static_cast<TripletCLUTQMenu *>(customMenu)->setColour(i, m_parentMainWidget->document()->currentSubPage()->CLUTtoQColor(i));
					connect(static_cast<TripletCLUTQMenu *>(customMenu)->action(i), &QAction::triggered, [=]() { updateModelFromCookedWidget(i, Qt::UserRole+1); updateAllCookedTripletWidgets(index); });
				}
				break;
			case 0x27: // Additional flash functions
				customMenu = new TripletFlashQMenu(this);

				static_cast<TripletFlashQMenu *>(customMenu)->setModeChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+1).toInt());
				static_cast<TripletFlashQMenu *>(customMenu)->setRatePhaseChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+2).toInt());

				for (int i=0; i<4; i++)
					connect(static_cast<TripletFlashQMenu *>(customMenu)->modeAction(i), &QAction::triggered, [=]() { updateModelFromCookedWidget(i, Qt::UserRole+1); updateAllCookedTripletWidgets(index); });
				for (int i=0; i<6; i++)
					connect(static_cast<TripletFlashQMenu *>(customMenu)->ratePhaseAction(i), &QAction::triggered, [=]() { updateModelFromCookedWidget(i, Qt::UserRole+2); updateAllCookedTripletWidgets(index); });
				break;
			case 0x2c: // Display attributes
				customMenu = new TripletDisplayAttrsQMenu(this);

				static_cast<TripletDisplayAttrsQMenu *>(customMenu)->setTextSizeChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+1).toInt());
				for (int i=0; i<4; i++) {
					static_cast<TripletDisplayAttrsQMenu *>(customMenu)->setOtherAttrChecked(i, index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+i+2).toBool());
					connect(static_cast<TripletDisplayAttrsQMenu *>(customMenu)->textSizeAction(i), &QAction::triggered, [=]() { updateModelFromCookedWidget(i, Qt::UserRole+1); updateAllCookedTripletWidgets(index); });
					connect(static_cast<TripletDisplayAttrsQMenu *>(customMenu)->otherAttrAction(i), &QAction::toggled, [=](const int checked) { updateModelFromCookedWidget(checked, Qt::UserRole+i+2); updateAllCookedTripletWidgets(index); });
				}
				break;
			case 0x2e: // Font style
				customMenu = new TripletFontStyleQMenu(this);

				for (int i=0; i<3; i++) {
					static_cast<TripletFontStyleQMenu *>(customMenu)->setStyleChecked(i, index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+i+1).toBool());
					connect(static_cast<TripletFontStyleQMenu *>(customMenu)->styleAction(i), &QAction::toggled, [=](const int checked) { updateModelFromCookedWidget(checked, Qt::UserRole+i+1); updateAllCookedTripletWidgets(index); });
				}
				static_cast<TripletFontStyleQMenu *>(customMenu)->setRowsChecked(index.model()->data(index.model()->index(index.row(), 0), Qt::UserRole+4).toInt());
				for (int i=0; i<8; i++)
					connect(static_cast<TripletFontStyleQMenu *>(customMenu)->rowsAction(i), &QAction::triggered, [=]() { updateModelFromCookedWidget(i, Qt::UserRole+4); updateAllCookedTripletWidgets(index); });
				break;
			case 0x21: // G1 mosaic character
				customMenu = new TripletCharacterQMenu(m_x26Model->data(index.model()->index(index.row(), 2), Qt::UserRole+3).toInt(), true, this);
				// fall-through
			case 0x22: // G3 mosaic character at level 1.5
			case 0x2b: // G3 mosaic character at level >=2.5
			case 0x29: // G0 character
			case 0x2f: // G2 character
			case 0x30:
			case 0x31:
			case 0x32:
			case 0x33:
			case 0x34:
			case 0x35:
			case 0x36:
			case 0x37:
			case 0x38:
			case 0x39:
			case 0x3a:
			case 0x3b:
			case 0x3c:
			case 0x3d:
			case 0x3e:
			case 0x3f: // G0 character with diacritical
				if (!customMenu)
					customMenu = new TripletCharacterQMenu(m_x26Model->data(index.model()->index(index.row(), 2), Qt::UserRole+2).toInt(), false, this);

				for (int i=0; i<96; i++)
					connect(static_cast<TripletCharacterQMenu *>(customMenu)->action(i), &QAction::triggered, [=]() { updateModelFromCookedWidget(i+32, Qt::UserRole+1); updateAllCookedTripletWidgets(index); });
				break;
		}

		if (customMenu)
			customMenu->addSeparator();
		else
			customMenu = new QMenu(this);

		TripletModeQMenu *modeChangeMenu = new TripletModeQMenu(this);
		modeChangeMenu->setTitle(tr("Change triplet mode"));
		customMenu->addMenu(modeChangeMenu);

		customMenu->addSeparator();

		TripletModeQMenu *insertBeforeQMenu = new TripletModeQMenu(this);
		insertBeforeQMenu->setTitle(tr("Insert before"));
		customMenu->addMenu(insertBeforeQMenu);

		TripletModeQMenu *insertAfterQMenu = new TripletModeQMenu(this);
		insertAfterQMenu->setTitle(tr("Insert after"));
		customMenu->addMenu(insertAfterQMenu);

		for (int i=0; i<64; i++) {
			connect(static_cast<TripletModeQMenu *>(modeChangeMenu)->action(i), &QAction::triggered, [=]() { cookedModeMenuSelected(i); });
			connect(static_cast<TripletModeQMenu *>(insertBeforeQMenu)->action(i), &QAction::triggered, [=]() { insertTriplet(i, false); });
			connect(static_cast<TripletModeQMenu *>(insertAfterQMenu)->action(i), &QAction::triggered, [=]() { insertTriplet(i, true); });
		}

		QAction *insertCopyAct = new QAction(tr("Insert copy"), this);
		customMenu->addAction(insertCopyAct);
		connect(insertCopyAct, &QAction::triggered, this, &X26DockWidget::insertTripletCopy);

		QAction *deleteAct = new QAction("Delete triplet", this);
		customMenu->addAction(deleteAct);
		connect(deleteAct, &QAction::triggered, this, &X26DockWidget::deleteTriplet);
	} else {
		customMenu = new QMenu(this);

		TripletModeQMenu *appendModeMenu = new TripletModeQMenu(this);
		appendModeMenu->setTitle(tr("Append"));
		customMenu->addMenu(appendModeMenu);

		for (int i=0; i<64; i++)
			connect(static_cast<TripletModeQMenu *>(appendModeMenu)->action(i), &QAction::triggered, [=]() { insertTriplet(i, m_x26Model->rowCount()); });
	}

	customMenu->popup(m_x26View->viewport()->mapToGlobal(pos));
}
