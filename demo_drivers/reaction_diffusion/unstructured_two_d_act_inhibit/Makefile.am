#Include commands common to every Makefile.am
include $(top_srcdir)/config/makefile_templates/demo_drivers

# Name of executables 
check_PROGRAMS= unstructured_two_d_act_inhibit

# Sources for first executable
unstructured_two_d_act_inhibit_SOURCES = \
 unstructured_two_d_act_inhibit.cc

# Required libraries:
# $(FLIBS) is included in case the solver involves fortran sources.
unstructured_two_d_act_inhibit_LDADD = \
-L@libdir@ -ladvection_diffusion_reaction \
	                        -lgeneric $(EXTERNAL_LIBS) $(FLIBS)
