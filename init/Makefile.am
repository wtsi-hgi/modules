###########################################################################
##
## File:        init/Makefile.am
## Versions:    $Id$
## Created:     2002/06/14
##
###########################################################################


AUTOMAKE_OPTIONS=foreign no-installinfo

EXTRA_SCRIPTS=							\
	sh ksh zsh bash bash_completion				\
	csh tcsh tcsh_completion				\
	perl.pm python.py ruby.rb cmake				\
	.modulespath

INIT_FILES=${EXTRA_SCRIPTS}

# all:    $(INIT_FILES)
#
# only make the init files when an install is done.
# why? because we want to get the latest $DESTDIR & $bindir
#
all:
	rm -f $(INIT_FILES)

install-exec-local : install-inits

install-inits : $(INIT_FILES)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(INITPATH)
	@list='$(INIT_FILES)'; for p in $$list; do \
	  if test -f $$p \
	  ; then \
	   echo " $(INSTALL_PROGRAM_ENV) $(INSTALL_DATA) $$p $(DESTDIR)$(INITPATH)/$$p"; \
	   $(INSTALL_PROGRAM_ENV) $(INSTALL_DATA) $$p $(DESTDIR)$(INITPATH)/$$p; \
	  else :; fi; \
	done
	rm -f $(INIT_FILES)

uninstall-local : uninstall-inits

uninstall-inits :
	@$(NORMAL_UNINSTALL)
	@list='$(INIT_FILES)'; for p in $$list; do \
	  echo " rm -f $(DESTDIR)$(INITPATH)/$$p"; \
	  rm -f $(DESTDIR)$(INITPATH)/$$p; \
	done

# We need to filter the *.in files to produce the init scripts
# since we don't know the value of '@prefix@' until Make time.
# (We can not rely on configure to give us correct values.)
% : %.in $(top_builddir)/config.status
	sed -e "/@$(if $(subst 0,,$(WANTS_VERSIONING)),NOT,)VERSIONING\@/d; \
	s,@$(if $(subst 0,,$(WANTS_VERSIONING)),,NOT)VERSIONING\@,,g; \
	s,@prefix\@,${prefix},g; \
	s,@datarootdir\@,${datarootdir},g; \
	s,@bindir\@,${bindir},g; \
	s,@VERSION\@,@VERSION@,g; \
	s,@INITPATH\@,@INITPATH@,g; \
	s,@BASEPREFIX\@,@BASEPREFIX@,g;" < $< > $@

tcsh : csh
	cp csh $@

check :

clean :
	rm -f ${INIT_FILES}
