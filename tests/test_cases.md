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
| 1 | Valid load at startup | Run `./lexicore` from the project root with `data/dictionary.txt` in place | `Dictionary loaded successfully.` then `Valid entries loaded: 67`, `Invalid lines ignored: 0`, `Duplicate words ignored: 0`, then the menu | | |
| 2 | Missing file at startup | Rename `data/dictionary.txt`, then run `./lexicore` | `Error: Could not open dictionary file 'data/dictionary.txt'.` then `Starting with an empty dictionary instead.` The menu still appears and the program does not exit | | |
| 3 | Empty file | Create an empty file `data/empty.txt`, then menu 8 and enter `data/empty.txt` | `Dictionary loaded successfully.` with `Valid entries loaded: 0`, `Invalid lines ignored: 0`, `Duplicate words ignored: 0`. No crash and no warning | | |
| 4 | Corrupted file | Start with the file renamed as in case 2 so the dictionary is empty, then menu 8 and enter `data/test_dictionary.txt` | Warnings for lines 7, 8, 9, 10 and 13, then `Valid entries loaded: 6`, `Invalid lines ignored: 5`, `Duplicate words ignored: 1` | | |
| 5 | Over-long line does not swallow the next one | Same load as case 4, then menu 6 | `jubilant` appears in the listing, proving the 5180 character line on line 13 did not consume line 14 | | |
| 6 | Exact search, hit | Menu 1, enter `program` | The four fields print: word `program`, part of speech `noun`, its definition and its example sentence | | |
| 7 | Exact search, miss | Menu 1, enter `zzzzz` | `'zzzzz' is not in the dictionary.` and the menu returns | | |
| 8 | Case-insensitive search | Menu 1, enter `PROGRAM` | The same entry as case 6 is found | | |
| 9 | Search with an empty answer | Menu 1, press Enter on its own | `That cannot be left empty. Nothing was done.` The dictionary is not touched | | |
| 10 | Add a word | Menu 2, enter `lattice`, `noun`, `A frame of crossed strips of wood.`, `Roses grew up the wooden lattice.` | `'lattice' was added to the dictionary.` Menu 1 for `lattice` then finds it | | |
| 11 | Duplicate add | Menu 2, enter `program` and any three other fields | `'program' is already in the dictionary. Nothing was added.` Menu 7 still reports the same total entries as before | | |
| 12 | Add with an empty field | Menu 2, enter `sample`, then press Enter on its own at the part of speech prompt | `That cannot be left empty. Nothing was done.` Nothing is added and the remaining prompts are skipped | | |
| 13 | Edit a word | Menu 3, enter `program`, choose field 2, enter `A list of steps a computer carries out.` | The current entry prints, then `Word 'program' updated successfully.` Menu 1 for `program` shows the new definition | | |
| 14 | Cancel an edit | Menu 3, enter `program`, choose 4 | `Update cancelled.` and the entry is unchanged | | |
| 15 | Edit a missing word | Menu 3, enter `zzzzz` | `Word 'zzzzz' not found in dictionary.` | | |
| 16 | Delete the first node in a chain | Menu 4, enter `ribbon`, answer `y`. `ribbon`, `protect` and `hesitate` all share bucket 498, and `ribbon` is at the head of that chain | `Word 'ribbon' deleted successfully.` Menu 1 still finds both `protect` and `hesitate` | | |
| 17 | Delete a middle node | Restart, then menu 4, enter `protect`, answer `y`. `protect` sits between `ribbon` and `hesitate` in bucket 498 | `Word 'protect' deleted successfully.` Menu 1 still finds both `ribbon` and `hesitate` | | |
| 18 | Delete the last node in a chain | Restart, then menu 4, enter `hesitate`, answer `y` | `Word 'hesitate' deleted successfully.` Menu 1 still finds both `ribbon` and `protect` | | |
| 19 | Cancel a deletion | Menu 4, enter `program`, answer `n` | `Deletion cancelled.` and menu 1 still finds `program` | | |
| 20 | Delete a missing word | Menu 4, enter `zzzzz` | `Word 'zzzzz' not found in dictionary.` and no confirmation prompt appears | | |
| 21 | Prefix search with matches | Menu 5, enter `pro` | Eight numbered words listed, then `8 matching words found.` | | |
| 22 | Prefix search, longer prefix | Menu 5, enter `process` | `process` and `processor` listed, then `2 matching words found.` | | |
| 23 | Prefix search, no matches | Menu 5, enter `zzz` | `No matching words found.` and no numbered list | | |
| 24 | Prefix search, mixed case | Menu 5, enter `PRO` | The same eight words as case 21 | | |
| 25 | Alphabetical display and paging | Menu 6 | Words listed A to Z, numbered 1 to 67. `Press Enter to continue...` after entries 20, 40 and 60, no pause after 67, then `67 words listed.` | | |
| 26 | Alphabetical display, empty dictionary | Start with the file renamed as in case 2, then menu 6 | `The dictionary is empty. There is nothing to display.` and no crash | | |
| 27 | Statistics, populated | Menu 7 | `Total entries: 67`, `Total buckets: 503`, `Used buckets: 62`, `Empty buckets: 441`, `Load factor: 0.13`, `Longest chain: 3`, and noun 20, verb 24, adjective 14, adverb 9, other 0 | | |
| 28 | Statistics, empty dictionary | Start with the file renamed as in case 2, then menu 7 | Every count 0, `Load factor: 0.00`, `Average used-chain length: 0.00`. No crash and no division by zero | | |
| 29 | Invalid menu input, letters | Menu, enter `abc` | `Invalid choice. Please enter a number from the menu.` and the menu is shown again | | |
| 30 | Invalid menu input, out of range | Menu, enter `99`, then `-5`, then press Enter on its own | The same message each time, and the program neither exits nor loops on its own | | |
| 31 | Save then reload | Menu 2 to add `lattice`, menu 9 and press Enter for the default file, answer `y` to the overwrite warning, then quit and run the program again | `Dictionary saved successfully.` with `Entries written: 68`, and on the next run `Valid entries loaded: 68`. Menu 1 finds `lattice` | | |
| 32 | Save is refused politely | Menu 9, press Enter for the default file, answer `n` to the overwrite warning | `Save cancelled. 'data/dictionary.txt' was not changed.` then `The dictionary was not saved.` The file on disk is unchanged | | |
| 33 | Load merges rather than replacing | Menu 8, press Enter for the default file (already loaded at startup) | `Valid entries loaded: 0` and `Duplicate words ignored: 67`. The entry total from menu 7 is unchanged | | |
| 34 | Exit with unsaved changes | Menu 2 to add any new word, then menu 0 | `Save changes before exiting? (y/n): ` is asked. Answering `n` prints `Exiting without saving.` and the word is gone on the next run | | |
| 35 | Exit with nothing changed | Menu 0 straight after startup | No save question is asked. `Goodbye.` and the program ends | | |
| 36 | Input runs out | `printf '7\n' \| ./lexicore` | The statistics print, then `Input has ended. Leaving the menu.` and `Goodbye.` The program does not loop forever on the closed input | | |
