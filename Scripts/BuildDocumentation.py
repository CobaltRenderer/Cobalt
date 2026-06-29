#!/usr/bin/env python3
"""
build_docs.py

Generates HTML documentation from XML sources, matching the behavior
of the original MSBuild BuildDocs.proj:

- Copies Resources.
- Discovers XML pages.
- Gets XMLOutputFileName from /doc:XMLDocContent/@PageName.
- Builds XMLPageFilePresentList = [Name1];[Name2];...
- Builds a combined TableOfContents.xml from TOCRoot.xml + TOC fragments
  in each page file.
- Runs XSLT (ConvertDocsToHTML, ConvertTOCToHTML, ConvertDocsProjectToHTML)
  via xsltproc.

Requires:
    - Python 3
    - xsltproc (path passed via --xslt-tool)
"""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple
import xml.etree.ElementTree as ET
import copy
from xml.sax.saxutils import escape

DOC_NS = "http://www.cobaltrenderer.com/schema/XMLDocSchema.xsd"


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def local_name(tag: str) -> str:
    if "}" in tag:
        return tag.split("}", 1)[1]
    return tag


def write_typefiles_present_doc(output_root: Path, value: str) -> Path:
    """
    Write the (potentially huge) TypeFilesPresent string into a small XML file
    and return its path.
    """
    doc_path = output_root / "TypeFilesPresent.xml"
    # Escape for XML attribute usage
    escaped_value = escape(value, {'"': '&quot;'})
    doc_path.write_text(
        f'<TypeFilesPresent value="{escaped_value}"/>',
        encoding="utf-8"
    )
    return doc_path

def run_xslt(xslt_tool: List[str],
             xsl_path: Path,
             xml_input: Path,
             html_output: Path,
             params: Dict[str, str] = None) -> None:
    """
    Run an XSLT transformation using xsltproc (or compatible tool),
    capturing stdout and writing it to html_output.
    """
    params = params or {}
    cmd = list(xslt_tool)

    # xsltproc: --stringparam Name Value
    for name, value in params.items():
        cmd.extend(["--stringparam", name, value])

    cmd.extend([str(xsl_path), str(xml_input)])

    print(f"[build_docs] XSLT: {' '.join(cmd)}")

    proc = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    if proc.returncode != 0:
        if proc.stderr:
            sys.stderr.write(proc.stderr)
        print(f"[build_docs] ERROR: XSLT tool failed with exit code {proc.returncode}",
              file=sys.stderr)
        sys.exit(proc.returncode)

    html_output.parent.mkdir(parents=True, exist_ok=True)
    html_output.write_text(proc.stdout, encoding="utf-8")


def copy_resources(resource_dirs: List[Path], output_root: Path) -> None:
    """
    Copy Resources/... into OutputRoot preserving relative paths under
    each resource dir, like BuildResourceFilesGroup + Copy task.
    """
    for res_root in resource_dirs:
        res_root = res_root.resolve()
        if not res_root.is_dir():
            continue

        for src_path in res_root.rglob("*"):
            if not src_path.is_file():
                continue

            rel = src_path.relative_to(res_root)
            dst_path = output_root / rel
            dst_path.parent.mkdir(parents=True, exist_ok=True)

            print(f"[build_docs] Copy resource: {src_path} -> {dst_path}")
            shutil.copy2(src_path, dst_path)


def discover_all_xml(xml_input_dirs: List[Path]) -> List[Path]:
    """
    Equivalent of XMLPageFileGroupRaw: %(XMLPageInputFoldersRaw.Identity)/**/*.xml
    """
    files: List[Path] = []
    for root in xml_input_dirs:
        root = root.resolve()
        if not root.is_dir():
            continue

        for path in root.rglob("*.xml"):
            files.append(path.resolve())
    return files


def get_page_name(xml_path: Path) -> str:
    """
    Equivalent of XmlPeek:
      Query="/doc:XMLDocContent/@PageName"

    We parse the XML and look at the root element's attributes, picking
    the one whose local name is "PageName".
    """
    try:
        tree = ET.parse(str(xml_path))
        root = tree.getroot()
    except Exception as e:
        print(f"[build_docs] WARNING: Failed to parse {xml_path}: {e}; skipping PageName",
              file=sys.stderr)
        return ""

    for attr_name, attr_value in root.attrib.items():
        if local_name(attr_name) == "PageName" and attr_value and attr_value.strip():
            return attr_value.strip()

    return ""


