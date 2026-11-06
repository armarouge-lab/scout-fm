#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <locale.h>
#include "explorer.h"

static ExplorerState g_state;
static int g_scroll_offset = 0;

/* Draws the file list based on the current scroll offset */
static void draw_list(int max_lines) {
    for (int i = 0; i < max_lines; i++) {
        int visible_index = g_scroll_offset + i;
        if (visible_index >= g_state.file_count) break;

        FileEntry *entry = &g_state.entries[visible_index];
        
        /* Color logic: Cursor > Marked > Directory > File */
        if (visible_index == g_state.selected_index) {
            attron(COLOR_PAIR(1) | A_BOLD);
        } else if (entry->is_selected) {
            attron(COLOR_PAIR(6) | A_BOLD);
        } else if (entry->is_directory) {
            attron(COLOR_PAIR(2));
        } else {
            attron(COLOR_PAIR(3));
        }
        
        /* Draw the tree structure */
        int x_pos = 2;
        for (int level = 0; level < entry->depth; level++) {
            if (level == entry->depth - 1) {
                mvaddstr(3 + i, x_pos, "├──");
            } else {
                mvaddstr(3 + i, x_pos, "│  ");
            }
            x_pos += 3;
        }
        
        const char *icon = entry->is_directory ? (entry->is_expanded ? "[-]" : "[+]") : "   ";
        const char *mark = entry->is_selected ? "*" : " ";
        
        mvprintw(3 + i, x_pos, "%s %s %s", mark, icon, entry->name);
        
        attroff(COLOR_PAIR(1) | COLOR_PAIR(2) | COLOR_PAIR(3) | COLOR_PAIR(6) | A_BOLD);
    }
}

/* Draws the two status bars at the bottom */
static void draw_status_bar(void) {
    int selected_count = 0;
    for (int i = 0; i < g_state.file_count; i++) {
        if (g_state.entries[i].is_selected) selected_count++;
    }

    /* Line 1: Shortcuts */
    attron(COLOR_PAIR(4));
    mvhline(LINES - 2, 0, ' ', COLS);
    mvprintw(LINES - 2, 0, " [j/k]move [v]mark [Enter]enter [l]expand [h]up [a/A]add [d]del [q]quit ");
    attroff(COLOR_PAIR(4));

    /* Line 2: Mode */
    attron(COLOR_PAIR(4) | A_REVERSE);
    mvhline(LINES - 1, 0, ' ', COLS);
    if (selected_count > 0) {
        mvprintw(LINES - 1, 0, " -- VISUAL -- (%d marked) ", selected_count);
    } else {
        mvprintw(LINES - 1, 0, " -- NORMAL -- ");
    }
    attroff(COLOR_PAIR(4) | A_REVERSE);
}

/* Reads text input from the user safely */
static void prompt_input(const char *prompt, char *buffer, int max_len) {
    wtimeout(stdscr, -1);
    echo();
    curs_set(1);
    attron(COLOR_PAIR(5) | A_BOLD);
    mvprintw(LINES - 1, 0, " %s: ", prompt);
    clrtoeol();
    attroff(COLOR_PAIR(5) | A_BOLD);
    
    wgetnstr(stdscr, buffer, max_len - 1);
    
    noecho();
    curs_set(0);
    wtimeout(stdscr, 100);
}

