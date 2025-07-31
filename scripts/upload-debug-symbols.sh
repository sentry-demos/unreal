#!/bin/bash

# Upload debug symbols to Sentry
# This script uploads debug symbols for the Sentry Unreal plugin

set -e

# Check if required environment variables are set
if [ -z "$SENTRY_AUTH_TOKEN" ]; then
    echo "Error: SENTRY_AUTH_TOKEN environment variable is required"
    exit 1
fi

if [ -z "$SENTRY_ORG" ]; then
    echo "Error: SENTRY_ORG environment variable is required"
    exit 1
fi

if [ -z "$SENTRY_PROJECT" ]; then
    echo "Error: SENTRY_PROJECT environment variable is required"
    exit 1
fi

echo "Uploading debug symbols to Sentry..."

# Install sentry-cli if not available
if ! command -v sentry-cli &> /dev/null; then
    echo "Installing sentry-cli..."
    pip install sentry-cli
fi

# Upload Sentry plugin debug symbols
echo "Uploading Sentry plugin debug symbols..."

# Upload the main Sentry plugin library
if [ -f "Plugins/Sentry/Binaries/Mac/UnrealEditor-Sentry.dylib" ]; then
    echo "Uploading UnrealEditor-Sentry.dylib..."
    sentry-cli debug-files upload \
        --org="$SENTRY_ORG" \
        --project="$SENTRY_PROJECT" \
        --auth-token="$SENTRY_AUTH_TOKEN" \
        --include-sources \
        "Plugins/Sentry/Binaries/Mac/UnrealEditor-Sentry.dylib"
else
    echo "Warning: UnrealEditor-Sentry.dylib not found"
fi

# Upload the Sentry editor plugin library
if [ -f "Plugins/Sentry/Binaries/Mac/UnrealEditor-SentryEditor.dylib" ]; then
    echo "Uploading UnrealEditor-SentryEditor.dylib..."
    sentry-cli debug-files upload \
        --org="$SENTRY_ORG" \
        --project="$SENTRY_PROJECT" \
        --auth-token="$SENTRY_AUTH_TOKEN" \
        --include-sources \
        "Plugins/Sentry/Binaries/Mac/UnrealEditor-SentryEditor.dylib"
else
    echo "Warning: UnrealEditor-SentryEditor.dylib not found"
fi

# Upload game debug symbols if available
if [ -f "Binaries/Mac/SentryTowerEditor.dylib" ]; then
    echo "Uploading SentryTowerEditor.dylib..."
    sentry-cli debug-files upload \
        --org="$SENTRY_ORG" \
        --project="$SENTRY_PROJECT" \
        --auth-token="$SENTRY_AUTH_TOKEN" \
        --include-sources \
        "Binaries/Mac/SentryTowerEditor.dylib"
else
    echo "Warning: SentryTowerEditor.dylib not found"
fi

# Upload any .dSYM files if they exist
if [ -d "Plugins/Sentry/Binaries/Mac" ]; then
    find "Plugins/Sentry/Binaries/Mac" -name "*.dSYM" -type d | while read -r dsym_file; do
        echo "Uploading debug symbols: $dsym_file"
        sentry-cli debug-files upload \
            --org="$SENTRY_ORG" \
            --project="$SENTRY_PROJECT" \
            --auth-token="$SENTRY_AUTH_TOKEN" \
            --include-sources \
            "$dsym_file"
    done
fi

echo "Debug symbol upload completed!" 