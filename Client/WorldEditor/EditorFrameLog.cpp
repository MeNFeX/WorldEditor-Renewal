#include "StdAfx.h"
#include "../EterLib/EditorFrameLog.h"

#ifdef WORLD_EDITOR

#include <stdarg.h>
#include <share.h>

namespace
{
	struct SBucket
	{
		char name[24];
		double ms;
	};

	const int kBucketMax = 40;
	const long kLogBytes = 1500000;

	bool g_inFrame = false;
	bool g_full = false;
	FILE* g_fp = nullptr;
	double g_frameStart = 0.0;
	double g_lastMark = 0.0;
	double g_secondStart = 0.0;
	double g_sumMs = 0.0;
	double g_worst = 0.0;
	int g_frames = 0;
	int g_slow = 0;
	long g_bytes = 0;
	int g_counts[EDITOR_COUNT_MAX];
	int g_secCounts[EDITOR_COUNT_MAX];
	SBucket g_frameBuckets[kBucketMax];
	int g_frameBucketCount = 0;
	SBucket g_secBuckets[kBucketMax];
	int g_secBucketCount = 0;

	double NowMs()
	{
		static LARGE_INTEGER freq = {};
		if (freq.QuadPart == 0)
			QueryPerformanceFrequency(&freq);
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return (double) now.QuadPart * 1000.0 / (double) freq.QuadPart;
	}

