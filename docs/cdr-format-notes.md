# Notes on the CorelDRAW format

Structures of the CorelDRAW format found while fixing libcdr for nitroview's CorelDRAW support
(https://github.com/gereoffy/libcdr), and how CorelDRAW draws them; one commit each (`git log`
has the details). Little of this is documented elsewhere, least of all for the old versions (4
and 5).

Each finding was checked against CorelDRAW's PDF exports of the same files (CorelDRAW 9 for
versions 4 and 5, CorelDRAW 2022 for the later ones: a PDF gives exact glyph positions, sizes and
colours), against the files' embedded previews, against one document saved by CorelDRAW 2022 in
every version from X5 to 2022, or, where noted, against CorelDRAW 5 itself.

Version numbers are libcdr's (`m_version`): 400 for CorelDRAW 4 (`RIFF....CDR4`), 500 for 5, 1400
for X4 and so on. Values are little-endian; offsets are from the start of a record's data.

A caveat about the CorelDRAW 9 PDFs of old files: CorelDRAW 9 does not have CorelDRAW 4/5's own
fonts (Switzerland, Frankenstein, Stamp, ...) and substitutes them (often with AvantGarde Bk BT),
so those PDFs say nothing about which font such a name stands for. Positions and sizes still match.

## CorelDRAW 4

**Font records.** `font`: u16 id, then the name (NUL-terminated); no encoding field and no 14 bytes
as in version 5 (reading them ran off the record: the files did not open).

**Artistic text.** The characters are inside the object, not in a text record: in a `loda` of
object type 0x04, the argument of type 0x1e is u16 0, u16 the number of characters, u16 (a
length), then 3 bytes a character: u8 flags, u8 character, u8 0. Flag bit 0: 22 more bytes follow
(a per-character style override; not decoded). Lines end in CR LF, one line break. The text's
style is the object's style argument. There is no text id in the record: one must be made up per
object (the stream position is relative to the substream: every text got the same one).

**Fill and outline.** A text's characters take the fill and outline of the text object itself;
its style's are the default text style's (a list of names drawn with an outline and no fill came
out filled black).

**Font style.** In a `styd` font argument: 1 normal, 2 bold, 4 italic (3 not seen); not
CorelDRAW 5's style word (bold headings came out normal).

## CorelDRAW 5

**Font records.** `font`: u16 id, u16 (always 0x40 in the samples: not a character set), 14
bytes, the name. As that field is no character set, the Symbol font's characters are mapped by the
font's name (its code page: S is Σ, m is μ, 0xB3 is ≥; formulas showed Latin letters, logic gates
"³1" for "≥1").

**Paragraph text frames.** The frame's argument: 2 bytes (not 4 as later), then the width and the
height (the height was read as the width, the next field, 1, as the height).

**Bitmaps.** The bitmap spans (0, 0) .. (x, y): its coordinates were read as x1, y1 with
x2 = y2 = 0, a 180 degree rotation.

**Text records (`txsm`).**

    u16 ?, u16 number of frames, frames (u16 id, u16 0) ...,
    u16 number of paragraphs, per paragraph:
        u16 style id, u16 number of style overrides, the overrides (36 bytes each),
        u16 number of characters, the characters (8 bytes each)

A style override; each field is used only when its flag bit is set:

