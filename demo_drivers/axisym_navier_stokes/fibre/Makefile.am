#Include commands common to every Makefile.am
include $(top_srcdir)/config/makefile_templates/demo_drivers


# Programs with self tests
check_PROGRAMS= fibre

# Note: The following only works if the libraries have been installed! 

# Required libraries: Only the "generic" and "navier_stokes" libraries, 
# which are accessible via the general library directory which 
# we specify with -L. $(FLIBS) get included just in case
# we decide to use a solver that involves fortran sources


#Sources
fibre_SOURCES =\
fibre.cc

# Required libraries: 
fibre_LDADD = -L@libdir@ \
              -laxisym_navier_stokes \
	      -lfluid_interface \
              -lgeneric $(EXTERNAL_LIBS) $(FLIBS)

