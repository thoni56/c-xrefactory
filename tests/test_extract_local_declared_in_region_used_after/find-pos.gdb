# Iterate the referenceableItemTable directly. The table is a static
# inside referenceableitemtable.c, so use the file-qualified name.
set $table = 'referenceableitemtable.c'::referenceableItemTable
printf "table size = %d\n", $table.size
set $i = 0
set $count = 0
while $i < $table.size
  set $item = $table.tab[$i]
  while $item != 0
    printf "[%d] %s  (refs start: %p)\n", $i, $item->linkName, $item->references
    set $count = $count + 1
    set $item = $item->next
  end
  set $i = $i + 1
end
printf "total items: %d\n", $count