def extract_toc_fragment(xml_path: Path) -> Optional[Tuple[str, ET.Element]]:
    """
    Equivalent of the BuildCombinedTOCFile XmlPeek calls:

      /doc:XMLDocTOC/doc:TOCFragment/@Name
      /doc:XMLDocTOC/doc:TOCFragment[@Name='...']

    Returns (fragment_name, fragment_element) if present, else None.
    """
    try:
        tree = ET.parse(str(xml_path))
        root = tree.getroot()
    except Exception as e:
        print(f"[build_docs] WARNING: Failed to parse {xml_path}: {e}; skipping TOC fragment",
              file=sys.stderr)
        return None

    xml_doc_toc = None
    for elem in root.iter():
        if local_name(elem.tag) == "XMLDocTOC":
            xml_doc_toc = elem
            break

    if xml_doc_toc is None:
        return None

    toc_fragment = None
    for child in xml_doc_toc:
        if local_name(child.tag) == "TOCFragment":
            toc_fragment = child
            break

    if toc_fragment is None:
        return None

    frag_name = ""
    for attr_name, attr_value in toc_fragment.attrib.items():
        if local_name(attr_name) == "Name" and attr_value and attr_value.strip():
            frag_name = attr_value.strip()
            break

    if not frag_name:
        return None

    return frag_name, toc_fragment


