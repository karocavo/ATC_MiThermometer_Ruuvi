# Building on Windows

## Overview

This guide helps you build the project on Windows, especially when using an external Telink SDK.

## Common Issues

### Issue 1: TC32 Toolchain Not Found

**Error:**
```
Cannot openC:\TelinkSDK\opt\tc32\bin\tc32-elf-ld.exe: invalid hex number '-L'
```

**Cause:**
The makefile cannot auto-detect your TC32 toolchain location.

**Solution:**
Set `TC32_PATH` when running make:

```cmd
make TC32_PATH=C:/TelinkSDK/opt/tc32/bin/
```

**Note:** Use forward slashes (`/`) even on Windows, not backslashes (`\`).

### Issue 2: External SDK Location

**Symptom:**
Build fails because it can't find SDK components.

**Solution:**
Set both `TEL_PATH` and `TC32_PATH`:

```cmd
make TEL_PATH=C:/TelinkSDK TEL_PATH=C:/TelinkSDK/opt/tc32/bin/
```

## Build Configuration Variables

The makefile supports these configuration variables:

| Variable | Default | Description |
|----------|---------|-------------|
| `TEL_PATH` | `./SDK` | Path to Telink SDK root |
| `TC32_PATH` | Auto-detected | Path to TC32 toolchain binaries |
| `PROJECT_NAME` | `ATC_Thermometer` | Output binary name |
| `PYTHON` | `python` (Windows) | Python interpreter |

## Setting Up Environment

### Option 1: Set TC32_PATH Each Time

```cmd
make clean
make TC32_PATH=C:/TelinkSDK/opt/tc32/bin/
```

### Option 2: Use Environment Variable

In Command Prompt:
```cmd
set TC32_PATH=C:/TelinkSDK/opt/tc32/bin/
make clean
make
```

In PowerShell:
```powershell
$env:TC32_PATH="C:/TelinkSDK/opt/tc32/bin/"
make clean
make
```

### Option 3: Create a Build Script

Create `build.cmd`:
```cmd
@echo off
set TC32_PATH=C:/TelinkSDK/opt/tc32/bin/
set TEL_PATH=C:/TelinkSDK
make %*
```

Then run:
```cmd
build.cmd clean
build.cmd
```

## Makefile Changes

The makefile now:
- Uses `?=` for `TC32_PATH` so it can be overridden
- Provides better fallback when toolchain isn't in local SDK
- Documents how to set paths for external SDKs

## Verifying Your Setup

Check if your toolchain is accessible:

```cmd
C:/TelinkSDK/opt/tc32/bin/tc32-elf-gcc --version
```

Should output something like:
```
tc32-elf-gcc (GCC) X.X.X
```

## Troubleshooting

### Path Issues

**Problem:** Spaces in path
**Solution:** Use quotes or avoid spaces:
```cmd
make "TC32_PATH=C:/Program Files/Telink/tc32/bin/"
```

**Problem:** Backslashes not working
**Solution:** Always use forward slashes in makefile paths:
- ✅ `C:/TelinkSDK/opt/tc32/bin/`
- ❌ `C:\TelinkSDK\opt\tc32\bin\`

### Python Issues

If Python is not found, set it explicitly:
```cmd
make PYTHON=python3 TC32_PATH=C:/TelinkSDK/opt/tc32/bin/
```

## Related Issues

- See `docs/FLASH_API_CONFIG.md` for SDK API compatibility
- See main README for general build instructions
