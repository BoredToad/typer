#include <curses.h>
#include <ncurses.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define BUFFERSIZE 1024
typedef char text[BUFFERSIZE];

typedef struct {
    uint32_t cursor;
    text target;
    text written;
} Game;

// what stats I provide I'll check later
typedef struct {
    struct timeval start_time;
    struct timeval end_time;
    uint32_t correct_words;
} Stats;

Game init_game() {
    Game game = {
        .target = "hello world",
        .written = {},
        .cursor = 0,
    };
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
    // possible rewrite?
    uint32_t words = 0;
    bool correct = true;
    for (int i = 0; game.written[i++] != '\0';) {
        if (game.written[i] != game.target[i]) correct = false;
        if (game.written[i] == ' ' || game.target[i] == '\0') {
            if (correct) words++;
            correct = true;
        }
    }

    Stats stats = {
        .start_time = start_time,
        .end_time = end_time,
        .correct_words = words,
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

int main(int argc, char** argv) {
    nc_init();

    // TODO:
    // - main loop for a game,
    // - post game screen
    // - main menu
    // - difficulty settings
    // - history
    Stats stats = run_game(init_game());

    curs_set(1);
    endwin();

    printf("game took: (%o milliseconds) (%.2f seconds)\n",
           game_duration(stats), game_duration(stats) / 1000.);
    printf("correct: %i\n", stats.correct_words);

    return 0;
}
