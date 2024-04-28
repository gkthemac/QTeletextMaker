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

#include "x26menus.h"

#include <QIcon>
#include <QMenu>
#include <QString>

#include "render.h"

TripletModeQMenu::TripletModeQMenu(QWidget *parent): QMenu(parent)
{
	addModeAction(this, 0x04);

	QMenu *rowTripletSubMenu = this->addMenu(tr("Row triplet"));
	addModeAction(rowTripletSubMenu, 0x00);
	addModeAction(rowTripletSubMenu, 0x01);
	addModeAction(rowTripletSubMenu, 0x07);
	addModeAction(rowTripletSubMenu, 0x18);

	QMenu *columnTripletSubMenu = this->addMenu(tr("Column triplet"));
	addModeAction(columnTripletSubMenu, 0x20);
	addModeAction(columnTripletSubMenu, 0x23);
	addModeAction(columnTripletSubMenu, 0x27);
	addModeAction(columnTripletSubMenu, 0x2c);
	addModeAction(columnTripletSubMenu, 0x2e);
	addModeAction(columnTripletSubMenu, 0x28);
	columnTripletSubMenu->addSeparator();
	addModeAction(columnTripletSubMenu, 0x29);
	addModeAction(columnTripletSubMenu, 0x2f);
	addModeAction(columnTripletSubMenu, 0x21);
	addModeAction(columnTripletSubMenu, 0x22);
	addModeAction(columnTripletSubMenu, 0x2b);
	addModeAction(columnTripletSubMenu, 0x2d);

	QMenu *diacriticalSubMenu = columnTripletSubMenu->addMenu(tr("G0 diacritical"));
	for (int i=0; i<16; i++)
		addModeAction(diacriticalSubMenu, 0x30 + i);

	QMenu *objectSubMenu = this->addMenu(tr("Object"));
	addModeAction(objectSubMenu, 0x10);
	addModeAction(objectSubMenu, 0x11);
	addModeAction(objectSubMenu, 0x12);
	addModeAction(objectSubMenu, 0x13);
	addModeAction(objectSubMenu, 0x15);
	addModeAction(objectSubMenu, 0x16);
	addModeAction(objectSubMenu, 0x17);
	addModeAction(this, 0x1f);
	this->addSeparator();

	QMenu *pdcSubMenu = this->addMenu(tr("PDC/reserved"));
	addModeAction(pdcSubMenu, 0x08);
	addModeAction(pdcSubMenu, 0x09);
	addModeAction(pdcSubMenu, 0x0a);
	addModeAction(pdcSubMenu, 0x0b);
	addModeAction(pdcSubMenu, 0x0c);
	addModeAction(pdcSubMenu, 0x0d);
	addModeAction(pdcSubMenu, 0x26);

	QMenu *reservedRowSubMenu = pdcSubMenu->addMenu(tr("Reserved row"));
	addModeAction(reservedRowSubMenu, 0x02);
	addModeAction(reservedRowSubMenu, 0x03);
	addModeAction(reservedRowSubMenu, 0x05);
	addModeAction(reservedRowSubMenu, 0x06);
	addModeAction(reservedRowSubMenu, 0x0e);
	addModeAction(reservedRowSubMenu, 0x0f);
	addModeAction(reservedRowSubMenu, 0x14);
	addModeAction(reservedRowSubMenu, 0x19);
	addModeAction(reservedRowSubMenu, 0x1a);
	addModeAction(reservedRowSubMenu, 0x1b);
	addModeAction(reservedRowSubMenu, 0x1c);
	addModeAction(reservedRowSubMenu, 0x1d);
	addModeAction(reservedRowSubMenu, 0x1e);

	QMenu *reservedColumnSubMenu = pdcSubMenu->addMenu(tr("Reserved column"));
	addModeAction(reservedColumnSubMenu, 0x24);
	addModeAction(reservedColumnSubMenu, 0x25);
	addModeAction(reservedColumnSubMenu, 0x2a);
}

void TripletModeQMenu::addModeAction(QMenu *menu, int mode)
{
	m_actions[mode] = menu->addAction(m_modeTripletNames.modeName(mode));
}


TripletCharacterQMenu::TripletCharacterQMenu(int charSet, QWidget *parent): QMenu(parent)
{
	QMenu *charRange[6];

	for (int r=0; r<6; r++) {
		charRange[r] = this->addMenu(QString("0x%010-0x%01f").arg(r+2));

		for (int c=0; c<16; c++)
			m_actions[r*16+c] = charRange[r]->addAction(QIcon(m_fontBitmap.charBitmap(r*16+c+32, charSet)), QString("0x%1").arg(r*16+c+32, 2, 16, QChar('0')));
	}
}
