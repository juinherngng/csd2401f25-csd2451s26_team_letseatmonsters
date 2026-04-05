param(
    [string[]]$Files
)

$ErrorActionPreference = 'Stop'

function Get-TargetFiles {
    param([string[]]$RequestedFiles)

    if ($RequestedFiles -and $RequestedFiles.Count -gt 0) {
        return $RequestedFiles
    }

    $roots = @('TheStove', 'MyoonchiDiner', 'src', 'tests')
    $extensions = @('*.cpp', '*.hpp', '*.h', '*.cxx', '*.cc')
    $results = New-Object System.Collections.Generic.List[string]

    foreach ($root in $roots) {
        if (-not (Test-Path $root)) {
            continue
        }

        foreach ($ext in $extensions) {
            Get-ChildItem $root -Recurse -File -Filter $ext | ForEach-Object {
                $results.Add($_.FullName)
            }
        }
    }

    return $results
}

function Get-LineEnding {
    param([string]$Content)

    if ($Content.Contains("`r`n")) {
        return "`r`n"
    }

    return "`n"
}

function Get-CanonicalIncludeFromResolvedPath {
    param(
        [string]$ResolvedPath,
        [string]$EngineCoreRoot,
        [string]$EngineGraphicsRoot,
        [string]$GameCoreRoot,
        [string]$GameRoot
    )

    if ($ResolvedPath.StartsWith($EngineCoreRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return 'EngineCore/{0}' -f (Split-Path $ResolvedPath -Leaf)
    }
    if ($ResolvedPath.StartsWith($EngineGraphicsRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return 'EngineGraphics/{0}' -f (Split-Path $ResolvedPath -Leaf)
    }
    if ($ResolvedPath.StartsWith($GameCoreRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return 'GameCore/{0}' -f (Split-Path $ResolvedPath -Leaf)
    }
    if ($ResolvedPath.StartsWith($GameRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return 'MyoonchiDiner/{0}' -f (Split-Path $ResolvedPath -Leaf)
    }

    return $null
}

function Get-CanonicalIncludeFromLeaf {
    param(
        [string]$Leaf,
        [string]$EngineCoreRoot,
        [string]$EngineGraphicsRoot,
        [string]$GameCoreRoot,
        [string]$GameRoot
    )

    $engineCoreCandidate = Join-Path $EngineCoreRoot $Leaf
    if (Test-Path $engineCoreCandidate) {
        return 'EngineCore/{0}' -f $Leaf
    }

    $engineGraphicsCandidate = Join-Path $EngineGraphicsRoot $Leaf
    if (Test-Path $engineGraphicsCandidate) {
        return 'EngineGraphics/{0}' -f $Leaf
    }

    $gameCoreCandidate = Join-Path $GameCoreRoot $Leaf
    if (Test-Path $gameCoreCandidate) {
        return 'GameCore/{0}' -f $Leaf
    }

    $gameRootCandidate = Join-Path $GameRoot $Leaf
    if (Test-Path $gameRootCandidate) {
        return 'MyoonchiDiner/{0}' -f $Leaf
    }

    return $null
}

function Normalize-IncludeBlock {
    param(
        [string]$Content,
        [string]$FilePath
    )

    $lineEnding = Get-LineEnding -Content $Content
    $lines = [System.Collections.Generic.List[string]]::new()
    foreach ($line in ($Content -split "\r?\n")) {
        $lines.Add($line)
    }

    $firstInclude = -1
    for ($i = 0; $i -lt $lines.Count; ++$i) {
        if ($lines[$i] -match '^\s*#include\s+[<"].+[>"](?:\s*//.*)?\s*$') {
            $firstInclude = $i
            break
        }
    }

    if ($firstInclude -lt 0) {
        return $null
    }

    $blockEnd = $firstInclude
    while ($blockEnd -lt $lines.Count) {
        $line = $lines[$blockEnd]
        if ($line -match '^\s*#include\s+[<"].+[>"](?:\s*//.*)?\s*$' -or $line -match '^\s*$') {
            ++$blockEnd
            continue
        }
        break
    }

    $repoRoot = (Resolve-Path '.').Path
    $engineCoreRoot = Join-Path $repoRoot 'TheStove\EngineCore'
    $engineGraphicsRoot = Join-Path $repoRoot 'TheStove\EngineGraphics'
    $gameCoreRoot = Join-Path $repoRoot 'MyoonchiDiner\GameCore'
    $gameRoot = Join-Path $repoRoot 'MyoonchiDiner'

    function Normalize-IncludeLine {
        param(
            [string]$Line,
            [string]$OwningFile,
            [string]$EngineCoreRoot,
            [string]$EngineGraphicsRoot,
            [string]$GameCoreRoot,
            [string]$GameRoot
        )

        $trimmed = $Line.Trim()
        $commentSuffix = ''
        if ($trimmed -match '^(#include\s+[<"].+[>"])\s*(//.*)$') {
            $trimmed = $matches[1]
            $commentSuffix = ' ' + $matches[2]
        }

        if ($trimmed -match '^#include\s+<') {
            return $trimmed + $commentSuffix
        }

        if ($trimmed -notmatch '^#include\s+"([^"]+)"$') {
            return $trimmed + $commentSuffix
        }

        $includePath = $matches[1].Replace('\\', '/')
        $includePath = $includePath -replace '^\.\./\.\./TheStove/Core/', 'EngineCore/'
        $includePath = $includePath -replace '^\.\./\.\./TheStove/Graphics/', 'EngineGraphics/'
        $includePath = $includePath -replace '^\.\./Core/', 'Core/'
        $includePath = $includePath -replace '^\.\./Graphics/', 'Graphics/'

        if ($includePath -match '^(EngineCore|EngineGraphics|GameCore)/') {
            return ('#include "{0}"' -f $includePath) + $commentSuffix
        }

        $owningDirectory = Split-Path -Parent $OwningFile
        $relativeCandidate = Join-Path $owningDirectory $includePath
        try {
            $resolvedCandidate = [System.IO.Path]::GetFullPath($relativeCandidate)
        }
        catch {
            $resolvedCandidate = $null
        }

        $canonicalPath = $null
        if ($resolvedCandidate -and (Test-Path $resolvedCandidate)) {
            $canonicalPath = Get-CanonicalIncludeFromResolvedPath -ResolvedPath $resolvedCandidate -EngineCoreRoot $EngineCoreRoot -EngineGraphicsRoot $EngineGraphicsRoot -GameCoreRoot $GameCoreRoot -GameRoot $GameRoot
        }

        if ($null -eq $canonicalPath) {
            $leaf = Split-Path $includePath -Leaf
            $canonicalPath = Get-CanonicalIncludeFromLeaf -Leaf $leaf -EngineCoreRoot $EngineCoreRoot -EngineGraphicsRoot $EngineGraphicsRoot -GameCoreRoot $GameCoreRoot -GameRoot $GameRoot
        }

        if ($null -ne $canonicalPath) {
            return ('#include "{0}"' -f $canonicalPath) + $commentSuffix
        }

        return ('#include "{0}"' -f $includePath) + $commentSuffix
    }

    $includeLines = @()
    for ($i = $firstInclude; $i -lt $blockEnd; ++$i) {
        if ($lines[$i] -match '^\s*#include\s+[<"].+[>"](?:\s*//.*)?\s*$') {
            $includeLines += (Normalize-IncludeLine -Line $matches[0] -OwningFile $FilePath -EngineCoreRoot $engineCoreRoot -EngineGraphicsRoot $engineGraphicsRoot -GameCoreRoot $gameCoreRoot -GameRoot $gameRoot)
        }
    }

    if ($includeLines.Count -eq 0) {
        return $null
    }

    $angleIncludes = $includeLines | Where-Object { $_ -match '^#include\s+<' } | Sort-Object -Unique
    $quoteIncludes = $includeLines | Where-Object { $_ -match '^#include\s+"' } | Sort-Object -Unique

    $newBlock = [System.Collections.Generic.List[string]]::new()
    foreach ($include in $angleIncludes) {
        $newBlock.Add($include)
    }
    if ($quoteIncludes.Count -gt 0) {
        if ($newBlock.Count -gt 0) {
            $newBlock.Add('')
        }
        foreach ($include in ($quoteIncludes | Sort-Object -Unique)) {
            $newBlock.Add($include)
        }
    }

    $before = @()
    if ($firstInclude -gt 0) {
        $before = $lines.GetRange(0, $firstInclude)
    }

    $after = @()
    if ($blockEnd -lt $lines.Count) {
        $after = $lines.GetRange($blockEnd, $lines.Count - $blockEnd)
    }

    function Normalize-ForwardDeclarationSpacing {
        param([string[]]$AfterLines)

        if (-not $AfterLines -or $AfterLines.Count -eq 0) {
            return $AfterLines
        }

        $normalizedAfter = [System.Collections.Generic.List[string]]::new()
        foreach ($line in $AfterLines) {
            $normalizedAfter.Add($line)
        }

        $index = 0
        while ($index -lt $normalizedAfter.Count -and $normalizedAfter[$index] -match '^\s*$') {
            ++$index
        }

        if ($index -ge $normalizedAfter.Count) {
            return $normalizedAfter
        }

        $forwardDeclarationPattern = '^\s*(class|struct)\s+\w+\s*;\s*$'
        $scan = $index
        $foundForwardDeclarations = $false

        while ($scan -lt $normalizedAfter.Count) {
            if ($normalizedAfter[$scan] -match $forwardDeclarationPattern) {
                $foundForwardDeclarations = $true
                ++$scan
                continue
            }

            if ($normalizedAfter[$scan] -match '^\s*namespace\s+\w+\s*\{\s*$') {
                $namespaceStart = $scan
                ++$scan
                $namespaceForwardDeclarations = $false
                while ($scan -lt $normalizedAfter.Count -and $normalizedAfter[$scan] -match $forwardDeclarationPattern) {
                    $namespaceForwardDeclarations = $true
                    ++$scan
                }
                if ($namespaceForwardDeclarations -and $scan -lt $normalizedAfter.Count -and $normalizedAfter[$scan] -match '^\s*\}\s*$') {
                    $foundForwardDeclarations = $true
                    ++$scan
                    continue
                }
                $scan = $namespaceStart
                break
            }

            break
        }

        if (-not $foundForwardDeclarations) {
            return $normalizedAfter
        }

        while ($scan -lt $normalizedAfter.Count -and $normalizedAfter[$scan] -match '^\s*$') {
            $normalizedAfter.RemoveAt($scan)
        }

        if ($scan -lt $normalizedAfter.Count) {
            $normalizedAfter.Insert($scan, '')
        }

        return $normalizedAfter
    }

    $after = Normalize-ForwardDeclarationSpacing -AfterLines $after

    $result = [System.Collections.Generic.List[string]]::new()
    foreach ($line in $before) { $result.Add($line) }
    foreach ($line in $newBlock) { $result.Add($line) }

    if ($result.Count -gt 0 -and $after.Count -gt 0 -and $after[0] -notmatch '^\s*$') {
        $result.Add('')
    }

    foreach ($line in $after) { $result.Add($line) }

    while ($result.Count -gt 0 -and $result[$result.Count - 1] -match '^\s*$') {
        $result.RemoveAt($result.Count - 1)
    }

    $newContent = [string]::Join($lineEnding, $result) + $lineEnding
    if ($newContent -ceq $Content) {
        return $null
    }

    return $newContent
}

function Normalize-TopOfFileLayout {
    param([string]$Content)

    $lineEnding = Get-LineEnding -Content $Content
    $lines = [System.Collections.Generic.List[string]]::new()
    foreach ($line in ($Content -split "\r?\n")) {
        $lines.Add($line)
    }

    while ($lines.Count -gt 0 -and $lines[$lines.Count - 1] -match '^\s*$') {
        $lines.RemoveAt($lines.Count - 1)
    }

    $commentEnd = -1
    for ($i = 0; $i -lt $lines.Count; ++$i) {
        if ($lines[$i] -match '^\s*\*/\s*$') {
            $commentEnd = $i
            break
        }
    }

    if ($commentEnd -ge 0) {
        $firstCode = $commentEnd + 1
        while ($firstCode -lt $lines.Count -and $lines[$firstCode] -match '^\s*$') {
            $lines.RemoveAt($firstCode)
        }
        if ($firstCode -lt $lines.Count) {
            $lines.Insert($firstCode, '')
        }
    }

    $pragmaIndex = -1
    for ($i = 0; $i -lt $lines.Count; ++$i) {
        if ($lines[$i] -match '^\s*#pragma\s+once\s*$') {
            $pragmaIndex = $i
            break
        }
    }

    if ($pragmaIndex -ge 0) {
        $afterPragma = $pragmaIndex + 1
        while ($afterPragma -lt $lines.Count -and $lines[$afterPragma] -match '^\s*$') {
            $lines.RemoveAt($afterPragma)
        }
        if ($afterPragma -lt $lines.Count) {
            $lines.Insert($afterPragma, '')
        }
    }

    $newContent = [string]::Join($lineEnding, $lines) + $lineEnding
    if ($newContent -ceq $Content) {
        return $null
    }

    return $newContent
}

$targetFiles = Get-TargetFiles -RequestedFiles $Files
if (-not $targetFiles -or $targetFiles.Count -eq 0) {
    Write-Host 'No source files found.'
    exit 0
}

$changed = 0
foreach ($file in $targetFiles) {
    $resolved = Resolve-Path $file -ErrorAction SilentlyContinue
    if (-not $resolved) {
        continue
    }

    $content = [System.IO.File]::ReadAllText($resolved)
    $updated = Normalize-IncludeBlock -Content $content -FilePath $resolved
    if ($null -ne $updated) {
        $content = $updated
    }

    $layoutUpdated = Normalize-TopOfFileLayout -Content $content
    if ($null -ne $layoutUpdated) {
        $content = $layoutUpdated
        $updated = $content
    }

    if ($null -eq $updated) {
        continue
    }

    [System.IO.File]::WriteAllText($resolved, $content, [System.Text.UTF8Encoding]::new($false))
    ++$changed
}

Write-Host "Sorted include blocks in $changed file(s)."
