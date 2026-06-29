// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <string>
namespace cobalt::graphics {

inline IGraphicsDevice::Vendor VendorCodeToVendor(uint32_t vendorCode)
{
	// These vendor IDs correspond with PCI vendor codes, which are returned for Vulkan and Direct3D. The authoritative
	// source for these IDs is the PCI-SIG members list, which can be viewed here:
	// https://pcisig.com/membership/member-companies
	// This list however only shows current official codes, and as such may not include codes for previous members which
	// are no longer registered, or codes which were used previously but changed, or cases where an incorrect code was
	// used. As such, we should cross-check high interest vendors (IE, AMD, Intel, NVidia) with other unofficial sources
	// to augment our results. Some examples are as follows:
	// https://pci-ids.ucw.cz/read/PC/
	// https://pcilookup.com/
	// https://www.reddit.com/r/vulkan/comments/4ta9nj/is_there_a_comprehensive_list_of_the_names_and/d5nso2t (vulkan.gpuinfo.org)
	switch (vendorCode)
	{
	case 0x1002: // Specifically listed as a used/seen code for AMD, even though not listed in PCI-SIG.
	case 0x1022:
		return IGraphicsDevice::Vendor::AMD;
	case 0x1010: // Mobile devices, listed on vulkan.gpuinfo.org
		return IGraphicsDevice::Vendor::ImgTec;
	case 0x106B:
		return IGraphicsDevice::Vendor::Apple;
	case 0x10DE:
		return IGraphicsDevice::Vendor::Nvidia;
	case 0x13B5: // Mobile devices, listed on vulkan.gpuinfo.org
		return IGraphicsDevice::Vendor::ARM;
	case 0x1414: // Microsoft WARP driver for Direct3D
		return IGraphicsDevice::Vendor::Microsoft;
	case 0x5143: // Mobile devices, listed on vulkan.gpuinfo.org
	case 0x17CB:
	case 0x4D4F4351:
		return IGraphicsDevice::Vendor::Qualcomm;
	case 0x163C: // Only 0x8086 is officially listed for Intel. Other codes are pulled from unofficial PCI vendor
	case 0x8086: // code lists. Probably only 0x8086 is used in reality, but playing it safe given the long history
	case 0x8087: // of Intel graphics devices.
		return IGraphicsDevice::Vendor::Intel;
	case 0x10001:
		return IGraphicsDevice::Vendor::Vivante;
	case 0x10002:
		return IGraphicsDevice::Vendor::VeriSilicon;
	case 0x10005:
		return IGraphicsDevice::Vendor::Mesa;
	}
	return IGraphicsDevice::Vendor::Unknown;
}

inline std::string VendorToVendorName(IGraphicsDevice::Vendor vendor)
{
	switch (vendor)
	{
	case IGraphicsDevice::Vendor::AMD:
		return "AMD";
	case IGraphicsDevice::Vendor::ImgTec:
		return "ImgTec";
	case IGraphicsDevice::Vendor::Apple:
		return "Apple";
	case IGraphicsDevice::Vendor::Nvidia:
		return "NVIDIA";
	case IGraphicsDevice::Vendor::ARM:
		return "ARM";
	case IGraphicsDevice::Vendor::Microsoft:
		return "Microsoft";
	case IGraphicsDevice::Vendor::Qualcomm:
		return "Qualcomm";
	case IGraphicsDevice::Vendor::Intel:
		return "Intel";
	case IGraphicsDevice::Vendor::Vivante:
		return "Vivante";
	case IGraphicsDevice::Vendor::VeriSilicon:
		return "VeriSilicon";
	case IGraphicsDevice::Vendor::Mesa:
		return "Mesa";
	}
	return "Unknown";
}

} // namespace cobalt::graphics
