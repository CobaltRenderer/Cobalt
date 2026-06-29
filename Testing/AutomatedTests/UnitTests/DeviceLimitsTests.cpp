// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <array>
#include <set>

namespace {
bool Utf8CheckIsValid(const std::string& string)
{
	size_t ix = string.length();
	for (size_t i = 0; i < ix; ++i)
	{
		int n;
		auto c = (uint8_t)string[i];
		//if (c==0x09 || c==0x0a || c==0x0d || (0x20 <= c && c <= 0x7e) ) n = 0; // is_printable_ascii
		if (c <= 0x7f)
		{
			n = 0; // 0bbbbbbb
		}
		else if ((c & 0xE0) == 0xC0)
		{
			n = 1; // 110bbbbb
		}
		else if (c == 0xed && i < (ix - 1) && ((uint8_t)string[i + 1] & 0xa0) == 0xa0)
		// NOLINTNEXTLINE bugprone-branch-clone
		{
			return false; //U+d800 to U+dfff
		}
		else if ((c & 0xF0) == 0xE0)
		{
			n = 2; // 1110bbbb
		}
		else if ((c & 0xF8) == 0xF0)
		{
			n = 3; // 11110bbb
		}
		else
		{
			return false;
		}
		for (int j = 0; j < n && i < ix; j++)
		{ // n bytes matching 10bbbbbb follow ?
			if ((++i == ix) || (((uint8_t)string[i] & 0xC0) != 0x80))
			{
				return false;
			}
		}
	}
	return true;
}
} // namespace

namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

