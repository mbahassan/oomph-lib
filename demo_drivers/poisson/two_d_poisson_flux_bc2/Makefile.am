#Include commands common to every Makefile.am
include $(top_srcdir)/config/makefile_templates/demo_drivers

# Name of executable
check_PROGRAMS= two_d_poisson_flux_bc2

# Sources for executable
two_d_poisson_flux_bc2_SOURCES = two_d_poisson_flux_bc2.cc

# Required libraries:
# $(FLIBS) is included in case the solver involves fortran sources.
two_d_poisson_flux_bc2_LDADD = -L@libdir@ -lpoisson -lgeneric \
                               $(EXTERNAL_LIBS) $(FLIBS)
