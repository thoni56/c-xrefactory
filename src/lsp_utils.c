#include "lsp_utils.h"

#include <stdio.h>
#include <string.h>

/* Convert file:// URI to local file path */
char *uriToFilePath(const char *uri) {
    if (strncmp(uri, "file://", 7) == 0) {
        return (char *)(uri + 7);  // Skip "file://" prefix
    }
    return (char *)uri;  // Return as-is if not a file:// URI
}

/* Convert LSP line/character position to byte offset in file */
int lspPositionToByteOffset(const char *filePath, int line, int character) {
    FILE *file = fopen(filePath, "r");
    if (!file) {
        return 0;
    }
    
    int currentLine = 0;
    int byteOffset = 0;
    int c;
    
    while ((c = fgetc(file)) != EOF) {
        if (currentLine == line) {
            if (character == 0) {
                break;
            }
            character--;
        }
        
        byteOffset++;
        
        if (c == '\n') {
            currentLine++;
            if (currentLine > line) {
                break;
            }
        }
    }
    
    fclose(file);
    return byteOffset;
}

/* Convert byte offset in file to LSP line/character position */
void byteOffsetToLspPosition(const char *filePath, int byteOffset, int *line, int *character) {
    FILE *file = fopen(filePath, "r");
    if (!file) {
        *line = 0;
        *character = 0;
        return;
    }
    
    int currentLine = 0;
    int currentChar = 0;
    int currentOffset = 0;
    int c;
    
    while ((c = fgetc(file)) != EOF && currentOffset < byteOffset) {
        if (c == '\n') {
            currentLine++;
            currentChar = 0;
        } else {
            currentChar++;
        }
        currentOffset++;
    }
    
    fclose(file);
    *line = currentLine;
    *character = currentChar;
}