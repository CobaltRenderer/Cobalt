// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <Cobalt/Debug/Debug.pkg>
#include <Cobalt/Logging/Logging.pkg>
#include <Internal/ShaderSupport/ShaderSupport.pkg>
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <cwctype>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#ifdef _WIN32
#include <Internal/CrashHandling/CrashHandling.pkg>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#ifdef __APPLE__
#import <Foundation/Foundation.h>
#endif
#include <csignal>
#include <cstring>
#include <execinfo.h>
#endif
using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------------------------------------
#ifdef _WIN32
static std::string UTF16ToUTF8(const std::wstring& stringUTF16)
{
	// Convert the encoding of the supplied string
	std::string stringUTF8;
	size_t sourceStringPos = 0;
	size_t sourceStringSize = stringUTF16.size();
	stringUTF8.reserve(sourceStringSize * 2);
	while (sourceStringPos < sourceStringSize)
	{
		// Check if a surrogate pair is used for this character
		bool usesSurrogatePair = (((unsigned int)stringUTF16[sourceStringPos] & 0xF800) == 0xD800);

		// Ensure that the requested number of code units are left in the source string
		if (usesSurrogatePair && ((sourceStringPos + 2) > sourceStringSize))
		{
			break;
		}

		// Decode the character from UTF-16 encoding
		unsigned int unicodeCodePoint;
		if (usesSurrogatePair)
		{
			unicodeCodePoint = 0x10000 + ((((unsigned int)stringUTF16[sourceStringPos] & 0x03FF) << 10) | ((unsigned int)stringUTF16[sourceStringPos + 1] & 0x03FF));
		}
		else
		{
			unicodeCodePoint = (unsigned int)stringUTF16[sourceStringPos];
		}

		// Encode the character into UTF-8 encoding
		if (unicodeCodePoint <= 0x7F)
		{
			stringUTF8.push_back((char)unicodeCodePoint);
		}
		else if (unicodeCodePoint <= 0x07FF)
		{
			char convertedCodeUnit1 = (char)(0xC0 | (unicodeCodePoint >> 6));
			char convertedCodeUnit2 = (char)(0x80 | (unicodeCodePoint & 0x3F));
			stringUTF8.push_back(convertedCodeUnit1);
			stringUTF8.push_back(convertedCodeUnit2);
		}
		else if (unicodeCodePoint <= 0xFFFF)
		{
			char convertedCodeUnit1 = (char)(0xE0 | (unicodeCodePoint >> 12));
			char convertedCodeUnit2 = (char)(0x80 | ((unicodeCodePoint >> 6) & 0x3F));
			char convertedCodeUnit3 = (char)(0x80 | (unicodeCodePoint & 0x3F));
			stringUTF8.push_back(convertedCodeUnit1);
			stringUTF8.push_back(convertedCodeUnit2);
			stringUTF8.push_back(convertedCodeUnit3);
		}
		else
		{
			char convertedCodeUnit1 = (char)(0xF0 | (unicodeCodePoint >> 18));
			char convertedCodeUnit2 = (char)(0x80 | ((unicodeCodePoint >> 12) & 0x3F));
			char convertedCodeUnit3 = (char)(0x80 | ((unicodeCodePoint >> 6) & 0x3F));
			char convertedCodeUnit4 = (char)(0x80 | (unicodeCodePoint & 0x3F));
			stringUTF8.push_back(convertedCodeUnit1);
			stringUTF8.push_back(convertedCodeUnit2);
			stringUTF8.push_back(convertedCodeUnit3);
			stringUTF8.push_back(convertedCodeUnit4);
		}

		// Advance past the converted code units
		sourceStringPos += (usesSurrogatePair) ? 2 : 1;
	}

	// Return the converted string to the caller
	return stringUTF8;
}

