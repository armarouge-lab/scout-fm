#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include "explorer.h"

static int compare_file_entries(const void *first_entry, const void *second_entry) {
    const FileEntry *file_a = (const FileEntry *)first_entry;
    const FileEntry *file_b = (const FileEntry *)second_entry;

    if (file_a->is_directory && !file_b->is_directory) {
        return -1;
    }
    if (!file_a->is_directory && file_b->is_directory) {
        return 1;
    }
    return strcasecmp(file_a->name, file_b->name);
}

void explorer_initialize(ExplorerState *state) {
    if (getcwd(state->current_path, sizeof(state->current_path)) == NULL) {
        perror("Failed to get current working directory");
        exit(EXIT_FAILURE);
    }
    explorer_refresh_listing(state);
}

void explorer_refresh_listing(ExplorerState *state) {
    DIR *directory_stream = opendir(state->current_path);
    if (directory_stream == NULL) {
        perror("Failed to open directory");
        return;
    }

    state->file_count = 0;
    struct dirent *directory_entry;

    while ((directory_entry = readdir(directory_stream)) != NULL && state->file_count < MAX_VISIBLE_FILES) {
        if (strcmp(directory_entry->d_name, ".") == 0 || strcmp(directory_entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(state->entries[state->file_count].name, MAX_NAME_LENGTH, "%s", directory_entry->d_name);
        state->entries[state->file_count].is_directory = (directory_entry->d_type == DT_DIR);
        
        state->file_count++;
    }
    closedir(directory_stream);

    qsort(state->entries, state->file_count, sizeof(FileEntry), compare_file_entries);
    state->selected_index = 0;
}

void explorer_execute_current_selection(ExplorerState *state) {
    if (state->file_count == 0) {
        return;
    }
    
    FileEntry *current_selection = &state->entries[state->selected_index];
    
    if (current_selection->is_directory) {
        char target_path[PATH_MAX];
        int written_length = snprintf(target_path, sizeof(target_path), "%s/%s", state->current_path, current_selection->name);
        
        if (written_length > 0 && written_length < (int)sizeof(target_path)) {
            if (chdir(target_path) == 0) {
                snprintf(state->current_path, sizeof(state->current_path), "%s", target_path);
                explorer_refresh_listing(state);
            }
        }
    } else {
        explorer_open_selected_file(state);
    }
}

void explorer_navigate_to_parent(ExplorerState *state) {
    if (chdir("..") == 0) {
        if (getcwd(state->current_path, sizeof(state->current_path)) != NULL) {
            explorer_refresh_listing(state);
        }
    }
}

void explorer_move_selection(ExplorerState *state, int direction) {
    state->selected_index += direction;
    
    if (state->selected_index < 0) {
        state->selected_index = 0;
    }
    if (state->selected_index >= state->file_count) {
        state->selected_index = state->file_count - 1;
    }
}

void explorer_open_selected_file(ExplorerState *state) {
    if (state->file_count == 0) {
        return;
    }
    
    FileEntry *current_selection = &state->entries[state->selected_index];
    if (!current_selection->is_directory) {
        char command_buffer[MAX_COMMAND_LENGTH];
        int written_length = snprintf(command_buffer, sizeof(command_buffer), "pyro \"%s/%s\" &", state->current_path, current_selection->name);
        
        if (written_length > 0 && written_length < (int)sizeof(command_buffer)) {
            system(command_buffer);
        }
    }
}