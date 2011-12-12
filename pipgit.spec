# Spec for OBS system. Checked on build.opensuse.org OBS platform
Name:           pipgit
Version:        0.6.7
Release:        3
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
* Alexander Golyshkin <Alexander.Golyshkin@teleca.com> v.0.6.7
- Initial release v.0.4