
# This directory is a bit unusual as it's not based
# *.txt file, therefore there's no ordinary target for
# the automated makefilery.

all: rotate_gifs.js

# How to create the js file:
rotate_gifs.js:
	./turn_into_rotation_instructions.sh

# Additional files to be included into the documenation
EXTRA_DIST = turn_into_rotation_instructions.sh index.html


# Additional files to clean (there's already a clean-local target in
# the generic makefile included above).
CLEANFILES=rotate_gifs.js

