// DocBoss's version, in one of the places release.ps1 keeps in lockstep:
// this file, CMakeLists.txt's project(VERSION), app/DocBoss.rc (string and
// numeric forms) and installer.iss.  tests/test_version.cpp fails the build
// if they drift.

#ifndef DOCBOSS_APP_VERSION_H
#define DOCBOSS_APP_VERSION_H

namespace docboss {

inline constexpr const char* kAppName = "DocBoss";
inline constexpr const char* kAppVersion = "1.0.1";
inline constexpr const char* kAttribution = "Bungee Studios 2026  B.Graham";

}  // namespace docboss

#endif  // DOCBOSS_APP_VERSION_H
