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

#include <QByteArray>

#include "pagex26base.h"

QByteArray PageX26Base::packetFromEnhancementList(int packetNumber) const
{
	QByteArray result(40, 0x00);

	int enhanceListPointer;
	X26Triplet lastTriplet;

	for (int i=0; i<13; i++) {
		enhanceListPointer = packetNumber*13+i;

		if (enhanceListPointer < m_enhancements.size()) {
			result[i*3+1] = m_enhancements.at(enhanceListPointer).address();
			result[i*3+2] = m_enhancements.at(enhanceListPointer).mode() | ((m_enhancements.at(enhanceListPointer).data() & 1) << 5);
			result[i*3+3] = m_enhancements.at(enhanceListPointer).data() >> 1;

			// If this is the last triplet, get a copy to repeat to the end of the packet
			if (enhanceListPointer == m_enhancements.size()-1) {
				lastTriplet = m_enhancements.at(enhanceListPointer);
				// If the last triplet was NOT a Termination Marker, make up one
				if (lastTriplet.mode() != 0x1f || lastTriplet.address() != 0x3f) {
					lastTriplet.setAddress(0x3f);
					lastTriplet.setMode(0x1f);
					lastTriplet.setData(0x07);
				}
			}
		} else {
			// We've gone past the end of the triplet list, so repeat the Termination Marker to the end
			result[i*3+1] = lastTriplet.address();
			result[i*3+2] = lastTriplet.mode() | ((lastTriplet.data() & 1) << 5);
			result[i*3+3] = lastTriplet.data() >> 1;
		}
	}

	return result;
}

void PageX26Base::setEnhancementListFromPacket(int packetNumber, QByteArray packetContents)
{
	// Preallocate entries in the m_enhancements list to hold our incoming triplets.
	// We write "dummy" reserved 11110 Row Triplets in the allocated entries which then get overwritten by the packet contents.
	// This is in case of missing packets so we can keep Local Object pointers valid.
	while (m_enhancements.size() < (packetNumber+1)*13)
		m_enhancements.append(m_paddingX26Triplet);

	int enhanceListPointer;
	X26Triplet newX26Triplet;

	for (int i=0; i<13; i++) {
		enhanceListPointer = packetNumber*13+i;

		newX26Triplet.setAddress(packetContents.at(i*3+1) & 0x3f);
		newX26Triplet.setMode(packetContents.at(i*3+2) & 0x1f);
		newX26Triplet.setData(((packetContents.at(i*3+3) & 0x3f) << 1) | ((packetContents.at(i*3+2) & 0x20) >> 5));
		m_enhancements.replace(enhanceListPointer, newX26Triplet);
	}
	if (newX26Triplet.mode() == 0x1f && newX26Triplet.address() == 0x3f && newX26Triplet.data() & 0x01)
		// Last triplet was a Termination Marker (without ..follows) so clean up the repeated ones
		while (m_enhancements.size()>1 && m_enhancements.at(m_enhancements.size()-2).mode() == 0x1f && m_enhancements.at(m_enhancements.size()-2).address() == 0x3f && m_enhancements.at(m_enhancements.size()-2).data() == newX26Triplet.data())
			m_enhancements.removeLast();
}
