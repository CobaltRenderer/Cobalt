@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SourceDir=."
if not "%~1"=="" (
	set "SourceDir=%~1"
	shift /1
)
set "OutputDir=TestResults"
if not "%~1"=="" (
	set "OutputDir=%~1"
	shift /1
)
set "AdditionalArgs="
:CollectAdditionalArgs
if "%~1"=="" goto AdditionalArgsCollected
if defined AdditionalArgs (
	set "AdditionalArgs=!AdditionalArgs! %1"
) else (
	set "AdditionalArgs=%1"
)
shift /1
goto CollectAdditionalArgs
:AdditionalArgsCollected
md "%OutputDir%" >nul 2>nul
del /F /S /Q "%OutputDir%\*" >nul 2>nul
set "PassFailDir=%OutputDir%\PassFail"
md "%PassFailDir%" >nul 2>nul
set "ScriptFailed=0"

call :RunTest Direct3D11_Discrete --use-discrete -e "Direct3D11_1" --no-prompt --html-report "%OutputDir%\index.html" --append-multi-report "Direct3D11 Discrete" --performance-all -k
call :RunTest Direct3D11_Integrated --use-integrated -e "Direct3D11_1" --no-prompt --html-report "%OutputDir%\index.html" --append-multi-report "Direct3D11 Integrated" --performance-all -k
call :RunTest Direct3D11_Software --use-software -e "Direct3D11_1" --no-prompt --html-report "%OutputDir%\index.html" --append-multi-report "Direct3D11 Software" --performance-all -k
call :RunTest Direct3D12_Discrete --use-discrete -e "Direct3D12_0" --no-prompt --html-report "%OutputDir%\index.html" --append-multi-report "Direct3D12 Discrete" --performance-all -k
call :RunTest Direct3D12_Integrated --use-integrated -e "Direct3D12_0" --no-prompt --html-report "%OutputDir%\index.html" --append-multi-report "Direct3D12 Integrated" --performance-all -k
call :RunTest Direct3D12_Software --use-software -e "Direct3D12_0" --no-prompt --html-report "%OutputDir%\index.html" --append-multi-report "Direct3D12 Software" --performance-all -k
call :RunTest OpenGL33_Discrete --use-discrete -e "OpenGL3_3" --no-prompt --html-report "%OutputDir%\index.html" --append-multi-report "OpenGL33 Discrete" --performance-all -k
call :RunTest OpenGL33_Integrated --use-integrated -e "OpenGL3_3" --no-prompt --html-report "%OutputDir%\index.html" --append-multi-report "OpenGL33 Integrated" --performance-all -k
call :RunTest OpenGL33_Software --use-software -e "OpenGL3_3" --no-prompt --html-report "%OutputDir%\index.html" --append-multi-report "OpenGL33 Software" --performance-all -k
call :RunTest OpenGL43_Discrete --use-discrete -e "OpenGL4_3" --no-prompt --html-report "%OutputDir%\index.html" --append-multi-report "OpenGL43 Discrete" --performance-all -k
call :RunTest OpenGL43_Integrated --use-integrated -e "OpenGL4_3" --no-prompt --html-report "%OutputDir%\index.html" --append-multi-report "OpenGL43 Integrated" --performance-all -k
call :RunTest OpenGL43_Software --use-software -e "OpenGL4_3" --no-prompt --html-report "%OutputDir%\index.html" --append-multi-report "OpenGL43 Software" --performance-all -k
call :RunTest Vulkan11_Discrete --use-discrete -e "Vulkan1_1" --no-prompt --html-report "%OutputDir%\index.html" --append-multi-report "Vulkan11 Discrete" --performance-all -k
call :RunTest Vulkan11_Integrated --use-integrated -e "Vulkan1_1" --no-prompt --html-report "%OutputDir%\index.html" --append-multi-report "Vulkan11 Integrated" --performance-all -k
call :RunTest Vulkan11_Software --use-software -e "Vulkan1_1" --no-prompt --html-report "%OutputDir%\index.html" --append-multi-report "Vulkan11 Software" --performance-all -k

start "" "%OutputDir%/index.html"
exit /b %ScriptFailed%

:RunTest
set "RunName=%~1"
set "RunArgs=%*"
for /f "tokens=1,* delims= " %%A in ("!RunArgs!") do set "RunArgs=%%B"
set "PassFailFile=%PassFailDir%\%RunName%.txt"
if exist "!PassFailFile!" del /F /Q "!PassFailFile!" >nul 2>nul

"%SourceDir%\AutomatedTests.exe" !RunArgs! !AdditionalArgs! --pass-fail "!PassFailFile!" --log-file "%OutputDir%\%RunName%_log.txt"
set "RunExitCode=!ERRORLEVEL!"

if "!RunExitCode!"=="2" (
	if not exist "!PassFailFile!" (
		echo Skipped !RunName!: no compatible graphics device.
		exit /b 0
	)
) else if not "!RunExitCode!"=="0" (
	echo Failed !RunName!: AutomatedTests exited with code !RunExitCode!.
	set "ScriptFailed=1"
	exit /b 0
)

if not exist "!PassFailFile!" (
	echo Failed !RunName!: no pass/fail file was written.
	set "ScriptFailed=1"
	exit /b 0
)

set "PassFailResult="
set /p "PassFailResult="<"!PassFailFile!"
if /I not "!PassFailResult!"=="PASS" (
	echo Failed !RunName!: pass/fail file reported !PassFailResult!.
	set "ScriptFailed=1"
)
exit /b 0
