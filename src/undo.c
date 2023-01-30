#include "undo.h"

#include "memory.h"

EditorUndo *editorUndo = NULL;

EditorUndo *newUndoReplace(EditorBuffer *buffer, unsigned offset, unsigned size,
                                 unsigned length, char *str, EditorUndo *next) {
    EditorUndo *undo;

    undo = editorAlloc(sizeof(EditorUndo));
    undo->buffer = buffer;
    undo->operation = UNDO_REPLACE_STRING;
    undo->u.replace.offset = offset;
    undo->u.replace.size = size;
    undo->u.replace.strlen = length;
    undo->u.replace.str = str;
    undo->next = next;

    return undo;
}

EditorUndo *newUndoMove(EditorBuffer *buffer, unsigned offset, unsigned size,
                              EditorBuffer *dbuffer, unsigned doffset,
                              EditorUndo *next) {
    EditorUndo *undo;

    undo = editorAlloc(sizeof(EditorUndo));
    undo->buffer = buffer;
    undo->operation = UNDO_MOVE_BLOCK;
    undo->u.moveBlock.offset = offset;
    undo->u.moveBlock.size = size;
    undo->u.moveBlock.dbuffer = dbuffer;
    undo->u.moveBlock.doffset = doffset;
    undo->next = next;

    return undo;
}