| offset | size | flag | |
|---|---|---|---|
| 0 | u8 | | flags |
| 2 | u8 | 0x01 | font id |
| 3 | u8 | 0x01 | character set (0: the font's) |
| 4 | u16 | 0x02 | font style: 0x40 normal, 0x80 italic, 0x1000 bold, 0x2000 bold italic |
| 9 | u8 | 0x02 | placement: 4 subscript |
| 10 | s16 | 0x04 | font size (16-bit coordinate) |
| 12 | s16 | 0x08 | rotation, 1/10 degree, counter-clockwise |
| 14 | u32 | 0x10 | fill id |
| 18 | u32 | 0x20 | outline id |
| 22 | s16 | 0x40 | horizontal shift, 1/100 em |
| 24 | s16 | 0x80 | vertical shift, 1/100 em, up positive |

(Bytes 1, 6..8 and 26..35 unknown.) The shifts and rotations are what CorelDRAW 5 stores for a
text fitted to a path (each character moved and turned onto the curve; the path itself is not
needed) and for characters moved with the Shape tool. A shift is in the em of the character's own
size; a rotation is about the character's origin on the baseline. A subscript is half the size
and a fifth of the full size lower (24 pt text: 12 pt index, 4.8 pt lower; 12 pt: 6 pt, 2.4 pt
lower); superscript was not seen.

A character: 4 bytes, u8 character, 1 byte, u16 whose bits 3 and up are the style override's
index (`(v >> 3) & 0xfe` as libcdr numbers styles). One byte per character: bit 0 of that value is
not "two-byte character" as later (texts came out as CJK). CR LF is one line break.

**Arrowheads (`arrw`).** u32 id (what an outline's start / end marker field refers to), 2 bytes
(4 from version 6), u16 number of points n, 4 bytes, n point types (as for paths), 1 byte, n
points: s16 along the line, s16 across it. The tip is at the origin, the body at negative "along"
values. libcdr read `arrw` from version 6 only: CorelDRAW 5 lines had none. Size, in every
version: 1000 units are twice the line's width, but no less than 0.028 inch (2.016 pt; measured
on four files: 2.016 pt on 0.216 and 0.72 pt lines, 2.88 pt on 1.44 pt lines; the same in versions
7, 8 and 9). The closed parts of an arrowhead are filled, its open parts (a bar across the tip, an
open "<") are outlined with the line's width (CorelDRAW 9's PDF export closes them, CorelDRAW does
not).

## CorelDRAW 4 and 5 styles (`styd`)

The intervals argument: six u16 percents: character spacing (signed), word spacing, line
spacing, spacing before a paragraph, after it, (unknown); they were skipped. The font argument
holds the font style after the size (see above). The idents argument has four values (indents);
not used yet.

## Text layout, all versions

CorelDRAW's rules as measured on its PDF exports; libcdr passes the values on (line height,
paragraph margins, its own `libcdr:` span properties), the renderer lays the text out so.

- **Line spacing**: a baseline is `spacing × (previous line's descent + this line's ascent)` below
  the previous one. In a paragraph text frame a paragraph's first line is
  `before × (previous descent + ascent) + after(previous paragraph) × (previous ascent + descent)`
  below it instead; artistic text has line spacing only. Versions 8+: the `stlt` interval records
  (52 bytes): u32 id, then (u32 fields) ?, ?, character spacing, ?, word spacing, line spacing,
  before, after, ?, ?, ?, unit (0: shares, 1000000 = 100%; 1 (version 14+): lengths of the line,
  before and after, 1/254000 inch). They were skipped.
- **Character spacing** is a share of a space's width, added after each character; a space's width
  is `space × (word spacing − character spacing)`, so a word's last character and its space
  together take the word spacing alone (measured per character on a PDF, to 0.1/1000 em). Texts
  fitted to a path include it: their characters' places are relative to the spaced layout.
- **Empty lines** have the previous line's ascent and descent, but not more than the text's own
  size gives; an empty paragraph keeps its style. A line break's own size is the text's (the
  line break character may be in a symbol font: it stays a line break, not a converted glyph).
- **A leading line break** makes no line, in every version (texts often start with one to sit a
  line lower than another block; counted, they sat a line lower than meant).
- **Shifts and rotations**: versions 7 to X5 text style fields 0x10 / 0x20 are the characters'
  horizontal / vertical shift (1/100 em) and 0x08 (4 bytes, before them) their rotation in
  1/1000000 degree; X6+ style strings `h_shift`, `v_shift`. PDF imports place each word of a line
  by shifts.
- **Texts fitted to a path**, versions 8 to 15: after the text and its 0 come 24 bytes a
  character: s32 shift x, s32 shift y (1/100 em, up positive), s32 rotation (1/1000000 degree,
  counter-clockwise), 12 bytes 0. Version 16 and later: the same (rotation 0..360 degrees). The
  text is laid out left aligned (the places include the alignment); a mirrored transform mirrors
  the places, not the characters (the vertical shift turns over).