DEFINE_UNIT_TEST_WITH_BASE("Device/DeviceLimits", UnitTestBase)
{
	auto& device = session.Device();

	// Validate those values returned from the various limits such that they're within reasonable
	// expectations of hardware capabilities and scope. We had a bug where renderers were reporting
	// they could handle 300 million concurrent textures or other ludicrous obviously wrong
	// values (-100 million AnisotropicFiltering levels?) - we need to test these values make sense.
	//
	// It's expected that these tests will eventually need to be updated as they include some
	// estimates of upper bound of hardware spec, however I've estimated at least an order of magnitude
	// above the best hardware I've seen in 2021, with the growth in hardware 2010 to 2021
	// extrapolated out to 2040 to provide the upper bounds used for these tests.

	{
		// The documentation for GetDeviceType states its "best effort" - so itd be unwise to test
		// it with any more rigour than that. I've observed VM card forwarding get identified as
		// integrated and not virtual, and software emulation get identified as descrete, so it's
		// to be taken with a grain of salt. I'm just validating it's within the known enum bounds.
		auto dt = device.GetDeviceType();

		REQUIRE((int32_t)dt >= (int32_t)IGraphicsDevice::DeviceType::Discrete);
		REQUIRE((int32_t)dt <= (int32_t)IGraphicsDevice::DeviceType::Unknown);

		session.AddTestInfo("DeviceType", "Device type: " + std::to_string((int32_t)dt));
	}

	{
		auto vendor = device.GetVendor();
		REQUIRE((int32_t)vendor >= (int32_t)IGraphicsDevice::Vendor::AMD);
		REQUIRE((int32_t)vendor <= (int32_t)IGraphicsDevice::Vendor::Unknown);
		session.AddTestInfo("Vendor", "Vendor enum: " + std::to_string((int32_t)vendor));
	}

	{
		auto sl = device.GetShaderLimits();
		session.AddTestInfo("ShaderLimits",
		                    "maxVertexShaderInputAttributes: " + std::to_string(sl.maxVertexShaderInputAttributes) + "\n" +
		                      "maxVertexShaderOutputComponents: " + std::to_string(sl.maxVertexShaderOutputComponents) + "\n" +
		                      "maxGeometryShaderInputComponents: " + std::to_string(sl.maxGeometryShaderInputComponents) + "\n" +
		                      "maxGeometryShaderOutputComponents: " + std::to_string(sl.maxGeometryShaderOutputComponents) + "\n" +
		                      "maxGeometryShaderOutputVertices: " + std::to_string(sl.maxGeometryShaderOutputVertices) + "\n" +
		                      "maxGeometryShaderTotalOutputComponents: " + std::to_string(sl.maxGeometryShaderTotalOutputComponents) + "\n" +
		                      "maxFragmentShaderInputComponents: " + std::to_string(sl.maxFragmentShaderInputComponents));

		REQUIRE(sl.maxVertexShaderInputAttributes >= 4);
		REQUIRE(sl.maxVertexShaderInputAttributes <= 256);

		REQUIRE(sl.maxVertexShaderOutputComponents >= 8);
		REQUIRE(sl.maxVertexShaderOutputComponents <= 2048);

		if (device.IsFeatureSupported(IGraphicsDevice::Feature::GeometryShaders))
		{
			REQUIRE(sl.maxGeometryShaderInputComponents >= 8);
			REQUIRE(sl.maxGeometryShaderInputComponents <= 2048);

			REQUIRE(sl.maxGeometryShaderOutputComponents >= 8);
			REQUIRE(sl.maxGeometryShaderOutputComponents <= 2048);

			REQUIRE(sl.maxGeometryShaderOutputVertices >= 8);
			REQUIRE(sl.maxGeometryShaderOutputVertices <= 32768);

			REQUIRE(sl.maxGeometryShaderTotalOutputComponents >= 16);
			REQUIRE(sl.maxGeometryShaderTotalOutputComponents <= 65536);
		}

		REQUIRE(sl.maxFragmentShaderInputComponents >= 16);
		REQUIRE(sl.maxFragmentShaderInputComponents <= 2048);
	}

	{
		auto il = device.GetImageLimits();
		session.AddTestInfo("ImageLimits",
		                    "maxImageDimensionTexture1D: " + std::to_string(il.maxImageDimensionTexture1D) + "\n" +
		                      "maxImageDimensionTexture2D: " + std::to_string(il.maxImageDimensionTexture2D) + "\n" +
		                      "maxImageDimensionTexture3D: " + std::to_string(il.maxImageDimensionTexture3D) + "\n" +
		                      "maxImageDimensionTextureCube: " + std::to_string(il.maxImageDimensionTextureCube) + "\n" +
		                      "maxImageArraySizeTexture1D: " + std::to_string(il.maxImageArraySizeTexture1D) + "\n" +
		                      "maxImageArraySizeTexture2D: " + std::to_string(il.maxImageArraySizeTexture2D) + "\n" +
		                      "maxSamplerAnisotropicFilteringLevel: " + std::to_string(il.maxSamplerAnisotropicFilteringLevel));

		REQUIRE(il.maxImageDimensionTexture1D >= 1024);
		REQUIRE(il.maxImageDimensionTexture1D <= 1048576);

		REQUIRE(il.maxImageDimensionTexture2D >= 1024);
		REQUIRE(il.maxImageDimensionTexture2D <= 1048576);

		REQUIRE(il.maxImageDimensionTexture3D >= 256);
		REQUIRE(il.maxImageDimensionTexture3D <= 1048576);

		REQUIRE(il.maxImageDimensionTextureCube >= 256);
		REQUIRE(il.maxImageDimensionTextureCube <= 1048576);

		REQUIRE(il.maxImageArraySizeTexture1D >= 1);
		REQUIRE(il.maxImageArraySizeTexture1D <= 1048576);
		REQUIRE(il.maxImageArraySizeTexture2D >= 1);
		REQUIRE(il.maxImageArraySizeTexture2D <= 1048576);

		REQUIRE(il.maxSamplerAnisotropicFilteringLevel >= 0);
		REQUIRE(il.maxSamplerAnisotropicFilteringLevel <= 256);
	}

	{
		auto dl = device.GetDrawLimits();
		session.AddTestInfo("DrawLimits",
		                    "maxTextureResourcesPerDraw: " + std::to_string(dl.maxTextureResourcesPerDraw) + "\n" +
		                      "maxResourcesPerDraw: " + std::to_string(dl.maxResourcesPerDraw) + "\n" +
		                      "maxVertexCountPerDraw: " + std::to_string(dl.maxVertexCountPerDraw) + "\n" +
		                      "maxIndexValue: " + std::to_string(dl.maxIndexValue));

		REQUIRE(dl.maxVertexCountPerDraw >= 32767);
		REQUIRE(dl.maxIndexValue >= 32767);

		// I've observed 4 textures per draw (OpenGL 3.3 on a handeld tablet in 2013) and
		// 1048576 textures per draw (Vulkan 1.1. Nvidia GTX 1060 in 2021). An AMD Radeon RX 560
		// on Vulkan returns 4294967295, so no upper bound check can be done.
		REQUIRE(dl.maxTextureResourcesPerDraw >= 4);

		// Many Vulkan implementations defines maxResourcesPerDraw as uint_32::max(). So lets just make
		// sure it isn't a really tiny number.
		REQUIRE(dl.maxResourcesPerDraw >= 16);
	}

	{
		auto fl = device.GetFrameBufferLimits();
		session.AddTestInfo("FrameBufferLimits",
		                    "maxFrameBufferWidth: " + std::to_string(fl.maxFrameBufferWidth) + "\n" +
		                      "maxFrameBufferHeight: " + std::to_string(fl.maxFrameBufferHeight) + "\n" +
		                      "maxFrameBufferColorAttachments: " + std::to_string(fl.maxFrameBufferColorAttachments) + "\n" +
		                      "depthRange: " + std::to_string((uint32_t)fl.depthRange));

		REQUIRE(fl.maxFrameBufferWidth >= 1024);
		REQUIRE(fl.maxFrameBufferWidth <= 1048576);

		REQUIRE(fl.maxFrameBufferHeight >= 1024);
		REQUIRE(fl.maxFrameBufferHeight <= 1048576);

		REQUIRE(fl.maxFrameBufferColorAttachments >= 1);
		REQUIRE(fl.maxFrameBufferColorAttachments <= 64);

		REQUIRE(fl.depthRange == IGraphicsDevice::DepthRange::NegativeOneToOne || fl.depthRange == IGraphicsDevice::DepthRange::ZeroToOne);
	}

	{
		// There's not a lot we can do here - we can make sure that it doesn't return
		// garbage, binary data, random memory, or other nonsense by validating its
		// valid utf8. We can guarantee this is valid UTF8 in:
		// Direct3D: (We convert to UTF-8 from UTF-16)
		// Vulkan: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPhysicalDeviceProperties.html
		// OpenGL: https://www.khronos.org/registry/OpenGL/specs/gl/glspec43.compatibility.pdf page 651
		auto vn = device.GetVendorName();
		session.AddTestInfo("VendorName", "Vendor name: " + vn);

		REQUIRE(Utf8CheckIsValid(vn));

		// Should only be a single line.
		REQUIRE(vn.Get().find('\n') == std::string::npos);
	}

	{
		auto dn = device.GetDeviceName();
		session.AddTestInfo("DeviceName", "Device name: " + dn);

		REQUIRE(Utf8CheckIsValid(dn));

		// Should only be a single line.
		REQUIRE(dn.Get().find('\n') == std::string::npos);
	}

	{
		auto dbl = device.GetDataBufferLimits();
		session.AddTestInfo("DataBufferLimits",
		                    "maxStateBufferPageSize: " + std::to_string(dbl.maxStateBufferPageSize) + "\n" +
		                      "stateBufferAlignmentFloatOrInt: " + std::to_string(dbl.stateBufferAlignmentFloatOrInt) + "\n" +
		                      "stateBufferAlignmentArrayWhole: " + std::to_string(dbl.stateBufferAlignmentArrayWhole) + "\n" +
		                      "stateBufferAlignmentArrayStride: " + std::to_string(dbl.stateBufferAlignmentArrayStride) + "\n" +
		                      "stateBufferAlignmentStruct: " + std::to_string(dbl.stateBufferAlignmentStruct) + "\n" +
		                      "stateBufferAlignmentVector4f: " + std::to_string(dbl.stateBufferAlignmentVector4f) + "\n" +
		                      "stateBufferAlignmentMatrix4f: " + std::to_string(dbl.stateBufferAlignmentMatrix4f));

		REQUIRE(dbl.maxStateBufferPageSize >= 16384);

		REQUIRE(dbl.stateBufferAlignmentFloatOrInt > 0 && dbl.stateBufferAlignmentFloatOrInt <= 64);
		REQUIRE(dbl.stateBufferAlignmentVector4f > 0 && dbl.stateBufferAlignmentVector4f <= 64);
		REQUIRE(dbl.stateBufferAlignmentMatrix4f > 0 && dbl.stateBufferAlignmentMatrix4f <= 128);
		REQUIRE(dbl.stateBufferAlignmentArrayWhole > 0 && dbl.stateBufferAlignmentArrayWhole <= 256);
		REQUIRE(dbl.stateBufferAlignmentArrayStride > 0 && dbl.stateBufferAlignmentArrayStride <= 64);
		REQUIRE(dbl.stateBufferAlignmentStruct > 0 && dbl.stateBufferAlignmentStruct <= 64);
	}

	{
		auto driverInfo = device.GetDriverInfo();
		session.AddTestInfo("DriverInfo", "Driver info: " + driverInfo);
	}

	{
		// Validate that aggregate feature queries match individual feature queries.
		const std::array<IGraphicsDevice::Feature, 15> allFeatures = {{
		  IGraphicsDevice::Feature::AnisotropicFiltering,
		  IGraphicsDevice::Feature::GeometryShaders,
		  IGraphicsDevice::Feature::ComputeShaders,
		  IGraphicsDevice::Feature::MeshShaders,
		  IGraphicsDevice::Feature::DepthBiasClamp,
		  IGraphicsDevice::Feature::IndirectDraw,
		  IGraphicsDevice::Feature::IndirectMultiDrawNative,
		  IGraphicsDevice::Feature::InstanceOffset,
		  IGraphicsDevice::Feature::PolygonWireframeFillMode,
		  IGraphicsDevice::Feature::ResourceArrays,
		  IGraphicsDevice::Feature::ShaderArraysOfArrays,
		  IGraphicsDevice::Feature::SeparateBlendModePerTarget,
		  IGraphicsDevice::Feature::SeparateTextureSamplers,
		  IGraphicsDevice::Feature::TextureCubeArray,
		  IGraphicsDevice::Feature::MipmapLevelBias,
		}};

		auto supportedFeatures = device.GetAllSupportedFeatures().Get();
		REQUIRE(device.AreAllFeaturesSupported(std::set<IGraphicsDevice::Feature>{}));
		REQUIRE(device.AreAllFeaturesSupported(supportedFeatures));
		for (auto feature : allFeatures)
		{
			const bool reportedInSet = supportedFeatures.find(feature) != supportedFeatures.end();
			REQUIRE(device.IsFeatureSupported(feature) == reportedInSet);
			REQUIRE(device.AreAllFeaturesSupported(std::set<IGraphicsDevice::Feature>{feature}) == reportedInSet);
		}

		std::set<IGraphicsDevice::Feature> everyKnownFeature(allFeatures.begin(), allFeatures.end());
		if (supportedFeatures.size() < everyKnownFeature.size())
		{
			REQUIRE(!device.AreAllFeaturesSupported(everyKnownFeature));
		}
		session.AddTestSuccess("FeatureSetContracts", "Feature set aggregate queries match individual feature support queries.");
	}

	{
		// Probe the device texture-format support matrix directly.
		const std::array<ITextureBuffer::ImageFormat, 12> imageFormats = {{
		  ITextureBuffer::ImageFormat::R,
		  ITextureBuffer::ImageFormat::RG,
		  ITextureBuffer::ImageFormat::RGB,
		  ITextureBuffer::ImageFormat::RGBA,
		  ITextureBuffer::ImageFormat::BGR,
		  ITextureBuffer::ImageFormat::BGRA,
		  ITextureBuffer::ImageFormat::X,
		  ITextureBuffer::ImageFormat::XY,
		  ITextureBuffer::ImageFormat::XYZ,
		  ITextureBuffer::ImageFormat::XYZW,
		  ITextureBuffer::ImageFormat::Depth,
		  ITextureBuffer::ImageFormat::DepthAndStencil,
		}};
		const std::array<ITextureBuffer::DataFormat, 26> dataFormats = {{
		  ITextureBuffer::DataFormat::Int8,
		  ITextureBuffer::DataFormat::Int16,
		  ITextureBuffer::DataFormat::Int32,
		  ITextureBuffer::DataFormat::UInt8,
		  ITextureBuffer::DataFormat::UInt16,
		  ITextureBuffer::DataFormat::UInt32,
		  ITextureBuffer::DataFormat::Norm8,
		  ITextureBuffer::DataFormat::Norm16,
		  ITextureBuffer::DataFormat::UNorm8,
		  ITextureBuffer::DataFormat::UNorm16,
		  ITextureBuffer::DataFormat::Float16,
		  ITextureBuffer::DataFormat::Float32,
		  ITextureBuffer::DataFormat::DXT1,
		  ITextureBuffer::DataFormat::DXT3,
		  ITextureBuffer::DataFormat::DXT5,
		  ITextureBuffer::DataFormat::BPTC,
		  ITextureBuffer::DataFormat::ETC2,
		  ITextureBuffer::DataFormat::ASTC4x4,
		  ITextureBuffer::DataFormat::ASTC5x5,
		  ITextureBuffer::DataFormat::ASTC6x6,
		  ITextureBuffer::DataFormat::ASTC8x8,
		  ITextureBuffer::DataFormat::DepthUNorm16,
		  ITextureBuffer::DataFormat::DepthUNorm24,
		  ITextureBuffer::DataFormat::DepthUNorm24StencilUInt8,
		  ITextureBuffer::DataFormat::DepthFloat32,
		  ITextureBuffer::DataFormat::DepthFloat32StencilUInt8,
		}};

		size_t supportedFormatCount = 0;
		for (auto imageFormat : imageFormats)
		{
			for (auto dataFormat : dataFormats)
			{
				if (device.IsTextureFormatSupported(imageFormat, dataFormat))
				{
					++supportedFormatCount;
				}
			}
		}
		REQUIRE(device.IsTextureFormatSupported(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8));
		REQUIRE(device.IsTextureFormatSupported(ITextureBuffer::ImageFormat::Depth, ITextureBuffer::DataFormat::DepthFloat32) ||
		        device.IsTextureFormatSupported(ITextureBuffer::ImageFormat::Depth, ITextureBuffer::DataFormat::DepthUNorm24) ||
		        device.IsTextureFormatSupported(ITextureBuffer::ImageFormat::Depth, ITextureBuffer::DataFormat::DepthUNorm16));
		REQUIRE(supportedFormatCount > 0);
		session.AddTestInfo("TextureFormatSupportMatrix", "Supported texture format combinations: " + std::to_string(supportedFormatCount) + " of " + std::to_string(imageFormats.size() * dataFormats.size()) + ".");
	}

	{
		auto mss = device.GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Shared);
		auto msd = device.GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated);

		// There's not a lot we can do here, these APIs are extremely unreliable, and
		// 0 is possible.

		auto total = mss + msd;
		session.AddTestInfo("DeviceMemory", std::to_string(total) + " bytes of RAM reported, with " + std::to_string(mss) + " bytes shared and " + std::to_string(msd) + " bytes dedicated.");

		if (total == 0)
		{
			// Yeah that's OK - Seen in VMs
		}
		else if (total < (size_t)(1024 * 1024))
		{
			session.AddTestFailure("DeviceMemory", "Sanity checks the reported device memory", std::to_string(total) + " bytes of RAM reported. More than 0 but less than 1mb of ram seems dubious");
		}
		else if constexpr (sizeof(size_t) >= 8)
		{
			if (total > 1024ull * 1024 * 1024 * 1024)
			{
				session.AddTestFailure("DeviceMemory", "Sanity checks the reported device memory", std::to_string(total) + " bytes of RAM reported. More than 32tb of ram seems dubious. Windows 10 only supports 6tb.");
			}
		}

		if ((mss > 0) && (mss < (size_t)(1024 * 1024)))
		{
			session.AddTestFailure("DeviceMemory", "Sanity checks the reported device memory", std::to_string(mss) + " bytes of shared RAM reported. More than 0 but less than 1mb of shared ram seems dubious");
		}

		if ((msd > 0) && (msd < (size_t)(1024 * 1024)))
		{
			session.AddTestFailure("DeviceMemory", "Sanity checks the reported device memory", std::to_string(msd) + " bytes of dedicated RAM reported. More than 0 but less than 1mb of dedicated ram seems dubious");
		}

		if constexpr (sizeof(size_t) >= 8)
		{
// RJS - As of now (2022-09-16) the Visual Studio compiler is looking inside this if constexpr on x86 platforms and
// throwing a truncation warning here.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4310)
#endif
			if (msd > (size_t)(8 * 1024ull * 1024 * 1024 * 1024))
			{
				session.AddTestFailure("DeviceMemory", "Sanity checks the reported device memory", std::to_string(msd) + " bytes of dedicated RAM reported. More than 8tb of dedicated GPU ram seems dubious");
			}
#ifdef _MSC_VER
#pragma warning(pop)
#endif
		}
	}

	return true;
}

} // namespace cobalt::graphics::testing