def build_combined_toc(toc_source: Path,
                       output_dir: Path,
                       all_xml_files: List[Path]) -> Path:
    """
    Reproduce CopyRootTOCFile + BuildCombinedTOCFile:

      - Copy TOCRoot.xml -> OutputFileDir\\TableOfContents.xml
      - For each XML page file:
          * find doc:XMLDocTOC/doc:TOCFragment Name="X"
          * find //doc:TOCFragmentRef[@Name='X'] in TableOfContents.xml
          * replace the TOCFragmentRef with the TOCFragment element
    """
    toc_dest = output_dir / "TableOfContents.xml"
    toc_dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(toc_source, toc_dest)
    print(f"[build_docs] Copied TOC root to {toc_dest}")

    tree = ET.parse(str(toc_dest))
    root = tree.getroot()

    # Build map of fragmentName -> fragmentElement
    fragments: Dict[str, ET.Element] = {}
    for xml_path in all_xml_files:
        result = extract_toc_fragment(xml_path)
        if not result:
            continue
        frag_name, frag_elem = result
        fragments[frag_name] = copy.deepcopy(frag_elem)
        print(f"[build_docs] Found TOC fragment '{frag_name}' in {xml_path}")

    # Replace TOCFragmentRef placeholders
    for frag_name, frag_elem in fragments.items():
        replaced_any = False

        for parent in root.iter():
            children = list(parent)
            for idx, child in enumerate(children):
                if local_name(child.tag) != "TOCFragmentRef":
                    continue

                name_attr = None
                for attr_name, attr_value in child.attrib.items():
                    if local_name(attr_name) == "Name":
                        name_attr = attr_value
                        break

                if name_attr == frag_name:
                    parent.remove(child)
                    parent.insert(idx, copy.deepcopy(frag_elem))
                    replaced_any = True

        if not replaced_any:
            print(f"[build_docs] WARNING: TOC fragment ref '{frag_name}' "
                  f"not found in TOC root.", file=sys.stderr)

    tree.write(str(toc_dest), encoding="utf-8", xml_declaration=True)
    return toc_dest


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(
        description="Generate HTML docs from XML using xsltproc, matching BuildDocs.proj logic."
    )

    parser.add_argument("--source-root", required=True)
    parser.add_argument("--output-root", required=True)
    parser.add_argument("--xslt-tool", required=True,
                        help="Path to xsltproc executable (no extra args).")
    parser.add_argument("--project-xml", default="Project.xml")
    parser.add_argument("--toc-xml", default="TOCRoot.xml")
    parser.add_argument("--xml-input-dirs", nargs="*", default=None)
    parser.add_argument("--resource-dirs", nargs="*", default=None)
    parser.add_argument("--xsl-docs",
                        default=os.path.join("XML Transforms", "ConvertDocsToHTML.xsl"))
    parser.add_argument("--xsl-toc",
                        default=os.path.join("XML Transforms", "ConvertTOCToHTML.xsl"))
    parser.add_argument("--xsl-project",
                        default=os.path.join("XML Transforms", "ConvertDocsProjectToHTML.xsl"))

    args = parser.parse_args(argv)

    source_root = Path(args.source_root).resolve()
    output_root = Path(args.output_root).resolve()
    html_root = output_root / "html"

    xslt_tool = [args.xslt_tool]

    project_xml = source_root / args.project_xml
    toc_xml = source_root / args.toc_xml

    if args.xml_input_dirs:
        xml_input_dirs = [
            Path(d) if Path(d).is_absolute() else (source_root / d)
            for d in args.xml_input_dirs
        ]
    else:
        xml_input_dirs = [source_root]

    if args.resource_dirs:
        resource_dirs = [
            (p if p.is_absolute() else source_root / p)
            for p in (Path(d) for d in args.resource_dirs)
        ]
    else:
        resource_dirs = [source_root / "Resources"]

    xsl_docs = source_root / args.xsl_docs
    xsl_toc = source_root / args.xsl_toc
    xsl_project = source_root / args.xsl_project

    print(f"[build_docs] Source root:        {source_root}")
    print(f"[build_docs] Output root:        {output_root}")
    print(f"[build_docs] HTML root:          {html_root}")
    print(f"[build_docs] XSLT tool:          {args.xslt_tool}")
    print(f"[build_docs] Project XML:        {project_xml}")
    print(f"[build_docs] TOC XML:            {toc_xml}")
    print(f"[build_docs] XML input dirs:     {xml_input_dirs}")
    print(f"[build_docs] Resource dirs:      {resource_dirs}")
    print(f"[build_docs] XSL (docs):         {xsl_docs}")
    print(f"[build_docs] XSL (toc):          {xsl_toc}")
    print(f"[build_docs] XSL (project):      {xsl_project}")

    for p in [project_xml, toc_xml, xsl_docs, xsl_toc, xsl_project]:
        if not p.is_file():
            print(f"ERROR: Required file not found: {p}", file=sys.stderr)
            return 1

    # 1) Copy resources (like BuildResourceFilesGroup + Copy)
    copy_resources(resource_dirs, output_root)

    # 2) Discover all XML page files (XMLPageFileGroupRaw)
    all_xml_files = discover_all_xml(xml_input_dirs)
    print(f"[build_docs] Found {len(all_xml_files)} XML files total.")

    # 3) Compute XMLOutputFileName from PageName (SetXMLPageFileGroupMetadata)
    page_name_by_xml: Dict[Path, str] = {}
    for xml_path in all_xml_files:
        name = get_page_name(xml_path)
        if name:
            page_name_by_xml[xml_path] = name
            print(f"[build_docs] Page '{name}' from {xml_path}")

    if not page_name_by_xml:
        print("[build_docs] WARNING: No XML pages with PageName found.")

    # 4) XMLPageFilePresentList: @(XMLPageFileGroup -> '[%(XMLOutputFileName)]')
    #    (semicolon-separated list)
    xml_page_file_present_list = ";".join(f"[{name}]" for name in page_name_by_xml.values())
    print(f"[build_docs] XMLPageFilePresentList: {xml_page_file_present_list}")
    typefiles_doc = write_typefiles_present_doc(output_root, xml_page_file_present_list)

    # 5) Build combined TOC: CopyRootTOCFile + BuildCombinedTOCFile
    combined_toc_xml = build_combined_toc(toc_xml, output_root, all_xml_files)

    # 6) Per-page docs: ConvertDocsToHTML.xsl
    for xml_path, out_name in page_name_by_xml.items():
        html_path = html_root / f"{out_name}.html"
        run_xslt(xslt_tool, xsl_docs, xml_path, html_path,
                 params={"TypeFilesPresentDoc": typefiles_doc.as_uri()})

    # 7) TOC HTML: ConvertTOCToHTML.xsl over combined TOC
    toc_html = html_root / "TableOfContents.html"
    run_xslt(xslt_tool, xsl_toc, combined_toc_xml, toc_html,
             params={"TypeFilesPresentDoc": typefiles_doc.as_uri()})

    # 8) Index page: ConvertDocsProjectToHTML.xsl
    index_html = output_root / "index.html"
    run_xslt(xslt_tool, xsl_project, project_xml, index_html,
             params={"TypeFilesPresentDoc": typefiles_doc.as_uri()})

    print("[build_docs] Documentation build complete.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
