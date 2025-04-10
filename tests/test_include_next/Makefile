include ../Makefile.boilerplate

# How c-xrefactory gets include paths:
#     LANG=C gcc -v -x c -o /dev/null /dev/null >out 2>&1

# and include standard defines:
#    gcc -E -dM - >out 2>&1

# And this should be what c-xrefactory actually sees
# gcc -E source.c

# "include_next" is non-standard but means that the preprocessor will include the indicated
# file, but will not re-process the include paths already tried. This is often used to amend
# a standard header file, such as stdint.h. E.g. if gcc preprends its own include directory
# to the standard ones there could be a stdint.h there which then includes "stdint.h" again.
# If this include would be processed in the normal way it would just find the same file again,
# but by using "#include_next" instead the preprocessor will look for "stdint.h" in the rest
# of the include path starting with the one after the one where the first "stdint.h" was found.

# So here we add the subdirectory "next" where there also is a "include.h". In the one in the top
# directory we want to include that one using "#include_next stdint.h".

ARGUMENTS = -create -I path1 -I path2 source.c -xrefactory-II -errors

$(TEST): .c-xrefrc
	$(COMMAND) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)
