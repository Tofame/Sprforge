---
description: Format C++ code using clang-format
---

# Code Formatting Workflow

This workflow describes how to format C++ code in the Sprforge project using clang-format.

## Configuration

The project uses `.clang-format` configuration file with the following key settings:

- **Indentation**: Tabs with width of 4 spaces
- **Pointer/Reference Alignment**: Left-aligned (attached to type)
  - Example: `Texture* texture` (NOT `Texture *texture`)
- **Base Style**: LLVM with custom modifications

## Format All Files

// turbo
To format all C++ files in the project:

```powershell
Get-ChildItem -Path "src" -Recurse -Include *.cpp,*.h | ForEach-Object { clang-format -i $_.FullName; Write-Host "Formatted: $($_.FullName)" }
```

## Format Single File

// turbo
To format a specific file:

```powershell
clang-format -i path/to/file.cpp
```

## Check Formatting Without Modifying

// turbo
To check if a file needs formatting (dry-run):

```powershell
clang-format --dry-run --Werror path/to/file.cpp
```

## Notes

- The `-i` flag formats files in-place
- All formatting is done according to `.clang-format` in the project root
- The configuration ensures consistent indentation and pointer alignment across the entire codebase
