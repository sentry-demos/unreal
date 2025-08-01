# Sentry Integration: External Scripts vs Integrated Build System

This document compares the previous external script approach with the new integrated build system approach for Sentry symbol upload.

## Comparison Table

| Aspect | External Scripts | Integrated Build System |
|--------|------------------|-------------------------|
| **Integration** | PostBuildSteps in `.uplugin` | PostBuildSteps in `.Target.cs` |
| **Scripts** | External shell scripts | C# build support class |
| **Configuration** | Manual setup required | Automatic detection |
| **Error Handling** | Basic script error handling | Integrated with Unreal logging |
| **Platform Support** | Separate scripts per platform | Unified C# implementation |
| **Maintenance** | Multiple script files | Single C# class |
| **CI/CD Integration** | Requires script setup | Native environment variable support |
| **Debugging** | Script-level debugging | Build system debugging |
| **Configuration Validation** | Manual validation | Automatic validation |
| **Build System Awareness** | Limited | Full integration |

## Detailed Comparison

### 1. **Setup Complexity**

#### External Scripts
```bash
# Required files:
- Plugins/Sentry/Scripts/upload-debug-symbols.sh
- Plugins/Sentry/Scripts/upload-debug-symbols-win.bat
- sentry.properties (manual creation)
- Environment variables (manual setup)
```

#### Integrated Build System
```csharp
// Single line in .Target.cs:
SentryTower.SentryBuildSupport.AddSentryPostBuildSteps(Target, PostBuildSteps);

// Configuration via:
- Environment variables (automatic detection)
- Project settings (automatic detection)
- Properties file (automatic detection)
```

### 2. **Configuration Management**

#### External Scripts
```bash
# Manual configuration in sentry.properties:
defaults.org=my-org
defaults.project=my-project
auth.token=my-token

# Or environment variables:
export SENTRY_ORG=my-org
export SENTRY_PROJECT=my-project
export SENTRY_AUTH_TOKEN=my-token
```

#### Integrated Build System
```ini
# Multiple configuration methods with automatic detection:

# Option 1: Environment variables (CI/CD friendly)
export SENTRY_UPLOAD_SYMBOLS_AUTOMATICALLY=True
export SENTRY_ORG=my-org
export SENTRY_PROJECT=my-project
export SENTRY_AUTH_TOKEN=my-token

# Option 2: Project settings
[/Script/Sentry.SentrySettings]
UploadSymbolsAutomatically=True
ProjectName=my-project
OrgName=my-org
AuthToken=my-token

# Option 3: Properties file (backward compatibility)
defaults.org=my-org
defaults.project=my-project
auth.token=my-token
```

### 3. **Error Handling**

#### External Scripts
```bash
# Basic error handling in scripts:
if [ -z "$SENTRY_AUTH_TOKEN" ]; then
    echo "Error: SENTRY_AUTH_TOKEN environment variable is required"
    exit 1
fi
```

#### Integrated Build System
```csharp
// Comprehensive error handling with Unreal logging:
if (!IsSentryPluginAvailable(Target))
{
    Log.TraceInformation("Sentry: Plugin not available, skipping symbol upload.");
    return;
}

if (!IsSymbolUploadEnabled(Target))
{
    Log.TraceInformation("Sentry: Symbol upload is disabled, skipping.");
    return;
}

var config = GetSentryConfiguration(Target);
if (config == null)
{
    Log.TraceWarning("Sentry: Configuration not found, skipping symbol upload.");
    return;
}
```

### 4. **Platform Support**

#### External Scripts
```bash
# Separate scripts for each platform:
- upload-debug-symbols.sh (Mac/Linux)
- upload-debug-symbols-win.bat (Windows)

# Different command syntax per platform:
# Windows: %SENTRY_ORG%
# Unix: $SENTRY_ORG
```

