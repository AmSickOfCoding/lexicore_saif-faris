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
            case MENU_DISPLAY_ALL:
                displayDictionaryAlphabetically(dictionary);
                break;

            case MENU_STATISTICS:
                displayStatistics(dictionary);
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
