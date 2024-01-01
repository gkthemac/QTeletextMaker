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

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPair>
#include <QRegularExpressionValidator>
#include <QString>

#include "pagecomposelinksdockwidget.h"

PageComposeLinksDockWidget::PageComposeLinksDockWidget(TeletextWidget *parent): QDockWidget(parent)
{
	QVBoxLayout *pageComposeLinksLayout = new QVBoxLayout;
	QWidget *pageComposeLinksWidget = new QWidget;

	m_parentMainWidget = parent;

	this->setObjectName("PageComposeLinksDockWidget");
	this->setWindowTitle("Compositional links");

	QGridLayout *x27Layout = new QGridLayout;

	QLabel *levelAllLabel = new QLabel(tr("Levels 2.5 and 3.5"));
	levelAllLabel->setAlignment(Qt::AlignCenter);
	x27Layout->addWidget(levelAllLabel, 0, 0, 1, 5);

	// Link functions
	x27Layout->addWidget(new QLabel(tr("GPOP")),  2, 0, 1, 1);
	x27Layout->addWidget(new QLabel(tr("POP")),   3, 0, 1, 1);
	x27Layout->addWidget(new QLabel(tr("GDRCS")), 4, 0, 1, 1);
	x27Layout->addWidget(new QLabel(tr("DRCS")),  5, 0, 1, 1);

	// Labels across the top
	x27Layout->addWidget(new QLabel(tr("L2.5")), 1, 1, 1, 1);
	x27Layout->addWidget(new QLabel(tr("L3.5")),  1, 2, 1, 1);
	x27Layout->addWidget(new QLabel(tr("Page no")), 1, 3, 1, 1);
	x27Layout->addWidget(new QLabel(tr("Sub pages required")), 1, 4, 1, 1);

	QLabel *level3p5OnlyLabel = new QLabel(tr("Level 3.5 only"));
	level3p5OnlyLabel->setAlignment(Qt::AlignCenter);
	x27Layout->addWidget(level3p5OnlyLabel, 6, 0, 1, 5);

	m_pageNumberValidator = new QRegularExpressionValidator(QRegularExpression("[1-8][0-9A-Fa-f][0-9A-Fa-f]"), this);

	for (int i=0; i<8; i++) {
		if (i < 4) {
			// Required at which Levels
			m_composeLinkLevelCheckbox[i][0] = new QCheckBox(this);
			x27Layout->addWidget(m_composeLinkLevelCheckbox[i][0], i+2, 1, 1, 1);
			connect(m_composeLinkLevelCheckbox[i][0], &QCheckBox::stateChanged, [=](bool active) { m_parentMainWidget->document()->currentSubPage()->setComposeLinkLevel2p5(i, active); });
			m_composeLinkLevelCheckbox[i][1] = new QCheckBox(this);
			x27Layout->addWidget(m_composeLinkLevelCheckbox[i][1], i+2, 2, 1, 1);
			connect(m_composeLinkLevelCheckbox[i][1], &QCheckBox::stateChanged, [=](bool active) { m_parentMainWidget->document()->currentSubPage()->setComposeLinkLevel3p5(i, active); });
		} else {
			// Selectable link functions for Level 3.5
			m_composeLinkFunctionComboBox[i-4] = new QComboBox;
			m_composeLinkFunctionComboBox[i-4]->addItem("GPOP");
			m_composeLinkFunctionComboBox[i-4]->addItem("POP");
			m_composeLinkFunctionComboBox[i-4]->addItem("GDRCS");
			m_composeLinkFunctionComboBox[i-4]->addItem("DRCS");
			x27Layout->addWidget(m_composeLinkFunctionComboBox[i-4], i+3, 0, 1, 1, Qt::AlignTop);
			connect(m_composeLinkFunctionComboBox[i-4], QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) { m_parentMainWidget->document()->currentSubPage()->setComposeLinkFunction(i, index); } );
		}
		// Page link
		m_composeLinkPageNumberLineEdit[i] = new QLineEdit("100");
		m_composeLinkPageNumberLineEdit[i]->setMaxLength(3);
		m_composeLinkPageNumberLineEdit[i]->setInputMask(">DHH");
		m_composeLinkPageNumberLineEdit[i]->setValidator(m_pageNumberValidator);
		x27Layout->addWidget(m_composeLinkPageNumberLineEdit[i], i+(i<4 ? 2 : 3), 3, 1, 1);
		connect(m_composeLinkPageNumberLineEdit[i], &QLineEdit::textEdited, [=](QString value) { setComposeLinkPageNumber(i, value); } );

		// Sub page numbers required
		m_composeLinkSubPageNumbersLineEdit[i] = new QLineEdit(this);
		x27Layout->addWidget(m_composeLinkSubPageNumbersLineEdit[i], i+(i<4 ? 2 : 3), 4, 1, 1);
		connect(m_composeLinkSubPageNumbersLineEdit[i], &QLineEdit::textEdited, [=](QString value) { setComposeLinkSubPageNumbers(i, value); } );
	}

	pageComposeLinksLayout->addLayout(x27Layout);

	pageComposeLinksLayout->addStretch(1);

	pageComposeLinksWidget->setLayout(pageComposeLinksLayout);
	this->setWidget(pageComposeLinksWidget);
}