- **Font styles**: the style word is 0x40 normal, 0x80 italic, 0x1000 bold, 0x2000 bold italic, in
  every version; it is read from paragraph styles too (the `stlt` font records: from CorelDRAW 10
  they hold three sizes, Latin, RTL, East Asian, before the styles); X6+ style strings
  `weight` (64 normal, 4096 bold) and `italic`. Font sizes are kept to 1/100 pt.
- **Text outlines** (a style's outline) are drawn over the characters' fill; a text with no fill
  has outlined characters only. X6+ style strings: an outline with a `type` (0: solid) and a
  width is drawn, one without (width 0) is none.
- **Artistic text** is anchored at its transformed origin (the first baseline at the alignment
  point) and turned around it; a stretched or squeezed one keeps its font size, the stretch is in
  its transform (`style:text-scale`). Text frames keep their rotation (three corners give it).
- **Code pages**: CorelDRAW wrote the system's ANSI code page when a text has no character set,
  and old Hungarian files mark Central European text as ANSI or DEFAULT (CorelDRAW 5 files also
  as an East Asian charset: dropped there only): windows-1250 when the text has o/u with double
  acute and no letter Hungarian never uses in windows-1250 (French / Portuguese grave accents, e
  with circumflex, a with tilde: those texts are windows-1252), else windows-1252. CorelDRAW for Mac
  writes charset 0 (or Baltic) with Mac Central European text (letters in 0x80..0x9f). This guess
  is the `--without-icu` build's; with ICU, its charset detector guesses.
- **Version 7 to X5**: a character style's 0x20 field is a fill id (+ 48 bytes from version 15)
  or a single 0 byte, which was not skipped (the text was read one byte off and dropped).

## Other drawing fixes

- Open paths are not filled.
- Layers with flag 0x100 and none of the shown / printed bits are skipped (hidden layers of
  imported PDFs).
- X6+ symbol definitions (`LIST sytr` in root.dat) are not drawn as a page of their own.
- A bitmap's transparency mask (8 bits, after the image in its `bmp` record, with a header of its
  own and no palette) becomes its alpha (a wholly transparent bitmap is left out: alpha 0
  everywhere reads as opaque); a bitmap's outline clips it (`libcdr:clip-path`); a 1-bit
  bitmap takes its object's colours.
- Embedded (OLE) objects: the document's `ole2` record is an OLE storage with an
  `Embedding <n>` storage each; an object of type 0x0a (width, height, two corners) is linked to one
  by its `olel` record (u32 ?, u32 n). The ones whose CONTENTS is an image (Photoshop, PNG, JPEG,
  BMP, GIF) are drawn. From version 16 both records are kept in a data file (the 16-byte form: data
  file number, length, offset).
- `CMYK100` (CorelDRAW 2022's colour model name) is read.
- **Fountain fills** go out as CorelDRAW has them (`libcdr:gradient-type`, `-angle`, `-center-x`
  / `-y`, `-edge`, `-stops`): ODF has no conical gradient, conical fills were drawn linear, square
  ones round. How CorelDRAW draws them, all over the object's bounding box (measured on CorelDRAW
  9 PDFs of one drawing saved as versions 5, 7 and 9; the same in each): the angle (counter-
  clockwise) points from the start colour to the end colour. Linear: across the box's extent along
  the angle; the edge pad (a share of it) is plain start / end colour at each end (30: the colours
  run from 30% to 70%). Radial: a circle (not an ellipse), the end colour in the centre, the start
  colour at the farthest corner. Square: a square turned by the angle, likewise; the edge pad of
  these two shrinks them by twice its share (25: to half the size). Conical: the end colour
  towards the angle, the start colour the other way, both sides alike. A two colour fill's
  midpoint (versions 7+, a byte in percent; 0 in version 5) is where the colours are half mixed,
  evenly on both sides of it; it goes out as a third stop. The centre's offset
  is a share of the box's width / height (up positive; 0.5 puts it on the edge); the vertical one
  was not passed on, and ODF's `svg:cx` / `svg:cy` had half of it (`svg:cy` the horizontal one).
