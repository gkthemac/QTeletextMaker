# QTeletextMaker
QTeletextMaker is a teletext page editor in development. It is written in C++ using the Qt 5 widget libraries, and released under the GNU General Public License v3.0

Features
- Load and save teletext pages in .tti format.
- Rendering of teletext pages in Levels 1, 1.5, 2.5 and 3.5
- Rendering of Local Objects and side panels.
- Import and export of single pages in .t42 format.
- Export PNG images of teletext pages.
- Undo and redo of editing actions.
- Interactive X/26 Local Enhancement Data triplet editor.
- Editing of X/27/4 and X/27/5 compositional links to enhancement data pages.
- Palette editor.
- Configurable zoom.
- View teletext pages in 4:3, 16:9 pillar-box and 16:9 stretch aspect ratios.

Although designed on and developed for Linux, the Qt 5 libraries are cross platform so a Windows executable can be built. A Windows executable can be found within the "Releases" link, compiled on a Linux host using [MXE](https://github.com/mxe/mxe) based on [these instructions](https://blog.8bitbuddhism.com/2018/08/22/cross-compiling-windows-applications-with-mxe/). After MXE is installed `make qtbase` should build and install the required dependencies to build QTeletextMaker.

## Building
### Linux
Install the QtCore, QtGui and QtWidgets libraries and build headers, along with the qmake tool. Depending on how qmake is installed, type `qmake && make -j3` or `qmake5 && make -j3` in a terminal to build, you can replace -j3 with the number of processor cores used for the compile process.

The above should place the qteletextmaker executable in the same directory as the source, type `./qteletextmaker` in the terminal to launch. Some Qt installs may place the executable into a "release" directory.

Optionally, type `make install` afterwards to place the executable into /usr/local/bin.

## Current limitations
The following X/26 enhancement triplets are not rendered by the editor, although the list is fully aware of them.
- Invocation of Objects from POP and GPOP pages.
- DRCS characters.
- Modified G0 and G2 character set designation using X/26 triplets with mode 01000.
- Full screen and full row colours set by Active Objects.
- Level 3.5 font style: bold, italic and proportional spacing.

## Using the X/26 triplet editor
The X/26 triplet editor sorts all the triplet modes available into categories, which are:
- Set Active Position
- Row triplet - full screen and full row colours, address row 0 and DRCS mode
- Column triplet - non-spacing attributes and overwriting characters
- Object - invocations and definitions
- Terminator
- PDC/reserved

After selecting the triplet mode the Row and Column spinboxes can then be used to place the Active Position of the selected triplet, whether or not each spinbox can be altered depends on the mode of the selected triplet. As well as the explicit "Set Active Position" triplet that can set both the row and the column, all column triplets can simultaneously set the column of the Active Position. Additionally the Full Row Colour triplet can set the row of the Active Position, with the column always set to 0.

Most triplet modes will present varying widgets below which can be used to alter the parameters of the currently selected triplet e.g. colour or character.

By checking "raw values" it is also possible to view and edit the raw Address, Mode and Data numbers of the triplets. When editing triplets this way, remember that address values 0-39 select a column triplet which has one set of modes and address values 40-63 select a row triplet which has a different set of modes.

The full behaviour of X/26 enhancement triplets can be found in section 12.3 of the [Enhanced Teletext specification ETS 300 706](https://web.archive.org/web/20160326062859/https://www.phecap.nl/download/enhenced-teletext-specs.pdf).

### Setting the Active Position
The Active Position, whether set explicitly by a "Set Active Position" triplet or by a row or column triplet, can only be moved in screen address order i.e. from left to right within a row and then from top to bottom across rows. In other words:
- The Active Position can never be moved up to a lesser numbered row.
- The Active Position can never be moved left *within the same row* to a lesser numbered column, but it can be moved left at the same time as it is moved down to a greater numbered row.

If this rule is not followed then triplets in earlier screen addresses will be ignored. Triplets that break this rule will be highlighted red in the X/26 triplet editor.

### Objects
"Invoke ... Object" triplets must point to a "Define ... Object" of the same type e.g. "Invoke *Active* Object" must point to a "Define *Active* Object", otherwise the Object won't appear.
