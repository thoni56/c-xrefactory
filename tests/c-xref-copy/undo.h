#ifndef UNDO_H_INCLUDED
#define UNDO_H_INCLUDED

#include "editorbuffer.h"

typedef enum editorUndoOperation {
    UNDO_REPLACE_STRING,
    UNDO_RENAME_BUFFER,
    UNDO_MOVE_BLOCK,
} EditorUndoOperation;

typedef struct editorUndo {
    EditorBuffer       *buffer;
    EditorUndoOperation operation;
    union editorUndoUnion {
        struct editorUndoStrReplace {
            unsigned offset;
            unsigned size;
            unsigned strlen;
            char    *str;
        } replace;
        struct editorUndoRenameBuff {
            char *name;
        } rename;
        struct editorUndoMoveBlock {
            unsigned      offset;
            unsigned      size;
            EditorBuffer *dbuffer;
            unsigned      doffset;
        } moveBlock;
    } u;
    struct editorUndo *next;
} EditorUndo;


extern EditorUndo *editorUndo;


EditorUndo *newUndoRename(EditorBuffer *buffer, char *name,
                          EditorUndo *next);
EditorUndo *newUndoReplace(EditorBuffer *buffer, unsigned offset, unsigned size,
                           unsigned length, char *str, EditorUndo *next);
EditorUndo *newUndoMove(EditorBuffer *buffer, unsigned offset, unsigned size,
                        EditorBuffer *dbuffer, unsigned doffset,
                        EditorUndo *next);
#endif
