###########################################################################
##
## File:        modulefiles/Makefile.am
## Versions:    $Id$
## Created:     2002/06/14
##
###########################################################################


AUTOMAKE_OPTIONS=foreign no-installinfo

BASEMODULES= \
	HOME dot module-git module-info modules null use.own notavail

# add any new languages for the lang/ modulefiles here
# and in the lang/.mklang script
LANGUAGES= \
	lang/de lang/en lang/es lang/fi lang/fr lang/gr			\
	lang/it lang/ja lang/no lang/ru lang/us lang/zh

ALLMODULES=${BASEMODULES} ${LANGUAGES} lang/.mklang

EXTRA_SCRIPTS= ${ALLMODULES} version

all:	${EXTRA_SCRIPTS}

${LANGUAGES} : lang/.mklang
	-(cd lang; ./.mklang)

install-data-local : install-basemodules install-version

install-basemodules : $(ALLMODULES)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(datarootdir)/modulefiles
	$(mkinstalldirs) $(DESTDIR)$(datarootdir)/modulefiles/lang
	@list='$(ALLMODULES)'; for p in $$list; do \
	  if test -f $$p \
	  ; then \
	   echo " $(INSTALL_DATA_ENV) $(INSTALL_DATA) $$p $(DESTDIR)$(datarootdir)/modulefiles/$$p"; \
	   $(INSTALL_DATA_ENV) $(INSTALL_DATA) $$p $(DESTDIR)$(datarootdir)/modulefiles/$$p; \
	  else :; fi; \
	done

@NOTVERSIONING@install-version : version
@VERSIONING@install-version-not :
	@echo Not installing version file

@VERSIONING@install-version : version
@NOTVERSIONING@install-version-not :
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(VERSIONPATH)
	@if test x$(WANTS_VERSIONING) = x1 ; then \
	  echo " $(INSTALL_DATA_ENV) $(INSTALL_DATA) version $(DESTDIR)$(VERSIONPATH)/$(VERSION)"; \
	  $(INSTALL_DATA_ENV) $(INSTALL_DATA) version $(DESTDIR)$(VERSIONPATH)/$(VERSION) ; \
	fi

uninstall-local : uninstall-basemodules uninstall-version

uninstall-basemodules :
	@$(NORMAL_UNINSTALL)
	@if test x$(WANTS_VERSIONING) = x1 ; then \
	  list='$(ALLMODULES)'; for p in $$list; do \
	    echo " rm -f $(DESTDIR)$(datarootdir)/modulefiles/$$p"; \
	    rm -f $(DESTDIR)$(datarootdir)/modulefiles/$$p; \
	  done; \
	else \
	  echo "*** BASE MODULES are not uninstalled from $(DESTDIR)$(datarootdir)/modulefiles ***"; \
	  echo "*** Do This Yourself If you really want to ***"; \
	fi

@NOTVERSIONING@uninstall-version :

@VERSIONING@uninstall-version :
@NOTVERSIONING@uninstall-version-not :
	@$(NORMAL_UNINSTALL)
	@if test x$(WANTS_VERSIONING) = x1 ; then \
	    echo " rm -rf $(DESTDIR)$(VERSIONPATH)/$(VERSION)" ; \
	    rm -rf $(DESTDIR)$(VERSIONPATH)/$(VERSION) ; \
	else \
	  echo "*** VERSION modulefile $(DESTDIR)$(VERSIONPATH)/$(VERSION) not uninstalled ***"; \
	  echo "*** Do This Yourself If you really want to ***"; \
	fi

##dot : dot.in ../config.status
##	(cd ..; config.status modulefiles/dot)
##
##module-git : module-git.in ../config.status
##	(cd ..; config.status modulefiles/module-git)
##
##module-info : module-info.in ../config.status
##	(cd ..; config.status modulefiles/module-info)
##
##modules : modules.in ../config.status
##	(cd ..; config.status modulefiles/modules)
##
##null : null.in ../config.status
##	(cd ..; config.status modulefiles/null)
##
##use.own : use.own.in ../config.status
##	(cd ..; config.status modulefiles/use.own)
##
##version : version.in ../config.status
##	(cd ..; config.status modulefiles/version)

check :

clean :
	-rm -f ${BASEMODULES}
	-(cd lang; ./.mklang clean)
