// DocBoss's identity: the names, folders and release stream the pulled MDBoss
// units run under (see MDBoss's app/AppIdentity.h for what each one steers).
//
// Built here, wx-free, so a test can check that nothing in it still names
// MD Boss -- a leftover would make DocBoss quietly read MD Boss's profile,
// share its single-instance slot, or update itself from MD Boss's releases.

#ifndef DOCBOSS_APP_IDENTITY_H
#define DOCBOSS_APP_IDENTITY_H

#include <string>

#include "mdboss/AppIdentity.h"

namespace docboss {

// `profile_dir` empty: the per-user default, %APPDATA%\DocBoss for data and
// %TEMP%\DocBoss for the preview's staging.  Otherwise both live under it
// (data in the folder itself, staging in <profile_dir>\staging) -- which is
// what --profile is for: running a copy against a scratch profile without
// touching the real one, or anything under AppData.
mdboss::AppIdentity make_identity(const std::string& profile_dir);

}  // namespace docboss

#endif  // DOCBOSS_APP_IDENTITY_H
