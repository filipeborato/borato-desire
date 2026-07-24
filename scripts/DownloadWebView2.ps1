$ErrorActionPreference = "Stop"
$packageVersion = "1.0.3485.44"
$packagePath = Join-Path $env:USERPROFILE "AppData\\Local\\PackageManagement\\NuGet\\Packages\\Microsoft.Web.WebView2.$packageVersion"

if (Test-Path -LiteralPath $packagePath) {
    exit 0
}

# A NuGet .nupkg IS a zip with the same "build/native/include/..." layout
# CMakeLists.txt expects at $packagePath -- downloading and extracting it
# directly is equivalent to what Install-Package would produce, without
# depending on PackageManagement's NuGet-provider bootstrap, which is flaky
# (and frequently fails outright) under PowerShell 7/pwsh, the default shell
# on GitHub's Windows runners.
$packageIdLower = "microsoft.web.webview2"
$url = "https://api.nuget.org/v3-flatcontainer/$packageIdLower/$packageVersion/$packageIdLower.$packageVersion.nupkg"
$tempZip = Join-Path ([System.IO.Path]::GetTempPath()) "WebView2-$packageVersion.zip"

Invoke-WebRequest -Uri $url -OutFile $tempZip -UseBasicParsing

New-Item -ItemType Directory -Force -Path $packagePath | Out-Null
Expand-Archive -LiteralPath $tempZip -DestinationPath $packagePath -Force
Remove-Item -LiteralPath $tempZip -Force

if (-not (Test-Path -LiteralPath $packagePath)) {
    throw "WebView2 SDK $packageVersion was not installed at $packagePath"
}
