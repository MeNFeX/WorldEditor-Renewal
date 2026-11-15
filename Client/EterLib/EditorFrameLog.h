#pragma once

#ifdef WORLD_EDITOR

enum EEditorFrameCount
{
	EDITOR_COUNT_TREE = 0,
	EDITOR_COUNT_TREE_ON,
	EDITOR_COUNT_GEO,
	EDITOR_COUNT_LEAF_LOCK,
	EDITOR_COUNT_SPLAT,
	EDITOR_COUNT_PATCH_BASE,
	EDITOR_COUNT_PATCH_FULL,
	EDITOR_COUNT_MAX
};

void EditorFrameBegin();
void EditorFrameMark(const char* name);
void EditorFrameEnd();
void EditorFrameCount(int id, int n = 1);
void EditorFrameNotef(const char* fmt, ...);

#else

inline void EditorFrameBegin() {}
inline void EditorFrameMark(const char*) {}
inline void EditorFrameEnd() {}
inline void EditorFrameCount(int, int = 1) {}
inline void EditorFrameNotef(const char*, ...) {}

#endif
