MAINTAINERCLEANFILES = Makefile.in
filesdir = $(DESTDIR)$(prefix)/bin
files_DATA = enlightenment_start.sh ecomp.sh


install-filesDATA:
	test -z $(filesdir) || /bin/mkdir -p $(filesdir)
	install -c -m 755 'enlightenment_start.sh' '$(filesdir)/enlightenment_start.sh'
	install -c -m 755 'ecomp.sh' '$(filesdir)/ecomp.sh'
