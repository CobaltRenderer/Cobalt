from __future__ import annotations

"""
BuildCodeCoverageReport.py

Runs the automated unit tests under the Visual Studio code coverage collector,
then generates the renderer plugin line coverage reports used by the project.

The script consumes the installed AutomatedTests bundle rather than build-tree
binaries. This keeps coverage runs aligned with the same self-contained test
package used by the normal unit-test targets.

Typical direct usage:

    python Scripts/BuildCodeCoverageReport.py ^
        --source-root . ^
        --test-root Output/AutomatedTests_x64_Debug ^
        --output-root Output/CodeCoverageReport
"""

import argparse
import html
import json
import os
import shutil
import subprocess
import xml.etree.ElementTree as ET
from collections import defaultdict
from datetime import datetime
from pathlib import Path


# Renderer plugins covered by this report. DLL paths are resolved from the installed
# AutomatedTests bundle, while source roots are resolved from --source-root.
RENDERERS = [
    {
        "key": "Direct3D11_1",
        "label": "Direct3D 11",
        "dll": Path("Renderers/Direct3D11Renderer/Direct3D11Renderer.dll"),
        "source_root": Path("Renderers/Direct3D11Renderer"),
        "module": "direct3d11renderer.dll",
    },
    {
        "key": "Direct3D12_0",
        "label": "Direct3D 12",
        "dll": Path("Renderers/Direct3D12Renderer/Direct3D12Renderer.dll"),
        "source_root": Path("Renderers/Direct3D12Renderer"),
        "module": "direct3d12renderer.dll",
    },
    {
        "key": "OpenGL3_3",
        "label": "OpenGL 3.3",
        "dll": Path("Renderers/OpenGLRenderer/OpenGL3Renderer.dll"),
        "source_root": Path("Renderers/OpenGLRenderer"),
        "module": "opengl3renderer.dll",
    },
    {
        "key": "OpenGL4_3",
        "label": "OpenGL 4.3",
        "dll": Path("Renderers/OpenGLRenderer/OpenGL4Renderer.dll"),
        "source_root": Path("Renderers/OpenGLRenderer"),
        "module": "opengl4renderer.dll",
    },
    {
        "key": "Vulkan1_1",
        "label": "Vulkan 1.1",
        "dll": Path("Renderers/VulkanRenderer/VulkanRenderer.dll"),
        "source_root": Path("Renderers/VulkanRenderer"),
        "module": "vulkanrenderer.dll",
    },
]

COVERAGE_RUN_MODES = (
    {
        "key": "normal",
        "label": "Normal",
        "test_args": (),
    },
    {
        "key": "headless",
        "label": "Headless",
        "test_args": ("--window-system", "Headless"),
    },
)


STATUS_RANK = {"no": 0, "partial": 1, "yes": 2}
RANK_STATUS = {0: "uncovered", 1: "partial", 2: "covered"}

# These filters remove generated third-party code, platform branches that cannot
# execute on the current Windows coverage host, and deliberately opt-in debug paths
# from the coverage denominator.
EXCLUDED_SOURCE_PREFIXES = (
    "Renderers/OpenGLRenderer/glad3.3/",
    "Renderers/OpenGLRenderer/glad4.3/",
)
EXCLUDED_SOURCE_PATHS_BY_RENDERER = {
    "OpenGL3_3": (
        "Renderers/OpenGLRenderer/OpenGLTextureBufferCubeArray.inl",
        "Renderers/OpenGLRenderer/OpenGLTextureSamplerCubeArray.inl",
    ),
}
EXCLUDED_SOURCE_FRAGMENTS = (
    "/AppKit/",
    "/XCB/",
    "/Xlib/",
    "/Wayland/",
    "/Posix/",
)
EXCLUDED_FUNCTION_FRAGMENTS = (
    "BuildNullDescriptorFallbacks",
    "BindNullDescriptorFallbacks",
    "GetNullDescriptorFallback",
    "CreateNullDescriptorFallbackBufferIfRequired",
    "GetNullDescriptorFallbackTexelSizeInBytes",
    "DestroyNullDescriptorFallbackResources",
    "NullDescriptorFeatureMissingOrBroken",
    "FlushTextureAttachmentWritesForSampling",
    "RegisterDebugMessengerCallback",
    "DebugMessengerCallback",
    "OpenGLRenderer::MessageCallback",
    "SetDebugName",
    "DebugName",
    "DebugMessage",
)
EXCLUDED_FUNCTION_FRAGMENTS_BY_RENDERER = {
    "Direct3D11_1": (
        "Direct3DRenderer::UnbindTextures",
        "Direct3DRenderer::UnbindSamplers",
        "Direct3DRenderer::UnbindStateBuffers",
        "StateBufferBindingInfo::UnbindStateBuffer",
    ),
    "OpenGL3_3": (
        "CubeArray",
        "ResourceArray",
        "SetComputeTask",
        "RemoveComputeTask",
    ),
}
EXCLUDED_FUNCTION_FRAGMENT_SEQUENCES = (
    ("IsSampleCountSupported", "ITextureBuffer1D"),
    ("IsSampleCountSupported", "ITextureBuffer1DArray"),
    ("IsSampleCountSupported", "ITextureBuffer2DArray"),
    ("IsSampleCountSupported", "ITextureBuffer3D"),
    ("IsSampleCountSupported", "ITextureBufferCube"),
    ("IsSampleCountSupported", "ITextureBufferCubeArray"),
)
EXCLUDED_FUNCTION_FRAGMENT_SEQUENCES_BY_RENDERER = {
    "Direct3D11_1": (
        ("TextureBindingInfo<", "::UnbindTexture"),
        ("TextureBindingWithCombinedSamplerInfo<", "::UnbindTexture"),
        ("SamplerBindingInfo<", "::UnbindSampler"),
    ),
}
EXCLUDED_LINE_FRAGMENTS = (
    "_useRenderMarkers",
    "EnableRenderMarkers",
    "RenderMarker",
    "renderMarkerLabel",
    "_renderAnnotation",
    "ID3DUserDefinedAnnotation",
    "D3DUserDefinedAnnotation",
    "EmitBeginEvent",
    "EmitEndEvent",
    "glPushDebugGroup",
    "glPopDebugGroup",
    "glDebugMessageInsert",
    "DebugName()",
    "vkCmdBeginDebugUtilsLabelEXT",
    "vkCmdEndDebugUtilsLabelEXT",
    "vkCmdInsertDebugUtilsLabelEXT",
    "_pfnCmdBeginDebugUtilsLabelEXT",
    "_pfnCmdEndDebugUtilsLabelEXT",
    "_pfnCmdInsertDebugUtilsLabelEXT",
    "VK_EXT_DEBUG_UTILS_EXTENSION_NAME",
    "DebugUtils",
    "debugUtilsAvailable",
    "NativeApiValidation",
    "enableValidationLayers",
    "validationLayerAvailable",
    "validationLayerName",
    "validationLayers",
    "VK_LAYER_KHRONOS_validation",
    "ignoredMessageNames",
    "_messageSeverityFilter",
    "_ignoredMessageNames",
    "_debugMessenger",
    "_enableDebugLogging",
    "EnableDebugLogging",
    "DebugLoggingEnabled()",
)
EXCLUDED_LINE_BLOCK_FRAGMENTS = (
    "_useRenderMarkers",
    "EnableRenderMarkers",
    "debugUtilsAvailable",
    "enableValidationLayers",
    "validationLayerAvailable",
    "_debugMessenger",
    "if (_enableDebugLogging",
    "if(_enableDebugLogging",
    "&& _enableDebugLogging",
    "if (_renderer->DebugLoggingEnabled()",
    "if(_renderer->DebugLoggingEnabled()",
    "if (!usingPixelBufferObjectsForCapture",
    "Options::EnableRenderMarkers",
)
ERROR_PATH_LINE_FRAGMENTS = (
    "_log->Error",
    "_log->Critical",
    "throw ",
    "std::terminate",
    "assert(",
    "ASSERT(",
)
ERROR_PATH_BLOCK_FRAGMENTS = (
    "if (FAILED(",
    "if(FAILED(",
    "FAILED(result)",
    "FAILED(hr)",
    "!= VK_SUCCESS",
    "== VK_NULL_HANDLE",
    "vkCreate",
    "vkAllocate",
    "vkBegin",
    "vkEnd",
    "vkQueue",
    "vkWait",
    "glCheckFramebufferStatus",
    "CheckGLError",
    "glGetError",
)


