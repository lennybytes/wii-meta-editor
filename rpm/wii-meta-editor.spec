Name:           wii-meta-editor
Version:        0.1.0
Release:        1
Summary:        Wii Homebrew Channel meta.xml editor
License:        GPL-2.0
URL:            https://github.com/lennybytes/wii-meta-editor
Source0:        %{name}-%{version}.tar.gz
BuildArch:      x86_64

BuildRequires:  gcc make pkg-config gtk3-devel libxml2-devel
Requires:       gtk3 libxml2

%description
A lightweight GTK3 editor for the meta.xml file of the Wii Homebrew Channel.
Opens, edits and saves meta.xml as clean, properly formatted UTF-8 XML.

%prep
%setup -q

%build
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%files
%{_bindir}/wii-meta-editor
%{_datadir}/applications/io.homebrew.WiiMetaEditor.desktop
%{_datadir}/icons/hicolor/scalable/apps/io.homebrew.WiiMetaEditor.svg

%changelog
* Mon Sep 28 2026 Lenny <lenny@example.com> - 2.0.0-1
- Initial RPM package
