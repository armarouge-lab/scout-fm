#ifndef EXPLORER_H
#define EXPLORER_H

#include <limits.h>

#define MAX_VISIBLE_FILES 1024
#define MAX_NAME_LENGTH 256
#define MAX_COMMAND_LENGTH (PATH_MAX + MAX_NAME_LENGTH + 32)

typedef struct {
    char name[MAX_NAME_LENGTH];
    int is_directory;
} FileEntry;

typedef struct {
    char current_path[PATH_MAX];
    FileEntry entries[MAX_VISIBLE_FILES];
    int file_count;
    int selected_index;
} ExplorerState;

void explorer_initialize(ExplorerState *state);
void explorer_refresh_listing(ExplorerState *state);
void explorer_execute_current_selection(ExplorerState *state);
void explorer_navigate_to_parent(ExplorerState *state);
void explorer_move_selection(ExplorerState *state, int direction);
void explorer_open_selected_file(ExplorerState *state);

#endif