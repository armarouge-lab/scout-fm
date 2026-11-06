#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "explorer.h"

typedef struct {
    char paths[1024][MAX_PATH_BUFFER];
    int count;
} PathCache;

static int compare_entries(const void *a, const void *b) {
    const FileEntry *file_a = (const FileEntry *)a;
    const FileEntry *file_b = (const FileEntry *)b;

    if (file_a->is_directory && !file_b->is_directory) return -1;
    if (!file_a->is_directory && file_b->is_directory) return 1;
    return strcasecmp(file_a->name, file_b->name);
}

static void path_join(char *dest, size_t dest_size, const char *base, const char *append) {
    int len = snprintf(dest, dest_size, "%s/%s", base, append);
    if (len < 0 || (size_t)len >= dest_size) {
        dest[0] = '\0';
    }
}

static void save_tree_state(ExplorerState *state, PathCache *expanded, PathCache *selected) {
    expanded->count = 0;
    selected->count = 0;
    for (int i = 0; i < state->file_count; i++) {
        if (state->entries[i].is_expanded && expanded->count < 1024) {
            snprintf(expanded->paths[expanded->count++], MAX_PATH_BUFFER, "%s", state->entries[i].full_path);
        }
        if (state->entries[i].is_selected && selected->count < 1024) {
            snprintf(selected->paths[selected->count++], MAX_PATH_BUFFER, "%s", state->entries[i].full_path);
        }
    }
}

static void load_tree(ExplorerState *state, const char *path, int depth, PathCache *expanded, PathCache *selected) {
    DIR *dir = opendir(path);
    if (!dir) return;

    FileEntry *temp = malloc(sizeof(FileEntry) * MAX_VISIBLE_FILES);
    if (!temp) { closedir(dir); return; }

    int count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL && count < MAX_VISIBLE_FILES) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        snprintf(temp[count].name, MAX_NAME_LENGTH, "%s", entry->d_name);
        path_join(temp[count].full_path, MAX_PATH_BUFFER, path, entry->d_name);
        if (temp[count].full_path[0] == '\0') continue;

        temp[count].is_directory = (entry->d_type == DT_DIR);
        temp[count].depth = depth;
        temp[count].is_expanded = 0;
        temp[count].is_selected = 0;

        if (expanded) {
            for (int i = 0; i < expanded->count; i++) {
                if (strcmp(expanded->paths[i], temp[count].full_path) == 0) temp[count].is_expanded = 1;
            }
        }
        if (selected) {
            for (int i = 0; i < selected->count; i++) {
                if (strcmp(selected->paths[i], temp[count].full_path) == 0) temp[count].is_selected = 1;
            }
        }
        count++;
    }
    closedir(dir);

    qsort(temp, count, sizeof(FileEntry), compare_entries);

    for (int i = 0; i < count; i++) {
        if (state->file_count >= MAX_VISIBLE_FILES) break;
        
        state->entries[state->file_count] = temp[i];
        state->file_count++;

        if (temp[i].is_directory && temp[i].is_expanded) {
            load_tree(state, temp[i].full_path, depth + 1, expanded, selected);
        }
    }
    free(temp);
}

void explorer_initialize(ExplorerState *state) {
    if (getcwd(state->current_path, sizeof(state->current_path)) == NULL) exit(EXIT_FAILURE);
    state->selected_index = 0;
    state->file_count = 0;
    explorer_refresh(state);
}

void explorer_refresh(ExplorerState *state) {
    char last_selected[MAX_PATH_BUFFER] = "";
    if (state->file_count > 0 && state->selected_index < state->file_count) {
        snprintf(last_selected, sizeof(last_selected), "%s", state->entries[state->selected_index].full_path);
    }

    PathCache *expanded = malloc(sizeof(PathCache));
    PathCache *selected = malloc(sizeof(PathCache));
    
    if (expanded && selected) {
        save_tree_state(state, expanded, selected);
    }

    state->file_count = 0;
    load_tree(state, state->current_path, 0, expanded, selected);

    if (expanded) free(expanded);
    if (selected) free(selected);

    for (int i = 0; i < state->file_count; i++) {
        if (strcmp(state->entries[i].full_path, last_selected) == 0) {
            state->selected_index = i;
            return;
        }
    }
    if (state->selected_index >= state->file_count) {
        state->selected_index = (state->file_count > 0) ? state->file_count - 1 : 0;
    }
}

void explorer_toggle_expand(ExplorerState *state) {
    if (state->file_count == 0) return;
    FileEntry *current = &state->entries[state->selected_index];
    if (current->is_directory) {
        current->is_expanded = !current->is_expanded;
        explorer_refresh(state);
    }
}

