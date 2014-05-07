Name:       libsocialcache
Summary:    A library that manages data from social networks
Version:    0.0.24
Release:    1
Group:      Applications/Multimedia
License:    LGPLv2.1
URL:        https://github.com/nemomobile/libsocialcache
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Sql)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Test)
BuildRequires:  qt5-qttools
BuildRequires:  qt5-qttools-linguist
BuildRequires:  pkgconfig(buteosyncfw5)
BuildRequires:  pkgconfig(libsailfishkeyprovider)

Requires:  qt5-plugin-sqldriver-sqlite

%description
libsocialcache is a  library that manages data from social
networks. It also provides higher level models with a QML
plugin.

%package qml-plugin-ts-devel
Summary:   Translation source for libsocialcache
License:   LGPLv2.1
Group:     Applications/Multimedia

%description qml-plugin-ts-devel
Translation source for socialcache qml plugin

%package devel
Summary:   Development files for libsocialcache
License:   LGPLv2.1
Group:     Development/Libraries
Requires:  libsocialcache = %{version}

%description devel
This package contains development files for libsocialcache.

%package qml-plugin
Summary:   QML plugin for libsocialcache
License:   LGPLv2.1
Group:     Applications/Multimedia

%description qml-plugin
This package contains the qml plugin for socialcache

%package tests
Summary:    Unit tests for libsocialcache
Group:      System/Libraries
BuildRequires:  pkgconfig(Qt5Test)
Requires:   %{name} = %{version}-%{release}

%description tests
This package contains unit tests for the libsocialcache library.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5
make %{_smp_mflags}

%install
rm -rf %{buildroot}
%qmake5_install

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libsocialcache.so.*

%files qml-plugin-ts-devel
%defattr(-,root,root,-)
%{_datadir}/translations/source/socialcache.ts

%files devel
%defattr(-,root,root,-)
%{_includedir}/socialcache/*.h
%{_libdir}/libsocialcache.so
%{_libdir}/pkgconfig/socialcache.pc

%files qml-plugin
%{_libdir}/qt5/qml/org/nemomobile/socialcache/qmldir
%{_libdir}/qt5/qml/org/nemomobile/socialcache/libsocialcacheqml.so
%{_datadir}/translations/socialcache_eng_en.qm

%files tests
%defattr(-,root,root,-)
/opt/tests/libsocialcache/*
