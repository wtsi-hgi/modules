###########################################################################
##
## File:        init/Makefile.am
## Versions:    $Id: Makefile.am,v 1.1 2002/06/14 21:57:03 rkowen Exp $
## Created:     2002/06/14
##
###########################################################################


AUTOMAKE_OPTIONS=foreign no-installinfo

EXTRA_SCRIPTS= \
	bash csh ksh perl python sh tcsh zsh ksh .modulespath

INIT_FILES=${EXTRA_SCRIPTS}

MAKEDIRS= $(initdir)

# all:    $(INIT_FILES)
#
# only make the init files when an install is done.
# why? because we want to get the latest $DESTDIR & $bindir
#
all:
	rm -f $(INIT_FILES)

install-data-local : install-inits

install-inits : $(INIT_FILES)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(prefix)/init
	@list='$(INIT_FILES)'; for p in $$list; do \
	  if test -f $$p \
	  ; then \
	   echo " $(INSTALL_PROGRAM_ENV) $(INSTALL_PROGRAM) $$p $(DESTDIR)$(prefix)/init/$$p"; \
	   $(INSTALL_PROGRAM_ENV) $(INSTALL_PROGRAM) $$p $(DESTDIR)$(prefix)/init/$$p; \
	  else :; fi; \
	done
	rm -f $(INIT_FILES)

uninstall-local : uninstall-inits

uninstall-inits :
	@$(NORMAL_UNINSTALL)
	@list='$(INIT_FILES)'; for p in $$list; do \
	  echo " rm -f $(DESTDIR)$(prefix)/init/$$p"; \
	  rm -f $(DESTDIR)$(prefix)/init/$$p; \
	done

#
# we need to filter the *.inp files to produce the init scripts
#   since we don't know the value of @prefix@ until Make time
#   therefore, we can not rely on configure to give us correct values.
# Cobbled together filter to substitute the configure macros
#   which are expanded and passed on the filter commandline.
#

#.modulespath : .modulespath.in
#	${SHELL} filter "@prefix@" "@bindir@" "@BASEPREFIX@" \
#		"@VERSION@" "@VERSIONING@" "@NOTVERSIONING@" \
#		< .modulespath.in > .modulespath

#.modulespath : .modulespath.in ../config.status
#	(cd ..; config.status init/.modulespath)

bash : bash.in ../config.status
	${SHELL} ./filter "@prefix@" "@bindir@" "@BASEPREFIX@" \
		"@VERSION@" "@VERSIONING@" "@NOTVERSIONING@" \
		< bash.in > bash

csh : csh.in ../config.status
	${SHELL} filter "@prefix@" "@bindir@" "@BASEPREFIX@" \
		"@VERSION@" "@VERSIONING@" "@NOTVERSIONING@" \
		< csh.in > csh

ksh : ksh.in ../config.status
	${SHELL} filter "@prefix@" "@bindir@" "@BASEPREFIX@" \
		"@VERSION@" "@VERSIONING@" "@NOTVERSIONING@" \
		< ksh.in > ksh

perl : perl.in ../config.status
	${SHELL} filter "@prefix@" "@bindir@" "@BASEPREFIX@" \
		"@VERSION@" "@VERSIONING@" "@NOTVERSIONING@" \
		< perl.in > perl

python : python.in ../config.status
	${SHELL} filter "@prefix@" "@bindir@" "@BASEPREFIX@" \
		"@VERSION@" "@VERSIONING@" "@NOTVERSIONING@" \
		< python.in > python

sh : sh.in ../config.status
	${SHELL} filter "@prefix@" "@bindir@" "@BASEPREFIX@" \
		"@VERSION@" "@VERSIONING@" "@NOTVERSIONING@" \
		< sh.in > sh

zsh : zsh.in ../config.status
	${SHELL} filter "@prefix@" "@bindir@" "@BASEPREFIX@" \
		"@VERSION@" "@VERSIONING@" "@NOTVERSIONING@" \
		< zsh.in > zsh

tcsh : csh
	cp $< $@

check :

clean :
	rm -f ${INIT_FILES}