//----------------------------------------------------------------------------------------
static std::vector<std::string> TokenizeCommandLine(const std::string& commandLine, bool stripFirstArgument = false, int literalRemainderAfterArg = -1)
{
	// Strip off leading whitespace from the start of the command line string
	std::string::const_iterator commandLineIterator = commandLine.begin();
	while ((commandLineIterator != commandLine.end()) && (iswspace(*commandLineIterator) != 0))
	{
		++commandLineIterator;
	}

	// Tokenize the command line string into a set of arguments, taking into account the correct rules for quotation and
	// escape characters.
	std::vector<std::string> arguments;
	while ((commandLineIterator != commandLine.end()) && ((literalRemainderAfterArg < 0) || ((int)arguments.size() < literalRemainderAfterArg)))
	{
		std::string argument;
		int backslashCount = 0;
		bool whitespacePreservationActive = false;
		bool foundEndOfArgument = false;
		while (!foundEndOfArgument && (commandLineIterator != commandLine.end()))
		{
			// Retrieve the next character from the command line string
			auto character = *commandLineIterator++;
			bool characterIsBackslash = (character == '\\');
			bool characterIsDoubleQuote = (character == '\"');

			// If the character is a backslash, increment the current backslash count and advance to the next character.
			if (characterIsBackslash)
			{
				++backslashCount;
				continue;
			}

			// Process backslashes in the command line. The way they are processed is very strange, but we mimic the
			// standard Windows behaviour here. The rules are basically this: backslashes are preserved and passed
			// through as-is, unless a double quote character is found directly following one or more backslashes, in
			// which case all preceding backslashes in the block become escape characters, so each consecutive pair of
			// backslashes emits one backslash. If the double quote itself is escaped by an unmatched (odd) number of
			// backslashes preceding it, it is passed through literally as part of the argument. If the double quote is
			// preceded by an even number of backslashes however, it is active in either enabling or disabling the
			// whitespace escape mode for the current argument.
			if (backslashCount > 0)
			{
				// Insert the required number of leading backslashes before the current character
				int backslashesToInsert = (characterIsDoubleQuote ? backslashCount / 2 : backslashCount);
				bool escapingDoubleQuote = characterIsDoubleQuote && ((backslashCount % 2) != 0);
				for (int i = 0; i < backslashesToInsert; ++i)
				{
					argument.push_back('\\');
				}
				backslashCount = 0;

				// If we're escaping a double quote, insert the character into the output argument and advance to the
				// next character.
				if (escapingDoubleQuote)
				{
					argument.push_back('\"');
					continue;
				}
			}

			// If we've encountered an unescaped double quote, toggle whitespace preservation mode, and advance to the
			// next character.
			if (characterIsDoubleQuote)
			{
				whitespacePreservationActive = !whitespacePreservationActive;
				continue;
			}

			// If whitespace preservation isn't active and we've encountered a whitespace character, flag that we've
			// reached the end of the current argument, and stop processing subsequent characters.
			if (!whitespacePreservationActive && (iswspace(character) != 0))
			{
				foundEndOfArgument = true;
				continue;
			}

			// Add this character to the current output argument
			argument.push_back(character);
		}

		// If the argument ended with one or more backslashes, append them to the parsed argument now. This case will
		// only occur here where the entire command line string ends with one or more backslashes.
		if (backslashCount > 0)
		{
			for (int i = 0; i < backslashCount; ++i)
			{
				argument.push_back('\\');
			}
		}

		// Add this separated argument to the set of arguments. Note that it is possible for this to be an empty string,
		// in particular if a pair of double quotes is followed by a whitespace character.
		arguments.push_back(argument);

		// Strip any trailing whitespace after this argument. This will either take us to the end of the command line or
		// the start of a new argument.
		while ((commandLineIterator != commandLine.end()) && (iswspace(*commandLineIterator) != 0))
		{
			++commandLineIterator;
		}
	}

	// If the literal remainder of the command line has been requested after a specified number of arguments are
	// extracted, and we reached the target argument count, append the remainder of the command line as an additional
	// argument without any processing.
	if ((literalRemainderAfterArg >= 0) && ((int)arguments.size() == literalRemainderAfterArg))
	{
		std::string commandLineRemainder;
		while (commandLineIterator != commandLine.end())
		{
			auto character = *commandLineIterator++;
			commandLineRemainder.push_back(character);
		}
		arguments.push_back(commandLineRemainder);
	}

	// If we've been requested to strip the first argument and at least one argument was found, remove the leading
	// argument now.
	if (stripFirstArgument && !arguments.empty())
	{
		arguments.erase(arguments.begin());
	}

	// Return the tokenized set of command line arguments to the caller
	return arguments;
}
#endif

