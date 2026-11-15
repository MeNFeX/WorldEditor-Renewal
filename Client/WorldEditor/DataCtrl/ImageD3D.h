#pragma once

inline bool ImageCreateSurface (LPDIRECT3DDEVICE9 device, int width, int height, LPDIRECT3DSURFACE9 * surface)
{
	if (surface)
		*surface = nullptr;
	if (!device || !surface || width <= 0 || height <= 0)
		return false;
	return SUCCEEDED (device->CreateOffscreenPlainSurface (width, height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, surface, nullptr));
}

inline void ImageClear (LPDIRECT3DSURFACE9 surface, int width, int height)
{
	if (!surface)
		return;
	D3DLOCKED_RECT lock;
	if (FAILED (surface->LockRect (&lock, nullptr, 0)) || !lock.pBits)
		return;
	for (int y = 0; y < height; ++y)
		memset ((BYTE *) lock.pBits + y * lock.Pitch, 0, width * 4);
	surface->UnlockRect();
}

inline bool ImageLoadFile (LPDIRECT3DDEVICE9 device, const char * path, int width, int height, LPDIRECT3DSURFACE9 * surface)
{
	if (!ImageCreateSurface (device, width, height, surface))
		return false;
	if (FAILED (D3DXLoadSurfaceFromFile (*surface, nullptr, nullptr, path, nullptr, D3DX_FILTER_TRIANGLE, 0, nullptr)))
	{
		(*surface)->Release();
		*surface = nullptr;
		return false;
	}
	return true;
}

inline bool ImageLoadMemory (LPDIRECT3DDEVICE9 device, const void * data, UINT size, int width, int height, LPDIRECT3DSURFACE9 * surface)
{
	if (!data || size == 0 || !ImageCreateSurface (device, width, height, surface))
		return false;
	if (FAILED (D3DXLoadSurfaceFromFileInMemory (*surface, nullptr, nullptr, data, size, nullptr, D3DX_FILTER_TRIANGLE, 0, nullptr)))
	{
		(*surface)->Release();
		*surface = nullptr;
		return false;
	}
	return true;
}

inline bool ImageCopy (LPDIRECT3DSURFACE9 dest, int x, int y, int width, int height, LPDIRECT3DSURFACE9 source)
{
	if (!dest || !source)
		return false;
	RECT destRect = { x, y, x + width, y + height };
	return SUCCEEDED (D3DXLoadSurfaceFromSurface (dest, nullptr, &destRect, source, nullptr, nullptr, D3DX_FILTER_TRIANGLE, 0));
}

inline bool ImageSave (const char * path, D3DXIMAGE_FILEFORMAT format, LPDIRECT3DSURFACE9 surface)
{
	return surface && path && SUCCEEDED (D3DXSaveSurfaceToFile (path, format, surface, nullptr, nullptr));
}