# ---------------------------------------------------------------------------
# Collection
# ---------------------------------------------------------------------------

def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def resolve_coverage_tool(requested_tool: str | None) -> Path:
    """
    Locate the Visual Studio coverage collector.

    A command-line path wins first, followed by COBALT_CODE_COVERAGE_TOOL, PATH,
    and then the standard Visual Studio 2022/2026 installation locations.
    """
    candidates: list[Path] = []
    if requested_tool:
        candidates.append(Path(requested_tool))

    env_tool = os.environ.get("COBALT_CODE_COVERAGE_TOOL")
    if env_tool:
        candidates.append(Path(env_tool))

    path_tool = shutil.which("Microsoft.CodeCoverage.Console.exe")
    if path_tool:
        candidates.append(Path(path_tool))

    program_files = Path(os.environ.get("ProgramFiles", r"C:\Program Files"))
    vs_editions = ("Enterprise", "Professional", "Community", "BuildTools")
    for version in ("18", "17"):
        for edition in vs_editions:
            candidates.append(
                program_files
                / "Microsoft Visual Studio"
                / version
                / edition
                / "Common7"
                / "IDE"
                / "Extensions"
                / "Microsoft"
                / "CodeCoverage.Console"
                / "Microsoft.CodeCoverage.Console.exe"
            )

    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()

    raise RuntimeError(
        "Could not find Microsoft.CodeCoverage.Console.exe. "
        "Install the Visual Studio code coverage tools, pass --coverage-tool, "
        "or set COBALT_CODE_COVERAGE_TOOL."
    )


def automated_tests_executable(test_root: Path) -> Path:
    suffix = ".exe" if os.name == "nt" else ""
    exe = test_root / f"AutomatedTests{suffix}"
    if not exe.is_file():
        raise RuntimeError(f"Could not find AutomatedTests executable at {exe}")
    return exe


def installed_renderer_dll(test_root: Path, renderer: dict) -> Path:
    return test_root / "Renderers" / renderer["dll"].name


def available_renderers(test_root: Path, renderer_keys: list[str] | None) -> list[dict]:
    """
    Use the renderers installed with AutomatedTests, skipping renderer plugins
    that are not present for the current platform/build configuration.
    """
    wanted = {key.lower() for key in renderer_keys or []}
    available = []
    missing = []
    for renderer in RENDERERS:
        if wanted and renderer["key"].lower() not in wanted and renderer["label"].lower() not in wanted:
            continue
        renderer_dll = installed_renderer_dll(test_root, renderer)
        if renderer_dll.is_file():
            available.append(renderer)
        else:
            missing.append(renderer["label"])

    if renderer_keys and not available:
        raise RuntimeError(f"None of the requested renderers are installed under {test_root / 'Renderers'}")
    if not available:
        raise RuntimeError(f"No renderer plugins were found under {test_root / 'Renderers'}")

    if missing and not renderer_keys:
        print(f"Skipping renderer plugins that are not installed: {', '.join(missing)}", flush=True)
    return available


def run_collect(repo: Path, test_root: Path, coverage_root: Path, test_filter: str | None, coverage_tool: Path, renderers: list[dict]) -> None:
    coverage_root.mkdir(parents=True, exist_ok=True)
    exe = automated_tests_executable(test_root)
    renderer_dir = test_root / "Renderers"
    reference_dir = test_root / "ReferenceImages"
    if not renderer_dir.is_dir():
        raise RuntimeError(f"Could not find installed renderer directory at {renderer_dir}")
    if not reference_dir.is_dir():
        raise RuntimeError(f"Could not find installed reference image directory at {reference_dir}")

    failed_runs = []

    for renderer in renderers:
        child_environment = os.environ.copy()
        if renderer["key"] == "Vulkan1_1":
            # Native coverage runs can crash inside the external Khronos validation layer during Vulkan descriptor
            # allocation. Normal test targets still exercise validation; coverage only needs the renderer module.
            disabled_layers = [layer.strip() for layer in child_environment.get("VK_LOADER_LAYERS_DISABLE", "").split(",") if layer.strip()]
            if "*validation*" not in disabled_layers:
                disabled_layers.append("*validation*")
            child_environment["VK_LOADER_LAYERS_DISABLE"] = ",".join(disabled_layers)

        renderer_dll = installed_renderer_dll(test_root, renderer)
        for mode in COVERAGE_RUN_MODES:
            run_name = f"{renderer['label']} ({mode['label']})"
            xml_path = coverage_root / f"{renderer['key']}_{mode['key']}.xml"
            test_report = coverage_root / f"{renderer['key']}_{mode['key']}_tests.html"
            pass_fail = coverage_root / f"{renderer['key']}_{mode['key']}.passfail.txt"
            stdout = coverage_root / f"{renderer['key']}_{mode['key']}_stdout.txt"
            log_file = coverage_root / f"{renderer['key']}_{mode['key']}_coverage.log"

            test_args = [
                str(exe),
                "-r",
                str(renderer_dll),
                "-d",
                str(renderer_dir),
                "-p",
                "-k",
                "--on-error",
                "Ignore",
                "--html-report",
                str(test_report),
                "--pass-fail",
                str(pass_fail),
                "--reference-dir",
                str(reference_dir),
                "-a",
                *mode["test_args"],
            ]
            if test_filter:
                test_args.extend(["--test-contains", test_filter])

            cmd = [
                str(coverage_tool),
                "collect",
                "--output",
                str(xml_path),
                "--output-format",
                "xml",
                "--include-files",
                str(renderer_dll),
                "--log-file",
                str(log_file),
                "--log-level",
                "Error",
                "--",
                *test_args,
            ]

            print(f"Collecting {run_name} -> {xml_path}", flush=True)
            with stdout.open("w", encoding="utf-8", newline="\r\n") as stdout_file:
                result = subprocess.run(cmd, cwd=repo, stdout=stdout_file, stderr=subprocess.STDOUT, env=child_environment)
            if result.returncode != 0:
                print(f"Coverage collection for {run_name} returned {result.returncode}.", flush=True)
                failed_runs.append(f"{run_name} returned {result.returncode}")
            if pass_fail.exists():
                status = pass_fail.read_text(encoding="utf-8", errors="replace").strip()
                print(f"{run_name} tests: {status}", flush=True)
                if status != "PASS":
                    failed_runs.append(f"{run_name} test status was {status}")
            else:
                print(f"{run_name} did not produce a pass/fail file.", flush=True)
                failed_runs.append(f"{run_name} did not produce a pass/fail file")

            if failed_runs:
                raise RuntimeError("Coverage collection did not complete cleanly: " + "; ".join(failed_runs))


def relative_source(repo: Path, source: str) -> str | None:
    try:
        path = Path(source)
        if not path.is_absolute():
            return source.replace("\\", "/")
        rel = path.resolve().relative_to(repo.resolve())
        return rel.as_posix()
    except Exception:
        return None


# ---------------------------------------------------------------------------
# Coverage XML summarisation
# ---------------------------------------------------------------------------

def source_is_excluded(rel: str, renderer_key: str) -> bool:
    return (
        any(rel.startswith(prefix) for prefix in EXCLUDED_SOURCE_PREFIXES)
        or any(fragment in rel for fragment in EXCLUDED_SOURCE_FRAGMENTS)
        or rel in EXCLUDED_SOURCE_PATHS_BY_RENDERER.get(renderer_key, ())
    )


def function_is_excluded(function: ET.Element, renderer_key: str) -> bool:
    display_name = function.attrib.get("name", "")
    if function.attrib.get("type_name"):
        display_name = f"{function.attrib['type_name']}::{display_name}"
    if function.attrib.get("namespace"):
        display_name = f"{function.attrib['namespace']}::{display_name}"
    return (
        any(fragment in display_name for fragment in EXCLUDED_FUNCTION_FRAGMENTS)
        or any(fragment in display_name for fragment in EXCLUDED_FUNCTION_FRAGMENTS_BY_RENDERER.get(renderer_key, ()))
        or any(all(fragment in display_name for fragment in sequence) for sequence in EXCLUDED_FUNCTION_FRAGMENT_SEQUENCES)
        or any(all(fragment in display_name for fragment in sequence) for sequence in EXCLUDED_FUNCTION_FRAGMENT_SEQUENCES_BY_RENDERER.get(renderer_key, ()))
    )


