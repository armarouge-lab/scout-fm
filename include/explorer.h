#ifndef EXPLORER_H
#define EXPLORER_H

#include <limits.h>

#define MAX_VISIBLE_FILES 2048
#define MAX_NAME_LENGTH 256
#define MAX_PATH_BUFFER (PATH_MAX + MAX_NAME_LENGTH)

typedef struct {
    char name[MAX_NAME_LENGTH];
    char full_path[MAX_PATH_BUFFER];
    int is_directory;
    int is_expanded;
    int is_selected;
    int depth;
} FileEntry;

typedef struct {
    char current_path[MAX_PATH_BUFFER];
    FileEntry entries[MAX_VISIBLE_FILES];
    int file_count;
    int selected_index;
} ExplorerState;

typedef enum {
    DELETE_SUCCESS = 0,
    DELETE_ERROR,
    DELETE_NOT_EMPTY
} DeleteResult;

void explorer_initialize(ExplorerState *state);
void explorer_refresh(ExplorerState *state);

void explorer_toggle_expand(ExplorerState *state);
void explorer_enter(ExplorerState *state);
void explorer_go_up(ExplorerState *state);
void explorer_move(ExplorerState *state, int direction);
void explorer_toggle_mark(ExplorerState *state);
void explorer_open_file(ExplorerState *state);

void explorer_create_file(ExplorerState *state, const char *name);
void explorer_create_dir(ExplorerState *state, const char *name);

int explorer_has_marks(ExplorerState *state);
int explorer_is_dir_not_empty(const char *path);
DeleteResult explorer_delete(ExplorerState *state, int force);

#endif