//----------------------------------------------------------------------------------------
static cobalt::logging::ILogger* UnhandledExceptionLogger = nullptr;
#ifdef _WIN32
static LONG WINAPI UnhandledExceptionHandler(LPEXCEPTION_POINTERS ep)
{
	// Log an error about this exception
	if (UnhandledExceptionLogger != nullptr)
	{
		UnhandledExceptionLogger->Critical("Unhandled exception:\n{0}", cobalt::debug::CrashHandler::GenerateExceptionReport(ep));
	}

	// Bypass the standard windows crash report dialog, and run the exception handler, which will terminate the process.
	return EXCEPTION_EXECUTE_HANDLER;
}

#else
#ifdef __APPLE__
//----------------------------------------------------------------------------------------
static void UnhandledExceptionHandler(NSException* ex)
{
	// Log an error about this exception
	if (UnhandledExceptionLogger != nullptr)
	{
		std::string exceptionReport;
		if (ex.name != nullptr)
		{
			exceptionReport += "name   = " + std::string([ex.name UTF8String]) + "\n";
		}
		if (ex.reason != nullptr)
		{
			exceptionReport += "reason = " + std::string([ex.reason UTF8String]) + "\n";
		}
		if ((ex.userInfo != nullptr) && (ex.userInfo.description != nullptr))
		{
			exceptionReport += "info   = " + std::string([ex.userInfo.description UTF8String]) + "\n";
		}
		if (ex.callStackSymbols != nullptr)
		{
			exceptionReport += "stack  = " + std::string([[ex.callStackSymbols componentsJoinedByString:@"\n"] UTF8String]) + "\n";
		}
		UnhandledExceptionLogger->Critical("Unhandled exception:\n{0}", exceptionReport);
	}
}
#endif

//----------------------------------------------------------------------------------------
static std::string GenerateStackTrace(int skipFrames = 0)
{
	constexpr int MaxFrames = 64;
	void* frames[MaxFrames];

	int frameCount = backtrace(&frames[0], MaxFrames);
	char** symbols = backtrace_symbols(&frames[0], frameCount);

	std::ostringstream oss;
	for (int i = skipFrames; i < frameCount; ++i)
	{
		oss << symbols[i] << '\n';
	}

	std::free(symbols); // NOLINT(cppcoreguidelines-no-malloc,hicpp-no-malloc)
	return oss.str();
}

//----------------------------------------------------------------------------------------
static void FatalSignalHandler(int sig, siginfo_t* info, void* /*context*/)
{
	if (UnhandledExceptionLogger != nullptr)
	{
		std::string report;

		report += "signal = ";
		report += strsignal(sig);
		report += "\n";

		if (sig == SIGSEGV)
		{
			report += "fault address = ";
			report += std::to_string(reinterpret_cast<uintptr_t>(info->si_addr));
			report += "\n";
		}

		report += "stack  = \n";
		report += GenerateStackTrace(2);

		UnhandledExceptionLogger->Critical("Unhandled exception:\n{0}", report);
	}

	// Restore default handler and re-raise to get proper exit code / core dump
	(void)signal(sig, SIG_DFL);
	(void)raise(sig);
}

//----------------------------------------------------------------------------------------
static void InstallSignalHandlers()
{
	struct sigaction sa = {};
	sa.sa_sigaction = FatalSignalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO | SA_RESETHAND;

	sigaction(SIGSEGV, &sa, nullptr);
	sigaction(SIGABRT, &sa, nullptr);
	sigaction(SIGFPE, &sa, nullptr);
	sigaction(SIGILL, &sa, nullptr);
	sigaction(SIGBUS, &sa, nullptr);
}

//----------------------------------------------------------------------------------------
static void UnhandledTerminateHandler()
{
	if (UnhandledExceptionLogger != nullptr)
	{
		std::string report = "Unhandled C++ exception\n";

		if (auto eptr = std::current_exception())
		{
			try
			{
				std::rethrow_exception(eptr);
			}
			catch (const std::exception& ex)
			{
				report += std::string("what() = ") + ex.what() + "\n";
			}
			catch (...)
			{
				report += "what() = <non-std exception>\n";
			}
		}

		report += "stack  = \n";
		report += GenerateStackTrace(2);

		UnhandledExceptionLogger->Critical("Unhandled exception:\n{0}", report);
	}

	std::_Exit(EXIT_FAILURE); // async-signal-safe termination
}
#endif