def add_statement_block(lines: list[str], excluded_lines: set[int], start_index: int) -> None:
    depth = 0
    seen_open_brace = False
    for index in range(start_index, len(lines)):
        excluded_lines.add(index + 1)
        line = lines[index]
        if "{" in line:
            seen_open_brace = True
        depth += line.count("{")
        depth -= line.count("}")
        if seen_open_brace and depth <= 0:
            break
        if not seen_open_brace and index > start_index and line.strip():
            break


_EXCLUDED_LINES_CACHE: dict[str, set[int]] = {}
_ERROR_PATH_LINES_CACHE: dict[str, set[int]] = {}


def excluded_lines_for_source(repo: Path, rel: str) -> set[int]:
    if rel in _EXCLUDED_LINES_CACHE:
        return _EXCLUDED_LINES_CACHE[rel]

    excluded_lines: set[int] = set()
    source_path = repo / rel
    try:
        lines = source_path.read_text(encoding="utf-8", errors="replace").splitlines()
    except Exception:
        _EXCLUDED_LINES_CACHE[rel] = excluded_lines
        return excluded_lines

    for index, line in enumerate(lines):
        if any(fragment in line for fragment in EXCLUDED_LINE_FRAGMENTS):
            excluded_lines.add(index + 1)
        if any(fragment in line for fragment in EXCLUDED_LINE_BLOCK_FRAGMENTS):
            add_statement_block(lines, excluded_lines, index)

    _EXCLUDED_LINES_CACHE[rel] = excluded_lines
    return excluded_lines


def error_path_lines_for_source(repo: Path, rel: str) -> set[int]:
    if rel in _ERROR_PATH_LINES_CACHE:
        return _ERROR_PATH_LINES_CACHE[rel]

    error_lines: set[int] = set()
    source_path = repo / rel
    try:
        lines = source_path.read_text(encoding="utf-8", errors="replace").splitlines()
    except Exception:
        _ERROR_PATH_LINES_CACHE[rel] = error_lines
        return error_lines

    for index, line in enumerate(lines):
        if any(fragment in line for fragment in ERROR_PATH_LINE_FRAGMENTS):
            error_lines.add(index + 1)
        if any(fragment in line for fragment in ERROR_PATH_BLOCK_FRAGMENTS):
            add_statement_block(lines, error_lines, index)

    _ERROR_PATH_LINES_CACHE[rel] = error_lines
    return error_lines


def find_renderer_module(xml_path: Path, module_name: str) -> ET.Element | None:
    with xml_path.open("rb") as xml_file:
        context = ET.iterparse(xml_file, events=("end",))
        for _, elem in context:
            if elem.tag == "module":
                if elem.attrib.get("name", "").lower() == module_name.lower():
                    return elem
                elem.clear()
    return None


def renderer_coverage_xml_paths(coverage_root: Path, renderer: dict) -> list[Path]:
    mode_paths = [coverage_root / f"{renderer['key']}_{mode['key']}.xml" for mode in COVERAGE_RUN_MODES]
    if all(path.exists() for path in mode_paths):
        return mode_paths

    legacy_path = coverage_root / f"{renderer['key']}_all.xml"
    if legacy_path.exists():
        return [legacy_path]

    return mode_paths


def summarize_module(repo: Path, xml_paths: list[Path], renderer: dict) -> dict:
    source_root = renderer["source_root"].as_posix()
    module_attrs: dict | None = None
    line_status: dict[tuple[str, int], int] = {}
    function_records: dict[tuple, dict] = {}

    for xml_path in xml_paths:
        module = find_renderer_module(xml_path, renderer["module"])
        if module is None:
            raise RuntimeError(f"Could not find module {renderer['module']} in {xml_path}")

        if module_attrs is None:
            module_attrs = dict(module.attrib)

        source_paths: dict[str, str] = {}
        source_files = module.find("source_files")
        if source_files is not None:
            for source in source_files.findall("source_file"):
                rel = relative_source(repo, source.attrib.get("path", ""))
                if rel is not None:
                    source_paths[source.attrib["id"]] = rel

        functions = module.find("functions")
        if functions is not None:
            for function in functions.findall("function"):
                if function_is_excluded(function, renderer["key"]):
                    continue
                ranges = function.find("ranges")
                if ranges is None:
                    continue

                touched_files = set()
                range_keys = []
                local_line_status: dict[tuple[str, int], int] = {}
                for source_range in ranges.findall("range"):
                    source_id = source_range.attrib.get("source_id")
                    rel = source_paths.get(source_id)
                    if rel is None or not rel.startswith(source_root) or source_is_excluded(rel, renderer["key"]):
                        continue
                    touched_files.add(rel)
                    start_line = int(source_range.attrib.get("start_line", "0"))
                    end_line = int(source_range.attrib.get("end_line", str(start_line)))
                    range_keys.append((rel, start_line, end_line))
                    covered = source_range.attrib.get("covered", "no")
                    rank = STATUS_RANK.get(covered, 0)
                    excluded_lines = excluded_lines_for_source(repo, rel)
                    for line in range(start_line, end_line + 1):
                        if line in excluded_lines:
                            continue
                        key = (rel, line)
                        if rank > line_status.get(key, -1):
                            line_status[key] = rank
                        if rank > local_line_status.get(key, -1):
                            local_line_status[key] = rank

                if not touched_files:
                    continue

                function_key = (
                    function.attrib.get("namespace", ""),
                    function.attrib.get("type_name", ""),
                    function.attrib.get("name", ""),
                    tuple(sorted(range_keys)),
                )
                record = function_records.setdefault(
                    function_key,
                    {
                        "name": function.attrib.get("name", ""),
                        "namespace": function.attrib.get("namespace", ""),
                        "type_name": function.attrib.get("type_name", ""),
                        "files": set(),
                        "line_status": {},
                        "line_coverage": set(),
                        "block_coverage": set(),
                    },
                )
                record["files"].update(touched_files)
                record["line_coverage"].add(function.attrib.get("line_coverage", ""))
                record["block_coverage"].add(function.attrib.get("block_coverage", ""))
                for key, rank in local_line_status.items():
                    if rank > record["line_status"].get(key, -1):
                        record["line_status"][key] = rank

        module.clear()

    if module_attrs is None:
        raise RuntimeError(f"Could not find module {renderer['module']} in any coverage XML")

    error_uncovered_lines: dict[str, list[int]] = defaultdict(list)
    function_summaries = []
    for record in function_records.values():
        local_counts = defaultdict(int)
        local_uncovered_lines: set[tuple[str, int]] = set()
        local_error_uncovered_lines: set[tuple[str, int]] = set()
        for key, rank in record["line_status"].items():
            rel, line = key
            local_counts[rank] += 1
            if rank == 0:
                local_uncovered_lines.add(key)
                if line in error_path_lines_for_source(repo, rel):
                    local_error_uncovered_lines.add(key)

        function_summaries.append(
            {
                "name": record["name"],
                "namespace": record["namespace"],
                "type_name": record["type_name"],
                "files": sorted(record["files"]),
                "covered_ranges": local_counts[2],
                "partial_ranges": local_counts[1],
                "uncovered_ranges": local_counts[0],
                "uncovered_lines": len(local_uncovered_lines),
                "error_uncovered_lines": len(local_error_uncovered_lines),
                "line_coverage": ", ".join(sorted(value for value in record["line_coverage"] if value)),
                "block_coverage": ", ".join(sorted(value for value in record["block_coverage"] if value)),
            }
        )

    files = []
    by_file: dict[str, dict[str, int]] = defaultdict(lambda: {"covered": 0, "partial": 0, "uncovered": 0})
    uncovered_lines: dict[str, list[int]] = defaultdict(list)
    for (rel, line), rank in line_status.items():
        status = RANK_STATUS[rank]
        by_file[rel][status] += 1
        if rank == 0:
            uncovered_lines[rel].append(line)
            if line in error_path_lines_for_source(repo, rel):
                error_uncovered_lines[rel].append(line)

    for rel, counts in by_file.items():
        total = counts["covered"] + counts["partial"] + counts["uncovered"]
        files.append(
            {
                "path": rel,
                "covered": counts["covered"],
                "partial": counts["partial"],
                "uncovered": counts["uncovered"],
                "error_uncovered": len(set(error_uncovered_lines[rel])),
                "error_uncovered_pct": len(set(error_uncovered_lines[rel])) * 100.0 / counts["uncovered"] if counts["uncovered"] else 0.0,
                "non_error_uncovered": counts["uncovered"] - len(set(error_uncovered_lines[rel])),
                "total": total,
                "hit_pct": (counts["covered"] + counts["partial"]) * 100.0 / total if total else 100.0,
                "hit_pct_excluding_error": (counts["covered"] + counts["partial"]) * 100.0 / (total - len(set(error_uncovered_lines[rel]))) if (total - len(set(error_uncovered_lines[rel]))) else 100.0,
                "full_pct": counts["covered"] * 100.0 / total if total else 100.0,
                "uncovered_lines": compact_lines(sorted(set(uncovered_lines[rel]))),
                "error_uncovered_lines": compact_lines(sorted(set(error_uncovered_lines[rel]))),
                "non_error_uncovered_lines": compact_lines(sorted(set(uncovered_lines[rel]) - set(error_uncovered_lines[rel]))),
            }
        )

    files.sort(key=lambda item: (item["non_error_uncovered"], item["partial"], item["total"]), reverse=True)
    total_covered = sum(item["covered"] for item in files)
    total_partial = sum(item["partial"] for item in files)
    total_uncovered = sum(item["uncovered"] for item in files)
    total_error_uncovered = sum(item["error_uncovered"] for item in files)
    total_non_error_uncovered = total_uncovered - total_error_uncovered
    total = total_covered + total_partial + total_uncovered

    function_summaries.sort(key=lambda item: (item["uncovered_ranges"], item["partial_ranges"]), reverse=True)

    summary = {
        "module_attrs": module_attrs,
        "key": renderer["key"],
        "label": renderer["label"],
        "coverage_xml": [str(xml_path) for xml_path in xml_paths],
        "source_root": source_root,
        "source_file_count": len(files),
        "function_count": len(function_summaries),
        "covered": total_covered,
        "partial": total_partial,
        "uncovered": total_uncovered,
        "error_uncovered": total_error_uncovered,
        "error_uncovered_pct": total_error_uncovered * 100.0 / total_uncovered if total_uncovered else 0.0,
        "non_error_uncovered": total_non_error_uncovered,
        "total": total,
        "hit_pct": (total_covered + total_partial) * 100.0 / total if total else 100.0,
        "hit_pct_excluding_error": (total_covered + total_partial) * 100.0 / (total - total_error_uncovered) if (total - total_error_uncovered) else 100.0,
        "full_pct": total_covered * 100.0 / total if total else 100.0,
        "files": files,
        "functions": function_summaries,
    }

    return summary


