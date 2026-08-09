/*
 * main.c - The menu loop that ties the whole of LexiCore together.
 *
 * This file owns exactly one piece of memory: the Dictionary created at
 * startup. Every menu option borrows that dictionary, and destroyDictionary
 * at the end of main is the single place it is ever freed. No entry, word,
 * or definition is allocated here; the dictionary owns all of that.
 */

#include "dictionary.h"
#include "file_manager.h"
#include "input.h"
#include "statistics.h"

#include <stdio.h>
#include <string.h>

/* A prime bucket count spreads the words evenly across the table. */
#define BUCKET_COUNT 503

/* The file the program loads at startup and offers to save back to. */
#define DEFAULT_DICTIONARY_FILE "data/dictionary.txt"

/* Mirrors the value readMenuChoice returns for input that is not a number. */
#define MENU_CHOICE_INVALID (-1)

/* Mirrors what the file functions return, so their result can be checked. */
#define FILE_OPERATION_SUCCESS 1

/* The menu options, named so the switch below reads like the menu itself. */
#define MENU_EXIT 0
#define MENU_SEARCH 1
#define MENU_ADD 2
#define MENU_EDIT 3
#define MENU_DELETE 4
#define MENU_PREFIX 5
#define MENU_DISPLAY_ALL 6
#define MENU_STATISTICS 7
#define MENU_LOAD 8
#define MENU_SAVE 9
#define MENU_HELP 10

/* Room for a short yes or no answer. */
#define ANSWER_BUFFER_SIZE 16

/* One word or one prefix, with plenty of room to spare. */
#define WORD_BUFFER_SIZE 256

/* A definition or an example sentence, which can be a long line. */
#define TEXT_BUFFER_SIZE 1024

/* A path typed at the load or save prompt. */
#define PATH_BUFFER_SIZE 512

/* The three results addWord can give back. */
#define ADD_WORD_SUCCESS 1
#define ADD_WORD_ERROR (-1)
#define ADD_WORD_DUPLICATE (-2)

/* updateWord and deleteWord both return this when they changed something. */
#define WORD_CHANGED 1

/*
 * Returns 1 once stdin has nothing left to give.
 *
 * The menu asks again after every unusable answer, so without this check a
 * closed or redirected input would make it ask forever. Reading the two
 * stream flags costs nothing and gives the loop a way to stop.
 */
static int inputIsFinished(void)
{
    if (feof(stdin) != 0 || ferror(stdin) != 0)
    {
        return 1;
    }

    return 0;
}

/*
 * Prints the main menu.
 */
static void printMenu(void)
{
    printf("\n========== LexiCore Dictionary ==========\n");
    printf("1.  Search for a word\n");
    printf("2.  Add a new word\n");
    printf("3.  Edit an existing word\n");
    printf("4.  Delete a word\n");
    printf("5.  Search by prefix\n");
    printf("6.  Display all words alphabetically\n");
    printf("7.  Display statistics\n");
    printf("8.  Load dictionary from file\n");
    printf("9.  Save dictionary to file\n");
    printf("10. Help\n");
    printf("0.  Exit\n");
    printf("=========================================\n");
    printf("Enter your choice: ");
    fflush(stdout);
}

/*
 * Prints a short explanation of what each menu option does.
 */
static void printHelp(void)
{
    printf("\n========== Help ==========\n");
    printf("1.  Search for a word    Type a word to see its full entry.\n");
    printf("                         Upper and lower case do not matter.\n");
    printf("2.  Add a new word       Asks for the word, its part of speech,\n");
    printf("                         its definition and an example sentence.\n");
    printf("                         A word already in the list is refused.\n");
    printf("3.  Edit a word          Choose one field of an existing word\n");
    printf("                         and type its new text.\n");
    printf("4.  Delete a word        Asks you to confirm before removing it.\n");
    printf("5.  Search by prefix     Lists every word starting with the\n");
    printf("                         letters you type, e.g. 'pro'.\n");
    printf("6.  Display all words    Lists the whole dictionary in\n");
    printf("                         alphabetical order, 20 words per page.\n");
    printf("7.  Statistics           Reports on the size and shape of the\n");
    printf("                         hash table and counts parts of speech.\n");
    printf("8.  Load from file       Reads entries from a file and adds them\n");
    printf("                         to the words already in memory.\n");
    printf("9.  Save to file         Writes every word to a file.\n");
    printf("0.  Exit                 Offers to save first if anything\n");
    printf("                         has changed.\n");
    printf("\n");
    printf("Dictionary files hold one entry per line, in this format:\n");
    printf("    word|part of speech|definition|example sentence\n");
    printf("==========================\n");
}

