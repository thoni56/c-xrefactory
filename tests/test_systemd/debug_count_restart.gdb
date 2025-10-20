# GDB script to count hits at getActualArguments breakpoint and restart with ignore
# Usage: gdb -x debug_count_restart.gdb --args $(COMMAND)

# Set up a counter variable
set $hit_count = 0

# Set breakpoint at getActualArgument
break yylex.c:1955
info breakpoints

# Define commands for the breakpoint
commands 1
set $hit_count = $hit_count + 1
printf "Breakpoint hit #%d at getActualArguments\n", $hit_count
continue
end

# Start the program
run

# After program finishes, show total count
printf "Total hits: %d\n", $hit_count

# Remove the commands from the breakpoint
commands 1
end

# Set ignore count to the number of hits we counted
ignore 1 $hit_count - 1

# Restart the program (it will now skip the breakpoint that many times)
printf "Restarting program with breakpoint ignored %d times...\n", $hit_count - 1
run