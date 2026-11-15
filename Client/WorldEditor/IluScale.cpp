#include "StdAfx.h"
#include <IL/ilu.h>
#include <cstring>
#include <vector>

namespace
{
	void SampleBilinear(const ILubyte* src, int width, int height, int bpp, float x, float y, ILubyte* out)
	{
		if (x < 0.0f)
			x = 0.0f;
		if (y < 0.0f)
			y = 0.0f;
		if (x > static_cast<float>(width - 1))
			x = static_cast<float>(width - 1);
		if (y > static_cast<float>(height - 1))
			y = static_cast<float>(height - 1);

		const int x0 = static_cast<int>(x);
		const int y0 = static_cast<int>(y);
		const int x1 = (x0 + 1 < width) ? x0 + 1 : width - 1;
		const int y1 = (y0 + 1 < height) ? y0 + 1 : height - 1;
		const float fx = x - static_cast<float>(x0);
		const float fy = y - static_cast<float>(y0);
		const ILubyte* p00 = src + (static_cast<size_t>(y0) * width + x0) * bpp;
		const ILubyte* p10 = src + (static_cast<size_t>(y0) * width + x1) * bpp;
		const ILubyte* p01 = src + (static_cast<size_t>(y1) * width + x0) * bpp;
		const ILubyte* p11 = src + (static_cast<size_t>(y1) * width + x1) * bpp;

		for (int channel = 0; channel < bpp; ++channel)
		{
			const float value =
				p00[channel] * (1.0f - fx) * (1.0f - fy) +
				p10[channel] * fx * (1.0f - fy) +
				p01[channel] * (1.0f - fx) * fy +
				p11[channel] * fx * fy;
			out[channel] = static_cast<ILubyte>(value + 0.5f);
		}
	}
}

// The x64 tree has DevIL but no ILU library. These two calls are the only ILU
// entry points the editor uses.
extern "C" ILboolean ILAPIENTRY iluScale(ILuint Width, ILuint Height, ILuint Depth)
{
	const ILint sourceWidth = ilGetInteger(IL_IMAGE_WIDTH);
	const ILint sourceHeight = ilGetInteger(IL_IMAGE_HEIGHT);
	const ILint bytesPerPixel = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
	const auto format = static_cast<ILenum>(ilGetInteger(IL_IMAGE_FORMAT));
	const auto type = static_cast<ILenum>(ilGetInteger(IL_IMAGE_TYPE));
	if (sourceWidth <= 0 || sourceHeight <= 0 || bytesPerPixel <= 0 || Width == 0 || Height == 0)
		return IL_FALSE;
	if (type != IL_UNSIGNED_BYTE)
		return IL_FALSE;

	ILubyte* source = ilGetData();
	if (!source)
		return IL_FALSE;

	std::vector<ILubyte> scaled(static_cast<size_t>(Width) * Height * bytesPerPixel);
	for (ILuint y = 0; y < Height; ++y)
	{
		const float sampleY = (Height == 1)
			? 0.0f
			: (static_cast<float>(y) + 0.5f) * static_cast<float>(sourceHeight) / static_cast<float>(Height) - 0.5f;
		for (ILuint x = 0; x < Width; ++x)
		{
			const float sampleX = (Width == 1)
				? 0.0f
				: (static_cast<float>(x) + 0.5f) * static_cast<float>(sourceWidth) / static_cast<float>(Width) - 0.5f;
			SampleBilinear(
				source,
				sourceWidth,
				sourceHeight,
				bytesPerPixel,
				sampleX,
				sampleY,
				&scaled[(static_cast<size_t>(y) * Width + x) * bytesPerPixel]);
		}
	}

	const ILuint depth = Depth == 0 ? 1 : Depth;
	if (!ilTexImage(Width, Height, depth, static_cast<ILubyte>(bytesPerPixel), format, type, scaled.data()))
		return IL_FALSE;
	return IL_TRUE;
}

extern "C" ILboolean ILAPIENTRY iluFlipImage(void)
{
	const ILint width = ilGetInteger(IL_IMAGE_WIDTH);
	const ILint height = ilGetInteger(IL_IMAGE_HEIGHT);
	const ILint bytesPerPixel = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
	if (width <= 0 || height <= 1 || bytesPerPixel <= 0)
		return width > 0 ? IL_TRUE : IL_FALSE;

	ILubyte* data = ilGetData();
	if (!data)
		return IL_FALSE;

	const size_t stride = static_cast<size_t>(width) * bytesPerPixel;
	std::vector<ILubyte> row(stride);
	for (ILint y = 0; y < height / 2; ++y)
	{
		ILubyte* top = data + stride * y;
		ILubyte* bottom = data + stride * (height - 1 - y);
		memcpy(row.data(), top, stride);
		memcpy(top, bottom, stride);
		memcpy(bottom, row.data(), stride);
	}
	return IL_TRUE;
}