def compact_lines(lines: list[int]) -> str:
    if not lines:
        return ""
    ranges = []
    start = prev = lines[0]
    for line in lines[1:]:
        if line == prev + 1:
            prev = line
            continue
        ranges.append((start, prev))
        start = prev = line
    ranges.append((start, prev))
    parts = []
    for start, end in ranges[:40]:
        if start == end:
            parts.append(str(start))
        else:
            parts.append(f"{start}-{end}")
    if len(ranges) > 40:
        parts.append("...")
    return ", ".join(parts)


# ---------------------------------------------------------------------------
# HTML report generation
# ---------------------------------------------------------------------------

def bar_pct(pct: float) -> str:
    return f"{pct:.2f}%"


def metric_class(pct: float) -> str:
    if pct >= 90.0:
        return "good"
    if pct >= 80.0:
        return "warn"
    return "bad"


def metric_cell(pct: float) -> str:
    return f"<td class='num metriccell {metric_class(pct)}' data-sort='{pct:.6f}'>{bar_pct(pct)}</td>"


def line_distribution_html(covered: int, partial: int, non_error_uncovered: int, error_uncovered: int, include_text: bool = True) -> str:
    total = covered + partial + non_error_uncovered + error_uncovered
    if total == 0:
        return "<span class='muted'>n/a</span>"
    covered_pct = covered * 100.0 / total
    partial_pct = partial * 100.0 / total
    error_pct = error_uncovered * 100.0 / total
    non_error_pct = non_error_uncovered * 100.0 / total
    title = f"{covered} full, {partial} partial, {error_uncovered} errors, {non_error_uncovered} unhit"
    text = f"<span class='small muted'>{covered} full, {partial} partial, {error_uncovered} errors, {non_error_uncovered} unhit</span>" if include_text else ""
    return (
        f"<div class='stackbar' title='{html.escape(title)}'>"
        f"<span class='seg covered' style='width:{covered_pct:.4f}%'></span>"
        f"<span class='seg partial' style='width:{partial_pct:.4f}%'></span>"
        f"<span class='seg errors' style='width:{error_pct:.4f}%'></span>"
        f"<span class='seg uncovered' style='width:{non_error_pct:.4f}%'></span>"
        f"</div>{text}"
    )


def area_for_path(path: str) -> str:
    leaf = Path(path).name
    if any(token in leaf for token in ("FrameBuffer", "RenderPass", "Output")):
        return "Framebuffers, outputs, and render passes"
    if "ShaderProgram" in leaf:
        return "Shader programs and reflection"
    if any(token in leaf for token in ("VertexBuffer", "IndexBuffer", "RenderableNode")):
        return "Geometry buffers and renderable nodes"
    if any(token in leaf for token in ("TextureBuffer", "TextureSampler")):
        return "Texture resources and samplers"
    if any(token in leaf for token in ("DataArray", "TexelArray", "TransferBatch")):
        return "Data resources and transfer paths"
    if any(token in leaf for token in ("State", "ProgramNode", "BindingHelpers")):
        return "Render tree state and bindings"
    if any(token in leaf for token in ("Renderer", "GraphicsDevice", "Factory", "Enumerator", "InstanceData", "Memory", "Heap", "CommandList", "Descriptor")):
        return "Device and renderer infrastructure"
    return "Other renderer code"


def area_heatmap_html(summary: dict) -> str:
    area_names = sorted({area_for_path(file_info["path"]) for run in summary["runs"] for file_info in run["files"]})
    rows = []
    for area in area_names:
        cells = [f"<td>{html.escape(area)}</td>"]
        for run in summary["runs"]:
            totals = defaultdict(int)
            for file_info in run["files"]:
                if area_for_path(file_info["path"]) != area:
                    continue
                for key in ("covered", "partial", "non_error_uncovered", "error_uncovered", "total"):
                    totals[key] += file_info[key]
            if totals["total"] == 0:
                cells.append("<td class='num muted'>n/a</td>")
                continue
            hit_pct = (totals["covered"] + totals["partial"]) * 100.0 / totals["total"]
            adjusted_total = totals["total"] - totals["error_uncovered"]
            adjusted_pct = (totals["covered"] + totals["partial"]) * 100.0 / adjusted_total if adjusted_total else 100.0
            cells.append(
                f"<td class='cell {metric_class(hit_pct)}' data-sort='{hit_pct:.6f}'>{bar_pct(hit_pct)}"
                f"<br><span class='small muted'>{totals['non_error_uncovered']} unhit, {totals['error_uncovered']} errors"
                f"<br>{bar_pct(adjusted_pct)} adjusted / {totals['total']} lines</span></td>"
            )
        rows.append(f"<tr>{''.join(cells)}</tr>")
    header_cells = "".join(f"<th class='num'>{html.escape(run['label'])}<br><span class='small'>hit / unhit</span></th>" for run in summary["runs"])
    return f"<div class='scroll'><table class='heat sortable'><thead><tr><th>Area</th>{header_cells}</tr></thead><tbody>{''.join(rows)}</tbody></table></div>"


def function_display_name(function: dict) -> str:
    display_name = function["name"]
    if function["type_name"]:
        display_name = f"{function['type_name']}::{display_name}"
    if function["namespace"]:
        display_name = f"{function['namespace']}::{display_name}"
    return display_name


def detail_button_html(title: str, detail_html: str) -> str:
    return (
        "<button type='button' class='detail-button' "
        f"data-title='{html.escape(title, quote=True)}' "
        f"data-detail='{html.escape(detail_html, quote=True)}'>Details</button>"
    )


def path_cell(value: str) -> str:
    escaped = html.escape(value)
    return f"<td class='path pathcell' title='{escaped}'>{escaped}</td>"


def source_files_text(files: list[str], limit: int = 4) -> str:
    text = ", ".join(files[:limit])
    if len(files) > limit:
        text += ", ..."
    return text


