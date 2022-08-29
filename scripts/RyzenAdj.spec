Name: RyzenAdj
Version: 0.11.1
Release: 1%{?dist}
Summary: Adjust power management settings for Ryzen APUs

License: LGPL-3.0 license 
URL: https://github.com/FlyGoat/RyzenAdj

Source0: https://github.com/FlyGoat/RyzenAdj/archive/refs/tags/v%{version}.tar.gz

BuildArch: x86_64

BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: cmake
BuildRequires: pciutils-devel

%description
Adjust power management settings for Ryzen APUs

%prep
%autosetup

%build
%cmake
%cmake_build

%install
mkdir -p %{buildroot}%{_bindir}
mv %{_builddir}/%{name}-%{version}/%__cmake_builddir/ryzenadj %{buildroot}%{_bindir}/ryzenadj

%files
%{_bindir}/ryzenadj
%license LICENSE
%doc README.md