void PageComposeLinksDockWidget::setComposeLinkPageNumber(int linkNumber, const QString &newPageNumberString)
{
	// The LineEdit should check if a valid hex number was entered, but just in case...
	bool newPageNumberOk;
	int newPageNumberRead = newPageNumberString.toInt(&newPageNumberOk, 16);
	if ((!newPageNumberOk) || newPageNumberRead < 0x100 || newPageNumberRead > 0x8ff)
		return;

	// Stored as page link with relative magazine number, convert from absolute page number that was entered
	newPageNumberRead ^= (m_parentMainWidget->document()->pageNumber() & 0x700);
	m_parentMainWidget->document()->currentSubPage()->setComposeLinkPageNumber(linkNumber, newPageNumberRead);
}

void PageComposeLinksDockWidget::setComposeLinkSubPageNumbers(int linkNumber, const QString &newSubPagesString)
{
	int newSubPageCodes = 0x0000;

	if (!newSubPagesString.isEmpty()) {
		// Turn user-entered comma separated subpages and subpage-ranges into bits
		// First we do the "comma separated"
		const QStringList items = newSubPagesString.split(QLatin1Char(','), Qt::SkipEmptyParts);
		// Now see if there's valid single numbers or ranges between the commas
		for (const QString &item : items) {
			if (item.isEmpty())
				return;
			if (item.contains(QLatin1Char('-'))) {
				// Dash found, should be a range of subpages: check for two things either side of dash
				const QStringList rangeItems = item.split('-');
				if (rangeItems.count() != 2)
					return;
				// Now check if the two things are valid numbers 0-15, with first number less than second
				bool ok1, ok2;
				const int number1 = rangeItems[0].toInt(&ok1);
				const int number2 = rangeItems[1].toInt(&ok2);
				if (!ok1 || !ok2 || number1 < 0 || number2 < 0 || number1 > 15 || number2 > 15 || number2 < number1)
					return;
				// Yes, it is a valid range. Apply it to the result
				for (int i=number1; i<=number2; i++)
					newSubPageCodes |= (1 << i);
			} else {
				// A single thing (maybe extracted between commas): check if the thing is a number 0-15
				bool ok;
				const int number = item.toInt(&ok);
				if (!ok || number < 0 || number > 15)
					return;
				// Yes, it is a valid number. Apply it to the result
				newSubPageCodes |= (1 << number);
			}
		}
	}

	m_parentMainWidget->document()->currentSubPage()->setComposeLinkSubPageCodes(linkNumber, newSubPageCodes);
}

void PageComposeLinksDockWidget::updateWidgets()
{
	for (int i=0; i<8; i++) {
		if (i >= 4) {
			m_composeLinkFunctionComboBox[i-4]->blockSignals(true);
			m_composeLinkFunctionComboBox[i-4]->setCurrentIndex(m_parentMainWidget->document()->currentSubPage()->composeLinkFunction(i));
			m_composeLinkFunctionComboBox[i-4]->blockSignals(false);
		} else {
			m_composeLinkLevelCheckbox[i][0]->blockSignals(true);
			m_composeLinkLevelCheckbox[i][0]->setChecked(m_parentMainWidget->document()->currentSubPage()->composeLinkLevel2p5(i));
			m_composeLinkLevelCheckbox[i][0]->blockSignals(false);
			m_composeLinkLevelCheckbox[i][1]->blockSignals(true);
			m_composeLinkLevelCheckbox[i][1]->setChecked(m_parentMainWidget->document()->currentSubPage()->composeLinkLevel3p5(i));
			m_composeLinkLevelCheckbox[i][1]->blockSignals(false);
		}
		// Stored as page link with relative magazine number, convert to absolute page number for display
		int absoluteLinkPageNumber = m_parentMainWidget->document()->currentSubPage()->composeLinkPageNumber(i) ^ (m_parentMainWidget->document()->pageNumber() & 0x700);
		// Fix magazine 0 to 8
		if ((absoluteLinkPageNumber & 0x700) == 0x000)
			absoluteLinkPageNumber |= 0x800;
		m_composeLinkPageNumberLineEdit[i]->blockSignals(true);
		m_composeLinkPageNumberLineEdit[i]->setText(QString::number(absoluteLinkPageNumber, 16).toUpper());
		m_composeLinkPageNumberLineEdit[i]->blockSignals(false);

		// Turn subpage bits into user-friendly comma separated numbers and number-ranges
		QString rangeString;

		if (m_parentMainWidget->document()->currentSubPage()->composeLinkSubPageCodes(i) != 0x0000) {
			// First build a list of consecutive ranges seen
			// The "b-index" is based on https://codereview.stackexchange.com/a/90074
			QMap<int, QPair<int, int>> ranges;
			int index = 0;
			for (int b=0; b<16; b++)
				if ((m_parentMainWidget->document()->currentSubPage()->composeLinkSubPageCodes(i) >> b) & 0x01) {
					if (!ranges.contains(b-index))
						ranges.insert(b-index, qMakePair(b, b));
					else
						ranges[b-index].second = b;
					index++;
				}
			// Now go through the list and add single numbers or ranges as appropriate
			QPair<int, int> range;
			foreach (range, ranges) {
				// For second and subsequent entries only, append a comma first
				if (!rangeString.isEmpty())
					rangeString.append(',');
				// Append the single number or the first number of the range
				rangeString.append(QString("%1").arg(range.first));
				// If that was part of a range and not a single orphaned number
				if (range.first != range.second) {
					// Ranges of 3 or more use a dash. A range of 2 can make do with a comma
					rangeString.append((range.first+1 == range.second) ? ',' : '-');
					// Append the second number of the range
					rangeString.append(QString("%1").arg(range.second));
				}
			}
		}
		m_composeLinkSubPageNumbersLineEdit[i]->blockSignals(true);
		m_composeLinkSubPageNumbersLineEdit[i]->setText(rangeString);
		m_composeLinkSubPageNumbersLineEdit[i]->blockSignals(false);
	}
}
