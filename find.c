#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// This function takes a long path like "a/b/c/file.txt" and returns just "file.txt"
char* extract_filename(char *full_path) {
    char *char_ptr;

    // 1. full_path + strlen(full_path) moves the pointer to the very end of the string.
    // 2. The loop moves the pointer backwards (char_ptr--) one character at a time.
    // 3. It stops if it reaches the beginning of the string, or if it hits a '/' slash.
    for (char_ptr = full_path + strlen(full_path); char_ptr >= full_path && *char_ptr != '/'; char_ptr--)
        ;

    // Step forward one character so the pointer sits on the first letter of the file name, 
    // immediately after the '/' we just found.
    char_ptr++;
    return char_ptr;
}

// A recursive function that searches a directory (and all sub-directories) for a target file.
void find(char *start_path, char *target_name) {
    char path_buffer[512];      // A buffer to build the new paths we are going to explore
    char *path_end_ptr;         // A pointer to keep track of the end of the path we are building
    int file_descriptor;        // The integer ID the OS gives us to read the file/directory
    struct dirent dir_entry;    // A structure holding one directory record (inode + file name)
    struct stat file_stats;     // A structure holding the physical metadata of the file (type, size)

    // Ask the OS to open whatever path we passed in. 0 means read-only.
    // If the OS returns a negative number, the path doesn't exist or we lack permissions.
    if ((file_descriptor = open(start_path, 0)) < 0) {
        printf("Can't open %s\n", start_path);
        return;
    }

    // Ask the OS to fill our file_stats structure with info about the opened file descriptor.
    if (fstat(file_descriptor, &file_stats) < 0) {
        close(file_descriptor);
        return;
    }

    // Base Case: If the path we opened is just a regular file...
    if (file_stats.type == T_FILE) {
        // Extract its short name and compare it to our target.
        // strcmp returns 0 if the two strings are exactly identical.
        if (strcmp(extract_filename(start_path), target_name) == 0) {
            printf("%s\n", start_path); // We found it! Print the full path.
        }

    // Recursive Case: If the path we opened is a directory (folder)...
    } else if (file_stats.type == T_DIR) {
        
        // Copy the current directory path into our buffer so we can append to it.
        strcpy(path_buffer, start_path);
        
        // Move our pointer to the end of the current path string.
        path_end_ptr = path_buffer + strlen(path_buffer);
        
        // Add a slash to the end (e.g., turning "folder" into "folder/")
        *path_end_ptr = '/';
        path_end_ptr++; // Move pointer one spot past the slash to get ready for the filename.

        // Loop to read the directory. A directory is essentially a file containing 'dirent' records.
        // We ask read() to grab exactly one 'dirent' size of bytes. It returns the number of bytes read.
        // As long as it successfully reads a full record, the loop continues.
        while (read(file_descriptor, &dir_entry, sizeof(dir_entry)) == sizeof(dir_entry)) {
            
            // If the inode number (inum) is 0, this file was deleted. Skip it.
            if (dir_entry.inum == 0) {
                continue;
            }

            // Every folder contains "." (current folder) and ".." (parent folder).
            // If we search these, we will be stuck in an infinite loop forever. Skip them.
            if (strcmp(dir_entry.name, ".") == 0 || strcmp(dir_entry.name, "..") == 0) {
                continue;
            }

            // Safely copy the file name we just read into our path buffer, right after the slash.
            // DIRSIZ is the maximum allowed length of a filename in xv6.
            memmove(path_end_ptr, dir_entry.name, DIRSIZ);
            
            // Ensure the string has a null-terminator at the very end so C knows where it stops.
            path_end_ptr[DIRSIZ] = 0;

            // RECURSION: We have built a new full path (e.g., "folder/file.txt").
            // Call find() on this new path to see if it's our target, or if it's another folder to search.
            find(path_buffer, target_name);
        }
    }
    
    // Always close the file descriptor when done to free up OS resources.
    close(file_descriptor);
}

int main(int argc, char *argv[]) {
    // argc is the argument count. If the user didn't provide exactly the program name, 
    // the starting directory, and the target file (3 arguments), show them how to use it.
    if (argc != 3) {
        printf("Usage: find <starting directory> <file/directory name>\n");
        exit(1);
    }

    // Kick off the search using the first argument (argv[1]) as the start path, 
    // and the second argument (argv[2]) as the target to look for.
    find(argv[1], argv[2]);

    // Tell the OS the program finished successfully.
    exit(0);
}
