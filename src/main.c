#define _DEFAULT_SOURCE
#include <ncurses.h>
#include <locale.h>
#include "explorer.h"

int main(void) {
    setlocale(LC_ALL, "");

    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    clear();
    refresh();

    wtimeout(stdscr, 100); 

    ExplorerState explorer_state;
    explorer_initialize(&explorer_state);

    int running = 1;

    while (running) {
        clear();
        
        attron(A_BOLD);
        mvprintw(0, 0, "ScoutFM - %s", explorer_state.current_path);
        attroff(A_BOLD);
        
        mvhline(1, 0, ACS_HLINE, COLS);

        for (int index = 0; index < explorer_state.file_count; index++) {
            if (index == explorer_state.selected_index) {
                attron(A_REVERSE);
            }
            
            const char *file_icon = explorer_state.entries[index].is_directory ? "📁" : "📄";
            mvprintw(3 + index, 2, "%s %s", file_icon, explorer_state.entries[index].name);
            
            if (index == explorer_state.selected_index) {
                attroff(A_REVERSE);
            }
        }

        mvhline(LINES - 1, 0, ACS_HLINE, COLS);
        attron(A_REVERSE);
        mvprintw(LINES - 1, 0, " -- NORMAL --  [q]uit  [Enter/l]open  [h/Backspace]up ");
        attroff(A_REVERSE);

        refresh();

        int pressed_key = getch();

        switch (pressed_key) {
            case KEY_UP:
            case 'k':
                explorer_move_selection(&explorer_state, -1);
                break;
                
            case KEY_DOWN:
            case 'j':
                explorer_move_selection(&explorer_state, 1);
                break;
                
            case KEY_BACKSPACE:
            case KEY_LEFT:
            case 127: 
            case 'h':
                explorer_navigate_to_parent(&explorer_state);
                break;
                
            case KEY_RIGHT:
            case 'l':
            case '\n':
            case '\r':      
            case KEY_ENTER:
                explorer_execute_current_selection(&explorer_state);
                break;
                
            case 'q':
                running = 0;
                break;
        }

        flushinp();
    }

    endwin();
    return 0;
}