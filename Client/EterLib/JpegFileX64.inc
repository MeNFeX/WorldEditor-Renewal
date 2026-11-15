#include <objidl.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

// x64 has no libjpeg import library in this tree. GDI+ writes the same RGB/RGBA
// layouts the libjpeg path produced: save functions take packed RGB, jpeg_load
// returns bytes ordered a,r,g,b.

namespace
{
	ULONG_PTR g_jpegGdiplusToken = 0;

	bool JpegGdiplusStartup()
	{
		if (g_jpegGdiplusToken)
			return true;
		Gdiplus::GdiplusStartupInput input;
		return Gdiplus::GdiplusStartup(&g_jpegGdiplusToken, &input, nullptr) == Gdiplus::Ok;
	}

	int JpegEncoderClsid(const WCHAR* mime, CLSID* clsid)
	{
		UINT count = 0;
		UINT bytes = 0;
		if (Gdiplus::GetImageEncodersSize(&count, &bytes) != Gdiplus::Ok || bytes == 0)
			return 0;
		auto* info = static_cast<Gdiplus::ImageCodecInfo*>(malloc(bytes));
		if (!info)
			return 0;
		int found = 0;
		if (Gdiplus::GetImageEncoders(count, bytes, info) == Gdiplus::Ok)
		{
			for (UINT i = 0; i < count; ++i)
			{
				if (wcscmp(info[i].MimeType, mime) == 0)
				{
					*clsid = info[i].Clsid;
					found = 1;
					break;
				}
			}
		}
		free(info);
		return found;
	}

	int JpegEncodeRgb(unsigned char* data, int width, int height, int quality, IStream* stream)
	{
		if (!data || width <= 0 || height <= 0 || !stream || !JpegGdiplusStartup())
			return 0;

		const int stride = (width * 3 + 3) & ~3;
		auto* bgr = static_cast<unsigned char*>(malloc(static_cast<size_t>(stride) * height));
		if (!bgr)
			return 0;

		for (int y = 0; y < height; ++y)
		{
			const unsigned char* src = data + static_cast<size_t>(y) * width * 3;
			unsigned char* dst = bgr + static_cast<size_t>(y) * stride;
			for (int x = 0; x < width; ++x)
			{
				dst[x * 3 + 0] = src[x * 3 + 2];
				dst[x * 3 + 1] = src[x * 3 + 1];
				dst[x * 3 + 2] = src[x * 3 + 0];
			}
		}

		Gdiplus::Bitmap bitmap(width, height, stride, PixelFormat24bppRGB, bgr);
		CLSID clsid;
		int ok = 0;
		if (JpegEncoderClsid(L"image/jpeg", &clsid))
		{
			ULONG qualityValue = static_cast<ULONG>(quality);
			Gdiplus::EncoderParameters params;
			params.Count = 1;
			params.Parameter[0].Guid = Gdiplus::EncoderQuality;
			params.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
			params.Parameter[0].NumberOfValues = 1;
			params.Parameter[0].Value = &qualityValue;
			ok = bitmap.Save(stream, &clsid, &params) == Gdiplus::Ok;
		}
		free(bgr);
		return ok;
	}

	int JpegWriteStream(IStream* stream, int (*write)(const void*, size_t, void*), void* context)
	{
		STATSTG stat{};
		if (FAILED(stream->Stat(&stat, STATFLAG_NONAME)))
			return 0;
		HGLOBAL memory = nullptr;
		if (FAILED(GetHGlobalFromStream(stream, &memory)))
			return 0;
		const size_t size = static_cast<size_t>(stat.cbSize.QuadPart);
		void* bits = GlobalLock(memory);
		const int ok = bits && write(bits, size, context);
		if (bits)
			GlobalUnlock(memory);
		return ok;
	}
}

int jpeg_save(unsigned char* data, int width, int height, int quality, const char* filename)
{
	if (!filename)
		return 0;
	IStream* stream = nullptr;
	if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream)))
		return 0;
	int ok = JpegEncodeRgb(data, width, height, quality, stream);
	if (ok)
	{
		ok = JpegWriteStream(stream, [](const void* bits, size_t size, void* context) -> int
		{
			FILE* file = fopen(static_cast<const char*>(context), "wb");
			if (!file)
				return 0;
			const int wrote = fwrite(bits, 1, size, file) == size;
			fclose(file);
			return wrote;
		}, const_cast<char*>(filename));
	}
	stream->Release();
	return ok;
}

int jpeg_save_to_file(unsigned char* data, int width, int height, int quality, FILE* file)
{
	if (!file)
		return 0;
	IStream* stream = nullptr;
	if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream)))
		return 0;
	int ok = JpegEncodeRgb(data, width, height, quality, stream);
	if (ok)
	{
		ok = JpegWriteStream(stream, [](const void* bits, size_t size, void* context) -> int
		{
			return fwrite(bits, 1, size, static_cast<FILE*>(context)) == size;
		}, file);
	}
	stream->Release();
	return ok;
}

