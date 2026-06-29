// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/Marshalling/Marshalling.pkg>
#include <map>
#include <memory>
#include <string>
namespace cobalt::graphics {
using namespace cobalt::marshalling::operators;
class IImageRGBA;

class IImageDiff
{
public:
	// Nested types
	struct Deleter
	{
		inline void operator()(IImageDiff* target)
		{
			target->Delete();
		}
	};
	typedef std::unique_ptr<IImageDiff, Deleter> unique_ptr;

	// Enumerations
	enum class Algorithm;

public:
	// Constructors
	static inline unique_ptr Create(const IImageRGBA& expected, const IImageRGBA& actual);

	// Delete method
	virtual void Delete() = 0;

	// Test methods
	virtual bool CompareImages(Algorithm algorithm, double passThreshold, double& diffValue, const Marshal::Out<std::map<std::string, std::string>>& explanationMetaDataString) const = 0;

protected:
	// Constructors
	inline ~IImageDiff() = default;
};

} // namespace cobalt::graphics
#include "IImageDiff.inl"
