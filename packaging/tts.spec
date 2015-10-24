Name:       tts
Summary:    Text To Speech client library and daemon
Version:    0.2.43
Release:    1
Group:      Graphics & UI Framework/Voice Framework
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(capi-media-audio-io)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(libprivilege-control)
BuildRequires:  pkgconfig(libxml-2.0)
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
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"

export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"


cmake . -DCMAKE_INSTALL_PREFIX=/usr
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
install LICENSE %{buildroot}/usr/share/license/%{name}

%make_install

%post 
/sbin/ldconfig

mkdir -p /usr/lib/voice
chsmack -a '_' /usr/lib/voice

mkdir -p /usr/share/voice/test
chsmack -a '_' /usr/share/voice
chsmack -a '_' /usr/share/voice/test

mkdir -p /usr/share/voice/tts
chsmack -a '_' /usr/share/voice/tts

mkdir -p /opt/home/app/.voice
chown 5000:5000 /opt/home/app/.voice

mkdir -p /opt/usr/data/voice/tts/1.0/engine-info

chsmack -a '_' /opt/usr/data/voice/
chsmack -a 'tts-server' /opt/usr/data/voice/tts/
chsmack -a 'tts-server' /opt/usr/data/voice/tts/1.0
chsmack -a 'tts-server' /opt/usr/data/voice/tts/1.0/engine-info

chown 5000:5000 /opt/usr/data/voice
chown 5000:5000 /opt/usr/data/voice/tts
chown 5000:5000 /opt/usr/data/voice/tts/1.0
chown 5000:5000 /opt/usr/data/voice/tts/1.0/engine-info

chsmack -a '_' /usr/share/dbus-1/system-services/org.tizen.voice.ttsserver.service
chsmack -a '_' /usr/share/dbus-1/system-services/org.tizen.voice.ttsnotiserver.service
chsmack -a '_' /usr/share/dbus-1/system-services/org.tizen.voice.ttssrserver.service

chsmack -a '_' /usr/bin/tts-daemon
chsmack -a '_' /usr/bin/tts-daemon-noti
chsmack -a '_' /usr/bin/tts-daemon-sr

%postun -p /sbin/ldconfig

%files
%manifest tts-server.manifest
/etc/smack/accesses.d/tts-server.rule
%defattr(-,root,root,-)
%{_libdir}/lib*.so
/usr/share/voice/tts/tts-config.xml
%{_bindir}/tts-daemon*
/usr/share/dbus-1/system-services/*
/usr/share/voice/test/tts-test
/usr/share/license/%{name}

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