def function_counts(function: dict) -> dict:
    covered = function["covered_ranges"]
    partial = function["partial_ranges"]
    uncovered = function["uncovered_ranges"]
    errors = min(function["error_uncovered_lines"], uncovered)
    non_error_uncovered = max(0, uncovered - errors)
    total = covered + partial + uncovered
    hit_pct = ((covered + partial) * 100.0 / total) if total else 100.0
    adjusted_total = total - errors
    hit_pct_excluding_error = ((covered + partial) * 100.0 / adjusted_total) if adjusted_total else 100.0
    return {
        "covered": covered,
        "partial": partial,
        "uncovered": uncovered,
        "errors": errors,
        "non_error_uncovered": non_error_uncovered,
        "total": total,
        "hit_pct": hit_pct,
        "hit_pct_excluding_error": hit_pct_excluding_error,
    }


def renderer_summary_html(run: dict) -> str:
    distribution = line_distribution_html(run["covered"], run["partial"], run["non_error_uncovered"], run["error_uncovered"])
    return (
        "<div class='metriccards'>"
        f"<div class='metric'><div class='label'>Hit line coverage</div><div class='value {metric_class(run['hit_pct'])}'>{bar_pct(run['hit_pct'])}</div><div class='small muted'>{run['covered']} full + {run['partial']} partial / {run['total']} lines</div></div>"
        f"<div class='metric'><div class='label'>Hit excluding likely error-only</div><div class='value {metric_class(run['hit_pct_excluding_error'])}'>{bar_pct(run['hit_pct_excluding_error'])}</div><div class='small muted'>Likely error-only lines remain listed separately</div></div>"
        f"<div class='metric'><div class='label'>Fully covered line rate</div><div class='value'>{bar_pct(run['full_pct'])}</div><div class='small muted'>{run['covered']} fully hit lines</div></div>"
        f"<div class='metric'><div class='label'>Remaining likely non-error lines</div><div class='value'>{run['non_error_uncovered']}</div><div class='small muted'>{run['error_uncovered']} likely error-only lines</div></div>"
        "</div>"
        f"<div class='chartrow'><div class='chartlabel'>Line distribution</div><div>{distribution}</div><div class='{metric_class(run['hit_pct'])}'>{bar_pct(run['hit_pct'])}</div></div>"
    )


def renderer_area_breakdown_html(run: dict) -> str:
    area_totals: dict[str, dict[str, int]] = defaultdict(lambda: {"covered": 0, "partial": 0, "non_error_uncovered": 0, "error_uncovered": 0, "total": 0})
    for file_info in run["files"]:
        area = area_for_path(file_info["path"])
        for key in ("covered", "partial", "non_error_uncovered", "error_uncovered", "total"):
            area_totals[area][key] += file_info[key]

    rows = []
    for area, totals in sorted(area_totals.items()):
        if totals["total"] == 0:
            continue
        hit_pct = (totals["covered"] + totals["partial"]) * 100.0 / totals["total"]
        adjusted_total = totals["total"] - totals["error_uncovered"]
        adjusted_pct = (totals["covered"] + totals["partial"]) * 100.0 / adjusted_total if adjusted_total else 100.0
        distribution = line_distribution_html(totals["covered"], totals["partial"], totals["non_error_uncovered"], totals["error_uncovered"], include_text=False)
        rows.append(
            f"<tr><td>{html.escape(area)}</td>"
            f"{metric_cell(hit_pct)}"
            f"{metric_cell(adjusted_pct)}"
            f"<td>{distribution}</td>"
            f"<td class='num' data-sort='{totals['covered']}'>{totals['covered']}</td>"
            f"<td class='num' data-sort='{totals['partial']}'>{totals['partial']}</td>"
            f"<td class='num' data-sort='{totals['non_error_uncovered']}'>{totals['non_error_uncovered']}</td>"
            f"<td class='num' data-sort='{totals['error_uncovered']}'>{totals['error_uncovered']}</td>"
            f"<td class='num' data-sort='{totals['total']}'>{totals['total']}</td></tr>"
        )

    return (
        "<table class='sortable area-table'><thead><tr>"
        "<th>Area</th><th class='num'>Hit</th><th class='num'>Hit % Excluding Error-Only</th><th>Line Distribution</th>"
        "<th class='num'>Full</th><th class='num'>Partial</th><th class='num'>Likely Non-Error Unhit</th><th class='num'>Likely Error-Only</th><th class='num'>Total</th>"
        f"</tr></thead><tbody>{''.join(rows)}</tbody></table>"
    )


def file_coverage_row(file_info: dict) -> str:
    distribution = line_distribution_html(file_info["covered"], file_info["partial"], file_info["non_error_uncovered"], file_info["error_uncovered"], include_text=False)
    display_name = Path(file_info["path"]).name
    detail = (
        "<dl>"
        f"<dt>Path</dt><dd><code>{html.escape(file_info['path'])}</code></dd>"
        f"<dt>Likely non-error uncovered lines</dt><dd>{html.escape(file_info['non_error_uncovered_lines'] or 'None')}</dd>"
        f"<dt>Likely error-only lines</dt><dd>{html.escape(file_info['error_uncovered_lines'] or 'None')}</dd>"
        f"<dt>Full / partial / likely non-error unhit / likely error-only</dt><dd>{file_info['covered']} / {file_info['partial']} / {file_info['non_error_uncovered']} / {file_info['error_uncovered']}</dd>"
        "</dl>"
    )
    return (
        "<tr>"
        f"{path_cell(display_name)}"
        f"{path_cell(file_info['path'])}"
        f"{metric_cell(file_info['hit_pct'])}"
        f"{metric_cell(file_info['hit_pct_excluding_error'])}"
        f"<td>{distribution}</td>"
        f"<td class='num' data-sort='{file_info['covered']}'>{file_info['covered']}</td>"
        f"<td class='num' data-sort='{file_info['partial']}'>{file_info['partial']}</td>"
        f"<td class='num' data-sort='{file_info['non_error_uncovered']}'>{file_info['non_error_uncovered']}</td>"
        f"<td class='num' data-sort='{file_info['error_uncovered']}'>{file_info['error_uncovered']}</td>"
        f"<td class='num' data-sort='{file_info['total']}'>{file_info['total']}</td>"
        f"<td class='detail-cell'>{detail_button_html(file_info['path'], detail)}</td>"
        "</tr>"
    )


def function_coverage_row(function: dict) -> str:
    counts = function_counts(function)
    display_name = function_display_name(function)
    files = source_files_text(function["files"])
    distribution = line_distribution_html(counts["covered"], counts["partial"], counts["non_error_uncovered"], counts["errors"], include_text=False)
    detail = (
        "<dl>"
        f"<dt>Function</dt><dd><code>{html.escape(display_name)}</code></dd>"
        f"<dt>Source file(s)</dt><dd>{html.escape(', '.join(function['files']))}</dd>"
        f"<dt>Reported line coverage</dt><dd>{html.escape(function['line_coverage'])}</dd>"
        f"<dt>Reported block coverage</dt><dd>{html.escape(function['block_coverage'])}</dd>"
        f"<dt>Full / partial / unhit / likely error-only</dt><dd>{counts['covered']} / {counts['partial']} / {counts['uncovered']} / {counts['errors']}</dd>"
        "</dl>"
    )
    return (
        "<tr>"
        f"{path_cell(display_name)}"
        f"{path_cell(files)}"
        f"{metric_cell(counts['hit_pct'])}"
        f"{metric_cell(counts['hit_pct_excluding_error'])}"
        f"<td>{distribution}</td>"
        f"<td class='num' data-sort='{counts['covered']}'>{counts['covered']}</td>"
        f"<td class='num' data-sort='{counts['partial']}'>{counts['partial']}</td>"
        f"<td class='num' data-sort='{counts['non_error_uncovered']}'>{counts['non_error_uncovered']}</td>"
        f"<td class='num' data-sort='{counts['errors']}'>{counts['errors']}</td>"
        f"<td class='num' data-sort='{counts['total']}'>{counts['total']}</td>"
        f"<td class='detail-cell'>{detail_button_html(display_name, detail)}</td>"
        "</tr>"
    )


def file_coverage_rows_html(run: dict) -> str:
    rows = [file_coverage_row(file_info) for file_info in sorted(run["files"], key=lambda item: (item["non_error_uncovered"], item["error_uncovered"], item["partial"], item["total"]), reverse=True)]
    return (
        "<details open><summary>File Coverage Rows</summary>"
        "<div class='scroll wide'><table class='sortable coverage-table'><thead><tr>"
        "<th>File</th><th>Path</th><th class='num'>Hit</th><th class='num'>Hit % Excluding Error-Only</th>"
        "<th>Line Distribution</th><th class='num'>Full</th><th class='num'>Partial</th><th class='num'>Likely Non-Error Unhit</th>"
        "<th class='num'>Likely Error-Only</th><th class='num'>Total</th><th>Details</th>"
        f"</tr></thead><tbody>{''.join(rows)}</tbody></table></div></details>"
    )


