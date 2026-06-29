#!/usr/bin/env bash
set -euo pipefail

SourceDir="${1:-.}"
if [[ "$#" -gt 0 ]]; then
    shift
fi
OutputDir="${1:-TestResults}"
if [[ "$#" -gt 0 ]]; then
    shift
fi
AdditionalArgCount="$#"
AdditionalArgs=("$@")

# Create output dir (if not exists) and clear its contents
mkdir -p "$OutputDir"
rm -rf "$OutputDir"/*
PassFailDir="$OutputDir/PassFail"
mkdir -p "$PassFailDir"
ScriptFailed=0
SuccessfulRunCount=0

run_test() {
    local run_name="$1"
    shift
    local pass_fail_file="$PassFailDir/$run_name.txt"
    rm -f "$pass_fail_file"

    set +e
    if [[ "$AdditionalArgCount" -gt 0 ]]; then
        "$SourceDir/AutomatedTests" "$@" "${AdditionalArgs[@]}" --pass-fail "$pass_fail_file" --log-file "$OutputDir/${run_name}_log.txt"
    else
        "$SourceDir/AutomatedTests" "$@" --pass-fail "$pass_fail_file" --log-file "$OutputDir/${run_name}_log.txt"
    fi
    local run_exit_code=$?
    set -e

    if [[ "$run_exit_code" -eq 2 && ! -f "$pass_fail_file" ]]; then
        echo "Skipped $run_name: no compatible graphics device or window system."
        return
    fi
    if [[ "$run_exit_code" -ne 0 && "$run_exit_code" -ne 2 ]]; then
        echo "Failed $run_name: AutomatedTests exited with code $run_exit_code."
        ScriptFailed=1
        return
    fi
    if [[ ! -f "$pass_fail_file" ]]; then
        echo "Failed $run_name: no pass/fail file was written."
        ScriptFailed=1
        return
    fi

    local pass_fail_result
    pass_fail_result="$(tr -d '\r\n' < "$pass_fail_file")"
    if [[ "$pass_fail_result" != "PASS" ]]; then
        echo "Failed $run_name: pass/fail file reported $pass_fail_result."
        ScriptFailed=1
    elif [[ "$run_exit_code" -eq 0 ]]; then
        SuccessfulRunCount=$((SuccessfulRunCount + 1))
    fi
}

run_test OpenGL33_Discrete --use-discrete -e "OpenGL3_3" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "OpenGL33 Discrete" --test-all -k
run_test OpenGL33_Integrated --use-integrated -e "OpenGL3_3" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "OpenGL33 Integrated" --test-all -k
run_test OpenGL33_Software --use-software -e "OpenGL3_3" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "OpenGL33 Software" --test-all -k
if [[ "$OSTYPE" != "darwin"* ]]; then
    # No OpenGL 4.3 support on macOS
    run_test OpenGL43_Discrete --use-discrete -e "OpenGL4_3" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "OpenGL43 Discrete" --test-all -k
    run_test OpenGL43_Integrated --use-integrated -e "OpenGL4_3" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "OpenGL43 Integrated" --test-all -k
    run_test OpenGL43_Software --use-software -e "OpenGL4_3" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "OpenGL43 Software" --test-all -k
fi
run_test Vulkan11_Discrete --use-discrete -e "Vulkan1_1" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "Vulkan11 Discrete" --test-all -k
run_test Vulkan11_Integrated --use-integrated -e "Vulkan1_1" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "Vulkan11 Integrated" --test-all -k
run_test Vulkan11_Software --use-software -e "Vulkan1_1" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "Vulkan11 Software" --test-all -k

run_test OpenGL33_Discrete_Headless --use-discrete -e "OpenGL3_3" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "OpenGL33 Discrete [Headless]" --window-system Headless --test-all -k
run_test OpenGL33_Integrated_Headless --use-integrated -e "OpenGL3_3" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "OpenGL33 Integrated [Headless]" --window-system Headless --test-all -k
run_test OpenGL33_Software_Headless --use-software -e "OpenGL3_3" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "OpenGL33 Software [Headless]" --window-system Headless --test-all -k
if [[ "$OSTYPE" != "darwin"* ]]; then
    # No OpenGL 4.3 support on macOS
    run_test OpenGL43_Discrete_Headless --use-discrete -e "OpenGL4_3" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "OpenGL43 Discrete [Headless]" --window-system Headless --test-all -k
    run_test OpenGL43_Integrated_Headless --use-integrated -e "OpenGL4_3" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "OpenGL43 Integrated [Headless]" --window-system Headless --test-all -k
    run_test OpenGL43_Software_Headless --use-software -e "OpenGL4_3" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "OpenGL43 Software [Headless]" --window-system Headless --test-all -k
fi
run_test Vulkan11_Discrete_Headless --use-discrete -e "Vulkan1_1" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "Vulkan11 Discrete [Headless]" --window-system Headless --test-all -k
run_test Vulkan11_Integrated_Headless --use-integrated -e "Vulkan1_1" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "Vulkan11 Integrated [Headless]" --window-system Headless --test-all -k
run_test Vulkan11_Software_Headless --use-software -e "Vulkan1_1" --no-prompt --html-report "$OutputDir/index.html" --append-multi-report "Vulkan11 Software [Headless]" --window-system Headless --test-all -k

# Open the HTML results in the default browser
if command -v xdg-open >/dev/null 2>&1; then
    xdg-open "$OutputDir/index.html" >/dev/null 2>&1 &
elif command -v open >/dev/null 2>&1; then  # macOS
    open "$OutputDir/index.html" >/dev/null 2>&1 &
fi

if [[ "$SuccessfulRunCount" -eq 0 ]]; then
    echo "Failed: no test run found a compatible graphics device and completed successfully."
    ScriptFailed=1
fi

exit "$ScriptFailed"