/*
 * Asks a yes or no question and returns 1 for yes, 0 for anything else.
 *
 * The answer buffer is a local array, so nothing is allocated or freed. An
 * answer that cannot be read counts as no, which is the safe choice for
 * every question this program asks.
 */
static int askYesNo(const char *question)
{
    char answer[ANSWER_BUFFER_SIZE] = {0};

    /* Never hand a NULL pointer to printf. */
    if (question == NULL)
    {
        return 0;
    }

    printf("%s", question);
    fflush(stdout);

    if (readLine(answer, sizeof(answer)) == 0)
    {
        return 0;
    }

    if (answer[0] == 'y' || answer[0] == 'Y')
    {
        return 1;
    }

    return 0;
}

/*
 * Prints a prompt and reads one line of text into the caller's buffer.
 *
 * Returns 1 when real text was typed and 0 when the line could not be read
 * or held nothing but spaces, printing a message in that case. The buffer
 * belongs to the caller and is never allocated or freed here. Every text
 * prompt in this file goes through here, so no empty field can reach the
 * dictionary functions.
 */
static int promptForText(const char *prompt, char *buffer, size_t bufferSize)
{
    /* Check the pointers before either of them is used. */
    if (prompt == NULL || buffer == NULL || bufferSize == 0)
    {
        return 0;
    }

    printf("%s", prompt);
    fflush(stdout);

    if (readLine(buffer, bufferSize) == 0)
    {
        printf("Nothing was entered.\n");
        return 0;
    }

    if (isBlank(buffer) == 1)
    {
        printf("That cannot be left empty. Nothing was done.\n");
        return 0;
    }

    return 1;
}

/*
 * Prints all four fields of one entry.
 *
 * The entry is only read, never changed or freed; it stays owned by the
 * dictionary. A field that was never filled in prints as an empty line
 * rather than being handed to printf as a NULL pointer.
 */
static void printEntryDetails(const DictionaryEntry *entry)
{
    /* Never follow the pointer before checking that it is real. */
    if (entry == NULL)
    {
        return;
    }

    printf("\nWord:            %s\n", entry->word != NULL ? entry->word : "");
    printf("Part of speech:  %s\n", entry->partOfSpeech != NULL ? entry->partOfSpeech : "");
    printf("Definition:      %s\n", entry->definition != NULL ? entry->definition : "");
    printf("Example:         %s\n", entry->exampleSentence != NULL ? entry->exampleSentence : "");
}

/*
 * Menu option 1: looks one word up and prints it.
 *
 * findWord hands back a pointer to an entry the dictionary still owns, so
 * the result is only read here and must never be freed.
 */
static void handleSearch(const Dictionary *dictionary)
{
    char word[WORD_BUFFER_SIZE] = {0};
    DictionaryEntry *found = NULL;

    if (promptForText("Enter the word to search for: ", word, sizeof(word)) == 0)
    {
        return;
    }

    /* The returned entry is borrowed from the dictionary, never freed here. */
    found = findWord(dictionary, word);
    if (found == NULL)
    {
        printf("'%s' is not in the dictionary.\n", word);
        return;
    }

    printEntryDetails(found);
}

/*
 * Menu option 2: asks for all four fields and adds the word.
 *
 * Returns 1 when the dictionary changed and 0 when it did not, so the menu
 * knows whether there is anything to save. The four buffers live on the
 * stack; addWord makes its own copies of the text, so nothing typed here
 * ever needs freeing and the buffers simply disappear on return.
 */
