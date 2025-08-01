# Sentry Build Integration for Unreal Engine

This document explains how to use the integrated Sentry symbol upload feature that's built directly into the Unreal build system, replacing the previous external script approach.

## Overview

The new integrated approach provides several advantages over the previous external script method:

### Previous Approach (External Scripts)
- Used PostBuildSteps in the plugin's `.uplugin` file
- Required external shell scripts (`upload-debug-symbols.sh`, `upload-debug-symbols-win.bat`)
- Manual configuration through `sentry.properties` or environment variables
- Less integrated with Unreal's build system

### New Integrated Approach
- **Direct integration** with Unreal's build system via `PostBuildSteps` in `.Target.cs` files
- **C# build support** class that handles configuration and command generation
- **Automatic detection** of Sentry plugin availability
- **Better error handling** and logging within the build process
- **Cleaner configuration** through Unreal's config system

## How It Works

### 1. Build Support Class
The `SentryBuildSupport.cs` class provides the core functionality:

```csharp
// Automatically adds PostBuildSteps for symbol upload
SentryTower.SentryBuildSupport.AddSentryPostBuildSteps(Target, PostBuildSteps);
```

### 2. Configuration Detection
The system automatically detects Sentry configuration from:
1. **Environment variables** (highest priority)
2. **Project settings** in `DefaultEngine.ini`
3. **Properties file** (`sentry.properties`)

### 3. Platform Support
Supports all major platforms:
- Windows (Win64)
- macOS
- Linux (x86_64 and ARM64)

## Setup Instructions

### 1. Enable Symbol Upload

#### Option A: Environment Variables (Recommended for CI/CD)
```bash
export SENTRY_UPLOAD_SYMBOLS_AUTOMATICALLY=True
export SENTRY_ORG=your-org-slug
export SENTRY_PROJECT=your-project-slug
export SENTRY_AUTH_TOKEN=your-auth-token
```

#### Option B: Project Settings
Edit `Config/DefaultEngine.ini`:
```ini
[/Script/Sentry.SentrySettings]
UploadSymbolsAutomatically=True
ProjectName=your-project-slug
OrgName=your-org-slug
AuthToken=your-auth-token
```

#### Option C: Properties File
Create `sentry.properties` in your project root:
```properties
defaults.org=your-org-slug
defaults.project=your-project-slug
auth.token=your-auth-token
```

### 2. Build Configuration

The system automatically:
- Skips symbol upload for Editor targets
- Only uploads for enabled build configurations
- Includes source files when configured
- Handles platform-specific paths and commands

### 3. Build Process

When you build your project:
1. Unreal's build system processes the `.Target.cs` files
2. `SentryBuildSupport` checks if Sentry is enabled
3. If enabled, adds the appropriate PostBuildStep
4. After the build completes, the PostBuildStep executes
5. Debug symbols are uploaded to Sentry

## Configuration Options

### Build Targets
Control which targets upload symbols:
```ini
[/Script/Sentry.SentrySettings]
EnableBuildTargets=(bEnableGame=True,bEnableClient=True,bEnableServer=False)
```

### Build Configurations
Control which configurations upload symbols:
```ini
[/Script/Sentry.SentrySettings]
EnableBuildConfigurations=(bEnableDebug=False,bEnableDevelopment=True,bEnableShipping=True)
```

### Source Inclusion
Include source files with symbols:
```ini
[/Script/Sentry.SentrySettings]
IncludeSources=True
```

### Diagnostic Level
Control sentry-cli logging:
```ini
[/Script/Sentry.SentrySettings]
DiagnosticLevel=Info
```

## Advantages of the Integrated Approach

### 1. **Better Integration**
- Directly integrated with Unreal's build system
- No external script dependencies
- Consistent with Unreal's build patterns

### 2. **Improved Reliability**
- Better error handling and logging
- Automatic detection of Sentry plugin availability
- Graceful fallbacks when configuration is missing

### 3. **Easier Configuration**
- Multiple configuration methods (env vars, config files, project settings)
- Automatic validation of configuration
- Clear error messages when configuration is invalid

### 4. **Platform Consistency**
- Unified approach across all platforms
- No need for separate shell scripts
- Consistent behavior regardless of platform

### 5. **Build System Awareness**
- Respects Unreal's build configuration
- Only runs for appropriate targets and configurations
- Integrates with Unreal's logging system

## Migration from External Scripts

If you're migrating from the previous external script approach:

1. **Remove external scripts**: You can delete `upload-debug-symbols.sh` and `upload-debug-symbols-win.bat`
2. **Update configuration**: Move your configuration to one of the supported methods above
3. **Test builds**: Verify that symbol upload works correctly
4. **Clean up**: Remove any custom PostBuildSteps that were calling the external scripts

## Troubleshooting

### Symbols Not Uploading
1. Check that `UploadSymbolsAutomatically=True` is set
2. Verify Sentry configuration (org, project, auth token)
3. Ensure you're building a non-Editor target
4. Check build logs for Sentry-related messages

### Configuration Issues
1. Verify environment variables are set correctly
2. Check that `sentry.properties` file exists and is readable
3. Ensure project settings are properly configured

### Platform-Specific Issues
1. Verify `sentry-cli` executable exists for your platform
2. Check that binary paths are correct for your platform
3. Ensure proper permissions for file access

## Example Build Output

When symbol upload is working correctly, you should see output like:
```
Sentry: Added PostBuildStep for symbol upload.
...
Sentry: Uploading debug symbols for SentryTower
Sentry: Upload completed successfully
```

## Security Considerations

- **Auth tokens**: Never commit auth tokens to version control
- **Properties file**: Add `sentry.properties` to `.gitignore`
- **Environment variables**: Use CI/CD secrets for production builds
- **Source inclusion**: Be careful when including source files in production builds

## Support

For issues with the integrated build system:
1. Check the build logs for Sentry-related messages
2. Verify your configuration is correct
3. Test with a simple configuration first
4. Check that the Sentry plugin is properly installed and enabled 