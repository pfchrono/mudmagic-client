Name: mudmagic
Version: 1.9
Release: 1%{?dist}
Summary: Mud client to connect to online text games
License: GPL
Group: Applications/Internet
URL: http://www.mudmagic.com/mud-client/
Source: http://www.mudmagic.com/mud-client/downloads/mudmagic-%{version}.tar.gz
Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Buildrequires: libglade2-devel, pcre-devel, sqlite-devel >= 2.8
Buildrequires: python-devel >= 2.3
Buildrequires: desktop-file-utils

# autoheader re-run
Buildrequires: autoconf
# dangling symlinks into automake-1.7 root
Buildrequires: automake16

Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
Gtk program for connecting to online text games ( Muds ). This is a 
cross platform Linux, Windows, and Mac OS X, Open Source mud client.

%package devel
Summary : Development files for MudMagic
Group: Development/Libraries
Requires: %{name} = %{version}-%{release} pkgconfig

%description devel
Development files for MudMagic.

%prep
%setup -q

%build
%configure --disable-static
make %{?_smp_mflags}


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

desktop-file-install --vendor fedora \
 --add-category X-Fedora \
 --dir %{buildroot}%{_datadir}/applications \
 --delete-original \
 %{buildroot}%{_datadir}/applications/mudmagic.desktop

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%doc COPYING AUTHORS ChangeLog

%{_bindir}/mudmagic-bin
%{_bindir}/mudmagic
%{_libdir}/mudmagic
%{_libdir}/libmudmagic.so.0
%{_libdir}/libmudmagic.so.0.1.9

%dir %{_datadir}/mudmagic
%doc %{_datadir}/mudmagic/doc
%{_datadir}/mudmagic/interface

%{_datadir}/applications/*.desktop
%{_datadir}/pixmaps/*.png
%{_mandir}/man1/*

%files devel
%defattr(-, root, root)
%doc COPYING AUTHORS ChangeLog
%{_libdir}/libmudmagic.so
%{_libdir}/libmudmagic.la
%{_libdir}/pkgconfig/mudmagic.pc

%changelog
* Sun Sep 3 2006 Kyndig <kyndig@mudmagic.com> - 1.9-1
- Replace Release: fdr, with: Release: 1%{?dist}
- Remove tabs and extra spaces from mudmagic.spec.in 
- Change description to simpler details
- Create devel package for so files
- placed mudmagic.pc in devel package

* Thu Jan 26 2006 Paul Howarth <paul@city-fan.org> - 1.8-2
- Fix non-root build
- Remove standard library rpath
- Don't build static library

* Wed Jan 25 2006 Calvin Ellis <kyndig[AT]mudmagic.com> - 1.8-1
- updates for Fedora requirements:
  https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=176200

* Tue Oct 26 2004 Michael Schwendt <mschwendt[AT]users.sf.net> - 1.4-2
- Use "make install", exclude unwanted files, and
  install missing pixmaps manually.

* Mon Oct 25 2004 Michael Schwendt <mschwendt[AT]users.sf.net> - 1.4-1
- Major cleanup/rewrite of upstream spec file
  to make this thing build cleanly on Fedora Core.
