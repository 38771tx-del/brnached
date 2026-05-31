param(
    [string]$LauncherPath = ".\PCM_SOURCE\PCM_LAUNCHER\launcher.cpp"
)

$ErrorActionPreference = "Stop"

if (!(Test-Path $LauncherPath)) {
    Write-Host "Could not find launcher.cpp at: $LauncherPath" -ForegroundColor Red
    Write-Host "Run this from the repo root, or pass -LauncherPath with the full path." -ForegroundColor Yellow
    exit 1
}

$fullPath = (Resolve-Path $LauncherPath).Path
$backupPath = "$fullPath.bak"

Copy-Item $fullPath $backupPath -Force
Write-Host "Backup created: $backupPath" -ForegroundColor Green

$code = Get-Content $fullPath -Raw

# 1) Add a compile-time tester flag after the WinHTTP pragma.
# PowerShell uses backtick for escaping in double-quoted strings, so this regex is single-quoted.
if ($code -notmatch 'PCM_TESTER_BUILD') {
    $marker = '#pragma comment\(lib, "winhttp\.lib"\)'
    $replacement = @'
#pragma comment(lib, "winhttp.lib")

// Temporary tester build switch.
// Set to 0 before public/release builds.
#ifndef PCM_TESTER_BUILD
#define PCM_TESTER_BUILD 1
#endif
'@

    $newCode = [regex]::Replace(
        $code,
        $marker,
        [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $replacement },
        1
    )

    if ($newCode -eq $code) {
        Write-Host "Warning: could not find WinHTTP pragma marker. Adding tester flag after includes instead." -ForegroundColor Yellow
        $code = "#ifndef PCM_TESTER_BUILD`r`n#define PCM_TESTER_BUILD 1`r`n#endif`r`n`r`n" + $code
    } else {
        $code = $newCode
    }
}

# Helper: patch only if the specific guard is not already directly inside the function.
function Add-TesterGuard {
    param(
        [string]$Source,
        [string]$SignatureRegex,
        [string]$Replacement
    )

    $match = [regex]::Match($Source, $SignatureRegex)
    if (!$match.Success) {
        Write-Host "Warning: could not find function matching: $SignatureRegex" -ForegroundColor Yellow
        return $Source
    }

    $start = $match.Index
    $previewLength = [Math]::Min(350, $Source.Length - $start)
    $preview = $Source.Substring($start, $previewLength)
    if ($preview -match 'PCM_TESTER_BUILD') {
        return $Source
    }

    return [regex]::Replace(
        $Source,
        $SignatureRegex,
        [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $Replacement },
        1
    )
}

# 2) Make the token database check pass only when PCM_TESTER_BUILD is enabled.
$code = Add-TesterGuard `
    -Source $code `
    -SignatureRegex 'static bool CheckTokenInDb\(const std::wstring& accessToken\) \{' `
    -Replacement @'
static bool CheckTokenInDb(const std::wstring& accessToken) {
#if defined(PCM_TESTER_BUILD) && PCM_TESTER_BUILD
    (void)accessToken;
    return true;
#endif
'@

# 3) Avoid Discord API lookup during tester builds.
$code = Add-TesterGuard `
    -Source $code `
    -SignatureRegex 'static std::wstring GetDiscordUsername\(const std::wstring& accessToken\) \{' `
    -Replacement @'
static std::wstring GetDiscordUsername(const std::wstring& accessToken) {
#if defined(PCM_TESTER_BUILD) && PCM_TESTER_BUILD
    (void)accessToken;
    return L"Tester";
#endif
'@

# 4) Disable the background revalidation thread in tester builds.
$code = Add-TesterGuard `
    -Source $code `
    -SignatureRegex 'static void StartTokenVerification\(\) \{' `
    -Replacement @'
static void StartTokenVerification() {
#if defined(PCM_TESTER_BUILD) && PCM_TESTER_BUILD
    return;
#endif
'@

# 5) Force a local tester token before the normal saved-token validation block.
if ($code -notmatch 'token = L"tester-build"') {
    $pattern = '// Validate token synchronously before showing UI\s*if \(!token\.empty\(\)\) \{'
    $replacement = @'
// Validate token synchronously before showing UI
#if defined(PCM_TESTER_BUILD) && PCM_TESTER_BUILD
    token = L"tester-build";
#endif
    if (!token.empty()) {
'@

    $newCode = [regex]::Replace(
        $code,
        $pattern,
        [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $replacement },
        1
    )

    if ($newCode -eq $code) {
        Write-Host "Warning: could not find saved-token validation block. Function guards were still applied." -ForegroundColor Yellow
    } else {
        $code = $newCode
    }
}

Set-Content -Path $fullPath -Value $code -NoNewline -Encoding UTF8

Write-Host "Patched tester launcher successfully:" -ForegroundColor Green
Write-Host "  $fullPath"
Write-Host ""
Write-Host "Next compile:" -ForegroundColor Cyan
Write-Host "  cd PCM_SOURCE\PCM_LAUNCHER"
Write-Host "  compile_launcher.bat"
Write-Host ""
Write-Host "Before release, either set PCM_TESTER_BUILD to 0 or restore:" -ForegroundColor Yellow
Write-Host "  Copy-Item `"$backupPath`" `"$fullPath`" -Force"