int jpeg_save_to_mem(unsigned char* data, int width, int height, int quality, unsigned char* dest, int destSize)
{
	if (!dest || destSize <= 0)
		return 0;
	IStream* stream = nullptr;
	if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream)))
		return 0;
	int written = 0;
	if (JpegEncodeRgb(data, width, height, quality, stream))
	{
		STATSTG stat{};
		HGLOBAL memory = nullptr;
		if (SUCCEEDED(stream->Stat(&stat, STATFLAG_NONAME)) && SUCCEEDED(GetHGlobalFromStream(stream, &memory)))
		{
			const int size = static_cast<int>(stat.cbSize.QuadPart);
			if (size > 0 && size <= destSize)
			{
				void* bits = GlobalLock(memory);
				if (bits)
				{
					memcpy(dest, bits, size);
					written = size;
					GlobalUnlock(memory);
				}
			}
		}
	}
	stream->Release();
	return written;
}

int jpeg_load(const char* filename, unsigned char** dest, int* width, int* height)
{
	if (dest)
		*dest = nullptr;
	if (!filename || !dest || !width || !height || !JpegGdiplusStartup())
		return 0;

	wchar_t widePath[MAX_PATH];
	if (!MultiByteToWideChar(CP_ACP, 0, filename, -1, widePath, MAX_PATH))
		return 0;

	Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(widePath, FALSE);
	if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok)
	{
		delete bitmap;
		return 0;
	}

	const int imageWidth = static_cast<int>(bitmap->GetWidth());
	const int imageHeight = static_cast<int>(bitmap->GetHeight());
	*width = imageWidth;
	*height = imageHeight;
	*dest = static_cast<unsigned char*>(malloc(static_cast<size_t>(imageWidth) * imageHeight * 4));
	if (!*dest)
	{
		delete bitmap;
		return 0;
	}

	Gdiplus::BitmapData locked{};
	Gdiplus::Rect rect(0, 0, imageWidth, imageHeight);
	if (bitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &locked) != Gdiplus::Ok)
	{
		free(*dest);
		*dest = nullptr;
		delete bitmap;
		return 0;
	}

	for (int y = 0; y < imageHeight; ++y)
	{
		const auto* src = static_cast<const unsigned char*>(locked.Scan0) + static_cast<size_t>(y) * locked.Stride;
		unsigned char* row = *dest + static_cast<size_t>(y) * imageWidth * 4;
		for (int x = 0; x < imageWidth; ++x)
		{
			row[x * 4 + 0] = 255;
			row[x * 4 + 1] = src[x * 4 + 2];
			row[x * 4 + 2] = src[x * 4 + 1];
			row[x * 4 + 3] = src[x * 4 + 0];
		}
	}

	bitmap->UnlockBits(&locked);
	delete bitmap;
	return 1;
}

int jpeg_load_from_mem(unsigned char* data, int size, unsigned char* dest, int width, int height)
{
	if (!data || size <= 0 || !dest || width <= 0 || height <= 0 || !JpegGdiplusStartup())
		return 0;

	HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, size);
	if (!memory)
		return 0;
	void* lockedMemory = GlobalLock(memory);
	if (!lockedMemory)
	{
		GlobalFree(memory);
		return 0;
	}
	memcpy(lockedMemory, data, size);
	GlobalUnlock(memory);

	IStream* stream = nullptr;
	if (FAILED(CreateStreamOnHGlobal(memory, TRUE, &stream)))
	{
		GlobalFree(memory);
		return 0;
	}

	Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromStream(stream, FALSE);
	int ok = 0;
	if (bitmap && bitmap->GetLastStatus() == Gdiplus::Ok)
	{
		const int imageWidth = static_cast<int>(bitmap->GetWidth());
		const int imageHeight = static_cast<int>(bitmap->GetHeight());
		const int copyWidth = width < imageWidth ? width : imageWidth;
		const int copyHeight = height < imageHeight ? height : imageHeight;
		Gdiplus::BitmapData locked{};
		Gdiplus::Rect rect(0, 0, imageWidth, imageHeight);
		if (bitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat24bppRGB, &locked) == Gdiplus::Ok)
		{
			for (int y = 0; y < copyHeight; ++y)
			{
				const auto* src = static_cast<const unsigned char*>(locked.Scan0) + static_cast<size_t>(y) * locked.Stride;
				unsigned char* row = dest + static_cast<size_t>(y) * width * 3;
				for (int x = 0; x < copyWidth; ++x)
				{
					row[x * 3 + 0] = src[x * 3 + 2];
					row[x * 3 + 1] = src[x * 3 + 1];
					row[x * 3 + 2] = src[x * 3 + 0];
				}
			}
			bitmap->UnlockBits(&locked);
			ok = 1;
		}
	}

	delete bitmap;
	stream->Release();
	return ok;
}
