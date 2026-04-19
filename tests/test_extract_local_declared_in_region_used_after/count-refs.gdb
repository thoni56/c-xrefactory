# Run at a breakpoint inside classifyVariableUsingDataFlow, where
# `variableNode` is in scope. Counts all Reference entries on the
# variable's referenceableItem (regardless of whether they were
# added to the program graph).
set $r = variableNode->referenceableItem->references
set $n = 0
while $r != 0
  set $n = $n + 1
  set $r = $r->next
end
printf "linkName : %s\n", variableNode->referenceableItem->linkName
printf "refs     : %d\n", $n
