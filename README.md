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

The above will place the qteletextmaker executable in the same directory as the source, type `./qteletextmaker` in the terminal to launch. Optionally, type `make install` afterwards to place the exectuable into /usr/local/bin.

## Current limitations
At the moment QTeletextMaker is in pre-alpha status so many features are not finished yet. The most-often encountered limitations are detailed here.

Spacing attributes are only accessible through an "Insert" menu. A toolbar to insert spacing attributes may be added in the future.

There is no visible widget for switching between subpages, use the PageUp and PageDown keys for this.

Undo only undoes keypresses, insertion of spacing attributes and CLUT changes. Other changes can't be undone, nor will they cause the "unsaved page" dialog to appear if the editor window is closed.

Keymapping between ASCII and Teletext character sets is not yet implemented e.g. in English pressing the hash key will type a pound sign instead, and in other languages the square and curly brackets keys need to be pressed to obtain the accented characters.

The following X/26 enhancement triplets are not rendered by the editor, although the list is fully aware of them.
- Invocation of Objects from POP and GPOP pages.
- DRCS characters.
- Modified G0 and G2 character set designation using X/26 triplets with mode 01000.
- Full screen and full row colours set by Active Objects.
- Level 3.5 font style: bold, italic and proportional spacing.

## Using the X/26 triplet editor
At present the editor only accepts raw triplet Address, Mode and Data numbers for input although the effects of the Triplet are shown in the list as a full description. The full behaviour of these enhancement triplets can be found in 
section 12.3 of the [Enhanced Teletext specification ETS 300 706](https://web.archive.org/web/20160326062859/https://www.phecap.nl/download/enhenced-teletext-specs.pdf) but some hints and tips are below.

### Address values
Address values 0-39 select a Column Triplet and, with the exception of reserved and PDC Modes, set the column co-ordinate of the Active Position.

Address values 40-63 select a Row Triplet which has a different set of modes. Modes 4 (Set Active Position) and 1 (Full Row Colour) will set the row co-ordinate of the Active Position to the Address value minus 40, except for Address value 40 which will set the row co-ordinate to 24.

### Objects
"Define ... Object" triplets need to declare that they are in the correct place in the triplet list e.g. if the Define Object triplet is at `d1 t3` in the list then the data field must show `Local: d1 t3`, otherwise the Object won't appear.

Insert and deleting triplets from the list will upset the Object pointers on both "Define" and "Invoke" triplets and will need to be corrected afterwards. A future version of the editor may adjust these pointers automatically.

"Invoke ... Object" triplets must point to a "Define ... Object" of the same type e.g. "Invoke *Active* Object" must point to a "Define *Active* Object", otherwise the Object won't appear.
