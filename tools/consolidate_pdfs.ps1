<#
.SYNOPSIS
    Move published PDFs from a parallel PDF tree to sit beside their Markdown
    sources, the layout DocBoss works with.

.DESCRIPTION
    For a Markdown tree and a PDF tree that mirror each other (the same
    folders, the same names), each PDF is paired with its source by RELATIVE
    PATH: <PdfRoot>\a\b\guide.pdf belongs to <MarkdownRoot>\a\b\guide.md (or
    .markdown/.mdown/.mkd/.mdwn).  Every PDF lands in exactly one group:

      Paired      its source exists; the PDF moves beside it, and PDFBoss's
                  sidecars (<stem>.toc, <stem>.json, <stem>.bookmarks.json)
                  move with it.
      Conflict    a PDF of that name is already beside the source.  Left
                  where it is, never overwritten.
      Orphan      no source.  Left where it is, unless -MoveOrphans, which
                  moves it to the same relative folder under MarkdownRoot.

    Markdown documents with no PDF are listed as "Not published".

    DRY RUN BY DEFAULT: without -Apply nothing is touched, and the report is
    what -Apply would do.  With -Apply, nothing is ever overwritten or
    deleted; each move is checked afterwards, and the group counts must add
    up to the number of PDFs found or the script says so and exits non-zero.

    The PDF tree is NOT removed afterwards.  Check the result, then delete it
    yourself.

    Output goes to the console only -- file names can be sensitive, so the
    script writes no log file anywhere.

.EXAMPLE
    .\consolidate_pdfs.ps1 -MarkdownRoot D:\Docs\src -PdfRoot D:\Docs\pdf
    .\consolidate_pdfs.ps1 -MarkdownRoot D:\Docs\src -PdfRoot D:\Docs\pdf -Apply
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$MarkdownRoot,
    [Parameter(Mandatory = $true)][string]$PdfRoot,
    [switch]$Apply,
    [switch]$MoveOrphans
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

# Bounded (Rule of 10): a tree bigger than this is not a documents folder.
$MaxFiles = 100000
$MarkdownExts = @(".md", ".markdown", ".mdown", ".mkd", ".mdwn")
$Sidecars = @(".toc", ".json", ".bookmarks.json")

