# Run at a breakpoint inside classifyVariableUsingDataFlow, where
# `variableNode` is in scope. Lists each Reference on the
# variable's referenceableItem with its usage and position.
set $r = variableNode->referenceableItem->references
set $n = 0
printf "linkName : %s\n", variableNode->referenceableItem->linkName
printf "  idx  usage   line:col\n"
while $r != 0
  printf "  %3d  %5d   %d:%d\n", $n, $r->usage, $r->position.line, $r->position.col
  set $n = $n + 1
  set $r = $r->next
end
printf "total : %d\n", $n
