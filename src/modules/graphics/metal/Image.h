/**
 * Copyright (c) 2006-2020 LOVE Development Team
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 **/

#pragma once

#include "common/config.h"
#include "common/Color.h"
#include "common/int.h"
#include "graphics/Image.h"
#include "Metal.h"

namespace love
{
namespace graphics
{
namespace metal
{

class Image final : public love::graphics::Image
{
public:

	Image(id<MTLDevice> device, const Slices &data, const Settings &settings);
	Image(id<MTLDevice> device, TextureType textype, PixelFormat format, int width, int height, int slices, const Settings &settings);
	virtual ~Image();

	ptrdiff_t getHandle() const override { return (ptrdiff_t) texture; }

	void setFilter(const Texture::Filter &f) override;
	bool setWrap(const Texture::Wrap &w) override;

	bool setMipmapSharpness(float sharpness) override;

private:

	void uploadByteData(PixelFormat pixelformat, const void *data, size_t size, int level, int slice, const Rect &r) override;
	void generateMipmaps() override;

	void create(id<MTLDevice> device);

	id<MTLTexture> texture;
	id<MTLSamplerState> sampler;

}; // Image

} // metal
} // graphics
} // love
