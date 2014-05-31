Name:       tts
Summary:    Text To Speech client library and daemon
Version:    0.2.36
Release:    1
Group:      libs
License:    Apache License Version 2.0
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(capi-media-audio-io)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(mm-session)
BuildRequires:  pkgconfig(vconf)


BuildRequires:  cmake

%description
Text To Speech client library and daemon.


%package devel
Summary:    Text To Speech header files for TTS development
Group:      libdevel
Requires:   %{name} = %{version}-%{release}

%package setting-devel
Summary:    Text To Speech setting header files for TTS development
Group:      libdevel
Requires:   %{name} = %{version}-%{release}

%package engine-devel
Summary:    Text To Speech engine header files for TTS development
Group:      libdevel
Requires:   %{name} = %{version}-%{release}

%description devel
Text To Speech header files for TTS development.

%description setting-devel
Text To Speech setting header files for TTS development.

%description engine-devel
Text To Speech engine header files for TTS development.


%prep
%setup -q -n %{name}-%{version}


%build
%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
%endif
%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif

%if "%{_repository}" == "wearable"
cmake . -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_PROFILE="wearable"
%else
cmake . -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_PROFILE="mobile"
%endif
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
%make_install

%post 
/sbin/ldconfig

mkdir -p /usr/lib/voice
chsmack -a '_' /usr/lib/voice

mkdir -p /usr/share/voice
chsmack -a '_' /usr/share/voice

mkdir -p /opt/home/app/.voice
chown 5000:5000 /opt/home/app/.voice

mkdir -p /opt/usr/data/voice/tts/1.0
chsmack -a '_' /opt/usr/data/voice/
chsmack -a 'tts-server' /opt/usr/data/voice/tts/
chsmack -a 'tts-server' /opt/usr/data/voice/tts/1.0
chown 5000:5000 /opt/usr/data/voice
chown 5000:5000 /opt/usr/data/voice/tts
chown 5000:5000 /opt/usr/data/voice/tts/1.0

%postun -p /sbin/ldconfig

%files
%manifest tts-server.manifest
%if "%{_repository}" == "wearable"
/etc/smack/accesses2.d/tts-server.rule
%else
/etc/smack/accesses.d/tts-server.rule
%endif
/etc/config/sysinfo-tts.xml
%defattr(-,root,root,-)
%{_libdir}/lib*.so
%{_libdir}/voice/tts/1.0/tts-config.xml
%{_bindir}/tts-daemon*
%{_bindir}/tts-test

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/tts.pc
%{_includedir}/tts.h

%files setting-devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/tts-setting.pc
%{_includedir}/tts_setting.h

%files engine-devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/tts-engine.pc
%{_includedir}/ttsp.h
