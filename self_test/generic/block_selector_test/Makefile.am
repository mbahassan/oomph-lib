#Include commands common to every Makefile.am
include $(top_srcdir)/config/makefile_templates/demo_drivers

# DO NOT NEED TO CHECK FOR MPI BECAUSE IF WE DO NOT HAVE MPI WE DO NOT
# DESCEND INTO THIS DIRECTORY

# Name of executable
check_PROGRAMS= \
block_selector_test

#----------------------------------------------------------------------

# Sources for executable
block_selector_test_SOURCES = block_selector_test.cc

# Required libraries:
# $(FLIBS) is included in case the solver involves fortran sources.
block_selector_test_LDADD = \
                -L@libdir@ -lgeneric $(EXTERNAL_LIBS) $(FLIBS)

#----------------------------------------------------------------------

# Include path for library headers: All library headers live in
# the include directory which we specify with -I
AM_CPPFLAGS +=   -I@includedir@  


EXTRA_DIST+=compare_block_selector_nonnull_replacement_block_pt.sh
