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

#include "Buffer.h"
#include "Graphics.h"

namespace love
{
namespace graphics
{

love::Type Buffer::type("GraphicsBuffer", &Object::type);

Buffer::Buffer(Graphics *gfx, const Settings &settings, const std::vector<DataDeclaration> &bufferformat, size_t size, size_t arraylength)
	: arrayLength(0)
	, arrayStride(0)
	, size(size)
	, typeFlags(settings.typeFlags)
	, usage(settings.usage)
	, mapFlags(settings.mapFlags)
	, mapped(false)
{
	if (size == 0 && arraylength == 0)
		throw love::Exception("Size or array length must be specified.");

	if (bufferformat.size() == 0)
		throw love::Exception("Data format must contain values.");

	const auto &caps = gfx->getCapabilities();
	bool supportsGLSL3 = caps.features[Graphics::FEATURE_GLSL3];

	bool indexbuffer = settings.typeFlags & TYPEFLAG_INDEX;
	bool vertexbuffer = settings.typeFlags & TYPEFLAG_VERTEX;
	bool texelbuffer = settings.typeFlags & TYPEFLAG_TEXEL;

	if (!indexbuffer && !vertexbuffer && !texelbuffer)
		throw love::Exception("Buffer must be created with at least one buffer type (index, vertex, or texel).");

	if (texelbuffer && !caps.features[Graphics::FEATURE_TEXEL_BUFFER])
		throw love::Exception("Texel buffers are not supported on this system.");

	size_t offset = 0;
	size_t stride = 0;

	for (const DataDeclaration &decl : bufferformat)
	{
		DataMember member(decl);

		DataFormat format = member.decl.format;
		const DataFormatInfo &info = member.info;

		if (indexbuffer)
		{
			if (format != DATAFORMAT_UINT16 && format != DATAFORMAT_UINT32)
				throw love::Exception("Index buffers only support uint16 and uint32 data types.");

			if (bufferformat.size() > 1)
				throw love::Exception("Index buffers only support a single value per element.");

			if (decl.arrayLength > 0)
				throw love::Exception("Arrays are not supported in index buffers.");
		}

		if (vertexbuffer)
		{
			if (decl.arrayLength > 0)
				throw love::Exception("Arrays are not supported in vertex buffers.");

			if (info.isMatrix)
				throw love::Exception("Matrix types are not supported in vertex buffers.");

			if (info.baseType == DATA_BASETYPE_BOOL)
				throw love::Exception("Bool types are not supported in vertex buffers.");

			if ((info.baseType == DATA_BASETYPE_INT || info.baseType == DATA_BASETYPE_UINT) && !supportsGLSL3)
				throw love::Exception("Integer vertex attribute data types require GLSL 3 support.");

			if (decl.name.empty())
				throw love::Exception("Vertex buffer attributes must have a name.");
		}

		if (texelbuffer)
		{
			if (format != bufferformat[0].format)
				throw love::Exception("All values in a texel buffer must have the same format.");

			if (decl.arrayLength > 0)
				throw love::Exception("Arrays are not supported in texel buffers.");

			if (info.isMatrix)
				throw love::Exception("Matrix types are not supported in texel buffers.");

			if (info.baseType == DATA_BASETYPE_BOOL)
				throw love::Exception("Bool types are not supported in texel buffers.");

			if (info.components == 3)
				throw love::Exception("3-component formats are not supported in texel buffers.");

			if (info.baseType == DATA_BASETYPE_SNORM)
				throw love::Exception("Signed normalized formats are not supported in texel buffers.");
		}

		// TODO: alignment
		member.offset = offset;
		member.size = member.info.size;

		offset += member.size;

		dataMembers.push_back(member);
	}

	stride = offset;

	if (size != 0)
	{
		size_t remainder = size % stride;
		if (remainder > 0)
			size += stride - remainder;
		arraylength = size / stride;
	}
	else
	{
		size = arraylength * stride;
	}

	this->arrayStride = stride;
	this->arrayLength = arraylength;
	this->size = size;

	if (texelbuffer && arraylength * dataMembers.size() > caps.limits[Graphics::LIMIT_TEXEL_BUFFER_SIZE])
		throw love::Exception("Cannot create texel buffer: total number of values in the buffer (%d * %d) is too large for this system (maximum %d).", (int) dataMembers.size(), (int) arraylength, caps.limits[Graphics::LIMIT_TEXEL_BUFFER_SIZE]);
}

Buffer::~Buffer()
{
}

int Buffer::getDataMemberIndex(const std::string &name) const
{
	for (size_t i = 0; i < dataMembers.size(); i++)
	{
		if (dataMembers[i].decl.name == name)
			return (int) i;
	}

	return -1;
}

std::vector<Buffer::DataDeclaration> Buffer::getCommonFormatDeclaration(CommonFormat format)
{
	switch (format)
	{
	case CommonFormat::NONE:
		return {};
	case CommonFormat::XYf:
		return {
			{ getConstant(ATTRIB_POS), DATAFORMAT_FLOAT_VEC2 }
		};
	case CommonFormat::XYZf:
		return {
			{ getConstant(ATTRIB_POS), DATAFORMAT_FLOAT_VEC3 }
		};
	case CommonFormat::RGBAub:
		return {
			{ getConstant(ATTRIB_COLOR), DATAFORMAT_UNORM8_VEC4 }
		};
	case CommonFormat::STf_RGBAub:
		return {
			{ getConstant(ATTRIB_TEXCOORD), DATAFORMAT_FLOAT_VEC2 },
			{ getConstant(ATTRIB_COLOR), DATAFORMAT_UNORM8_VEC4 },
		};
	case CommonFormat::STPf_RGBAub:
		return {
			{ getConstant(ATTRIB_TEXCOORD), DATAFORMAT_FLOAT_VEC3 },
			{ getConstant(ATTRIB_COLOR), DATAFORMAT_UNORM8_VEC4 },
		};
	case CommonFormat::XYf_STf:
		return {
			{ getConstant(ATTRIB_POS), DATAFORMAT_FLOAT_VEC2 },
			{ getConstant(ATTRIB_TEXCOORD), DATAFORMAT_FLOAT_VEC2 },
		};
	case CommonFormat::XYf_STPf:
		return {
			{ getConstant(ATTRIB_POS), DATAFORMAT_FLOAT_VEC2 },
			{ getConstant(ATTRIB_TEXCOORD), DATAFORMAT_FLOAT_VEC3 },
		};
	case CommonFormat::XYf_STf_RGBAub:
		return {
			{ getConstant(ATTRIB_POS), DATAFORMAT_FLOAT_VEC2 },
			{ getConstant(ATTRIB_TEXCOORD), DATAFORMAT_FLOAT_VEC2 },
			{ getConstant(ATTRIB_COLOR), DATAFORMAT_UNORM8_VEC4 },
		};
	case CommonFormat::XYf_STus_RGBAub:
		return {
			{ getConstant(ATTRIB_POS), DATAFORMAT_FLOAT_VEC2 },
			{ getConstant(ATTRIB_TEXCOORD), DATAFORMAT_UNORM16_VEC2 },
			{ getConstant(ATTRIB_COLOR), DATAFORMAT_UNORM8_VEC4 },
		};
	case CommonFormat::XYf_STPf_RGBAub:
		return {
			{ getConstant(ATTRIB_POS), DATAFORMAT_FLOAT_VEC2 },
			{ getConstant(ATTRIB_TEXCOORD), DATAFORMAT_FLOAT_VEC2 },
			{ getConstant(ATTRIB_COLOR), DATAFORMAT_UNORM8_VEC4 },
		};
	}

	return {};
}

} // graphics
} // love
