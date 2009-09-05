MAINTAINERCLEANFILES = Makefile.in
filesdir = /usr/local/bin
files_DATA = enlightenment_start.sh ecomorph.sh


install-filesDATA:
	test -z $(scriptdir) || /bin/mkdir -p $(filesdir)
	install -c -m 755 'enlightenment_start.sh' '$(filesdir)/enlightenment_start.sh'
	install -c -m 755 'ecomorph.sh' '$(filesdir)/ecomorph.sh'
