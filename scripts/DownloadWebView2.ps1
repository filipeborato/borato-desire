$ErrorActionPreference = "Stop"
$packageSourceName = "nugetRepository"
$packageVersion = "1.0.3485.44"
$packagePath = Join-Path $env:USERPROFILE "AppData\\Local\\PackageManagement\\NuGet\\Packages\\Microsoft.Web.WebView2.$packageVersion"

if (Test-Path -LiteralPath $packagePath) {
    exit 0
}

Install-PackageProvider -Name NuGet -MinimumVersion 2.8.5.201 -Scope CurrentUser -Force | Out-Null
Register-PackageSource -Provider NuGet -Name $packageSourceName `
    -Location "https://www.nuget.org/api/v2" -Trusted -Force | Out-Null
Install-Package Microsoft.Web.WebView2 -Scope CurrentUser -RequiredVersion $packageVersion `
    -Source $packageSourceName -Force | Out-Null

if (-not (Test-Path -LiteralPath $packagePath)) {
    throw "WebView2 SDK $packageVersion was not installed at $packagePath"
}
