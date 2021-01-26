# QTeletextMaker
QTeletextMaker is a teletext page editor in development. It is written in C++ using the Qt 5 widget libraries, and released under the GNU General Public License v3.0

Features
- Load and save teletext pages in .tti format.
- Rendering of teletext pages in Levels 1, 1.5, 2.5 and 3.5
- Rendering of Local Objects and side panels.
- Interactive X/26 Local Enhancement Data triplet editor.
- Editing of X/27/4 and X/27/5 compositional links to enhancement data pages.
- Palette editor.
- Configurable zoom.
- View teletext pages in 4:3, 16:9 pillar-box and 16:9 stretch aspect ratios.

Although designed on and developed for Linux, the Qt 5 libraries are cross platform so hopefully it should be compilable on Windows. And maybe MacOS as well.

## Building
### Linux
Install the QtCore, QtGui and QtWidgets libraries and build headers, along with the qmake tool. Then type `qmake && make -j3` in a terminal, you can replace -j3 with the number of processor cores used for the compile process.

The above will place the qteletextmaker executable in the same directory as the source, type `./qteletextmaker` in the terminal to launch. Optionally, type `make install` afterwards to place the executable into /usr/local/bin.

## Current limitations
At the moment QTeletextMaker is in pre-alpha status so many features are not finished yet. The most-often encountered limitations are detailed here.

Keymapping between ASCII and Teletext character sets is not yet implemented e.g. in English pressing the hash key will type a pound sign instead, and in other languages the square and curly brackets keys need to be pressed to obtain the accented characters.

The following X/26 enhancement triplets are not rendered by the editor, although the list is fully aware of them.
- Invocation of Objects from POP and GPOP pages.
- DRCS characters.
- Modified G0 and G2 character set designation using X/26 triplets with mode 01000.
- Full screen and full row colours set by Active Objects.
- Level 3.5 font style: bold, italic and proportional spacing.

## Using the X/26 triplet editor
The X/26 triplet editor sorts all the triplet modes available into categories selected by the triplet *type* dropdown on the left. The categories are:
- Set Active Position
- Row triplet
- Column triplet
- Object
- Terminator
- PDC/reserved

Selecting "Set Active Position" or "Terminator" will change the triplet mode immediately, other selections will activate the triplet *mode* dropdown on the right which can be used to then change the triplet mode. Most triplet modes will present varying widgets below which can be used to alter the parameters of the currently selected triplet (e.g. colour or character).

Between the two dropdowns are the Row and Column spinboxes that are used to place the Active Position of the selected triplet, whether or not each spinbox can be altered depends on the mode of the selected triplet. As well as the explicit "Set Active Position" triplet that can set both the row and the column, all column triplets can simultaneously set the column of the Active Position. Additionally the Full Row Colour triplet can set the row of the Active Position, with the column always set to 0.

By checking "raw values" it is also possible to view and edit the raw Address, Mode and Data numbers of the triplets. When editing triplets this way, remember that address values 0-39 select a column triplet which has one set of modes and address values 40-63 select a row triplet which has a different set of modes.

The full behaviour of X/26 enhancement triplets can be found in section 12.3 of the [Enhanced Teletext specification ETS 300 706](https://web.archive.org/web/20160326062859/https://www.phecap.nl/download/enhenced-teletext-specs.pdf).

### Setting the Active Position
The Active Position, whether set explicitly by a "Set Active Position" triplet or by a row or column triplet, can only be moved in screen address order i.e. from left to right within a row and then from top to bottom across rows. In other words:
- The Active Position can never be moved up to a lesser numbered row.
- The Active Position can never be moved left *within the same row* to a lesser numbered column, but it can be moved left at the same time as it is moved down to a greater numbered row.

If this rule is not followed then triplets in earlier screen addresses will be ignored.

### Objects
"Define ... Object" triplets need to declare that they are in the correct place in the triplet list e.g. if the Define Object triplet is at `d1 t3` in the list then the data field must show `Local: d1 t3`, otherwise the Object won't appear.

Insert and deleting triplets from the list will upset the Object pointers on both "Define" and "Invoke" triplets and will need to be corrected afterwards. A future version of the editor may adjust these pointers automatically.

"Invoke ... Object" triplets must point to a "Define ... Object" of the same type e.g. "Invoke *Active* Object" must point to a "Define *Active* Object", otherwise the Object won't appear.
