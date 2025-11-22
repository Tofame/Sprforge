# Building for macOS from Windows

Since your application uses macOS-specific APIs (Cocoa/Objective-C++), you **cannot directly compile for macOS on a Windows machine**. macOS requires Apple's development tools and frameworks that only run on macOS.

Here are your options:

## Option 1: GitHub Actions (Recommended - Free & Easy) ✅

Use GitHub Actions to automatically build for macOS in the cloud.

### Setup:
1. Push your code to GitHub (if not already done)
2. The `.github/workflows/build-macos.yml` workflow will automatically build on every push
3. Download the built binary from the Actions tab

### Manual Trigger:
- Go to your GitHub repository
- Click on "Actions" tab
- Select "Build macOS" workflow
- Click "Run workflow" button

**Pros:** Free, automatic, no macOS required  
**Cons:** Requires GitHub account and repository

---

## Option 2: Use a macOS Virtual Machine

### Requirements:
- Mac computer (to create VM) OR valid macOS license
- Virtualization software (VMware, Parallels, or VirtualBox)
- macOS installer

### Steps:
1. Install macOS in a VM on your Windows PC
2. Install Xcode Command Line Tools: `xcode-select --install`
3. Clone your repository in the VM
4. Build normally:
   ```bash
   cmake -B build
   cmake --build build
   ```

**Pros:** Full control, can test locally  
**Cons:** Requires macOS license, significant disk space, slower

---

## Option 3: Remote macOS Machine

### Setup:
1. Access a Mac (physical machine, MacStadium, or cloud Mac service)
2. SSH into the machine from Windows
3. Clone and build your project

### SSH from Windows:
```bash
# Using PowerShell or Git Bash
ssh username@mac-ip-address
cd /path/to/project
cmake -B build
cmake --build build
```

**Services offering macOS hosting:**
- **MacStadium** (paid, dedicated Macs)
- **GitHub Codespaces** (limited macOS support)
- **CircleCI** (macOS runners, paid)

**Pros:** Real macOS, good performance  
**Cons:** Requires access to a Mac, may cost money

---

## Option 4: Cross-compilation (Not Recommended ❌)

Cross-compiling macOS apps from Windows is extremely difficult because:
- Cocoa frameworks require macOS
- Apple's tools don't run on Windows
- No official cross-compilation toolchain

**osxcross** exists but:
- Violates Apple's terms of service
- Doesn't support all frameworks (Cocoa in particular)
- Very complex setup
- Won't work for your app due to Cocoa APIs

---

## Quick Start with GitHub Actions

1. **Make sure you have the workflow file:**
   ```bash
   .github/workflows/build-macos.yml
   ```

2. **Push to GitHub:**
   ```bash
   git add .
   git commit -m "Add macOS build workflow"
   git push
   ```

3. **Check Actions:**
   - Go to your GitHub repository
   - Click "Actions" tab
   - Wait for build to complete
   - Download the artifact

---

## Building Locally on macOS

If you have access to a Mac:

```bash
# Install dependencies
brew install cmake

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# The executable will be at:
# build/src/Sprforge
```

---

## Troubleshooting

### GitHub Actions fails?
- Check the Actions logs for errors
- Ensure all source files are committed
- Verify CMake configuration is correct

### Need to test on macOS?
- Use GitHub Actions to build
- Download and test the binary
- Or use a Mac VM/remote Mac

### Want to automate releases?
- Modify the workflow to create releases
- Add code signing (requires Apple Developer account)
- Package as `.app` bundle