void explorer_enter(ExplorerState *state) {
    if (state->file_count == 0) return;
    
    /* Cópia temporária para evitar o erro -Werror=restrict do snprintf */
    char target_path[MAX_PATH_BUFFER];
    snprintf(target_path, sizeof(target_path), "%s", state->entries[state->selected_index].full_path);
    
    if (state->entries[state->selected_index].is_directory) {
        if (chdir(target_path) == 0) {
            snprintf(state->current_path, sizeof(state->current_path), "%s", target_path);
            state->selected_index = 0;
            state->file_count = 0;
            explorer_refresh(state);
        }
    } else {
        explorer_open_file(state);
    }
}

void explorer_go_up(ExplorerState *state) {
    if (chdir("..") == 0) {
        if (getcwd(state->current_path, sizeof(state->current_path)) != NULL) {
            state->selected_index = 0;
            state->file_count = 0;
            explorer_refresh(state);
        }
    }
}

void explorer_move(ExplorerState *state, int direction) {
    state->selected_index += direction;
    if (state->selected_index < 0) state->selected_index = 0;
    if (state->selected_index >= state->file_count && state->file_count > 0) state->selected_index = state->file_count - 1;
}

void explorer_toggle_mark(ExplorerState *state) {
    if (state->file_count == 0) return;
    state->entries[state->selected_index].is_selected = !state->entries[state->selected_index].is_selected;
    explorer_move(state, 1);
}

void explorer_open_file(ExplorerState *state) {
    if (state->file_count == 0) return;
    FileEntry *current = &state->entries[state->selected_index];
    if (!current->is_directory) {
        char cmd[MAX_PATH_BUFFER + 32];
        int len = snprintf(cmd, sizeof(cmd), "pyro \"%s\" &", current->full_path);
        if (len > 0 && (size_t)len < sizeof(cmd)) {
            system(cmd);
        }
    }
}

void explorer_create_file(ExplorerState *state, const char *name) {
    char path[MAX_PATH_BUFFER];
    path_join(path, sizeof(path), state->current_path, name);
    if (path[0] == '\0') return;
    
    FILE *f = fopen(path, "w");
    if (f) { fclose(f); explorer_refresh(state); }
}

void explorer_create_dir(ExplorerState *state, const char *name) {
    char path[MAX_PATH_BUFFER];
    path_join(path, sizeof(path), state->current_path, name);
    if (path[0] == '\0') return;
    
    if (mkdir(path, 0755) == 0) explorer_refresh(state);
}

int explorer_has_marks(ExplorerState *state) {
    for (int i = 0; i < state->file_count; i++) {
        if (state->entries[i].is_selected) return 1;
    }
    return 0;
}

int explorer_is_dir_not_empty(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return 0;
    int has_files = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        has_files = 1;
        break;
    }
    closedir(dir);
    return has_files;
}

static DeleteResult delete_recursive(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return (remove(path) == 0) ? DELETE_SUCCESS : DELETE_ERROR;

    struct dirent *entry;
    char child[MAX_PATH_BUFFER];
    DeleteResult result = DELETE_SUCCESS;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        path_join(child, sizeof(child), path, entry->d_name);
        if (child[0] == '\0') continue;

        if (entry->d_type == DT_DIR) {
            if (delete_recursive(child) != DELETE_SUCCESS) result = DELETE_ERROR;
        } else {
            if (unlink(child) != 0) result = DELETE_ERROR;
        }
    }
    closedir(dir);
    if (rmdir(path) != 0) result = DELETE_ERROR;
    return result;
}

DeleteResult explorer_delete(ExplorerState *state, int force) {
    if (state->file_count == 0) return DELETE_ERROR;
    
    int has_marks = explorer_has_marks(state);
    DeleteResult final_result = DELETE_SUCCESS;

    for (int i = 0; i < state->file_count; i++) {
        if (has_marks && !state->entries[i].is_selected) continue;
        if (!has_marks && i != state->selected_index) continue;

        FileEntry *current = &state->entries[i];
        DeleteResult res;

        if (current->is_directory) {
            res = force ? delete_recursive(current->full_path) : 
                         ((rmdir(current->full_path) == 0) ? DELETE_SUCCESS : 
                         (errno == ENOTEMPTY ? DELETE_NOT_EMPTY : DELETE_ERROR));
        } else {
            res = (remove(current->full_path) == 0) ? DELETE_SUCCESS : DELETE_ERROR;
        }
        
        if (res != DELETE_SUCCESS) final_result = res;
    }
    
    explorer_refresh(state);
    return final_result;
}