static int handleAdd(Dictionary *dictionary)
{
    char word[WORD_BUFFER_SIZE] = {0};
    char partOfSpeech[WORD_BUFFER_SIZE] = {0};
    char definition[TEXT_BUFFER_SIZE] = {0};
    char exampleSentence[TEXT_BUFFER_SIZE] = {0};
    int result = 0;

    /* Every field is checked here so no empty text reaches addWord. */
    if (promptForText("Enter the new word: ", word, sizeof(word)) == 0)
    {
        return 0;
    }

    if (promptForText("Enter its part of speech: ", partOfSpeech, sizeof(partOfSpeech)) == 0)
    {
        return 0;
    }

    if (promptForText("Enter its definition: ", definition, sizeof(definition)) == 0)
    {
        return 0;
    }

    if (promptForText("Enter an example sentence: ", exampleSentence, sizeof(exampleSentence)) == 0)
    {
        return 0;
    }

    result = addWord(dictionary, word, partOfSpeech, definition, exampleSentence);

    if (result == ADD_WORD_SUCCESS)
    {
        printf("'%s' was added to the dictionary.\n", word);
        return 1;
    }

    if (result == ADD_WORD_DUPLICATE)
    {
        printf("'%s' is already in the dictionary. Nothing was added.\n", word);
        return 0;
    }

    if (result == ADD_WORD_ERROR)
    {
        printf("Error: '%s' could not be added. A field may be empty or there\n", word);
        printf("was not enough memory. Nothing was changed.\n");
        return 0;
    }

    /* An unknown result is still reported rather than passing silently. */
    printf("Error: '%s' could not be added (unexpected result %d).\n", word, result);
    return 0;
}

/*
 * Menu option 3: asks which word to edit and hands over to updateWord.
 *
 * updateWord asks which field to change and prints its own messages, so the
 * only job left here is to say whether the dictionary changed. Returns 1
 * when it did. Nothing is allocated or freed in this function.
 */
static int handleEdit(Dictionary *dictionary)
{
    char word[WORD_BUFFER_SIZE] = {0};
    int result = 0;

    if (promptForText("Enter the word to edit: ", word, sizeof(word)) == 0)
    {
        return 0;
    }

    result = updateWord(dictionary, word);
    if (result == WORD_CHANGED)
    {
        return 1;
    }

    return 0;
}

/*
 * Menu option 4: asks which word to delete and hands over to deleteWord.
 *
 * deleteWord asks the user to confirm, frees the entry itself, and prints
 * its own messages. Returns 1 here when a word was really removed. The
 * entry is freed inside deleteWord, so nothing in this file may touch it
 * afterwards.
 */
static int handleDelete(Dictionary *dictionary)
{
    char word[WORD_BUFFER_SIZE] = {0};
    int result = 0;

    if (promptForText("Enter the word to delete: ", word, sizeof(word)) == 0)
    {
        return 0;
    }

    result = deleteWord(dictionary, word);
    if (result == WORD_CHANGED)
    {
        return 1;
    }

    return 0;
}

/*
 * Menu option 5: lists every word starting with the letters the user types.
 *
 * findWordsByPrefix prints the matches and the total itself, so there is
 * nothing left to report here.
 */
static void handlePrefixSearch(const Dictionary *dictionary)
{
    char prefix[WORD_BUFFER_SIZE] = {0};

    if (promptForText("Enter prefix: ", prefix, sizeof(prefix)) == 0)
    {
        return;
    }

    /* The count is already printed by the function, so it is not needed here. */
    (void)findWordsByPrefix(dictionary, prefix);
}

/*
 * Asks which file to use, offering the usual one when Enter is pressed.
 *
 * Returns 1 when the buffer holds a usable filename. The buffer belongs to
 * the caller. An empty answer is not an error here: it means "the usual
 * file", which is copied in with strncpy and terminated by hand.
 */
