;;; charscript.el --- character script table
;;; Automatically generated from admin/unidata/Blocks.txt
(let (script-list)
  (dolist (elt '(
    (#x0000 #x007F latin) ; Basic Latin
    (#x00A0 #x024F latin) ; Latin-1 Supplement, Latin Extended-A, Latin Extended-B
    (#x0250 #x02AF phonetic) ; IPA Extensions
    (#x02B0 #x036F latin) ; Spacing Modifier Letters, Combining Diacritical Marks
    (#x0370 #x03E1 greek) ; Greek and Coptic
    (#x03E2 #x03EF coptic)
    (#x03F0 #x03FF greek)
    (#x0400 #x052F cyrillic) ; Cyrillic, Cyrillic Supplement
    (#x0530 #x058F armenian)
    (#x0590 #x05FF hebrew)
    (#x0600 #x06FF arabic)
    (#x0700 #x074F syriac)
    (#x0750 #x077F arabic) ; Arabic Supplement
    (#x0780 #x07BF thaana)
    (#x07C0 #x07FF nko)
    (#x0800 #x083F samaritan)
    (#x0840 #x085F mandaic)
    (#x0860 #x086F syriac) ; Syriac Supplement
    (#x08A0 #x08FF arabic) ; Arabic Extended-A
    (#x0900 #x097F devanagari)
    (#x0980 #x09FF bengali)
    (#x0A00 #x0A7F gurmukhi)
    (#x0A80 #x0AFF gujarati)
    (#x0B00 #x0B7F oriya)
    (#x0B80 #x0BFF tamil)
    (#x0C00 #x0C7F telugu)
    (#x0C80 #x0CFF kannada)
    (#x0D00 #x0D7F malayalam)
    (#x0D80 #x0DFF sinhala)
    (#x0E00 #x0E7F thai)
    (#x0E80 #x0EFF lao)
    (#x0F00 #x0FFF tibetan)
    (#x1000 #x109F burmese) ; Myanmar
    (#x10A0 #x10FF georgian)
    (#x1100 #x11FF hangul) ; Hangul Jamo
    (#x1200 #x139F ethiopic) ; Ethiopic, Ethiopic Supplement
    (#x13A0 #x13FF cherokee)
    (#x1400 #x167F canadian-aboriginal)
    (#x1680 #x169F ogham)
    (#x16A0 #x16FF runic)
    (#x1700 #x171F tagalog)
    (#x1720 #x173F hanunoo)
    (#x1740 #x175F buhid)
    (#x1760 #x177F tagbanwa)
    (#x1780 #x17FF khmer)
    (#x1800 #x18AF mongolian)
    (#x18B0 #x18FF canadian-aboriginal)
    (#x1900 #x194F limbu)
    (#x1950 #x197F tai-le)
    (#x1980 #x19DF tai-lue)
    (#x19E0 #x19FF khmer) ; Khmer Symbols
    (#x1A00 #x1A1F buginese)
    (#x1A20 #x1AAF tai-tham)
    (#x1AB0 #x1AFF latin) ; Combining Diacritical Marks Extended
    (#x1B00 #x1B7F balinese)
    (#x1B80 #x1BBF sundanese)
    (#x1BC0 #x1BFF batak)
    (#x1C00 #x1C4F lepcha)
    (#x1C50 #x1C7F ol-chiki)
    (#x1C80 #x1C8F cyrillic) ; Cyrillic Extended-C
    (#x1C90 #x1CBF georgian) ; Georgian Extended
    (#x1CC0 #x1CCF sundanese) ; Sundanese Supplement
    (#x1CD0 #x1CFF vedic) ; Vedic Extensions
    (#x1D00 #x1DBF phonetic) ; Phonetic Extensions, Phonetic Extensions Supplement
    (#x1DC0 #x1EFF latin) ; Combining Diacritical Marks Supplement, Latin Extended Additional
    (#x1F00 #x1FFF greek) ; Greek Extended
    (#x2000 #x27FF symbol) ; General Punctuation, Superscripts and Subscripts, Currency Symbols, Combining Diacritical Marks for Symbols, Letterlike Symbols, Number Forms, Arrows, Mathematical Operators, Miscellaneous Technical, Control Pictures, Optical Character Recognition, Enclosed Alphanumerics, Box Drawing, Block Elements, Geometric Shapes, Miscellaneous Symbols, Dingbats, Miscellaneous Mathematical Symbols-A, Supplemental Arrows-A
    (#x2800 #x28FF braille) ; Braille Patterns
    (#x2900 #x2BFF symbol) ; Supplemental Arrows-B, Miscellaneous Mathematical Symbols-B, Supplemental Mathematical Operators, Miscellaneous Symbols and Arrows
    (#x2C00 #x2C5F glagolitic)
    (#x2C60 #x2C7F latin) ; Latin Extended-C
    (#x2C80 #x2CFF coptic)
    (#x2D00 #x2D2F georgian) ; Georgian Supplement
    (#x2D30 #x2D7F tifinagh)
    (#x2D80 #x2DDF ethiopic) ; Ethiopic Extended
    (#x2DE0 #x2DFF cyrillic) ; Cyrillic Extended-A
    (#x2E00 #x2E7F symbol) ; Supplemental Punctuation
    (#x2E80 #x2FDF han) ; CJK Radicals Supplement, Kangxi Radicals
    (#x2FF0 #x2FFF ideographic-description)
    (#x3000 #x303F cjk-misc)
    (#x3040 #x30FF kana) ; Hiragana, Katakana
    (#x3100 #x312F bopomofo)
    (#x3130 #x318F hangul) ; Hangul Compatibility Jamo
    (#x3190 #x319F kanbun)
    (#x31A0 #x31BF bopomofo) ; Bopomofo Extended
    (#x31C0 #x31EF cjk-misc)
    (#x31F0 #x31FF kana) ; Katakana Phonetic Extensions
    (#x3200 #x9FFF han) ; Enclosed CJK Letters and Months, CJK Compatibility, CJK Unified Ideographs Extension A, Yijing Hexagram Symbols, CJK Unified Ideographs
    (#xA000 #xA4CF yi) ; Yi Syllables, Yi Radicals
    (#xA4D0 #xA4FF lisu)
    (#xA500 #xA63F vai)
    (#xA640 #xA69F cyrillic) ; Cyrillic Extended-B
    (#xA6A0 #xA6FF bamum)
    (#xA700 #xA7FF latin) ; Modifier Tone Letters, Latin Extended-D
    (#xA800 #xA82F syloti-nagri)
    (#xA830 #xA83F north-indic-number)
    (#xA840 #xA87F phags-pa)
    (#xA880 #xA8DF saurashtra)
    (#xA8E0 #xA8FF devanagari) ; Devanagari Extended
    (#xA900 #xA92F kayah-li)
    (#xA930 #xA95F rejang)
    (#xA960 #xA97F hangul) ; Hangul Jamo Extended-A
    (#xA980 #xA9DF javanese)
    (#xA9E0 #xA9FF burmese) ; Myanmar Extended-B
    (#xAA00 #xAA5F cham)
    (#xAA60 #xAA7F burmese) ; Myanmar Extended-A
    (#xAA80 #xAADF tai-viet)
    (#xAAE0 #xAAFF meetei-mayek)
    (#xAB00 #xAB2F ethiopic) ; Ethiopic Extended-A
    (#xAB30 #xAB6F latin) ; Latin Extended-E
    (#xAB70 #xABBF cherokee) ; Cherokee Supplement
    (#xABC0 #xABFF meetei-mayek)
    (#xAC00 #xD7FF hangul) ; Hangul Syllables, Hangul Jamo Extended-B
    (#xF900 #xFAFF han) ; CJK Compatibility Ideographs
    (#xFB00 #xFB06 latin) ; Alphabetic Presentation Forms
    (#xFB13 #xFB17 armenian)
    (#xFB1D #xFB4F hebrew)
    (#xFB50 #xFDFF arabic) ; Arabic Presentation Forms-A
    (#xFE10 #xFE1F vertical-form)
    (#xFE20 #xFE2F latin) ; Combining Half Marks
    (#xFE30 #xFE4F han) ; CJK Compatibility Forms
    (#xFE50 #xFE6F symbol) ; Small Form Variants
    (#xFE70 #xFEFF arabic) ; Arabic Presentation Forms-B
    (#xFF00 #xFF60 cjk-misc)
    (#xFF61 #xFF9F kana)
    (#xFFA0 #xFFDF hangul)
    (#xFFE0 #xFFEF cjk-misc)
    (#x10000 #x100FF linear-b)
    (#x10100 #x1013F aegean-number)
    (#x10140 #x1018F ancient-greek-number)
    (#x10190 #x101CF ancient-symbol)
    (#x101D0 #x101FF phaistos-disc)
    (#x10280 #x1029F lycian)
    (#x102A0 #x102DF carian)
    (#x102E0 #x102FF coptic) ; Coptic Epact Numbers
    (#x10300 #x1032F old-italic)
    (#x10330 #x1034F gothic)
    (#x10350 #x1037F old-permic)
    (#x10380 #x1039F ugaritic)
    (#x103A0 #x103DF old-persian)
    (#x10400 #x1044F deseret)
    (#x10450 #x1047F shavian)
    (#x10480 #x104AF osmanya)
    (#x104B0 #x104FF osage)
    (#x10500 #x1052F elbasan)
    (#x10530 #x1056F caucasian-albanian)
    (#x10600 #x1077F linear-a)
    (#x10800 #x1083F cypriot-syllabary)
    (#x10840 #x1085F aramaic) ; Imperial Aramaic
    (#x10860 #x1087F palmyrene)
    (#x10880 #x108AF nabataean)
    (#x108E0 #x108FF hatran)
    (#x10900 #x1091F phoenician)
    (#x10920 #x1093F lydian)
    (#x10980 #x109FF meroitic) ; Meroitic Hieroglyphs, Meroitic Cursive
    (#x10A00 #x10A5F kharoshthi)
    (#x10A60 #x10A7F old-south-arabian)
    (#x10A80 #x10A9F old-north-arabian)
    (#x10AC0 #x10AFF manichaean)
    (#x10B00 #x10B3F avestan)
    (#x10B40 #x10B5F inscriptional-parthian)
    (#x10B60 #x10B7F inscriptional-pahlavi)
    (#x10B80 #x10BAF psalter-pahlavi)
    (#x10C00 #x10C4F old-turkic)
    (#x10C80 #x10CFF old-hungarian)
    (#x10D00 #x10D3F hanifi-rohingya)
    (#x10E60 #x10E7F rumi-number)
    (#x10F00 #x10F2F old-sogdian)
    (#x10F30 #x10F6F sogdian)
    (#x11000 #x1107F brahmi)
    (#x11080 #x110CF kaithi)
    (#x110D0 #x110FF sora-sompeng)
    (#x11100 #x1114F chakma)
    (#x11150 #x1117F mahajani)
    (#x11180 #x111DF sharada)
    (#x111E0 #x111FF sinhala-archaic-number)
    (#x11200 #x1124F khojki)
    (#x11280 #x112AF multani)
    (#x112B0 #x112FF khudawadi)
    (#x11300 #x1137F grantha)
    (#x11400 #x1147F newa)
    (#x11480 #x114DF tirhuta)
    (#x11580 #x115FF siddham)
    (#x11600 #x1165F modi)
    (#x11660 #x1167F mongolian) ; Mongolian Supplement
    (#x11680 #x116CF takri)
    (#x11700 #x1173F ahom)
    (#x11800 #x1184F dogra)
    (#x118A0 #x118FF warang-citi)
    (#x11A00 #x11A4F zanabazar-square)
    (#x11A50 #x11AAF soyombo)
    (#x11AC0 #x11AFF pau-cin-hau)
    (#x11C00 #x11C6F bhaiksuki)
    (#x11C70 #x11CBF marchen)
    (#x11D00 #x11D5F masaram-gondi)
    (#x11D60 #x11DAF gunjala-gondi)
    (#x11EE0 #x11EFF makasar)
    (#x12000 #x123FF cuneiform)
    (#x12400 #x1247F cuneiform-numbers-and-punctuation)
    (#x12480 #x1254F cuneiform) ; Early Dynastic Cuneiform
    (#x13000 #x1342F egyptian) ; Egyptian Hieroglyphs
    (#x14400 #x1467F anatolian) ; Anatolian Hieroglyphs
    (#x16800 #x16A3F bamum) ; Bamum Supplement
    (#x16A40 #x16A6F mro)
    (#x16AD0 #x16AFF bassa-vah)
    (#x16B00 #x16B8F pahawh-hmong)
    (#x16E40 #x16E9F medefaidrin)
    (#x16F00 #x16F9F miao)
    (#x16FE0 #x16FFF symbol) ; Ideographic Symbols and Punctuation
    (#x17000 #x187FF tangut)
    (#x18800 #x18AFF tangut-components)
    (#x1B000 #x1B12F kana) ; Kana Supplement, Kana Extended-A
    (#x1B170 #x1B2FF nushu)
    (#x1BC00 #x1BCAF duployan-shorthand)
    (#x1D000 #x1D0FF byzantine-musical-symbol)
    (#x1D100 #x1D1FF musical-symbol)
    (#x1D200 #x1D24F ancient-greek-musical-notation)
    (#x1D2E0 #x1D2FF mayan-numeral)
    (#x1D300 #x1D35F tai-xuan-jing-symbol)
    (#x1D360 #x1D37F counting-rod-numeral)
    (#x1D400 #x1D7FF mathematical) ; Mathematical Alphanumeric Symbols
    (#x1D800 #x1DAAF sutton-sign-writing)
    (#x1E000 #x1E02F glagolitic) ; Glagolitic Supplement
    (#x1E800 #x1E8DF mende-kikakui)
    (#x1E900 #x1E95F adlam)
    (#x1EC70 #x1ECBF indic-siyaq-number)
    (#x1EE00 #x1EEFF arabic) ; Arabic Mathematical Alphabetic Symbols
    (#x1F000 #x1F02F mahjong-tile)
    (#x1F030 #x1F09F domino-tile)
    (#x1F0A0 #x1F0FF playing-cards)
    (#x1F100 #x1F1FF symbol) ; Enclosed Alphanumeric Supplement
    (#x1F200 #x1F2FF han) ; Enclosed Ideographic Supplement
    (#x1F300 #x1F9FF symbol) ; Miscellaneous Symbols and Pictographs, Emoticons, Ornamental Dingbats, Transport and Map Symbols, Alchemical Symbols, Geometric Shapes Extended, Supplemental Arrows-C, Supplemental Symbols and Pictographs
    (#x1FA00 #x1FA6F chess-symbol)
    (#x20000 #x2A6DF han) ; CJK Unified Ideographs Extension B
    (#x2A700 #x2EBEF han) ; CJK Unified Ideographs Extension C, CJK Unified Ideographs Extension D, CJK Unified Ideographs Extension E, CJK Unified Ideographs Extension F
    (#x2F800 #x2FA1F han) ; CJK Compatibility Ideographs Supplement
    ))
    (set-char-table-range char-script-table
			  (cons (car elt) (nth 1 elt)) (nth 2 elt))
    (or (memq (nth 2 elt) script-list)
	(setq script-list (cons (nth 2 elt) script-list))))
  (set-char-table-extra-slot char-script-table 0 (nreverse script-list)))

(provide 'charscript)
