# Run at a breakpoint inside classifyVariableUsingDataFlow, where
# `program` and `variableNode` are in scope. Counts program nodes
# matching variableNode's referenceableItem, split by regionSide.
set $p = program
set $in = 0
set $out = 0
set $other = 0
while $p != 0
  if $p->referenceableItem == variableNode->referenceableItem && $p->reference->usage != 0
    if $p->regionSide == 2
      set $in = $in + 1
    else
      if $p->regionSide == 1
        set $out = $out + 1
      else
        set $other = $other + 1
      end
    end
  end
  set $p = $p->next
end
printf "linkName : %s\n", variableNode->referenceableItem->linkName
printf "inside   : %d\n", $in
printf "outside  : %d\n", $out
printf "other    : %d\n", $other