//----------------------------------------------------------------------------------------
struct ShaderBlock
{
	IShaderCode::Language inputLanguage = {};
	IShaderCode::Language outputLanguage = {};
	IShaderCode::Environment inputEnvironment = {};
	IShaderCode::Environment outputEnvironment = {};
	IShaderCode::Stage shaderStage = {};
	std::string shaderStageAsString;
	std::string entryPointName;
	IShaderCode::unique_ptr shaderCode;
	cobalt::logging::Logger::unique_ptr log;
	std::vector<uint8_t> inputCodeBlock;
	std::vector<uint8_t> outputCodeBlock;
	std::filesystem::path inputFilePath;
	std::filesystem::path outputFilePath;
};

//----------------------------------------------------------------------------------------
static std::string ShaderStageToString(IShaderCode::Stage stage)
{
	switch (stage)
	{
	case IShaderCode::Stage::Vertex:
		return "Vertex";
	case IShaderCode::Stage::Fragment:
		return "Fragment";
	case IShaderCode::Stage::Geometry:
		return "Geometry";
	case IShaderCode::Stage::Compute:
		return "Compute";
	}
	return "Unknown";
}

//----------------------------------------------------------------------------------------
static bool ShaderStageFromString(const std::string& stageAsString, IShaderCode::Stage& stage)
{
	if (stageAsString == "Vertex")
	{
		stage = IShaderCode::Stage::Vertex;
		return true;
	}
	if (stageAsString == "Fragment")
	{
		stage = IShaderCode::Stage::Fragment;
		return true;
	}
	if (stageAsString == "Geometry")
	{
		stage = IShaderCode::Stage::Geometry;
		return true;
	}
	if (stageAsString == "Compute")
	{
		stage = IShaderCode::Stage::Compute;
		return true;
	}
	stage = IShaderCode::Stage::Vertex;
	return false;
}

//----------------------------------------------------------------------------------------
static bool ShaderLanguageFromString(const std::string& languageAsString, IShaderCode::Language& language)
{
	if (languageAsString == "HLSL")
	{
		language = IShaderCode::Language::HLSL;
		return true;
	}
	if (languageAsString == "GLSL")
	{
		language = IShaderCode::Language::GLSL;
		return true;
	}
	if (languageAsString == "MSL")
	{
		language = IShaderCode::Language::MSL;
		return true;
	}
	if (languageAsString == "SPIRV")
	{
		language = IShaderCode::Language::SPIRV;
		return true;
	}
	if (languageAsString == "SPIRVAssembly")
	{
		language = IShaderCode::Language::SPIRVAssembly;
		return true;
	}
	language = IShaderCode::Language::HLSL;
	return false;
}

//----------------------------------------------------------------------------------------
static bool ShaderEnvironmentFromString(const std::string& environmentAsString, IShaderCode::Environment& environment)
{
	if (environmentAsString == "OpenGL3.3")
	{
		environment = IShaderCode::Environment::OpenGL_33;
		return true;
	}
	if (environmentAsString == "OpenGL4.3")
	{
		environment = IShaderCode::Environment::OpenGL_43;
		return true;
	}
	if (environmentAsString == "Direct3D11")
	{
		environment = IShaderCode::Environment::General;
		return true;
	}
	if (environmentAsString == "Direct3D12")
	{
		environment = IShaderCode::Environment::General;
		return true;
	}
	if (environmentAsString == "Vulkan1.1")
	{
		environment = IShaderCode::Environment::Vulkan_11;
		return true;
	}
	environment = IShaderCode::Environment::General;
	return false;
}

//----------------------------------------------------------------------------------------
static bool ParseVersionString(const std::string& versionString, unsigned int& majorVersion, unsigned int& minorVersion)
{
	// Extract the major version component
	const char* versionStringEnd = versionString.data() + versionString.size();
	const char* majorVersionStart = versionString.data();
	auto majorVersionExtractResult = std::from_chars(majorVersionStart, versionStringEnd, majorVersion, 10);
	if ((majorVersionExtractResult.ec != std::errc()) || (majorVersionExtractResult.ptr == majorVersionStart))
	{
		return false;
	}

	// Ensure the period separator between the major and minor version component is present, and that a minor version
	// number follows.
	if ((majorVersionExtractResult.ptr == versionStringEnd) || (*majorVersionExtractResult.ptr != '.') || ((majorVersionExtractResult.ptr + 1) == versionStringEnd))
	{
		return false;
	}

	// Extract the minor version component
	const char* minorVersionStart = majorVersionExtractResult.ptr + 1;
	auto minorVersionExtractResult = std::from_chars(minorVersionStart, versionStringEnd, minorVersion, 10);
	return (minorVersionExtractResult.ec == std::errc()) && (minorVersionExtractResult.ptr == versionStringEnd);
}

