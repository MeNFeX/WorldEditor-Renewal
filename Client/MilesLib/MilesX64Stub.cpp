#include "StdAfx.h"

// Retired. The x64 editor links Miles 9.3 (mss64.lib). This file stays in
// the project but is excluded from every build, so these stubs cannot
// satisfy the linker ahead of mss64.dll.

#undef AIL_startup

int __cdecl MSS_auto_cleanup(void)
{
	return 0;
}

S32 WINAPI AIL_startup(void)
{
	return 0;
}

void WINAPI AIL_shutdown(void)
{
}

char FAR* WINAPI AIL_last_error(void)
{
	return const_cast<char*>("Miles is not available in the x64 editor");
}

char FAR* WINAPI AIL_set_redist_directory(char const FAR* dir)
{
	return const_cast<char*>(dir);
}

void WINAPI AIL_set_file_callbacks(AIL_file_open_callback, AIL_file_close_callback, AIL_file_seek_callback, AIL_file_read_callback)
{
}

S32 WINAPI AIL_WAV_info(void const FAR*, AILSOUNDINFO FAR*)
{
	return 0;
}

S32 WINAPI AIL_file_type(void const FAR*, U32)
{
	return 0;
}

void FAR* WINAPI AIL_file_read(char const FAR*, void FAR*)
{
	return 0;
}

void WINAPI AIL_mem_free_lock(void FAR*)
{
}

S32 WINAPI AIL_decompress_ADPCM(AILSOUNDINFO const FAR*, void FAR* FAR*, U32 FAR*)
{
	return 0;
}

S32 WINAPI AIL_decompress_ASI(void const FAR*, U32, char const FAR*, void FAR* FAR*, U32 FAR*, AILLENGTHYCB)
{
	return 0;
}

HDIGDRIVER WINAPI AIL_open_digital_driver(U32, S32, S32, U32)
{
	return 0;
}

void WINAPI AIL_close_digital_driver(HDIGDRIVER)
{
}

HSAMPLE WINAPI AIL_allocate_sample_handle(HDIGDRIVER)
{
	return 0;
}

void WINAPI AIL_release_sample_handle(HSAMPLE)
{
}

void WINAPI AIL_init_sample(HSAMPLE)
{
}

S32 WINAPI AIL_set_sample_file(HSAMPLE, void const FAR*, S32)
{
	return 0;
}

void WINAPI AIL_set_sample_loop_count(HSAMPLE, S32)
{
}

void WINAPI AIL_start_sample(HSAMPLE)
{
}

void WINAPI AIL_stop_sample(HSAMPLE)
{
}

void WINAPI AIL_resume_sample(HSAMPLE)
{
}

void WINAPI AIL_end_sample(HSAMPLE)
{
}

U32 WINAPI AIL_sample_status(HSAMPLE)
{
	return SMP_DONE;
}

void WINAPI AIL_sample_volume_pan(HSAMPLE, F32 FAR* volume, F32 FAR* pan)
{
	if (volume)
		*volume = 0.0f;
	if (pan)
		*pan = 0.5f;
}

void WINAPI AIL_set_sample_volume_pan(HSAMPLE, F32, F32)
{
}

S32 WINAPI AIL_enumerate_3D_providers(HPROENUM FAR*, HPROVIDER FAR*, C8 FAR* FAR*)
{
	return 0;
}

M3DRESULT WINAPI AIL_open_3D_provider(HPROVIDER)
{
	return 0;
}

void WINAPI AIL_close_3D_provider(HPROVIDER)
{
}

H3DPOBJECT WINAPI AIL_open_3D_listener(HPROVIDER)
{
	return 0;
}

void WINAPI AIL_close_3D_listener(H3DPOBJECT)
{
}

H3DSAMPLE WINAPI AIL_allocate_3D_sample_handle(HPROVIDER)
{
	return 0;
}

void WINAPI AIL_release_3D_sample_handle(H3DSAMPLE)
{
}

S32 WINAPI AIL_set_3D_sample_file(H3DSAMPLE, void const FAR*)
{
	return 0;
}

void WINAPI AIL_set_3D_sample_loop_count(H3DSAMPLE, U32)
{
}

void WINAPI AIL_set_3D_sample_volume(H3DSAMPLE, F32)
{
}

void WINAPI AIL_start_3D_sample(H3DSAMPLE)
{
}

void WINAPI AIL_stop_3D_sample(H3DSAMPLE)
{
}

void WINAPI AIL_resume_3D_sample(H3DSAMPLE)
{
}

void WINAPI AIL_end_3D_sample(H3DSAMPLE)
{
}

U32 WINAPI AIL_3D_sample_status(H3DSAMPLE)
{
	return SMP_DONE;
}

F32 WINAPI AIL_3D_sample_volume(H3DSAMPLE)
{
	return 0.0f;
}

void WINAPI AIL_set_3D_position(H3DPOBJECT, F32, F32, F32)
{
}

void WINAPI AIL_set_3D_orientation(H3DPOBJECT, F32, F32, F32, F32, F32, F32)
{
}

void WINAPI AIL_set_3D_velocity(H3DPOBJECT, F32, F32, F32, F32)
{
}

void WINAPI AIL_auto_update_3D_position(H3DPOBJECT, S32)
{
}

void WINAPI AIL_update_3D_position(H3DPOBJECT, F32)
{
}

HSTREAM WINAPI AIL_open_stream(HDIGDRIVER, char const FAR*, S32)
{
	return 0;
}

void WINAPI AIL_close_stream(HSTREAM)
{
}

S32 WINAPI AIL_stream_status(HSTREAM)
{
	return -1;
}

void WINAPI AIL_start_stream(HSTREAM)
{
}

void WINAPI AIL_pause_stream(HSTREAM, S32)
{
}

void WINAPI AIL_set_stream_loop_count(HSTREAM, S32)
{
}

void WINAPI AIL_set_stream_volume_levels(HSTREAM, F32, F32)
{
}

void WINAPI AIL_stream_volume_levels(HSTREAM, F32 FAR* leftLevel, F32 FAR* rightLevel)
{
	if (leftLevel)
		*leftLevel = 0.0f;
	if (rightLevel)
		*rightLevel = 0.0f;
}
