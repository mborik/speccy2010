#ifndef SHELL_SNAPSHOT_H_INCLUDED
#define SHELL_SNAPSHOT_H_INCLUDED

bool LoadSnapshot(const char *fileName, const char *title = "");
void SaveSnapshot(const char *name = 0);
void LoadSnapshotName(bool showScreen = true);
void SaveSnapshotAs();

#endif
