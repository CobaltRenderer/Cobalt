// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <Cobalt/CBindings/CBindings.pkg>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PNG_SETJMP_NOT_SUPPORTED
#include <png.h>
// When using the actual SDK, this function declaration would be found by the following include line:
//   #include "Cobalt/Renderers/VulcanRendererFactory.h"
// We predeclare the function manually here since we don't have the same include layout within this repo as we get for
// the SDK.
#ifdef _WIN32
__declspec(dllimport) int GetVulkanRendererPlugin(void);
#else
int GetVulkanRendererPlugin(void);
#endif

// PNG Writing functions
typedef struct
{
	uint8_t* data;
	size_t size;
	size_t capacity;
} PngBuffer;

static void PngWriteCallback(png_structp png_ptr, png_bytep data, png_size_t length)
{
	PngBuffer* buffer = (PngBuffer*)png_get_io_ptr(png_ptr);

	if (buffer->size + length > buffer->capacity)
	{
		size_t newCapacity = buffer->capacity == 0 ? 8192 : buffer->capacity * 2;

		while (newCapacity < buffer->size + length)
		{
			newCapacity *= 2;
		}

		uint8_t* newData = (uint8_t*)realloc(buffer->data, newCapacity);
		if (!newData)
		{
			png_error(png_ptr, "Out of memory");
		}

		buffer->data = newData;
		buffer->capacity = newCapacity;
	}

	memcpy(buffer->data + buffer->size, data, length);
	buffer->size += length;
}

static void PngFlushCallback(png_structp png_ptr)
{
	(void)png_ptr;
}

static uint8_t* ToPngData(
  uint8_t* rawPixels,
  uint32_t width,
  uint32_t height,
  size_t* outPngSize)
{
	if (!rawPixels || width == 0 || height == 0 || !outPngSize)
	{
		return NULL;
	}

	*outPngSize = 0;

	png_structp png = png_create_write_struct(
	  PNG_LIBPNG_VER_STRING,
	  NULL,
	  NULL,
	  NULL);

	if (!png)
	{
		return NULL;
	}

	png_infop info = png_create_info_struct(png);
	if (!info)
	{
		png_destroy_write_struct(&png, NULL);
		return NULL;
	}

	PngBuffer buffer = {0};

	if (setjmp(png_jmpbuf(png)))
	{
		free(buffer.data);
		png_destroy_write_struct(&png, &info);
		return NULL;
	}

	png_set_write_fn(png, &buffer, PngWriteCallback, PngFlushCallback);

	png_set_IHDR(
	  png,
	  info,
	  width,
	  height,
	  8,
	  PNG_COLOR_TYPE_RGBA,
	  PNG_INTERLACE_NONE,
	  PNG_COMPRESSION_TYPE_DEFAULT,
	  PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png, info);

	png_bytep* rows = (png_bytep*)malloc(sizeof(png_bytep) * height);
	if (!rows)
	{
		png_error(png, "Out of memory");
	}

	for (uint32_t y = 0; y < height; ++y)
	{
		rows[y] = rawPixels + (size_t)(y * width * 4);
	}

	png_write_image(png, rows);
	png_write_end(png, info);

	free(rows);
	png_destroy_write_struct(&png, &info);

	*outPngSize = buffer.size;
	return buffer.data;
}

// Cobalt Renderer logging callback
static void LogCallback(Cobalt_LogSeverity severity, const char* scope, size_t scopeLength, const char* message, size_t messageLength)
{
	switch (severity)
	{
	case Cobalt_LogSeverity_Debug:
	{
		printf("DEBUG: %s: %s\n", scope, message);
		break;
	}
	case Cobalt_LogSeverity_Info:
	{
		printf("INFO: %s: %s\n", scope, message);
		break;
	}
	case Cobalt_LogSeverity_Warning:
	{
		printf("WARN: %s: %s\n", scope, message);
		break;
	}
	case Cobalt_LogSeverity_Error:
	{
		printf("ERROR: %s: %s\n", scope, message);
		break;
	}
	case Cobalt_LogSeverity_Critical:
	{
		printf("CRITICAL: %s: %s\n", scope, message);
		break;
	}
	default:
		break;
	}
}