- Every fill is even-odd, as CorelDRAW fills (it was solid fills only: a ring of two circles
  drawn the same way round came out a disc with a fountain or pattern fill).
- **Full colour (vector) patterns**: the pattern is a CMX file (`vpat`) of its objects, which
  reach beyond the tile; its header's bounding box is their box, the page of the SVG libcdr makes
  of it. The tile is in the fill record (versions 6 to 12; 16 and X3+ not seen): of the 21 bytes
  after the flags, one byte, then the tile as a rectangle of the pattern's own drawing (x0, y0,
  x1, y1: 32-bit, 1/254000 inch, y up), then 4 bytes. The fill's SVG is cut to it (the root's
  viewBox and size): the whole page tiled showed the objects with margins round them, or a
  quarter of the tile. The tile is stretched to the fill's tile size. CorelDRAW 5 patterns are
  CDR objects on a page of their own, which is the tile.
- Radial and square fills in SVG (vector patterns' tiles): `svg:r` reaches the box's farthest corner
  from the centre, shrunk by twice the edge pad (librevenge made it the box's width: the start
  colour never reached the corners).
- A tiled fill's vertical offset from its tile transform (ftil, CorelDRAW 9) is a share of the
  tile's height (it was divided by the width: a texture of 1.6 x 1.0 inch moved by 40%).
- CorelDRAW 5's bitmap patterns are full colour patterns (a picture on a page of its own): in the
  SVG made of a pattern a picture is stretched to its box (`preserveAspectRatio="none"`), as
  CorelDRAW draws it; it kept its proportions and left a strip of the tile empty.
- A fountain fill's angle is the page's in CorelDRAW 5 (-47.5 for a fill of 0 on an object turned
  by it) and the object's own from version 6: it is turned (and mirrored) with the object.
- CMX fountain fill types count from 0 (linear, radial, conical, square), CDR's from 1: the
  radial fills of CMX (vector patterns) came out linear.
- Two-colour patterns: CorelDRAW 9's PDF export mirrors them (a diagonal hatch turns the other
  way; CorelDRAW itself shows them the way libcdr draws them).
- Arrowheads go out in their own units (`libcdr:marker-start` / `-end`, SVG path data, with
  `libcdr:marker-unit`), as `draw:marker-*-path` is transformed for ODF's drawing of markers.
- Speed: bitmaps written into a plain buffer with each colour converted once, X6+ style strings
  parsed once, path nodes moved instead of copied.
- `./configure --without-icu`: iconv (MultiByteToWideChar on Windows) instead of ICU.

## CorelDRAW 2 (`WL`) and 3

- An outline's type byte 0 is no outline (it was a black hairline round every shape).
- A group's transform is the last 24 bytes of the 0x8c part of its "more data" record; that part
  may or may not have 10 more bytes before them (a drawing came out stretched into a line).
- Bitmaps are embedded PCX (`0a 05 01 01`, 1-bit, RLE) and TIFF (`II*\0`; one seen: 1-bit,
  PackBits) files; libcdr takes only BMP (`BM`), so they are missing. Not done.
- Not decoded: CorelDRAW 2 text (object type 3: characters of 2 bytes, character and flags, a
  flagged one followed by a 12-byte style) and CorelDRAW 3 (`RIFF....CDR `) text (3 bytes a
  character after a style block). No reference renders exist: CorelDRAW 5 (under Windows 2000
  in a VM) crashes opening CorelDRAW 2 files with text (WIN87EM.DLL; with a floating point fix,
  CDRTXT50.DLL or CDRMAI50.DLL), and CorelDRAW 4 files as well; CorelDRAW 9 does not open
  CorelDRAW 2 files. Only CorelDRAW 2 files without text opened (two clip arts: their shapes
  match).