def function_coverage_rows_html(run: dict) -> str:
    functions = sorted(
        run["functions"],
        key=lambda function: (
            function_counts(function)["non_error_uncovered"],
            function_counts(function)["errors"],
            function_counts(function)["partial"],
            function_counts(function)["total"],
        ),
        reverse=True,
    )
    rows = [function_coverage_row(function) for function in functions]
    return (
        "<details open><summary>Function Coverage Rows</summary>"
        "<div class='scroll wide'><table class='sortable coverage-table'><thead><tr>"
        "<th>Function</th><th>Source File(s)</th><th class='num'>Hit</th><th class='num'>Hit % Excluding Error-Only</th>"
        "<th>Line Distribution</th><th class='num'>Full</th><th class='num'>Partial</th><th class='num'>Likely Non-Error Unhit</th>"
        "<th class='num'>Likely Error-Only</th><th class='num'>Total</th><th>Details</th>"
        f"</tr></thead><tbody>{''.join(rows)}</tbody></table></div></details>"
    )


def grouped_function_entries(functions: list[dict], mode: str) -> list[dict]:
    groups: dict[tuple[str, tuple[str, ...]], dict] = {}
    for function in functions:
        counts = function_counts(function)
        if counts["total"] == 0:
            continue
        is_unhit = counts["covered"] == 0 and counts["partial"] == 0
        if mode == "unhit" and not is_unhit:
            continue
        if mode == "partial" and (is_unhit or (counts["partial"] == 0 and counts["uncovered"] == 0)):
            continue

        display_name = function_display_name(function)
        files = tuple(function["files"])
        key = (display_name, files)
        group = groups.setdefault(
            key,
            {
                "name": display_name,
                "files": list(files),
                "instances": 0,
                "covered": 0,
                "partial": 0,
                "uncovered": 0,
                "non_error_uncovered": 0,
                "errors": 0,
                "total": 0,
                "line_coverage": set(),
                "block_coverage": set(),
            },
        )
        group["instances"] += 1
        for key_name in ("covered", "partial", "uncovered", "non_error_uncovered", "errors", "total"):
            group[key_name] += counts[key_name]
        group["line_coverage"].add(function["line_coverage"])
        group["block_coverage"].add(function["block_coverage"])

    entries = list(groups.values())
    entries.sort(key=lambda item: (item["non_error_uncovered"], item["errors"], item["partial"], item["total"]), reverse=True)
    return entries


def function_entries_table_html(run: dict, mode: str) -> str:
    entries = grouped_function_entries(run["functions"], mode)
    title = "Unhit Function Entries" if mode == "unhit" else "Partially Covered Function Entries"
    if not entries:
        return f"<details><summary>{title}</summary><p class='muted'>No entries.</p></details>"

    rows = []
    for entry in entries:
        hit_pct = ((entry["covered"] + entry["partial"]) * 100.0 / entry["total"]) if entry["total"] else 100.0
        adjusted_total = entry["total"] - entry["errors"]
        adjusted_pct = ((entry["covered"] + entry["partial"]) * 100.0 / adjusted_total) if adjusted_total else 100.0
        files = source_files_text(entry["files"])
        detail = (
            "<dl>"
            f"<dt>Function</dt><dd><code>{html.escape(entry['name'])}</code></dd>"
            f"<dt>Source file(s)</dt><dd>{html.escape(', '.join(entry['files']))}</dd>"
            f"<dt>Reported line coverage values</dt><dd>{html.escape(', '.join(sorted(entry['line_coverage'])) or 'None')}</dd>"
            f"<dt>Reported block coverage values</dt><dd>{html.escape(', '.join(sorted(entry['block_coverage'])) or 'None')}</dd>"
            f"<dt>Instances</dt><dd>{entry['instances']}</dd>"
            "</dl>"
        )
        if mode == "unhit":
            rows.append(
                f"<tr>{path_cell(entry['name'])}{path_cell(files)}"
                f"<td class='num' data-sort='{entry['instances']}'>{entry['instances']}</td>"
                f"<td class='num' data-sort='{entry['non_error_uncovered']}'>{entry['non_error_uncovered']}</td>"
                f"<td class='num' data-sort='{entry['errors']}'>{entry['errors']}</td>"
                f"<td class='num' data-sort='{entry['uncovered']}'>{entry['uncovered']}</td>"
                f"<td class='detail-cell'>{detail_button_html(entry['name'], detail)}</td></tr>"
            )
        else:
            distribution = line_distribution_html(entry["covered"], entry["partial"], entry["non_error_uncovered"], entry["errors"], include_text=False)
            rows.append(
                f"<tr>{path_cell(entry['name'])}{path_cell(files)}"
                f"{metric_cell(hit_pct)}{metric_cell(adjusted_pct)}"
                f"<td>{distribution}</td>"
                f"<td class='num' data-sort='{entry['instances']}'>{entry['instances']}</td>"
                f"<td class='num' data-sort='{entry['covered']}'>{entry['covered']}</td>"
                f"<td class='num' data-sort='{entry['partial']}'>{entry['partial']}</td>"
                f"<td class='num' data-sort='{entry['non_error_uncovered']}'>{entry['non_error_uncovered']}</td>"
                f"<td class='num' data-sort='{entry['errors']}'>{entry['errors']}</td>"
                f"<td class='num' data-sort='{entry['total']}'>{entry['total']}</td>"
                f"<td class='detail-cell'>{detail_button_html(entry['name'], detail)}</td></tr>"
            )

    if mode == "unhit":
        header = "<th>Function</th><th>Location</th><th class='num'>Instances</th><th class='num'>Likely Non-Error Unhit</th><th class='num'>Likely Error-Only</th><th class='num'>Unhit Instrumented Lines</th><th>Details</th>"
    else:
        header = "<th>Function</th><th>Location</th><th class='num'>Hit</th><th class='num'>Hit % Excluding Error-Only</th><th>Line Distribution</th><th class='num'>Instances</th><th class='num'>Full</th><th class='num'>Partial</th><th class='num'>Likely Non-Error Unhit</th><th class='num'>Likely Error-Only</th><th class='num'>Total</th><th>Details</th>"
    open_attr = " open" if mode == "unhit" else ""
    return f"<details{open_attr}><summary>{title}</summary><div class='scroll wide'><table class='sortable compact-table'><thead><tr>{header}</tr></thead><tbody>{''.join(rows)}</tbody></table></div></details>"


def renderer_section_html(run: dict) -> str:
    return (
        f"<section><h2>{html.escape(run['label'])}</h2>"
        f"{renderer_summary_html(run)}"
        "<h3>Area Breakdown</h3>"
        f"{renderer_area_breakdown_html(run)}"
        "<h3>Coverage Rows</h3>"
        f"{file_coverage_rows_html(run)}"
        f"{function_coverage_rows_html(run)}"
        "</section>"
    )


def exclusion_rule_value_html(value) -> str:
    if isinstance(value, dict):
        if not value:
            return "<span class='muted'>None</span>"
        items = []
        for key, entry in value.items():
            items.append(f"<li><code>{html.escape(str(key))}</code>: {exclusion_rule_value_html(entry)}</li>")
        return f"<ul class='rules-list nested'>{''.join(items)}</ul>"
    if isinstance(value, (tuple, list)):
        if not value:
            return "<span class='muted'>None</span>"
        items = []
        for entry in value:
            if isinstance(entry, (tuple, list)):
                items.append(f"<li><code>{html.escape(' + '.join(str(part) for part in entry))}</code></li>")
            else:
                items.append(f"<li><code>{html.escape(str(entry))}</code></li>")
        return f"<ul class='rules-list'>{''.join(items)}</ul>"
    return f"<code>{html.escape(str(value))}</code>"


