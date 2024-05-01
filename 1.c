#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>

#define BUFFER_SIZE 4096

void reverse_string(char *buffer, size_t length);
void add_path_component(char *path, char *component);
int reverse_file_copy(const char *source_path, const char *destination_path);
void reverse_directory(const char *source_path, const char *destination_path);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        return -1;
    }

    reverse_directory(argv[1], argv[1]);

    return 0;
}

void reverse_string(char *buffer, size_t length) {
    char temporary_character;
    for (size_t i = 0; i < length / 2; i++) {
        temporary_character = buffer[i];
        buffer[i] = buffer[length - i - 1];
        buffer[length - i - 1] = temporary_character;
    }
}

void add_path_component(char *path, char *component) {
    size_t path_length = strlen(path);
    size_t component_length = strlen(component);
    path[path_length] = '/';
    path[path_length + 1] = '\0';
    strncat(path, component, component_length);
}

int reverse_file_copy(const char *source_path, const char *destination_path) {
    int source_file_descriptor;
    int destination_file_descriptor;
    source_file_descriptor = open(source_path, O_RDONLY);

    if (source_file_descriptor < 0) {
        perror("Could not open source file");
        return -1;
    }

    destination_file_descriptor = open(destination_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

    if (destination_file_descriptor < 0) {
        perror("Could not open destination file");
        return -1;
    }

    struct stat source_file_status;
    fstat(source_file_descriptor, &source_file_status);
    long start_position = source_file_status.st_size - 1;
    char buffer[BUFFER_SIZE];
    while (true) {
        start_position = start_position - BUFFER_SIZE < 0 ? 0 : start_position - BUFFER_SIZE;

        lseek(source_file_descriptor, start_position, 0);

        size_t number_of_bytes_read = read(source_file_descriptor, buffer, BUFFER_SIZE);

        reverse_string(buffer, number_of_bytes_read);

        write(destination_file_descriptor, buffer, number_of_bytes_read);

        if (start_position == 0) {
            break;
        }
    }

    close(source_file_descriptor);
    close(destination_file_descriptor);
    return 0;
}

void reverse_directory(const char *source_path, const char *destination_path) {
    DIR *directory_stream = opendir(source_path);
    if (directory_stream == NULL) {
        return;
    }

    char original_directory[256];
    char reversed_directory[256];

    if (getcwd(reversed_directory, BUFFER_SIZE) == NULL) {
        perror("Could not get current working directory");
        return;
    }

    strncpy(original_directory, source_path, strlen(source_path));
    char *delimiter_position = strrchr(source_path, '/');
    char temporary_buffer[256];
    size_t temporary_buffer_length;
    if (delimiter_position != NULL) {
        temporary_buffer_length = strlen(delimiter_position) - 1;
        strncpy(temporary_buffer, delimiter_position + 1, temporary_buffer_length);
    } else {
        temporary_buffer_length = strlen(source_path);
        strncpy(temporary_buffer, source_path, temporary_buffer_length);
    }

    reverse_string(temporary_buffer, temporary_buffer_length);
    add_path_component(reversed_directory, temporary_buffer);

    int result_code = mkdir(reversed_directory, S_IRWXU);
    if (result_code != 0) {
        perror("Could not create reversed directory");
        return;
    }

    size_t original_directory_end_index = strlen(original_directory);
    size_t reversed_directory_end_index = strlen(reversed_directory);
    char *original_file_path = original_directory;
    char *reversed_file_path = reversed_directory;
    for (struct dirent *entry = readdir(directory_stream); entry != NULL; entry = readdir(directory_stream)) {
        if (entry->d_type == DT_REG) {
            original_file_path[original_directory_end_index] = '\0';
            reversed_file_path[reversed_directory_end_index] = '\0';

            add_path_component(original_file_path, entry->d_name);

            char *extension_position = strrchr(entry->d_name, '.');
            size_t length_to_reverse = 0;

            if (extension_position != NULL) {
                length_to_reverse = extension_position - entry->d_name;
            } else {
                length_to_reverse = strlen(entry->d_name);
            }

            reverse_string(entry->d_name, length_to_reverse);

            add_path_component(reversed_file_path, entry->d_name);

            int result = reverse_file_copy(original_file_path, reversed_file_path);
            if (result != 0) {
                return;
            }
        } else if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            original_file_path[original_directory_end_index] = '\0';
            reversed_file_path[reversed_directory_end_index] = '\0';

            add_path_component(original_file_path, entry->d_name);

            char *reversed_subdirectory = malloc(strlen(entry->d_name) + 1);
            strcpy(reversed_subdirectory, entry->d_name);
            reverse_string(reversed_subdirectory, strlen(reversed_subdirectory));
            add_path_component(reversed_file_path, reversed_subdirectory);
            free(reversed_subdirectory);

            reverse_directory(original_file_path, reversed_file_path);
        }
    }

    closedir(directory_stream);
}
