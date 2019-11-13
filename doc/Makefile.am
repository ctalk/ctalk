# $Id: Makefile.am,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $
SUBDIRS = 
info_TEXINFOS=ctpp.texi ctalk.texi tutorial.texi ctalktools.texi
man_MANS=ctpp.1 ctalk.1 ctcc.1 ctdb.1 methods.1 ctxlogo.1 ctalkintro.1 \
	classes.1 inspect.3ctalk methodsource.1 classdoc.1 membervars.1 \
	fnnames.5ctalk templates.5ctalk template.1 searchlib.1 \
	ctalkclasses.1 ctalkregex.1 ctalkmethods.1
dist_pkgdata_DATA=inspect_long_help inspect_brief_help libdoc classlibdoc
EXTRA_DIST=$(man_MANS) \
	$(top_builddir)/doc/ChangeLog.* \
	$(top_builddir)/doc/ctalk.info* \
	$(top_builddir)/doc/ctalk.texi \
	$(top_builddir)/doc/ctpp.info* \
	$(top_builddir)/doc/ctpp.texi \
	$(top_builddir)/doc/Makefile.djgpp \
	$(top_builddir)/doc/manual.css \
	$(top_builddir)/doc/README \
	$(top_builddir)/doc/texinfo.tex \
	$(top_builddir)/doc/style.tex \
	$(top_builddir)/doc/tutorial.info* \
	$(top_builddir)/doc/tutorial.texi \
	$(top_builddir)/doc/ctalktools.texi \
	$(top_builddir)/doc/ctalktools.info \
	$(top_builddir)/doc/XftConfig.sample \
	$(top_builddir)/doc/classes.el \
	$(top_builddir)/doc/methods-brief.el \
	$(top_builddir)/doc/methods-full.el

ctalk-docs.tar.gz: ctalk.texi tutorial.texi
	makeinfo --html --css-include=manual.css ctalk.texi && mv ctalk ctalk-ref && \
	makeinfo --plaintext ctalk.texi -o ctalk.txt && \
	makeinfo ctalk.texi -o ctalk.info && \
	tex ctalk.texi ;\
	texindex ctalk.?? && tex ctalk.texi ;\
	texindex ctalk.?? && tex ctalk.texi ;\
	tar zcvf ctalk-ref-html.tar.gz ctalk-ref ;\
	dvips ctalk.dvi -o ctalk.ps ; \
	ps2pdf13 ctalk.ps ctalk.pdf ; \
	makeinfo --html --css-include=manual.css tutorial.texi && mv tutorial ctalk-tutorial && \
	makeinfo --plaintext tutorial.texi -o tutorial.txt && \
	makeinfo tutorial.texi -o tutorial.info && \
	tex tutorial.texi ;\
	texindex tutorial.?? && tex tutorial.texi ;\
	texindex tutorial.?? && tex tutorial.texi ;\
	dvips tutorial.dvi -o tutorial.ps ; \
	ps2pdf13 tutorial.ps tutorial.pdf ; \
	tar zcvf ctalk-tutorial-html.tar.gz ctalk-tutorial ;\
	cp ../programs/utils.texi . && \
	makeinfo --html --css-include=manual.css utils.texi && \
	makeinfo --plaintext utils.texi -o utils.txt && \
	makeinfo utils.texi -o utils.info && \
	tex utils.texi && \
	texindex utils.?? && tex utils.texi && \
	texindex utils.?? && tex utils.texi && \
	dvips utils.dvi -o utils.ps && \
	ps2pdf13 utils.ps utils.pdf ; \
	tar zcvf ctalk-utils-html.tar.gz utils ;\
	cp ../programs/cgi/web.texi . && \
	makeinfo --html --css-include=manual.css web.texi && \
	makeinfo --plaintext web.texi -o web.txt && \
	makeinfo web.texi -o web.info && \
	tex web.texi && \
	texindex web.?? && tex web.texi && \
	texindex web.?? && tex web.texi && \
	dvips web.dvi -o web.ps && \
	ps2pdf13 web.ps web.pdf ; \
	cp ../programs/win32/win32.texi . && \
	makeinfo --html --css-include=manual.css win32.texi && \
	makeinfo --plaintext win32.texi -o win32.txt && \
	makeinfo win32.texi -o win32.info && \
	tex win32.texi && \
	texindex win32.?? && tex win32.texi && \
	texindex win32.?? && tex win32.texi && \
	dvips win32.dvi -o win32.ps && \
	ps2pdf13 win32.ps win32.pdf ; \
	tar zcvf ctalk-web-html.tar.gz web ;\
	tar zcvf ctalk-win32-html.tar.gz win32 ;\
	tar zcvf ctalk-docs.tar.gz \
		ctalk-ref-html.tar.gz \
		ctalk-tutorial-html.tar.gz \
		ctalk-utils-html.tar.gz \
		ctalk-web-html.tar.gz \
		ctalk-win32-html.tar.gz \
		ctalk.info \
		ctalk.dvi \
		ctalk.txt \
		ctalk.ps \
		ctalk.pdf \
		tutorial.info \
		tutorial.txt \
		tutorial.ps \
		tutorial.pdf \
		tutorial.dvi \
		utils.info \
		utils.txt \
		utils.ps \
		utils.dvi \
		utils.pdf \
		web.info \
		web.txt \
		web.ps \
		web.dvi \
		web.pdf \
		win32.info \
		win32.txt \
		win32.dvi \
		win32.ps \
		win32.pdf

docs-clean:
	rm -rf 	ctalk-ref ctalk-tutorial tutorial utils \
		ctalk.txt tutorial.txt utils.txt \
		*.ps *.pdf *.dvi *.info \
		*.vr *.vrs *.tp *.tps *.pg *pgs *.ky *.kys *.cp *.cps \
		*.fn *.fns *.tmp *.aux *.toc *.log \
		*.tar.gz				

