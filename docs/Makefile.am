#
# pepper - SCM statistics report generator
# Copyright (C) 2010-present Jonas Gehring
#
# Released under the GNU General Public License, version 3.
# Please see the COPYING file in the source distribution for license
# terms and conditions, or see http://www.gnu.org/licenses/.
#

man_MANS = pepper.1
EXTRA_DIST = manpage.txt

manpage.xml: $(srcdir)/manpage.txt 
	$(ASCIIDOC) -b docbook -d manpage -o manpage.xml -a "version=$(VERSION)" -a "email=&lt;\$(PACKAGE_BUGREPORT)&gt;" $(srcdir)/manpage.txt

pepper.1: manpage.xml
	$(XMLTO) man --skip-validation manpage.xml

pepper.1.html: manpage.xml
	$(XMLTO) html-nochunks --skip-validation manpage.xml
	mv manpage.html pepper.1.html

distclean-local:
	$(RM) -f pepper.1
	$(RM) -f manpage.xml

htmlman: manpage.html
