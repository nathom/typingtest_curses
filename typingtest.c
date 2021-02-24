#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <ncurses.h>

#define MAX_WORD_LEN 20
#define WORDS_IN_FILE 400
#define MAX_TEST_LEN 1000

void free_word_bank(char **words, int num_words);
void moveback(int y, int x, int row, int col, const char *text);
void get_words(FILE *file, char **words, const char delimiter);
int get_num_words(char *text);
static unsigned long get_nanos();
void trychar(int, char *, int *);
int randint(int n);
void get_random_words(int, char **, char *, int);

// TODO: fix word breaking problem
// TODO: add timed test feature

// compile with -lncurses
int main(int argc, char **argv)
{   
    const char word_delimiter = '\n';
    const char* word_list_path = 
        "fingers.txt";
    unsigned long start, end; // timestamps
    double time_taken;
    

    int c;
    int y, x;
    int end_y, end_x; // end pos of text in ncurses screen
    int row, col;
    int es_row, es_col; // end screen row/col
    int num_words; // word bank size
    int words_in_test;
    int errors;

    if (argc > 1)
        words_in_test = atoi(argv[1]);
    else
        words_in_test = 30;

    srand(time(NULL));


    num_words = 0;

    FILE *file = fopen(word_list_path, "r");

    /* Get the number of words in the file so that we
     * can allocate the correct amount of memory */
    while ((c = fgetc(file)) != EOF)
        if (c ==  word_delimiter)
            num_words++;

    if (num_words <= words_in_test) {
        printf(":", word_delimiter, word_list_path);
        exit(1);
    }

    char *word_bank[num_words]; // allocate space

    fseek(file, 0, 0); // move file pointer to start of file
    get_words(file, word_bank, word_delimiter);
    fclose(file);

    
    // fill `text` with random words
    char text[MAX_TEST_LEN];

    get_random_words(words_in_test, word_bank, text, num_words);
    free_word_bank(word_bank, num_words);

    int text_len = strlen(text);

    // initialize ncurses
    initscr();
    noecho();
    start_color();
    use_default_colors();


    init_pair(1, COLOR_GREEN, -1);
    init_pair(2, COLOR_WHITE, COLOR_RED);

    addstr(text);
    getyx(stdscr, end_y, end_x);
    move(0, 0);


    getyx(stdscr, y, x);
    errors = 0;
    start = get_nanos();

    /* main program */
    while (!(y == end_y && x == end_x-1)) {
        c = getch();
        trychar(c, text, &errors);
        getyx(stdscr, y, x);
    }

    end = get_nanos();
    time_taken = (end-start)/1000000000.0f;
    curs_set(0);

    /* end screen */
    getmaxyx(stdscr, row, col);
    es_row = end_y + 2;
    es_col = col/2 - 10;

    mvprintw(es_row, es_col, "time: ");
    mvprintw(es_row++, es_col + 10,  "%4.2fs", time_taken);

    mvprintw(es_row, es_col, "WPM: ");
    mvprintw(es_row++, es_col + 10,  "%3.2f", 60.0 * get_num_words(text)/time_taken);

    // assume each word is 4 characters long
    mvprintw(es_row, es_col, "Adj. WPM: ");
    mvprintw(es_row++, es_col + 10,  "%3.2f", 60.0 * text_len/4.0 / time_taken);

    mvprintw(es_row, es_col, "Accuracy: ");
    float acc = 100.0 * (text_len - errors)/text_len;
    mvprintw(es_row++, es_col + 10,  "%3d%%", (int) acc);

    es_row++;
    mvprintw(es_row++, es_col, "Press esc to continue.");


    while (getch() != 27)
        ;

    endwin();

    return 0;
}


void trychar(int c, char *text, int *errors)
{
    int y, x, row, col;

    getyx(stdscr, y, x);
    getmaxyx(stdscr, row, col);
    switch (c)
    {
        case 27:
            endwin();
            exit(0);

        case 127: // backspace
            if (x == 0 && y == 0)
                break;

            moveback(y, x, row, col, text);
            
            break;

         default:
            if (c == text[x + y*col]) {
                attron(COLOR_PAIR(1));
                addch(c);
                attroff(COLOR_PAIR(1));
            } else {
                attron(COLOR_PAIR(2));
                addch(text[x + y*col]);
                attroff(COLOR_PAIR(2));
                // TODO: fix so that errors arent double counted i.e. red is counted twice
                (*errors)++;
            }

   }

}


void moveback(int y, int x, int row, int col, const char *text)
{
    getyx(stdscr, y, x);
    attron(COLOR_PAIR(0));
    addch(text[y*col + x]);
    attroff(COLOR_PAIR(0));

    if (x > 0)
        move(y, x-1);
    else
        move(y-1, col-1);


}

static unsigned long get_nanos()
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (long) ts.tv_sec * 1000000000L + ts.tv_nsec;
}


int get_num_words(char *text)
{
    int wordcount = 0;
    for (int i = 0; i < strlen(text); i++)
        if (text[i] == ' ')
            wordcount++;
    wordcount++; // to account for the last word

    return wordcount;
}


void get_words(FILE *file, char **words, const char delimiter)
{
    int i, j, c;
    
    i = 0;
    j = 0;
    words[i] = malloc(MAX_WORD_LEN);
    while ((c = fgetc(file)) != EOF) {
        if (c == delimiter) {
            words[i][j] = '\0';
            words[++i] = malloc(MAX_WORD_LEN);
            j = 0;
        } else if (!isspace(c))
            words[i][j++] = c;
    }

}

void get_random_words(int count, char **words, char *text, int num_words)
{
    int r;
    int j = 0;
    char *w;

    for (int i = 0; i < count; i++) {
        r = randint(num_words);
        w = words[r];
        while (*w)
            text[j++] = *w++;

        text[j++] = ' ';
    }
    text[j] = '\0';

}


int randint(int n) 
{
  if ((n - 1) == RAND_MAX) {
    return rand();
  } else {
    // Supporting larger values for n would requires an even more
    // elaborate implementation that combines multiple calls to rand()
    assert (n <= RAND_MAX);

    // Chop off all of the values that would cause skew...
    int end = RAND_MAX / n; // truncate skew
    assert (end > 0);
    end *= n;

    // ... and ignore results from rand() that fall above that limit.
    // (Worst case the loop condition should succeed 50% of the time,
    // so we can expect to bail out of this loop pretty quickly.)
    int r;
    while ((r = rand()) >= end);

    return r % n;
  }
}


void free_word_bank(char **words, int num_words)
{
    for (int i = 0; i < num_words; i++)
        free(words[i]);
}
