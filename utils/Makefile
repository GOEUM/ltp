################################################################################
#
# Copyright (c) International Business Machines  Corp., 2008 ##
#
# This program is free software;  you can redistribute it and#or modify ##
# it under the terms of the GNU General Public License as published by ##
# the Free Software Foundation; either version 2 of the License, or ##
# (at your option) any later version. ##
#
# This program is distributed in the hope that it will be useful, but ##
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY ##
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   ##
# for more details. ##
#
# You should have received a copy of the GNU General Public License ##
# along with this program;  if not, write to the Free Software ##
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA ##
#
################################################################################

top_srcdir		?= ..

include $(top_srcdir)/include/mk/env_pre.mk

MAKE_TARGETS         += ffsb

FFSBDIR			:= ffsb-6.0-rc2
FILTER_OUT_DIRS		:= $(FFSBDIR)
FFSB			:= $(FFSBDIR)/ffsb

$(FFSB): $(abs_srcdir)/$(FFSBDIR)
	mkdir -p $(FFSBDIR)
	cd $(FFSBDIR) && $(MAKE) top_srcdir="$^" all

ffsb: $(FFSB)
	cp $(FFSB) ffsb

trunk-all: $(FFSB)

trunk-clean:: | ffsb-clean

ffsb-clean:: $(abs_srcdir)/$(FFSBDIR)
	cd $(FFSBDIR); $(MAKE) top_srcdir="$^" clean

include $(top_srcdir)/include/mk/generic_trunk_target.mk