#### Integrated Build System
```csharp
// Unified platform handling:
switch (Platform)
{
    case UnrealTargetPlatform.Win64:
        return Path.Combine(pluginDir, "Source", "ThirdParty", "CLI", "sentry-cli-Windows-x86_64.exe");
    case UnrealTargetPlatform.Mac:
        return Path.Combine(pluginDir, "Source", "ThirdParty", "CLI", "sentry-cli-Darwin-x86_64");
    case UnrealTargetPlatform.Linux:
    case UnrealTargetPlatform.LinuxArm64:
        return Path.Combine(pluginDir, "Source", "ThirdParty", "CLI", "sentry-cli-Linux-x86_64");
}
```

### 5. **CI/CD Integration**

#### External Scripts
```yaml
# GitHub Actions example:
- name: Setup Sentry
  run: |
    echo "SENTRY_ORG=${{ secrets.SENTRY_ORG }}" >> $GITHUB_ENV
    echo "SENTRY_PROJECT=${{ secrets.SENTRY_PROJECT }}" >> $GITHUB_ENV
    echo "SENTRY_AUTH_TOKEN=${{ secrets.SENTRY_AUTH_TOKEN }}" >> $GITHUB_ENV

- name: Build
  run: ue5 build project.uproject

- name: Upload Symbols
  run: ./Plugins/Sentry/Scripts/upload-debug-symbols.sh
```

#### Integrated Build System
```yaml
# GitHub Actions example:
- name: Setup Sentry
  run: |
    echo "SENTRY_UPLOAD_SYMBOLS_AUTOMATICALLY=True" >> $GITHUB_ENV
    echo "SENTRY_ORG=${{ secrets.SENTRY_ORG }}" >> $GITHUB_ENV
    echo "SENTRY_PROJECT=${{ secrets.SENTRY_PROJECT }}" >> $GITHUB_ENV
    echo "SENTRY_AUTH_TOKEN=${{ secrets.SENTRY_AUTH_TOKEN }}" >> $GITHUB_ENV

- name: Build (with automatic symbol upload)
  run: ue5 build project.uproject
```

### 6. **Maintenance and Updates**

#### External Scripts
- **Multiple files to maintain**: Shell scripts for each platform
- **Manual updates**: Scripts need to be updated separately
- **Version control**: Multiple script files in repository
- **Testing**: Each platform script needs separate testing

#### Integrated Build System
- **Single file**: One C# class handles all platforms
- **Automatic updates**: Part of the build system
- **Version control**: Single file in repository
- **Testing**: Integrated with Unreal's build testing

### 7. **Debugging and Troubleshooting**

#### External Scripts
```bash
# Limited debugging options:
echo "Sentry: Starting debug symbols upload..."
# Manual logging throughout scripts
```

#### Integrated Build System
```csharp
// Rich debugging with Unreal's logging system:
Log.TraceInformation("Sentry: Added PostBuildStep for symbol upload.");
Log.TraceWarning("Sentry: Configuration not found, skipping symbol upload.");
Log.TraceError("Sentry: Failed to upload symbols: {0}", errorMessage);
```

### 8. **Performance Impact**

#### External Scripts
- **Build time**: Additional script execution time
- **Dependencies**: External script dependencies
- **Platform overhead**: Different script execution per platform

#### Integrated Build System
- **Build time**: Minimal overhead (single C# method call)
- **Dependencies**: No external dependencies
- **Platform overhead**: Unified approach across platforms

## Migration Benefits

### Immediate Benefits
1. **Simplified setup**: No external scripts to manage
2. **Better integration**: Native Unreal build system integration
3. **Improved reliability**: Better error handling and validation
4. **Easier configuration**: Multiple configuration methods with automatic detection

### Long-term Benefits
1. **Reduced maintenance**: Single codebase to maintain
2. **Better debugging**: Integrated with Unreal's logging system
3. **Future-proof**: Easier to extend and modify
4. **Consistent behavior**: Unified approach across all platforms

## Conclusion

The integrated build system approach provides significant advantages over the external script method:

- **Better integration** with Unreal's build system
- **Simplified maintenance** with a single C# implementation
- **Improved reliability** with comprehensive error handling
- **Easier configuration** with automatic detection of multiple configuration methods
- **Better CI/CD integration** with native environment variable support
- **Enhanced debugging** with integrated logging

The integrated approach is recommended for all new projects and existing projects looking to improve their Sentry symbol upload workflow. 