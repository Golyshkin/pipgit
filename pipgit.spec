# Spec for OBS system. Checked on build.opensuse.org OBS platform
Name:           pipgit
Version:        0.8.7
Release:        1
License:        GPL v2
Summary:        PIPGIT Tool
Url:            http://www.teleca.com
Group:          Applications/Development
Source0:        %{name}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
BuildRequires:  libqt4-devel gcc-c++
Requires:       git >= 1.7.1

%description
# PIPGIT tool for inspect/BR GIT chnages

%prep
%setup -q -n %{name}

%build
qmake
make

%install
make install INSTALL_ROOT=%{buildroot}

%files
%defattr(-,root,root,-)
%{_bindir}/pipgit

%changelog
* Wed May 04 2012 Alexander Golyshkin 0.8.7
* Tue Apr 17 2012 Alexander Golyshkin 0.8.5
* Sun Mar 31 2012 Alexander Golyshkin 0.8.1
* Sat Mar 10 2012 Alexander Golyshkin 0.7.1
* Mon Dec 19 2011 Alexander Golyshkin 0.6.8
- Fixed QtProcess error which skipped part of output result
* Mon Dec 12 2011 Alexander Golyshkin 0.6.7
- Fixed BR screen
