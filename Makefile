#---------------------------------------------------------------------------------
# sdbench build dispatcher.
#
#   make                 build both Switch and Switch 2 binaries
#   make nx              build the Switch (libnx) binary only
#   make ounce           build the Switch 2 (libounce) binary only
#   make clean           clean both platforms
#
# Use make -f Makefile.nx / -f Makefile.ounce to invoke a single platform
# Makefile directly. Both share the rules in Makefile.common.
#---------------------------------------------------------------------------------
WRAPPERS := $(dir $(abspath $(firstword $(MAKEFILE_LIST))))

.PHONY: all nx ounce clean clean-nx clean-ounce

all: nx ounce

nx:
	@$(MAKE) --no-print-directory -f $(WRAPPERS)Makefile.nx

ounce:
	@$(MAKE) --no-print-directory -f $(WRAPPERS)Makefile.ounce

clean: clean-nx clean-ounce

clean-nx:
	@$(MAKE) --no-print-directory -f $(WRAPPERS)Makefile.nx clean

clean-ounce:
	@$(MAKE) --no-print-directory -f $(WRAPPERS)Makefile.ounce clean
