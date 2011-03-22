#
# pepper - SCM statistics report generator
# Copyright (C) 2010-2011 Jonas Gehring
#
# Released under the GNU General Public License, version 3.
# Please see the COPYING file in the source distribution for license
# terms and conditions, or see http://www.gnu.org/licenses/.
#

man_MANS = pepper.1
EXTRA_DIST = manpage.txt

manpage.xml: $(srcdir)/manpage.txt 
	$(ASCIIDOC) -b docbook -d manpage -o manpage.xml -a "version=$(VERSION)" -a "email=&lt;\$(PACKAGE_BUGREPORT)&gt;" $(srcdir)/manpage.txt

manpage.html: $(srcdir)/manpage.txt
	$(ASCIIDOC) -d manpage -o manpage.html -a "version=$(VERSION)" -a "email=&lt;\FORENAME.SURENAME@boolsoft.org&gt;" -a toc $(srcdir)/manpage.txt

pepper.1: manpage.xml
	$(XMLTO) man --skip-validation manpage.xml

distclean-local:
	$(RM) -f pepper.1
	$(RM) -f manpage.xml

htmlman: manpage.html