//----------------------------------------------------------------------------------------
int main(int argc, const char* argv[])
{
	// Create the log manager
	cobalt::logging::LogManager logManager;
	logManager.SetIncludeSeverity(cobalt::logging::ILogger::SeverityFilter::All);
	auto log = logManager.GetLogger("");

	// Create a console window log target
	auto consoleLogTarget = cobalt::logging::LogTargetStandardOut::Create(true);
	logManager.AddLogTarget(std::move(consoleLogTarget));

	// Register handlers to log unhandled exceptions
	UnhandledExceptionLogger = log.get();
#ifdef _WIN32
	SetUnhandledExceptionFilter(UnhandledExceptionHandler);
#else
#if defined(__APPLE__)
	NSSetUncaughtExceptionHandler(UnhandledExceptionHandler);
#endif
	std::set_terminate(UnhandledTerminateHandler);
	InstallSignalHandlers();
#endif

	// Retrieve and tokenize the command line arguments
#ifdef _WIN32
	std::string originalCommandLine = UTF16ToUTF8(GetCommandLineW());
	std::vector<std::string> commandLineArguments = TokenizeCommandLine(originalCommandLine, true);
#else
	std::vector<std::string> commandLineArguments(argv + 1, argv + argc);
#endif

	// Parse our command line arguments
	IShaderCode::Language inputLanguageNext = IShaderCode::Language::HLSL;
	IShaderCode::Language outputLanguageNext = IShaderCode::Language::HLSL;
	IShaderCode::Environment inputEnvironmentNext = IShaderCode::Environment::General;
	IShaderCode::Environment outputEnvironmentNext = IShaderCode::Environment::General;
	std::unordered_map<IShaderCode::Stage, ShaderBlock> shaderBlocks;
	bool showHelp = commandLineArguments.empty();
	bool performShaderValidation = false;
	unsigned int hlslShaderModelMajor = 5;
	unsigned int hlslShaderModelMinor = 1;
	std::vector<std::string> switchPrefixes = {"/", "--", "-"};
	auto stringStartsWith = [](const std::string& str, const std::string& prefix) { return (str.size() >= prefix.size()) && (str.compare(0, prefix.size(), prefix) == 0); };
	auto stringToLower = [](const std::string& str) { auto stringConverted = str; std::transform(stringConverted.begin(), stringConverted.end(), stringConverted.begin(), [](char c) { return (char)std::tolower((unsigned char)c); }); return stringConverted; };
	size_t nextArgumentIndex = 0;
	while (nextArgumentIndex < commandLineArguments.size())
	{
		// Determine if the next command line parameter is a switch or a parameter.
		auto argument = commandLineArguments[nextArgumentIndex++];
		bool argumentIsSwitch = false;
		for (const auto& prefix : switchPrefixes)
		{
			if (stringStartsWith(argument, prefix))
			{
				argumentIsSwitch = true;
				argument = argument.substr(prefix.size());
				break;
			}
		}

		// Process the next command line argument
		auto remainingArgumentCount = commandLineArguments.size() - nextArgumentIndex;
		if (argumentIsSwitch)
		{
			bool missingRequiredArguments = false;
			size_t requiredArgumentCount = 0;
			auto argumentLowerCase = stringToLower(argument);
			if ((argumentLowerCase == "help") || (argumentLowerCase == "h") || (argumentLowerCase == "?"))
			{
				showHelp = true;
			}
			else if ((argumentLowerCase == "v") || (argumentLowerCase == "validate"))
			{
				performShaderValidation = true;
			}
			else if ((argumentLowerCase == "f") || (argumentLowerCase == "format"))
			{
				requiredArgumentCount = 2;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					auto inputLanguageNextAsString = commandLineArguments[nextArgumentIndex++];
					auto outputLanguageNextAsString = commandLineArguments[nextArgumentIndex++];
					if (!ShaderLanguageFromString(inputLanguageNextAsString, inputLanguageNext))
					{
						log->Error("Unrecognized code format \"{0}\" specified", inputLanguageNextAsString);
					}
					else if (!ShaderLanguageFromString(outputLanguageNextAsString, outputLanguageNext))
					{
						log->Error("Unrecognized code format \"{0}\" specified", outputLanguageNextAsString);
					}
				}
			}
			else if ((argumentLowerCase == "p") || (argumentLowerCase == "platform"))
			{
				requiredArgumentCount = 2;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					auto inputEnvironmentNextAsString = commandLineArguments[nextArgumentIndex++];
					auto outputEnvironmentNextAsString = commandLineArguments[nextArgumentIndex++];
					if (!ShaderEnvironmentFromString(inputEnvironmentNextAsString, inputEnvironmentNext))
					{
						log->Error("Unrecognized environment \"{0}\" specified", inputEnvironmentNextAsString);
					}
					if (!ShaderEnvironmentFromString(outputEnvironmentNextAsString, outputEnvironmentNext))
					{
						log->Error("Unrecognized environment \"{0}\" specified", outputEnvironmentNextAsString);
					}
				}
			}
			else if ((argumentLowerCase == "s") || (argumentLowerCase == "stage"))
			{
				requiredArgumentCount = 3;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					auto shaderStageAsString = commandLineArguments[nextArgumentIndex++];
					std::filesystem::path inputFilePath = commandLineArguments[nextArgumentIndex++];
					std::filesystem::path outputFilePath = commandLineArguments[nextArgumentIndex++];

					// If the next argument is the optional entry point name, extract it now.
					std::string entryPointName = IShaderCode::StandardShaderEntryPointName;
					const auto& nextArgument = commandLineArguments[nextArgumentIndex++];
					bool nextArgumentIsSwitch = false;
					for (const auto& prefix : switchPrefixes)
					{
						if (stringStartsWith(nextArgument, prefix))
						{
							nextArgumentIsSwitch = true;
							break;
						}
					}
					if (!nextArgumentIsSwitch)
					{
						entryPointName = commandLineArguments[nextArgumentIndex++];
					}

					IShaderCode::Stage shaderStage;
					if (!ShaderStageFromString(shaderStageAsString, shaderStage))
					{
						log->Error("Unrecognized shader stage \"{0}\" specified", shaderStageAsString);
					}
					else if (shaderBlocks.find(shaderStage) != shaderBlocks.end())
					{
						log->Warning("Ignored second specification for shader stage {0}", shaderStageAsString);
					}
					else
					{
						ShaderBlock shaderBlock = {};
						shaderBlock.inputLanguage = inputLanguageNext;
						shaderBlock.outputLanguage = outputLanguageNext;
						shaderBlock.inputEnvironment = inputEnvironmentNext;
						shaderBlock.outputEnvironment = outputEnvironmentNext;
						shaderBlock.shaderStage = shaderStage;
						shaderBlock.shaderStageAsString = ShaderStageToString(shaderStage);
						shaderBlock.entryPointName = entryPointName;
						shaderBlock.inputFilePath = inputFilePath;
						shaderBlock.outputFilePath = outputFilePath;
						shaderBlocks[shaderStage] = std::move(shaderBlock);
					}
				}
			}
			else if (argumentLowerCase == "shadermodel")
			{
				requiredArgumentCount = 1;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					auto shaderModelAsString = commandLineArguments[nextArgumentIndex++];
					if (!ParseVersionString(shaderModelAsString, hlslShaderModelMajor, hlslShaderModelMinor))
					{
						log->Error("Unable to parse shader model version string \"{0}\"", shaderModelAsString);
					}
				}
			}
			else
			{
				log->Warning("Unrecognized command line option {0}", argument);
			}
			if (missingRequiredArguments)
			{
				log->Error("Missing required arguments for option \"{0}\". {1} arguments were required, but {2} were provided.", argument, requiredArgumentCount, remainingArgumentCount);
			}
		}
		else
		{
			log->Warning("Unrecognized command line parameter {0}", argument);
		}
	}

	// If command line help has been requested, display it, and abort any further processing.
	if (showHelp)
	{
		std::cout << R"qwerty(Converts shader programs from one format to another. Conversions are performed to manage names and bindings in a suitable form for use in the Cobalt renderer.

ShaderConverter.exe [/V] [/F inputFormat outputFormat] [/P inputPlatform outputPlatform] [/S stage inputFile outputFile [entryPointName]] [/ShaderModel Major.Minor]

  /F    Specifies the input and output formats for the shader stages that follow it. The format must be one of the following:
          HLSL
          GLSL
          MSL
          SPIRV
          SPIRVAssembly
  /P    Specifies the input and output target platform for the shader stages that follow it. The platform must be one of the following:
          OpenGL3.3
          OpenGL4.3
          Direct3D11
          Direct3D12
          Vulkan1.1
  /S    Provides a shader stage to convert, with paths specified to the input and output files. Stage must be one of the following:
          Vertex
          Fragment
          Geometry
          Compute
  /V    Validate supplied shader code
  /ShaderModel  Specify the Direct3D shader model version to target in the form Major.Minor, IE, 5.1, when exporting to HLSL. Default is 5.1.
)qwerty";
		return 0;
	}

	// Sort the shader blocks
	std::unordered_map<IShaderCode::Stage, bool> shaderStagePresent;
	shaderStagePresent[IShaderCode::Stage::Vertex] = (shaderBlocks.find(IShaderCode::Stage::Vertex) != shaderBlocks.end());
	shaderStagePresent[IShaderCode::Stage::Fragment] = (shaderBlocks.find(IShaderCode::Stage::Fragment) != shaderBlocks.end());
	shaderStagePresent[IShaderCode::Stage::Geometry] = (shaderBlocks.find(IShaderCode::Stage::Geometry) != shaderBlocks.end());
	shaderStagePresent[IShaderCode::Stage::Compute] = (shaderBlocks.find(IShaderCode::Stage::Compute) != shaderBlocks.end());
	std::vector<ShaderBlock> shaderBlocksSorted;
	if (shaderStagePresent[IShaderCode::Stage::Vertex])
	{
		shaderBlocksSorted.push_back(std::move(shaderBlocks[IShaderCode::Stage::Vertex]));
	}
	if (shaderStagePresent[IShaderCode::Stage::Fragment])
	{
		shaderBlocksSorted.push_back(std::move(shaderBlocks[IShaderCode::Stage::Fragment]));
	}
	if (shaderStagePresent[IShaderCode::Stage::Geometry])
	{
		shaderBlocksSorted.push_back(std::move(shaderBlocks[IShaderCode::Stage::Geometry]));
	}
	if (shaderStagePresent[IShaderCode::Stage::Compute])
	{
		shaderBlocksSorted.push_back(std::move(shaderBlocks[IShaderCode::Stage::Compute]));
	}
	shaderBlocks.clear();

	// Load the code blocks for each shader stage from their input files
	for (auto& shaderBlock : shaderBlocksSorted)
	{
		log->Info("Loading shader stage {0} from file {1}", shaderBlock.shaderStageAsString, shaderBlock.inputFilePath);
		std::ifstream file;
		file.open(shaderBlock.inputFilePath, std::ios::in | std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			log->Error("Can't open input shader file: {0}", shaderBlock.inputFilePath);
			return 1;
		}
		std::streamsize fileSizeInBytes = file.tellg();
		file.seekg(0, std::ios::beg);
		shaderBlock.inputCodeBlock.resize(static_cast<size_t>(fileSizeInBytes));
		file.read(reinterpret_cast<char*>(shaderBlock.inputCodeBlock.data()), fileSizeInBytes);
		if (file.fail())
		{
			log->Error("Can't open input shader file: {0}", shaderBlock.inputFilePath);
			return 1;
		}
	}

	// Load each shader block into SPIR-V
	bool shaderBlockLoadFailed = false;
	int baseBindingNo = 0;
	std::vector<IShaderCode::Resource> shaderResourceSet;
	for (auto& shaderBlock : shaderBlocksSorted)
	{
		log->Info("Parsing shader stage {0}", shaderBlock.shaderStageAsString);
		shaderBlock.log = log->GetLoggerChildScope("Stage:" + shaderBlock.shaderStageAsString);
		shaderBlock.shaderCode = IShaderCode::Create(shaderBlock.log->CloneLogger());
		if (!shaderBlock.shaderCode->LoadCode(shaderBlock.inputLanguage, shaderBlock.shaderStage, shaderBlock.inputEnvironment, shaderBlock.outputEnvironment, shaderBlock.inputCodeBlock.data(), shaderBlock.inputCodeBlock.size(), shaderBlock.entryPointName, baseBindingNo, shaderResourceSet))
		{
			log->Error("Failed to load the provided code for shader stage {0}", shaderBlock.shaderStageAsString);
			shaderBlockLoadFailed = true;
			continue;
		}
	}
	if (shaderBlockLoadFailed)
	{
		log->Error("Failed to load the provided code for a shader stage");
		return 1;
	}

	// Validate each shader block if requested
	if (performShaderValidation)
	{
		bool shaderValidationFailed = false;
		for (auto& shaderBlock : shaderBlocksSorted)
		{
			log->Info("Validating shader stage {0}", shaderBlock.shaderStageAsString);
			shaderBlock.log = log->GetLoggerChildScope("Stage:" + shaderBlock.shaderStageAsString);
			if (!shaderBlock.shaderCode->ValidateCode())
			{
				log->Error("Failed to validate the provided code for shader stage {0}", shaderBlock.shaderStageAsString);
				shaderValidationFailed = true;
				continue;
			}
		}
		if (shaderValidationFailed)
		{
			log->Error("Shader validation failed");
			return 1;
		}
	}

	// Export each shader block into the requested code format
	bool shaderBlockExportFailed = false;
	for (auto& shaderBlock : shaderBlocksSorted)
	{
		// Export the code to the target language
		log->Info("Converting shader stage {0}", shaderBlock.shaderStageAsString);
		bool result = false;
		std::string outputCodeBlockString;
		std::vector<uint32_t> outputCodeBlockSpirv;
		switch (shaderBlock.outputLanguage)
		{
		case IShaderCode::Language::HLSL:
			result = shaderBlock.shaderCode->ExportCodeAsHLSL(outputCodeBlockString, hlslShaderModelMajor, hlslShaderModelMinor);
			break;
		case IShaderCode::Language::SPIRVAssembly:
			result = shaderBlock.shaderCode->ExportCodeAsSPIRVAssembly(outputCodeBlockString);
			break;
		case IShaderCode::Language::SPIRV:
			result = shaderBlock.shaderCode->ExportCodeAsSPIRV(outputCodeBlockSpirv);
			break;
		case IShaderCode::Language::GLSL:
			result = shaderBlock.shaderCode->ExportCodeAsGLSL(outputCodeBlockString, shaderStagePresent[IShaderCode::Stage::Geometry]);
			break;
		case IShaderCode::Language::MSL:
			result = shaderBlock.shaderCode->ExportCodeAsMSL(outputCodeBlockString);
			break;
		}
		if (!result)
		{
			log->Error("Failed to export the requested code for shader stage {0}", shaderBlock.shaderStageAsString);
			shaderBlockExportFailed = true;
			continue;
		}

		// Transfer the exported code into the output code block
		if (shaderBlock.outputLanguage != IShaderCode::Language::SPIRV)
		{
			shaderBlock.outputCodeBlock.assign(reinterpret_cast<const uint8_t*>(outputCodeBlockString.data()), reinterpret_cast<const uint8_t*>(outputCodeBlockString.data()) + outputCodeBlockString.size());
		}
		else
		{
			shaderBlock.outputCodeBlock.assign(reinterpret_cast<const uint8_t*>(outputCodeBlockSpirv.data()), reinterpret_cast<const uint8_t*>(outputCodeBlockSpirv.data()) + outputCodeBlockSpirv.size());
		}
	}
	if (shaderBlockExportFailed)
	{
		log->Error("Failed to load the provided code for a shader stage");
		return 1;
	}

	// Save each shader block to the target output file
	for (auto& shaderBlock : shaderBlocksSorted)
	{
		log->Info("Saving converted shader stage {0} to file {1}", shaderBlock.shaderStageAsString, shaderBlock.outputFilePath);
		std::ofstream file;
		file.open(shaderBlock.outputFilePath, std::ios::out | std::ios::trunc | std::ios::binary);
		file.write(reinterpret_cast<const char*>(shaderBlock.outputCodeBlock.data()), shaderBlock.outputCodeBlock.size());
		file.close();
		if (file.fail())
		{
			log->Error("Can't create output shader file: {0}", shaderBlock.outputFilePath);
			return 1;
		}
	}
	return 0;
}
