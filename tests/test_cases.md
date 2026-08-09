# LexiCore Test Cases

Build first, from the project root:

```
make
./lexicore
```

Unless a case says otherwise, start the program fresh from the project root
so that `data/dictionary.txt` (67 entries) is loaded at startup.

## What each line of `data/test_dictionary.txt` is meant to trigger

That file is deliberately broken. It is never loaded at startup; load it from
the menu with option 8 and the path `data/test_dictionary.txt`.

| Line | Content | Meant to trigger |
|------|---------|------------------|
| 1-4 | `anchor`, `beacon`, `carve`, `eagerly` | The normal path: four well formed entries, one of each part of speech. |
| 5 | completely empty line | The blank line check in the loader. Skipped in silence, with no warning and no count. |
| 6 | spaces and a tab only | The same check by way of `isBlank`, proving whitespace counts as blank too. |
| 7 | `fog\|noun` | Too few fields. Two fields instead of four, so the line is reported and ignored. |
| 8 | `gale\|...\|...\|...\|and one field too many` | Too many fields. `splitIntoFields` returns more than four, so the line is reported and ignored. |
| 9 | `harvest\|noun\|\|The harvest lasted...` | An empty definition field. Four fields are present but one holds no text. |
| 10 | `\|noun\|A definition with no word...` | An empty word field, which is the same check applied to field one. |
| 11 | `anchor\|noun\|A second definition...` | A duplicate of the word on line 1. Counted as a duplicate, not as an error, and the first definition is kept. |
| 12 | `   ivory   \|   noun   \|   ...   ` | Extra spaces around every field. The entry is valid: the loader trims each field before storing it. |
| 13 | `toolong\|noun\|...` (5180 characters) | A line longer than the 4096 character buffer. The line is reported, ignored, and the rest of it is thrown away rather than being read as a second line. |
| 14 | `jubilant\|adjective\|...` | Recovery. A valid entry straight after the over-long line, which only loads if the loader resynchronised properly. |

Loading that file into an empty dictionary should report exactly:

```
Warning: Invalid entry ignored at line 7.
Warning: Invalid entry ignored at line 8.
Warning: Invalid entry ignored at line 9.
Warning: Invalid entry ignored at line 10.
Warning: Invalid entry ignored at line 13.
Dictionary loaded successfully.
Valid entries loaded: 6
Invalid lines ignored: 5
Duplicate words ignored: 1
```

## Test cases

| # | Test | Input / action | Expected result | Actual result | Status |
|---|------|----------------|-----------------|---------------|--------|