def exclusion_rules_appendix_html() -> str:
    rows = (
        ("Excluded source prefixes", EXCLUDED_SOURCE_PREFIXES),
        ("Excluded renderer-specific source paths", EXCLUDED_SOURCE_PATHS_BY_RENDERER),
        ("Excluded platform source fragments", EXCLUDED_SOURCE_FRAGMENTS),
        ("Excluded device-specific workaround functions", EXCLUDED_FUNCTION_FRAGMENTS),
        ("Excluded renderer-specific unsupported feature functions", EXCLUDED_FUNCTION_FRAGMENTS_BY_RENDERER),
        ("Excluded non-API template instantiations", EXCLUDED_FUNCTION_FRAGMENT_SEQUENCES),
        ("Excluded renderer-specific function patterns", EXCLUDED_FUNCTION_FRAGMENT_SEQUENCES_BY_RENDERER),
        ("Excluded diagnostic/debug line fragments", EXCLUDED_LINE_FRAGMENTS),
        ("Excluded diagnostic/debug/dead-branch line block fragments", EXCLUDED_LINE_BLOCK_FRAGMENTS),
    )
    table_rows = "".join(
        f"<tr><th>{html.escape(label)}</th><td>{exclusion_rule_value_html(value)}</td></tr>"
        for label, value in rows
    )
    return (
        "<section><h2>Appendix</h2>"
        "<details><summary>Exclusion Rules</summary>"
        "<p class='muted'>These rules are applied before renderer coverage percentages are calculated.</p>"
        f"<table class='rules-table'><tbody>{table_rows}</tbody></table>"
        "</details></section>"
    )


def generate_styled_html(repo: Path, coverage_root: Path, summary: dict, report_path: Path) -> None:
    summary_rows = []
    chart_rows = []
    for run in summary["runs"]:
        distribution = line_distribution_html(run["covered"], run["partial"], run["non_error_uncovered"], run["error_uncovered"])
        summary_rows.append(
            f"<tr><td><b>{html.escape(run['label'])}</b></td>"
            f"{metric_cell(run['hit_pct'])}"
            f"{metric_cell(run['hit_pct_excluding_error'])}"
            f"<td class='num' data-sort='{run['full_pct']:.6f}'>{bar_pct(run['full_pct'])}</td>"
            f"<td>{distribution}</td>"
            f"<td class='num' data-sort='{run['total']}'>{run['total']}</td>"
            f"<td class='num' data-sort='{run['non_error_uncovered']}'>{run['non_error_uncovered']}</td>"
            f"<td class='num' data-sort='{run['error_uncovered']}'>{run['error_uncovered']}</td>"
            f"<td class='num' data-sort='{run['error_uncovered_pct']:.6f}'>{bar_pct(run['error_uncovered_pct'])}</td></tr>"
        )
        chart_rows.append(
            f"<div class='chartrow'><div class='chartlabel'>{html.escape(run['label'])}</div>"
            f"<div>{distribution}</div><div class='{metric_class(run['hit_pct'])}'>{bar_pct(run['hit_pct'])}</div></div>"
        )

    renderer_sections = [renderer_section_html(run) for run in summary["runs"]]

    payload = f"""<!doctype html><html><head><meta charset='utf-8'><title>Unit Test Code Coverage Report</title>
<style>
:root{{--bg:#f7f8fa;--panel:#fff;--ink:#17202a;--muted:#5c6670;--line:#d8dde3;--covered:#2f9e44;--partial:#f0ad00;--uncovered:#d64545;--errors:#2b6cb0;--accent:#2458a7;}}
body{{font-family:Segoe UI,Arial,sans-serif;margin:0;background:var(--bg);color:var(--ink);line-height:1.42;}}
header{{background:#182536;color:#fff;padding:28px 36px;}} header h1{{margin:0 0 8px;font-size:28px;}} header p{{margin:4px 0;color:#dbe4ef;max-width:1200px;}}
main{{padding:24px 36px 60px;}} section{{background:var(--panel);border:1px solid var(--line);border-radius:8px;margin:0 0 22px;padding:20px;box-shadow:0 1px 2px rgba(0,0,0,.04);}} h2{{font-size:22px;margin:0 0 14px;}} h3{{font-size:17px;margin:18px 0 8px;}}
table{{border-collapse:collapse;width:100%;font-size:13px;}} th,td{{border:1px solid var(--line);padding:6px 8px;vertical-align:top;}} th{{background:#eef2f6;text-align:left;}} th.sortable-header{{cursor:pointer;user-select:none;}} th.sortable-header::after{{content:" \\2195";color:#77808a;font-weight:400;}}
td.num,th.num{{text-align:right;font-variant-numeric:tabular-nums;}} code{{background:#eef2f6;padding:1px 4px;border-radius:4px;}} .muted{{color:var(--muted);}} .small{{font-size:12px;}} .path{{font-family:Consolas,Menlo,monospace;font-size:12px;}} .scroll{{max-height:620px;overflow:auto;border:1px solid var(--line);}} .wide{{overflow:auto;}}
.stackbar{{height:14px;background:#e8ebef;border-radius:7px;overflow:hidden;display:flex;min-width:150px;}} .seg{{height:100%;display:block}} .covered{{background:var(--covered)}}.partial{{background:var(--partial)}}.uncovered{{background:var(--uncovered)}}.errors{{background:var(--errors)}} .legend{{display:flex;gap:16px;flex-wrap:wrap;align-items:center;margin:8px 0 18px;line-height:1.4;clear:both;}}.legend-item{{display:inline-flex;align-items:center;gap:6px;white-space:nowrap;}}.legend-swatch{{display:inline-block;width:12px;height:12px;border-radius:3px;flex:0 0 12px;}}
.bad{{color:#a42121;font-weight:600}}.warn{{color:#855f00;font-weight:600}}.good{{color:#206b2d;font-weight:600}}.metriccell.good{{background:#dff3df;color:#0d5614;}}.metriccell.warn{{background:#fff1c7;color:#6a4a00;}}.metriccell.bad{{background:#ffdada;color:#7f1515;}}
.callout{{border-left:4px solid var(--accent);background:#f3f7fd;padding:10px 12px;margin:10px 0;}} details{{margin:10px 0;}} summary{{cursor:pointer;font-weight:600;color:#1f3f6f}} .chartrow{{display:grid;grid-template-columns:150px minmax(180px,1fr) 80px;gap:10px;align-items:center;margin:8px 0}}.chartlabel{{font-weight:600}}.heat td{{font-variant-numeric:tabular-nums}}.heat .cell{{font-weight:600;text-align:right}}.footer{{font-size:12px;color:var(--muted)}}.rules-table th{{width:280px;vertical-align:top;}}.rules-list{{margin:0;padding-left:18px;}}.rules-list.nested{{margin-top:4px;}}
.metriccards{{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:12px;margin:12px 0;}}.metric{{border:1px solid var(--line);border-radius:8px;padding:12px;background:#fbfcfd;}}.metric .value{{font-size:24px;font-weight:700;}}.metric .label{{color:var(--muted);font-size:12px;}}
.coverage-table{{min-width:1500px;table-layout:fixed;}}.coverage-table th:nth-child(1){{width:340px;}}.coverage-table th:nth-child(2){{width:320px;}}.coverage-table th:nth-child(5){{width:190px;}}.coverage-table th:nth-child(11){{width:76px;}}.coverage-table td{{white-space:nowrap;overflow:hidden;text-overflow:ellipsis;vertical-align:middle;}}.coverage-table .pathcell{{overflow:hidden;text-overflow:ellipsis;}}.coverage-table .stackbar{{min-width:120px;}}.compact-table{{min-width:1200px;}}.compact-table td{{white-space:nowrap;vertical-align:middle;}}.area-table .stackbar{{min-width:160px;}}
.detail-button{{border:1px solid var(--accent);background:#fff;color:var(--accent);border-radius:4px;padding:2px 8px;font:inherit;cursor:pointer;}}.detail-button:hover{{background:#f3f7fd;}}.detail-cell{{text-align:center;}}
.modal-backdrop{{position:fixed;inset:0;background:rgba(16,24,38,.45);display:none;align-items:center;justify-content:center;padding:30px;z-index:10;}}.modal-backdrop.open{{display:flex;}}.modal{{background:#fff;border-radius:8px;box-shadow:0 20px 50px rgba(0,0,0,.25);max-width:900px;max-height:80vh;overflow:auto;padding:18px 22px;}}.modal h2{{margin:0 36px 12px 0;}}.modal-close{{position:sticky;top:0;float:right;border:0;background:#eef2f6;border-radius:4px;padding:4px 8px;cursor:pointer;}}.modal dl{{display:grid;grid-template-columns:max-content minmax(260px,1fr);gap:8px 14px;}}.modal dt{{font-weight:600;color:var(--muted);}}.modal dd{{margin:0;}}
</style></head><body>
<header><h1>Unit Test Code Coverage Report</h1><p>Instrumented coverage analysis of the current automated unit tests against each renderer plugin in normal and headless window-system modes.</p><p>The line distribution bars split executable lines into full, partial, likely error-only, and likely non-error unhit sections.</p></header><main>
<section><h2>Executive Summary</h2>
<div class='legend'><span class='legend-item'><span class='legend-swatch covered'></span>Full</span><span class='legend-item'><span class='legend-swatch partial'></span>Partial</span><span class='legend-item'><span class='legend-swatch errors'></span>Errors</span><span class='legend-item'><span class='legend-swatch uncovered'></span>Unhit</span></div>
<table class='sortable'><thead><tr><th>Renderer</th><th class='num'>Hit</th><th class='num'>Hit % Excluding Error-Only</th><th class='num'>Fully Covered</th><th>Line Distribution</th><th class='num'>Executable Lines</th><th class='num'>Likely Non-Error Unhit</th><th class='num'>Likely Error-Only</th><th class='num'>Error-Only %</th></tr></thead><tbody>{''.join(summary_rows)}</tbody></table>
<div class='callout'><b>Interpretation.</b> The headline hit rate counts full and partial lines as hit. The adjusted hit rate removes likely error-only lines from the denominator only, while still showing them in blue in the distribution bars. The red bar segment is therefore reserved for likely non-error executable lines that were not hit.</div>
<h3>Coverage Chart</h3>{''.join(chart_rows)}
</section>
<section><h2>Cross-Renderer Area Heatmap</h2><p class='muted'>Logical area coverage by renderer. Cells show raw hit rate, likely non-error unhit lines, likely error-only lines, adjusted hit rate, and total executable lines.</p>{area_heatmap_html(summary)}</section>
{''.join(renderer_sections)}
<section><h2>Methodology and Scope</h2><p>Coverage root: <code>{html.escape(str(coverage_root))}</code>. Each renderer result merges line coverage from a normal automated-test run and a second run using <code>--window-system Headless</code>. Likely error-only lines are detected heuristically from explicit backend API failure handling, fatal logging, and native error checks. They remain visible in tables and are not removed from the primary hit percentage.</p><p class='footer'>Generated {html.escape(summary['timestamp'])}. Summary JSON: <code>{html.escape(str(coverage_root / 'renderer_coverage_summary.json'))}</code>.</p></section>
{exclusion_rules_appendix_html()}
<div id='detailOverlay' class='modal-backdrop' role='dialog' aria-modal='true' aria-labelledby='detailTitle'><div class='modal'><button type='button' class='modal-close'>Close</button><h2 id='detailTitle'></h2><div id='detailBody'></div></div></div>
</main>
<script>
document.querySelectorAll('table.sortable').forEach(function(table) {{
  table.querySelectorAll('th').forEach(function(header, columnIndex) {{
    header.classList.add('sortable-header');
    header.addEventListener('click', function() {{
      const tbody = table.tBodies[0];
      const rows = Array.from(tbody.rows);
      const direction = header.dataset.sortDirection === 'asc' ? -1 : 1;
      table.querySelectorAll('th').forEach(function(otherHeader) {{ delete otherHeader.dataset.sortDirection; }});
      header.dataset.sortDirection = direction === 1 ? 'asc' : 'desc';
      rows.sort(function(left, right) {{
        const leftCell = left.cells[columnIndex];
        const rightCell = right.cells[columnIndex];
        const leftRaw = leftCell.dataset.sort || leftCell.innerText.trim();
        const rightRaw = rightCell.dataset.sort || rightCell.innerText.trim();
        const leftNumber = Number(String(leftRaw).replace(/,/g, '').replace(/%$/, ''));
        const rightNumber = Number(String(rightRaw).replace(/,/g, '').replace(/%$/, ''));
        if (!Number.isNaN(leftNumber) && !Number.isNaN(rightNumber)) {{
          return (leftNumber - rightNumber) * direction;
        }}
        return String(leftRaw).localeCompare(String(rightRaw), undefined, {{ numeric: true, sensitivity: 'base' }}) * direction;
      }});
      rows.forEach(function(row) {{ tbody.appendChild(row); }});
    }});
  }});
}});
const detailOverlay = document.getElementById('detailOverlay');
const detailTitle = document.getElementById('detailTitle');
const detailBody = document.getElementById('detailBody');
function closeDetailOverlay() {{
  detailOverlay.classList.remove('open');
}}
document.addEventListener('click', function(event) {{
  const detailButton = event.target.closest('.detail-button');
  if (detailButton) {{
    detailTitle.textContent = detailButton.dataset.title || 'Details';
    detailBody.innerHTML = detailButton.dataset.detail || '';
    detailOverlay.classList.add('open');
    event.preventDefault();
    return;
  }}
  if (event.target === detailOverlay || event.target.closest('.modal-close')) {{
    closeDetailOverlay();
  }}
}});
document.addEventListener('keydown', function(event) {{
  if (event.key === 'Escape') {{
    closeDetailOverlay();
  }}
}});
</script></body></html>
"""
    report_path.write_text(payload, encoding="utf-8", newline="\r\n")


