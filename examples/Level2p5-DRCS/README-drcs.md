# DRCS examples
## Viewing the examples
Each example is in two page files: the main display page and the DRCS downloading page.
- From the "File" menu select "Open".
- Select the TTI file you wish to view with the `MainPage` extension.
- From the "View" menu go to the "DRCS pages" submenu and under "Normal DRCS" select "Open file".
- Select the TTI file with the same name but with the `Nptus` extension.

## DRCS downloading pages
A teletext page can use X/26 triplets to invoke downloaded DRCS characters, but the Pattern Transfer Units (or bitmaps) of the DRCS characters themselves are stored on a separate hidden DRCS downloading page. Any teletext page for display can reference up to two DRCS downloading pages: one "Global" table and one "Normal" table.

## Viewing pages with DRCS characters in QTeletextMaker
Since QTeletextMaker is a single page editor and does not see an entire teletext service, the DRCS downloading page(s) must be loaded manually after the main display page has been loaded in.

All the examples supplied with QTeletextMaker use DRCS characters from the "Normal" table only. For other pages it is required to check whether the page invokes DRCS characters from the "Global" or "Normal" table using the X/26 triplets dockwindow. Where an enhancement triplet mode is listed as "DRCS character" the data will either say "Global" or "Normal". Some pages may use DRCS characters from *both* tables.

From the "View" menu go to the "DRCS pages" submenu where there are two headings: "Global DRCS" and "Normal DRCS". Under each heading is a "Load file" option to load in the DRCS downloading page into that corresponding table, along with a "Clear" option.

If a Global DRCS page is accidentally loaded into the Normal DRCS table or vice versa, the "DRCS pages" submenu has a "Swap Global and Normal" option to correct this.

## Defining DRCS characters
QTeletextMaker does not feature DRCS character bitmap *editing*. The Python script [image2drcs](https://github.com/gkthemac/image2drcs) can be used to convert small bitmaps in various image formats to DRCS downloading pages.