function Resolve-Root([string]$path, [string]$what) {
    if (-not (Test-Path -LiteralPath $path -PathType Container)) {
        throw "$what '$path' is not a folder."
    }
    return (Resolve-Path -LiteralPath $path).ProviderPath.TrimEnd('\')
}

function Get-Relative([string]$root, [string]$full) {
    return $full.Substring($root.Length).TrimStart('\')
}

# Where a PDF's sidecar would be: "<dir>\<stem><suffix>".
function Get-Sidecar([string]$pdf, [string]$suffix) {
    $dir = [System.IO.Path]::GetDirectoryName($pdf)
    $stem = [System.IO.Path]::GetFileNameWithoutExtension($pdf)
    return Join-Path $dir ($stem + $suffix)
}

function Move-Checked([string]$from, [string]$to) {
    if (Test-Path -LiteralPath $to) {
        throw "refusing to overwrite $to"
    }
    $folder = [System.IO.Path]::GetDirectoryName($to)
    if (-not (Test-Path -LiteralPath $folder)) {
        New-Item -ItemType Directory -Path $folder | Out-Null
    }
    Move-Item -LiteralPath $from -Destination $to
    if (-not (Test-Path -LiteralPath $to) -or (Test-Path -LiteralPath $from)) {
        throw "move did not complete: $from -> $to"
    }
}

$md = Resolve-Root $MarkdownRoot "MarkdownRoot"
$pdfRoot = Resolve-Root $PdfRoot "PdfRoot"
if ($md -ieq $pdfRoot) {
    throw "MarkdownRoot and PdfRoot are the same folder; nothing to consolidate."
}
if ($pdfRoot.StartsWith($md + '\', [System.StringComparison]::OrdinalIgnoreCase) -or
    $md.StartsWith($pdfRoot + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
    Write-Warning "One root is inside the other.  That works, but check the dry run carefully."
}

$pdfs = @(Get-ChildItem -LiteralPath $pdfRoot -Recurse -File -Filter *.pdf |
          Select-Object -First ($MaxFiles + 1))
if ($pdfs.Count -gt $MaxFiles) { throw "More than $MaxFiles PDFs under $pdfRoot; refusing." }
$sources = @(Get-ChildItem -LiteralPath $md -Recurse -File |
             Where-Object { $MarkdownExts -contains $_.Extension.ToLowerInvariant() } |
             Select-Object -First ($MaxFiles + 1))
if ($sources.Count -gt $MaxFiles) { throw "More than $MaxFiles documents under $md; refusing." }

$paired = @(); $conflict = @(); $orphan = @(); $published = @{}
foreach ($pdf in $pdfs) {
    $rel = Get-Relative $pdfRoot $pdf.FullName
    $relDir = [System.IO.Path]::GetDirectoryName($rel)
    $stem = [System.IO.Path]::GetFileNameWithoutExtension($rel)
    $source = $null
    foreach ($ext in $MarkdownExts) {
        $candidate = Join-Path (Join-Path $md $relDir) ($stem + $ext)
        if (Test-Path -LiteralPath $candidate -PathType Leaf) { $source = $candidate; break }
    }
    if ($null -eq $source) {
        $orphan += [pscustomobject]@{ Pdf = $pdf.FullName; Rel = $rel }
        continue
    }
    $published[$source.ToLowerInvariant()] = $true
    $target = Join-Path ([System.IO.Path]::GetDirectoryName($source)) ($stem + ".pdf")
    if (Test-Path -LiteralPath $target) {
        $conflict += [pscustomobject]@{ Pdf = $pdf.FullName; Target = $target }
    } else {
        $paired += [pscustomobject]@{ Pdf = $pdf.FullName; Target = $target }
    }
}
$unpublished = @($sources | Where-Object { -not $published.ContainsKey($_.FullName.ToLowerInvariant()) })

$mode = if ($Apply) { "APPLY" } else { "DRY RUN (nothing will be changed; add -Apply)" }
Write-Host "consolidate_pdfs: $mode"
Write-Host "  Markdown root: $md"
Write-Host "  PDF root:      $pdfRoot"
Write-Host ""

Write-Host "Paired ($($paired.Count)) - moves beside its source:"
foreach ($p in $paired) { Write-Host "  $($p.Pdf)`n    -> $($p.Target)" }
Write-Host "Conflict ($($conflict.Count)) - a PDF is already there; left in place:"
foreach ($c in $conflict) { Write-Host "  $($c.Pdf)`n    (exists: $($c.Target))" }
$orphanNote = if ($MoveOrphans) { "moved to the same folder under the Markdown root" } else { "left in place (-MoveOrphans to move)" }
Write-Host "Orphan ($($orphan.Count)) - no source; ${orphanNote}:"
foreach ($o in $orphan) { Write-Host "  $($o.Pdf)" }
Write-Host "Not published ($($unpublished.Count)) - documents with no PDF:"
foreach ($u in $unpublished) { Write-Host "  $($u.FullName)" }
Write-Host ""

$total = $paired.Count + $conflict.Count + $orphan.Count
if ($total -ne $pdfs.Count) {
    Write-Error "Group counts ($total) do not add up to the PDFs found ($($pdfs.Count)); nothing done."
    exit 2
}

if (-not $Apply) {
    Write-Host "Dry run complete: $($pdfs.Count) PDF(s) found, $($paired.Count) would move."
    exit 0
}

$moved = 0; $sidecarsMoved = 0; $failed = 0
$work = @($paired)
if ($MoveOrphans) {
    foreach ($o in $orphan) {
        $target = Join-Path $md $o.Rel
        if (Test-Path -LiteralPath $target) {
            Write-Warning "Orphan not moved, target exists: $target"
            continue
        }
        $work += [pscustomobject]@{ Pdf = $o.Pdf; Target = $target }
    }
}
foreach ($item in $work) {
    try {
        Move-Checked $item.Pdf $item.Target
        $moved++
        foreach ($suffix in $Sidecars) {
            $side = Get-Sidecar $item.Pdf $suffix
            if (Test-Path -LiteralPath $side -PathType Leaf) {
                $sideTarget = Get-Sidecar $item.Target $suffix
                if (-not (Test-Path -LiteralPath $sideTarget)) {
                    Move-Checked $side $sideTarget
                    $sidecarsMoved++
                }
            }
        }
    } catch {
        $failed++
        Write-Warning "Not moved: $($item.Pdf) -- $($_.Exception.Message)"
    }
}
Write-Host "Moved $moved PDF(s) and $sidecarsMoved sidecar(s); $failed failed."
Write-Host "The PDF tree was not removed.  Check the result, then delete what is left of it yourself."
if ($failed -gt 0) { exit 1 }
exit 0