def parse_and_report(
    repo: Path,
    coverage_root: Path,
    report_path: Path,
    renderers: list[dict],
    test_root: Path | None,
    configuration: str | None,
) -> dict:
    runs = []
    for renderer in renderers:
        xml_paths = renderer_coverage_xml_paths(coverage_root, renderer)
        runs.append(summarize_module(repo, xml_paths, renderer))

    summary = {
        "repo": str(repo),
        "coverage_root": str(coverage_root),
        "test_root": str(test_root) if test_root is not None else "",
        "configuration": configuration or "",
        "coverage_modes": [
            {
                "key": mode["key"],
                "label": mode["label"],
                "test_args": list(mode["test_args"]),
            }
            for mode in COVERAGE_RUN_MODES
        ],
        "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "runs": runs,
    }
    (coverage_root / "renderer_coverage_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8", newline="\r\n")
    generate_styled_html(repo, coverage_root, summary, report_path)
    return summary


# ---------------------------------------------------------------------------
# Command line entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(description="Generate renderer plugin unit test line coverage reports.")
    parser.add_argument("--source-root", default=str(repo_root()), help="Repository/source root used to resolve source files.")
    parser.add_argument("--test-root", default=None, help="Installed AutomatedTests bundle to run.")
    parser.add_argument("--output-root", default=None, help="Directory where coverage reports and raw data are written.")
    parser.add_argument("--data-root", default=None, help="Raw coverage data directory. Defaults to <output-root>/Data_<stamp>.")
    parser.add_argument("--stamp", default=datetime.now().strftime("%Y%m%d_%H%M%S"))
    parser.add_argument("--parse-only", action="store_true")
    parser.add_argument("--test-filter", default=None)
    parser.add_argument("--renderer", action="append", default=None, help="Renderer key or display label to run. Can be repeated.")
    parser.add_argument("--coverage-tool", default=None, help="Path to Microsoft.CodeCoverage.Console.exe.")
    parser.add_argument("--configuration", default=None, help="Build configuration label to write into the summary metadata.")
    args = parser.parse_args()

    repo = Path(args.source_root).resolve()
    output_root = Path(args.output_root).resolve() if args.output_root else repo / "Output" / "CodeCoverageReport"
    output_root.mkdir(parents=True, exist_ok=True)
    coverage_root = Path(args.data_root).resolve() if args.data_root else output_root / f"Data_{args.stamp}"
    test_root = Path(args.test_root).resolve() if args.test_root else None
    renderers = available_renderers(test_root, args.renderer) if test_root is not None else RENDERERS

    report_path = output_root / f"UnitTestCodeCoverageReport_{args.stamp}.html"

    if not args.parse_only:
        if test_root is None:
            raise RuntimeError("--test-root is required unless --parse-only is used")
        coverage_tool = resolve_coverage_tool(args.coverage_tool)
        print(f"Using coverage tool: {coverage_tool}", flush=True)
        print(f"Using installed test bundle: {test_root}", flush=True)
        run_collect(repo, test_root, coverage_root, args.test_filter, coverage_tool, renderers)

    summary = parse_and_report(repo, coverage_root, report_path, renderers, test_root, args.configuration)
    print(f"Report: {report_path}")
    for run in summary["runs"]:
        print(f"{run['label']}: {run['hit_pct']:.2f}% hit, {run['hit_pct_excluding_error']:.2f}% excluding likely error-only, {run['full_pct']:.2f}% full")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