	void OpenLog()
	{
		if (g_fp || g_full)
			return;

		g_fp = _fsopen("D:\\ymir work\\editor_frame_debug.txt", "wb", _SH_DENYWR);
		if (!g_fp)
			g_fp = _fsopen("editor_frame_debug.txt", "wb", _SH_DENYWR);
		if (!g_fp)
			return;

		SYSTEMTIME st;
		GetLocalTime(&st);
		int n = fprintf(g_fp,
			"Jurnal de cadre WorldEditor. Pornit %04d-%02d-%02d %02d:%02d:%02d\r\n"
			"AVG = media pe o secunda. SLOW = un cadru mai lent de 25 ms.\r\n"
			"Misca-te pe harta cateva secunde. Timpii sunt in milisecunde.\r\n",
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		if (n > 0)
			g_bytes += n;
		fflush(g_fp);
	}

	void WriteLine(const char* line)
	{
		OpenLog();
		if (!g_fp || g_full)
			return;

		int n = fprintf(g_fp, "%s\r\n", line);
		if (n > 0)
			g_bytes += n;
		fflush(g_fp);
		if (g_bytes > kLogBytes)
		{
			fprintf(g_fp, "Jurnalul s-a oprit la 1.5 MB.\r\n");
			fflush(g_fp);
			g_full = true;
		}
	}

	void AddBucket(SBucket* buckets, int& count, const char* name, double ms)
	{
		for (int i = 0; i < count; ++i)
		{
			if (strcmp(buckets[i].name, name) == 0)
			{
				buckets[i].ms += ms;
				return;
			}
		}
		if (count >= kBucketMax)
			return;
		strncpy(buckets[count].name, name, sizeof(buckets[count].name) - 1);
		buckets[count].name[sizeof(buckets[count].name) - 1] = 0;
		buckets[count].ms = ms;
		++count;
	}

	void AppendBuckets(char* buf, int& pos, int cap, SBucket* buckets, int count, double divisor)
	{
		int order[kBucketMax];
		for (int i = 0; i < count; ++i)
			order[i] = i;
		for (int i = 0; i < count; ++i)
		{
			for (int j = i + 1; j < count; ++j)
			{
				if (buckets[order[j]].ms > buckets[order[i]].ms)
				{
					int tmp = order[i];
					order[i] = order[j];
					order[j] = tmp;
				}
			}
		}
		for (int i = 0; i < count && pos < cap - 48; ++i)
		{
			double ms = divisor > 0.0 ? buckets[order[i]].ms / divisor : buckets[order[i]].ms;
			if (ms < 0.2)
				continue;
			pos += _snprintf(buf + pos, cap - pos, " | %s %.1f", buckets[order[i]].name, ms);
		}
	}

	void AppendCounts(char* buf, int& pos, int cap, const int* counts, int frames)
	{
		const double d = frames > 0 ? (double) frames : 1.0;
		pos += _snprintf(buf + pos, cap - pos,
			" | tree %d on %d geo %d leafLock %d splat %d base %d full %d",
			(int) (counts[EDITOR_COUNT_TREE] / d),
			(int) (counts[EDITOR_COUNT_TREE_ON] / d),
			(int) (counts[EDITOR_COUNT_GEO] / d),
			(int) (counts[EDITOR_COUNT_LEAF_LOCK] / d),
			(int) (counts[EDITOR_COUNT_SPLAT] / d),
			(int) (counts[EDITOR_COUNT_PATCH_BASE] / d),
			(int) (counts[EDITOR_COUNT_PATCH_FULL] / d));
	}
}

void EditorFrameBegin()
{
	if (g_inFrame)
		EditorFrameEnd();

	OpenLog();
	const double now = NowMs();
	if (g_secondStart == 0.0)
		g_secondStart = now;
	g_inFrame = true;
	g_frameStart = now;
	g_lastMark = now;
	g_frameBucketCount = 0;
	memset(g_counts, 0, sizeof(g_counts));
}

void EditorFrameMark(const char* name)
{
	if (!g_inFrame || !name)
		return;

	const double now = NowMs();
	AddBucket(g_frameBuckets, g_frameBucketCount, name, now - g_lastMark);
	g_lastMark = now;
}

void EditorFrameCount(int id, int n)
{
	if (!g_inFrame || id < 0 || id >= EDITOR_COUNT_MAX || n == 0)
		return;
	g_counts[id] += n;
}

void EditorFrameEnd()
{
	if (!g_inFrame)
		return;

	const double now = NowMs();
	const double tail = now - g_lastMark;
	if (tail >= 0.05)
		AddBucket(g_frameBuckets, g_frameBucketCount, "rest", tail);

	const double total = now - g_frameStart;
	g_inFrame = false;
	++g_frames;
	g_sumMs += total;
	if (total > g_worst)
		g_worst = total;

	for (int i = 0; i < g_frameBucketCount; ++i)
		AddBucket(g_secBuckets, g_secBucketCount, g_frameBuckets[i].name, g_frameBuckets[i].ms);
	for (int i = 0; i < EDITOR_COUNT_MAX; ++i)
		g_secCounts[i] += g_counts[i];

	if (total >= 25.0)
	{
		++g_slow;
		char buf[1600];
		int pos = _snprintf(buf, sizeof(buf), "SLOW %.1f ms", total);
		AppendBuckets(buf, pos, (int) sizeof(buf) - 160, g_frameBuckets, g_frameBucketCount, 1.0);
		AppendCounts(buf, pos, (int) sizeof(buf), g_counts, 1);
		WriteLine(buf);
	}

	if (now - g_secondStart >= 1000.0 && g_frames > 0)
	{
		char buf[1600];
		int pos = _snprintf(buf, sizeof(buf), "AVG frames %d avg %.1f ms worst %.1f slow %d",
			g_frames, g_sumMs / (double) g_frames, g_worst, g_slow);
		AppendBuckets(buf, pos, (int) sizeof(buf) - 180, g_secBuckets, g_secBucketCount, (double) g_frames);
		AppendCounts(buf, pos, (int) sizeof(buf), g_secCounts, g_frames);
		WriteLine(buf);

		g_frames = 0;
		g_slow = 0;
		g_sumMs = 0.0;
		g_worst = 0.0;
		g_secondStart = now;
		g_secBucketCount = 0;
		memset(g_secCounts, 0, sizeof(g_secCounts));
	}
}

void EditorFrameNotef(const char* fmt, ...)
{
	char msg[700];
	va_list args;
	va_start(args, fmt);
	_vsnprintf(msg, sizeof(msg) - 1, fmt, args);
	msg[sizeof(msg) - 1] = 0;
	va_end(args);

	char line[760];
	_snprintf(line, sizeof(line), "NOTE %.0f %s", NowMs(), msg);
	line[sizeof(line) - 1] = 0;
	WriteLine(line);
}

#endif