// Simple error handling macro for code brevity
#define REQUIRE(...)                                                          \
	do                                                                        \
	{                                                                         \
		if (!(__VA_ARGS__))                                                   \
		{                                                                     \
			printf("REQUIRE failed at %s:%zu\n", __FILE__, (size_t)__LINE__); \
			exit(1);                                                          \
		}                                                                     \
	} while (0)

static const char SHADER_CODE[] = "\
struct VSInput {\
    float3 position: position;\
    float3 color: color;\
};\
\
struct VSOutput {\
    float4 position: SV_Position;\
    float3 color: COLOR0;\
};\
\
VSOutput vertex(VSInput IN) {\
   VSOutput OUT;\
   OUT.position = float4(IN.position, 1.0);\
   OUT.color = IN.color;\
   return OUT;\
}\
\
float4 fragment(VSOutput IN) : SV_Target0 {\
   return float4(IN.color, 1.0);\
}\
";

int main(void)
{
	// Renderer setup
	Cobalt_Library library = NULL;
	REQUIRE(Cobalt_Initialize(LogCallback, Cobalt_LogSeverityFilter_DebugOrHigher, &library) == COBALT_SUCCESS);

	Cobalt_RendererPlugin plugin = NULL;
	REQUIRE(Cobalt_GetRendererPluginStatic(library, (void*)GetVulkanRendererPlugin, &plugin) == COBALT_SUCCESS);

	Cobalt_GraphicsDeviceEnumerator enumerator = NULL;
	Cobalt_RendererPlugin_CreateGraphicsDeviceEnumerator(plugin, &enumerator);
	REQUIRE(Cobalt_GraphicsDeviceEnumerator_EnumerateDevices(enumerator, Cobalt_DeviceEnumerationFlags_None) == COBALT_SUCCESS);

	Cobalt_GraphicsDevice device = NULL;
	Cobalt_GraphicsDeviceEnumerator_GetPreferredDevice(enumerator, &device);

	char vendorName[256];
	size_t vendorNameLength = 255;
	Cobalt_GraphicsDevice_GetVendorName(device, vendorName, &vendorNameLength);
	vendorNameLength = vendorNameLength < 256 ? vendorNameLength : 255;
	vendorName[vendorNameLength] = '\0';

	char deviceName[256];
	size_t deviceNameLength = 255;
	Cobalt_GraphicsDevice_GetDeviceName(device, deviceName, &deviceNameLength);
	deviceNameLength = deviceNameLength < 256 ? deviceNameLength : 255;
	deviceName[deviceNameLength] = '\0';

	char driverInfo[256];
	size_t driverInfoLength = 255;
	Cobalt_GraphicsDevice_GetDriverInfo(device, driverInfo, &driverInfoLength);
	driverInfoLength = driverInfoLength < 256 ? driverInfoLength : 255;
	driverInfo[driverInfoLength] = '\0';

	printf("Preferred Device: %s, %s, %s\n", vendorName, deviceName, driverInfo);

	Cobalt_Renderer renderer = NULL;
	Cobalt_GraphicsDevice_CreateRenderer(device, NULL, 0, NULL, 0, &renderer);

	Cobalt_WindowSystemInfoBase windowInfo = {
	  .type = Cobalt_WindowSystemType_Headless,
	};
	REQUIRE(Cobalt_Renderer_Initialize(renderer, &windowInfo, Cobalt_RendererInitializationFlags_None) == COBALT_SUCCESS);

	// Renderpass setup
	const uint32_t textureSize[2] = {640, 480};
	const uint32_t zeroZero[2] = {0, 0};
	Cobalt_TextureBuffer2D texture = NULL;
	Cobalt_Renderer_CreateTextureBuffer2D(renderer, &texture);
	Cobalt_TextureBuffer2D_SetTextureFormat(texture, Cobalt_ImageFormat_RGBA, Cobalt_DataFormat_UNorm8);
	Cobalt_TextureBuffer2D_SetTextureDimensions(texture, textureSize, 1);
	Cobalt_TextureBuffer_SetUsageFlags((Cobalt_TextureBuffer)texture, Cobalt_TextureUsageFlags_FrameBufferOutput);
	REQUIRE(Cobalt_TextureBuffer2D_AllocateMemory(texture) == COBALT_SUCCESS);

	Cobalt_FrameBufferOutput output = NULL;
	Cobalt_Renderer_CreateFrameBufferOutput(renderer, &output);

	Cobalt_FrameBuffer frameBuffer = NULL;
	Cobalt_Renderer_CreateFrameBuffer(renderer, &frameBuffer);
	REQUIRE(Cobalt_FrameBuffer_BindTexture(frameBuffer, texture, Cobalt_AttachmentType_Color, 0) == COBALT_SUCCESS);
	Cobalt_FrameBuffer_AddOutputCaptureTarget(frameBuffer, output, Cobalt_AttachmentType_Color, 0);
	Cobalt_FrameBuffer_DefineViewportRegion(frameBuffer, zeroZero, textureSize);

	const float clearColor[4] = {0.5f, 0.5f, 0.5f, 1.0f};
	Cobalt_RenderPassNode renderPass = NULL;
	Cobalt_Renderer_CreateRenderPassNode(renderer, &renderPass);
	Cobalt_RenderPassNode_SetAttachmentClearDataF(renderPass, Cobalt_AttachmentType_Color, 0, clearColor);
	Cobalt_RenderPassNode_BindFrameBuffer(renderPass, frameBuffer);

	// Progam setup
	Cobalt_ShaderProgram shader = NULL;
	Cobalt_Renderer_CreateShaderProgram(renderer, &shader);
	Cobalt_ShaderSourceInfoHLSL vertexInfo = {
	  .base = {
	    .type = Cobalt_ShaderSourceInfoType_HLSL,
	  },
	  .code = SHADER_CODE,
	  .codeSizeInBytes = sizeof(SHADER_CODE),
	  .entryPointFunctionName = "vertex",
	  .entryPointFunctionNameSizeInBytes = 6,
	};
	REQUIRE(Cobalt_ShaderProgram_LoadShaderStage(shader, Cobalt_ShaderStage_Vertex, (Cobalt_ShaderSourceInfoBase*)&vertexInfo) == COBALT_SUCCESS);

	Cobalt_ShaderSourceInfoHLSL fragmentInfo = {
	  .base = {
	    .type = Cobalt_ShaderSourceInfoType_HLSL,
	  },
	  .code = SHADER_CODE,
	  .codeSizeInBytes = sizeof(SHADER_CODE),
	  .entryPointFunctionName = "fragment",
	  .entryPointFunctionNameSizeInBytes = 8,
	};
	REQUIRE(Cobalt_ShaderProgram_LoadShaderStage(shader, Cobalt_ShaderStage_Fragment, (Cobalt_ShaderSourceInfoBase*)&fragmentInfo) == COBALT_SUCCESS);
	REQUIRE(Cobalt_ShaderProgram_CompileProgram(shader) == COBALT_SUCCESS);

	Cobalt_ProgramNode program = NULL;
	Cobalt_Renderer_CreateProgramNode(renderer, &program);
	REQUIRE(Cobalt_ProgramNode_BindShaderProgram(program, shader) == COBALT_SUCCESS);

	// Group setup
	Cobalt_StateGroupNode group = NULL;
	Cobalt_Renderer_CreateStateGroupNode(renderer, &group);

	// Renderable setup
	const float positions[3][3] = {
	  {-0.7f, -0.7f, 0.5f},
	  {0.7f, -0.7f, 0.5f},
	  {0.0f, 0.7f, 0.5f},
	};
	const float colors[3][3] = {
	  {1.0f, 0.0f, 0.0f},
	  {0.0f, 1.0f, 0.0f},
	  {0.0f, 0.0f, 1.0f},
	};

	Cobalt_VertexAttribute positionAttribute = NULL;
	Cobalt_VertexAttribute colorAttribute = NULL;
	Cobalt_VertexPerformanceHint cpuHint = (Cobalt_VertexPerformanceHint)((int)Cobalt_VertexPerformanceHint_ReadNever | (int)Cobalt_VertexPerformanceHint_WriteNever);
	Cobalt_VertexPerformanceHint gpuHint = (Cobalt_VertexPerformanceHint)((int)Cobalt_VertexPerformanceHint_ReadOften | (int)Cobalt_VertexPerformanceHint_WriteNever);
	Cobalt_Renderer_CreateVertexAttribute(renderer, &positionAttribute, Cobalt_VertexAttributeType_Float32, 3, 3, cpuHint, gpuHint, Cobalt_VertexDataPersistenceFlags_PersistAlways);
	Cobalt_Renderer_CreateVertexAttribute(renderer, &colorAttribute, Cobalt_VertexAttributeType_Float32, 3, 3, cpuHint, gpuHint, Cobalt_VertexDataPersistenceFlags_PersistAlways);

	Cobalt_VertexBuffer vertexBuffer = NULL;
	Cobalt_Renderer_CreateVertexBuffer(renderer, &vertexBuffer);
	REQUIRE(Cobalt_VertexBuffer_BindVertexAttribute(vertexBuffer, positionAttribute) == COBALT_SUCCESS);
	REQUIRE(Cobalt_VertexBuffer_BindVertexAttribute(vertexBuffer, colorAttribute) == COBALT_SUCCESS);
	REQUIRE(Cobalt_VertexAttribute_SetInitialData(positionAttribute, (const uint8_t*)positions, 3, 12) == COBALT_SUCCESS);
	REQUIRE(Cobalt_VertexAttribute_SetInitialData(colorAttribute, (const uint8_t*)colors, 3, 12) == COBALT_SUCCESS);
	REQUIRE(Cobalt_VertexBuffer_AllocateMemory(vertexBuffer) == COBALT_SUCCESS);

	Cobalt_RenderableNode renderable = NULL;
	Cobalt_Renderer_CreateRenderableNode(renderer, &renderable);
	REQUIRE(Cobalt_RenderableNode_SetPrimitiveMode(renderable, Cobalt_PrimitiveMode_Triangles, 0, 0) == COBALT_SUCCESS);
	REQUIRE(Cobalt_RenderableNode_SetVertexCount(renderable, 3, 0, 0, 0) == COBALT_SUCCESS);

	int32_t positionId = Cobalt_ShaderProgram_GetVertexAttributeId(shader, "position", 8);
	int32_t colorId = Cobalt_ShaderProgram_GetVertexAttributeId(shader, "color", 5);

	REQUIRE(Cobalt_RenderableNode_BindVertexAttribute(renderable, positionAttribute, positionId) == COBALT_SUCCESS);
	REQUIRE(Cobalt_RenderableNode_BindVertexAttribute(renderable, colorAttribute, colorId) == COBALT_SUCCESS);

	// Render
	Cobalt_StateGroupNode_AddChildNode(group, renderable);
	Cobalt_ProgramNode_AddChildNode(program, group);
	Cobalt_RenderPassNode_AddChildNode(renderPass, program, NULL);
	int32_t order = 0;
	Cobalt_Renderer_SetRenderPasses(renderer, &renderPass, 1, &order);

	Cobalt_Renderer_StartNewFrame(renderer);
	Cobalt_Renderer_WaitForOutputCaptureComplete(renderer);

	size_t imageBufferLength = (size_t)textureSize[0] * (size_t)textureSize[1] * 4;
	uint8_t* imageBuffer = malloc(imageBufferLength);
	REQUIRE(Cobalt_FrameBufferOutput_ReadBufferData(output, imageBuffer, imageBufferLength, Cobalt_SourceImageFormat_RGBA, Cobalt_SourceDataFormat_UNorm8, zeroZero, textureSize) == COBALT_SUCCESS);

	size_t pngDataLength = 0;
	uint8_t* pngData = ToPngData(imageBuffer, textureSize[0], textureSize[1], &pngDataLength);

	FILE* file = fopen("./out.png", "wb");
	REQUIRE(file != NULL);
	REQUIRE(fwrite(pngData, 1, pngDataLength, file) == pngDataLength);
	REQUIRE(fclose(file) == 0);

	// Cleanup
	free(imageBuffer);
	free(pngData);
	Cobalt_VertexBuffer_Delete(vertexBuffer);
	Cobalt_VertexAttribute_Delete(positionAttribute);
	Cobalt_VertexAttribute_Delete(colorAttribute);
	Cobalt_RenderableNode_Delete(renderable);
	Cobalt_StateGroupNode_Delete(group);
	Cobalt_ShaderProgram_Delete(shader);
	Cobalt_ProgramNode_Delete(program);
	Cobalt_FrameBuffer_Delete(frameBuffer);
	Cobalt_TextureBuffer2D_Delete(texture);
	Cobalt_FrameBufferOutput_Delete(output);
	Cobalt_RenderPassNode_Delete(renderPass);
	Cobalt_Renderer_Delete(renderer);
	Cobalt_GraphicsDeviceEnumerator_Delete(enumerator);
	Cobalt_Terminate(library);

	printf("Wrote 'out.png' in current working directory\n");

	return 0;
}
