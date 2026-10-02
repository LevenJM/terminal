# Fork notes: middle-click paste

Private build of Windows Terminal with `middleClickPaste` (see microsoft/terminal#6250).
Not an upstream contribution.

## What the branch contains

`my/middle-click-paste` sits on top of an upstream release tag:

1. Feature: selecting text saves it; middle-click pastes it. Profile setting `middleClickPaste` (default `false`).
2. Settings UI toggle: Settings > profile > Advanced > "Middle-click paste".
3. Build workaround: `src/common.build.pre.props` suppresses warning C4875.
4. This file.

## Remotes

- `origin`: https://github.com/LevenJM/terminal (this fork)
- `upstream`: https://github.com/microsoft/terminal

Pushes use HTTPS with the LevenJM token from the WSL `gh` CLI, via the helper `C:\Users\Jonathan\.gh-levenjm-cred.sh` (set in this repo's local git config).

## Updating to a new release

```powershell
$env:Path += ';C:\Program Files\Git\cmd'
git fetch upstream --tags
git tag --list 'v1.*' --sort=-v:refname | Select-Object -First 5
git rebase <new-tag>                       # fix conflicts, then: git add ...; git rebase --continue
git push --force-with-lease origin my/middle-click-paste
```

Likely conflict files: `ControlInteractivity.cpp`, `TerminalPage.cpp`, `MTSMSettings.h`.
Track release tags, not `main` (`main` ran out of compiler memory on this machine).

## Build

Close the dev Terminal first. Not committed: `build-openconsole.ps1` (local paths and toolchain versions).

```powershell
cd C:\Users\Jonathan\Documents\Personal_projects\terminal-release
$env:CL_MPCount = '4'     # more parallel cl.exe workers hit PCH virtual-memory errors
pwsh -NoProfile -File .\build-openconsole.ps1 -BuildTarget 'Terminal\CascadiaPackage' -Configuration Release -ToolsetVersion 14.50.35717 > run.out 2>&1
```

Requirements: Windows SDK 10.0.26100.0, MSVC 14.50.35717, Developer Mode on.
First full build is about 56 minutes; incremental builds take 2 to 10 minutes.

## Install and launch

Register from an unpacked MSIX. Registering from `bin\x64\Release` leaves the `Images` folder out and the taskbar icon is blank.

```powershell
$r   = 'C:\Users\Jonathan\Documents\Personal_projects\terminal-release'
$msix = (Get-ChildItem "$r\src\cascadia\CascadiaPackage\AppPackages" -Recurse -Filter *.msix | Select-Object -First 1).FullName
$loose = "$r\src\cascadia\CascadiaPackage\AppPackages\loose"
$mk = (Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.0.26100.0\x64\makeappx.exe").FullName
Remove-Item $loose -Recurse -Force -ErrorAction SilentlyContinue
& $mk unpack /o /p $msix /d $loose
Get-AppxPackage WindowsTerminalDev* | Remove-AppxPackage     # same-version re-register fails otherwise
Add-AppxPackage -Register "$loose\AppxManifest.xml"
Start-Process 'shell:AppsFolder\WindowsTerminalDev_8wekyb3d8bbwe!App'
```

Also launchable as `wtd.exe` or "Windows Terminal Dev" in the Start menu.
