#ifndef LSP_UTILS_H_INCLUDED
#define LSP_UTILS_H_INCLUDED

/* LSP Coordinate and URI Conversion Utilities */

/* Convert file:// URI to local file path */
extern char *uriToFilePath(const char *uri);

/* Convert LSP line/character position to byte offset in file */
extern int lspPositionToByteOffset(const char *filePath, int line, int character);

/* Convert byte offset in file to LSP line/character position */
extern void byteOffsetToLspPosition(const char *filePath, int byteOffset, int *line, int *character);

#endif /* LSP_UTILS_H_INCLUDED */