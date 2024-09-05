#include <assert.h>
#include <curses.h>
#include <ncurses.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define KNOWN_WORDS 10000
#define BUFFERSIZE 1024
typedef char text[BUFFERSIZE];

typedef enum { SHORT = 10, MEDIUM = 25, LONG = 100, QUOTE, NONE } Game_type;

typedef struct {
    uint32_t cursor;
    text target;
    text written;
    Game_type gametype;
} Game;

typedef struct {
    struct timeval start_time;
    struct timeval end_time;
    uint32_t correct_words;
    Game_type gametype;
} Stats;

void load_dictionary(char* dictionary[KNOWN_WORDS]) {
    // PATH is relative, have to fix for release
    FILE* list = fopen("./wordlist.txt", "r");
    assert(list);
    char* line;
    size_t len = 0;
    ssize_t read;

    int i = 0;
    while ((read = getline(&line, &len, list)) != -1) {
        dictionary[i] = malloc(read);
        strcpy(dictionary[i], line);
        dictionary[i][strlen(line) - 1] = '\0';
        i++;
    }

    free(line);
    fclose(list);
}

void free_dictionary(char* dictionary[KNOWN_WORDS]) {
    for (int i = 0; i < KNOWN_WORDS; i++) free(dictionary[i]);
}

Game init_game(Game_type len, char* dictionary[KNOWN_WORDS]) {
    srand(time(NULL));

    text target;
    uint32_t pointer = 0;

    for (int i = 0; i < len; i++) {
        strcpy(&target[pointer], dictionary[rand() % KNOWN_WORDS]);
        pointer += strlen(&target[pointer]);
        target[pointer++] = ' ';
    }

    Game game = {
        .written = {},
        .cursor = 0,
        .gametype = len,
    };
    strcpy(game.target, target);
    return game;
}

bool input(Game* game) {
    char ch = getch();
    if (ch == '\n') return false;
    if (ch == 127) {  // backspace
        if (game->cursor) game->written[--game->cursor] = '\0';
        return true;
    }
    if (game->cursor >= BUFFERSIZE - 1) return true;
    game->written[game->cursor++] = ch;
    printf("%c\n", ch);

    return true;
}

void render(const Game game) {  // possibly gonna refactor
    clear();

    for (int i = 0; i < BUFFERSIZE; i++) {
        if (i == game.cursor) attron(standout());
        if (game.target[i] == '\0' && game.written[i] == '\0') {
            addch(' ');
            break;
        };

        if (game.written[i] == '\0') {
            attron(COLOR_PAIR(1));
            addch(game.target[i]);

        } else if (game.written[i] == game.target[i]) {
            attron(COLOR_PAIR(2));
            addch(game.written[i]);

        } else {
            attron(COLOR_PAIR(3));
            if (game.written[i] == ' ' && game.target[i] != '\0')
                addch(game.target[i]);
            else
                addch(game.written[i]);
        }
        standend();
    }

    refresh();
}

Stats get_stats(const Game game, struct timeval start_time,
                struct timeval end_time) {
    uint32_t words = 0;
    bool correct = true;
    for (int i = 0; game.written[i++] != '\0';) {
        if (game.written[i] != game.target[i]) correct = false;
        if (game.written[i] == ' ' || game.written[i + 1] == '\0') {
            if (correct) words++;
            correct = true;
        }
    }

    Stats stats = {
        .start_time = start_time,
        .end_time = end_time,
        .correct_words = words,
        .gametype = game.gametype,
    };
    return stats;
}

Stats run_game(Game game) {
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    do render(game);
    while (input(&game));

    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    return get_stats(game, start_time, end_time);
}

void nc_init() {
    initscr();
    start_color();
    curs_set(0);
    init_pair(1, COLOR_WHITE, COLOR_BLACK);  // target
    init_pair(2, COLOR_BLUE, COLOR_BLACK);   // correct
    init_pair(3, COLOR_RED, COLOR_BLACK);    // false
}

// returns game duration in milliseconds
uint64_t game_duration(const Stats stats) {
    return (stats.end_time.tv_sec - stats.start_time.tv_sec) * 1000 +
           (stats.end_time.tv_usec - stats.start_time.tv_usec) / 1000;
}

Game_type handle_args(int argc, char** argv) {
    if (argc != 2) {
        printf(
            "usage:\t./typer [game type: [S]hort, [M]edium, [L]ong, "
            "([Q]uote?)]\n");
        return NONE;
    }
    if (!strcmp(argv[1], "short") || !strcmp(argv[1], "s")) return SHORT;
    if (!strcmp(argv[1], "medium") || !strcmp(argv[1], "m")) return MEDIUM;
    if (!strcmp(argv[1], "long") || !strcmp(argv[1], "l")) return LONG;
    printf("%s is not a game type, try short, medium, or long\n", argv[1]);
    return NONE;
}

// TODO:
// - possibly add a quote mode?
int main(int argc, char** argv) {
    Game_type gametype = handle_args(argc, argv);
    if (gametype == NONE) return 0;

    char* dictionary[KNOWN_WORDS];
    load_dictionary(dictionary);
    nc_init();

    Stats stats = run_game(init_game(gametype, dictionary));

    curs_set(1);
    endwin();
    free_dictionary(dictionary);

    printf("game took: %.2f seconds\n", game_duration(stats) / 1000.);
    printf("correct words: %i\n", stats.correct_words);
    float minutes = game_duration(stats) / 1000. / 60.;
    float wpm = stats.correct_words / minutes;
    printf("wpm: %f\n", wpm);
    return 0;
}