static int promptForFilename(char *buffer, size_t bufferSize)
{
    /* Check the buffer before writing anything into it. */
    if (buffer == NULL || bufferSize == 0)
    {
        return 0;
    }

    printf("Enter the filename, or press Enter for '%s': ", DEFAULT_DICTIONARY_FILE);
    fflush(stdout);

    if (readLine(buffer, bufferSize) == 0)
    {
        printf("Nothing was entered.\n");
        return 0;
    }

    /* An empty answer means the default file rather than a mistake. */
    if (isBlank(buffer) == 1)
    {
        strncpy(buffer, DEFAULT_DICTIONARY_FILE, bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }

    return 1;
}

/*
 * Menu option 8: reads a file and adds its words to the ones in memory.
 *
 * Returns 1 when the load worked, because words read from a file have not
 * been saved to whatever file the user might write next. Loading adds to
 * the table rather than replacing it, and any word already there is kept.
 * The entries belong to the dictionary; this function allocates nothing.
 */
static int handleLoad(Dictionary *dictionary)
{
    char filename[PATH_BUFFER_SIZE] = {0};

    if (promptForFilename(filename, sizeof(filename)) == 0)
    {
        return 0;
    }

    printf("Words already in memory are kept, and duplicates are skipped.\n");

    /* loadDictionaryFromFile prints its own summary of what it read. */
    if (loadDictionaryFromFile(dictionary, filename) != FILE_OPERATION_SUCCESS)
    {
        printf("Nothing was loaded. The dictionary is unchanged.\n");
        return 0;
    }

    return 1;
}

/*
 * Menu option 9: writes every word to a file.
 *
 * Returns 1 when the file was written, which is what lets the menu forget
 * about unsaved changes. saveDictionaryToFile asks before overwriting a
 * file that already exists and prints its own messages.
 */
static int handleSave(const Dictionary *dictionary)
{
    char filename[PATH_BUFFER_SIZE] = {0};

    if (promptForFilename(filename, sizeof(filename)) == 0)
    {
        return 0;
    }

    if (saveDictionaryToFile(dictionary, filename) != FILE_OPERATION_SUCCESS)
    {
        printf("The dictionary was not saved.\n");
        return 0;
    }

    return 1;
}

/*
 * Creates the dictionary, runs the menu until the user leaves, and frees
 * everything on the way out.
 *
 * The dictionary is created once here and destroyed once at the end, so
 * there is exactly one allocation and one matching free for the whole
 * program. Returns 0 on a normal exit and 1 when the dictionary could not
 * be created at all.
 */
int main(void)
{
    Dictionary *dictionary = NULL;
    int choice = 0;
    int running = 1;
    int modified = 0;

    printf("Welcome to LexiCore.\n");

    /* The one allocation this file makes; everything below only borrows it. */
    dictionary = createDictionary(BUCKET_COUNT);
    if (dictionary == NULL)
    {
        printf("Error: The dictionary could not be created. Exiting.\n");
        return 1;
    }

    /*
     * A missing dictionary file is not a reason to give up. The program
     * simply starts with nothing in the table and says so.
     */
    printf("\nLoading '%s'...\n", DEFAULT_DICTIONARY_FILE);
    if (loadDictionaryFromFile(dictionary, DEFAULT_DICTIONARY_FILE) != FILE_OPERATION_SUCCESS)
    {
        printf("Starting with an empty dictionary instead.\n");
    }

    while (running == 1)
    {
        printMenu();
        choice = readMenuChoice();

        switch (choice)
        {
            case MENU_SEARCH:
                handleSearch(dictionary);
                break;

            /* Each of these three reports whether anything really changed. */
            case MENU_ADD:
                if (handleAdd(dictionary) == 1)
                {
                    modified = 1;
                }
                break;

            case MENU_EDIT:
                if (handleEdit(dictionary) == 1)
                {
                    modified = 1;
                }
                break;

            case MENU_DELETE:
                if (handleDelete(dictionary) == 1)
                {
                    modified = 1;
                }
                break;

            case MENU_PREFIX:
                handlePrefixSearch(dictionary);
                break;

            case MENU_DISPLAY_ALL:
                displayDictionaryAlphabetically(dictionary);
                break;

            case MENU_STATISTICS:
                displayStatistics(dictionary);
                break;

            case MENU_LOAD:
                if (handleLoad(dictionary) == 1)
                {
                    modified = 1;
                }
                break;

            /* A successful save is the one thing that clears the flag. */
            case MENU_SAVE:
                if (handleSave(dictionary) == 1)
                {
                    modified = 0;
                }
                break;

            case MENU_HELP:
                printHelp();
                break;

            case MENU_EXIT:
                running = 0;
                break;

            /* Anything that is not a menu number lands here and is refused. */
            case MENU_CHOICE_INVALID:
            default:
                printf("Invalid choice. Please enter a number from the menu.\n");
                break;
        }

        /*
         * A closed input stream would answer every prompt the same way for
         * ever, so the loop stops instead of spinning.
         */
        if (running == 1 && inputIsFinished() == 1)
        {
            printf("\nInput has ended. Leaving the menu.\n");
            running = 0;
        }
    }

    /* Unsaved work is only lost after the user has been asked about it. */
    if (modified == 1)
    {
        if (inputIsFinished() == 1)
        {
            printf("Input has ended, so unsaved changes were not saved.\n");
        }
        else if (askYesNo("Save changes before exiting? (y/n): ") == 1)
        {
            if (saveDictionaryToFile(dictionary, DEFAULT_DICTIONARY_FILE) != FILE_OPERATION_SUCCESS)
            {
                printf("The dictionary could not be saved. Exiting anyway.\n");
            }
        }
        else
        {
            printf("Exiting without saving.\n");
        }
    }

    /* The single free that matches the createDictionary above. */
    destroyDictionary(dictionary);
    dictionary = NULL;

    printf("Goodbye.\n");
    return 0;
}
