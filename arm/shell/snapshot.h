#ifndef SHELL_SNAPSHOT_H_INCLUDED
#define SHELL_SNAPSHOT_H_INCLUDED

bool LoadSnapshot(const char *fileName, const char *title = "");
void SaveSnapshot(const char *name);
void SaveSnapshotAs();

const char *UpdateSnaName();

#endif
