%define name	phantomjs
%define info    version and release are automaticaly change on building
%define prefix	/usr

Summary:	a headless WebKit with JavaScript API
Name:		%{name}
Version:	%{version}
License:	BSD
Release:	%{release}
Packager:	Matthew Barr <mbarr@snap-interactive.com>
Group:		Utilities/Misc
Source:		%{name}-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Requires:       libpng libjpeg-turbo zlib freetype fontconfig bison flex gperf

%description
PhantomJS is a headless WebKit with JavaScript API. It has fast and native
support for various web standards: DOM handling, CSS selector, JSON,
Canvas, and SVG. PhantomJS is created by Ariya Hidayat.

%prep
%setup -q

%install
%__mkdir_p -p $RPM_BUILD_ROOT%{prefix}/bin
%__mkdir_p -p $RPM_BUILD_ROOT%{prefix}/share/%{name}/examples
%__cp bin/%{name} $RPM_BUILD_ROOT%{prefix}/bin/%{name}
%__cp examples/* $RPM_BUILD_ROOT%{prefix}/share/%{name}/examples/
%__cp ChangeLog $RPM_BUILD_ROOT%{prefix}/share/%{name}/
%__cp LICENSE.BSD $RPM_BUILD_ROOT%{prefix}/share/%{name}/
%__cp README.md $RPM_BUILD_ROOT%{prefix}/share/%{name}/

%files
%defattr(0444,root,root)
%attr(0555,root,root)%{prefix}/bin/%{name}
%{prefix}/share/%{name}/ChangeLog
%{prefix}/share/%{name}/LICENSE.BSD
%{prefix}/share/%{name}/README.md
%{prefix}/share/%{name}/examples/*.js

%changelog
* Thu Feb 9 2016 Jef Leponot <jefleponot@laposte.net>
- remove coffee files and change build structure

* Fri Apr 18 2014 Eric Heydenberk <heydenberk@gmail.com>
- add missing filenames for examples to files section

* Tue Apr 30 2013 Eric Heydenberk <heydenberk@gmail.com>
- add missing filenames for examples to files section

* Wed Apr 24 2013 Robin Helgelin <lobbin@gmail.com>
- updated to version 1.9

* Thu Jan 24 2013 Matthew Barr <mbarr@snap-interactive.com>
- updated to version 1.8

* Thu Nov 15 2012 Jan Schaumann <jschauma@etsy.com>
- first rpm version