int main(void) {
    setlocale(LC_ALL, "");
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    scrollok(stdscr, FALSE); 
    
    start_color();
    use_default_colors();
    init_pair(1, 235, 208); /* Cursor */
    init_pair(2, 208, -1);  /* Directories */
    init_pair(3, 250, -1);  /* Files */
    init_pair(4, 235, 250); /* Status bar */
    init_pair(5, 196, -1);  /* Prompts/Errors */
    init_pair(6, 46, -1);   /* Marked */

    explorer_initialize(&g_state);
    wtimeout(stdscr, 100); 

    int running = 1;

    while (running) {
        erase();
        
        int max_lines = LINES - 4;
        if (max_lines < 1) max_lines = 1;

        /* Adjust scroll offset to keep cursor on screen */
        if (g_state.selected_index < g_scroll_offset) {
            g_scroll_offset = g_state.selected_index;
        } else if (g_state.selected_index >= g_scroll_offset + max_lines) {
            g_scroll_offset = g_state.selected_index - max_lines + 1;
        }

        /* Draw Header */
        attron(A_BOLD);
        mvprintw(0, 0, "ScoutFM - %s", g_state.current_path);
        attroff(A_BOLD);
        mvhline(1, 0, ACS_HLINE, COLS);

        draw_list(max_lines);
        draw_status_bar();
        refresh();

        int key = getch();

        switch (key) {
            case KEY_RESIZE:
                clear();
                break;
            case KEY_UP: case 'k':
                explorer_move(&g_state, -1);
                break;
            case KEY_DOWN: case 'j':
                explorer_move(&g_state, 1);
                break;
            case ' ': case 'v':
                explorer_toggle_mark(&g_state);
                break;
            case KEY_LEFT: case 'h':
                if (g_state.file_count > 0 && g_state.entries[g_state.selected_index].is_directory && g_state.entries[g_state.selected_index].is_expanded) {
                    g_state.entries[g_state.selected_index].is_expanded = 0;
                    explorer_refresh(&g_state);
                } else {
                    explorer_go_up(&g_state);
                    g_scroll_offset = 0;
                }
                break;
            case KEY_RIGHT: case 'l':
                explorer_toggle_expand(&g_state);
                break;
            case '\n': case '\r': case KEY_ENTER:
                explorer_enter(&g_state);
                g_scroll_offset = 0;
                break;
            case 'a': {
                char name[MAX_NAME_LENGTH] = {0};
                prompt_input("New file", name, MAX_NAME_LENGTH);
                if (strlen(name) > 0) explorer_create_file(&g_state, name);
                break;
            }
            case 'A': {
                char name[MAX_NAME_LENGTH] = {0};
                prompt_input("New directory", name, MAX_NAME_LENGTH);
                if (strlen(name) > 0) explorer_create_dir(&g_state, name);
                break;
            }
            case 'd': {
                if (g_state.file_count > 0) {
                    int has_marks = explorer_has_marks(&g_state);
                    int needs_force = 0;
                    char folder_name[MAX_NAME_LENGTH] = "";

                    for (int i = 0; i < g_state.file_count; i++) {
                        if (has_marks && !g_state.entries[i].is_selected) continue;
                        if (!has_marks && i != g_state.selected_index) continue;

                        if (g_state.entries[i].is_directory && explorer_is_dir_not_empty(g_state.entries[i].full_path)) {
                            needs_force = 1;
                            snprintf(folder_name, sizeof(folder_name), "%s", g_state.entries[i].name);
                            break;
                        }
                    }

                    attron(COLOR_PAIR(5) | A_BOLD);
                    if (needs_force) {
                        mvprintw(LINES - 1, 0, " Folder '%s' has files on it. Delete anyway? (y/n): ", folder_name);
                    } else if (has_marks) {
                        mvprintw(LINES - 1, 0, " Delete marked files? (y/n): ");
                    } else {
                        mvprintw(LINES - 1, 0, " Delete '%s'? (y/n): ", g_state.entries[g_state.selected_index].name);
                    }
                    clrtoeol();
                    attroff(COLOR_PAIR(5) | A_BOLD);
                    refresh();
                    
                    wtimeout(stdscr, -1); 
                    int choice = getch(); 
                    wtimeout(stdscr, 100);
                    
                    if (choice == 'y' || choice == 'Y') {
                        explorer_delete(&g_state, needs_force);
                    }
                }
                break;
            }
            case 'q':
                running = 0;
                break;
        }
        flushinp();
    }
    endwin();
    return 0;
}