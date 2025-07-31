# Sentry Tower Defense - Built with Unreal Engine

A Sentry-themed tower defence game written for Unreal Engine in C++/Blueprints featuring:
* Non-stop Idle shooting action!
* 3 turret power-ups with their own unique behavior
* 3 different enemies

![](/Media/gameplay.gif?raw=true)

## Gameplay

* Destroy enemies to pick up XP
* Select attack modifiers for each level
* Survive as long as you can!

## System requirements

* Windows
* Unreal Engine 5.5

## Sentry Configuration

This project includes Sentry crash reporting and performance monitoring. The DSN can be configured in two ways:

### Environment Variable (Recommended)
Set the `SENTRY_DSN` environment variable:
```bash
export SENTRY_DSN="https://your-dsn@your-org.ingest.sentry.io/project-id"
```

### Configuration File (Fallback)
If no environment variable is set, the DSN can be configured in `Config/DefaultEngine.ini`:
```ini
[/Script/Sentry.SentrySettings]
Dsn="https://your-dsn@your-org.ingest.sentry.io/project-id"
```

**Note:** For security reasons, it's recommended to use environment variables instead of hardcoding the DSN in configuration files.

## Debug Symbol Upload

To enable proper stack traces in Sentry, debug symbols need to be uploaded. This can be done in two ways:

### Local Upload
Set the required environment variables and run the upload script:
```bash
export SENTRY_AUTH_TOKEN="your-auth-token"
export SENTRY_ORG="your-org-slug"
export SENTRY_PROJECT="your-project-slug"

./scripts/upload-debug-symbols.sh
```

### GitHub Actions
The project includes two workflows for debug symbol upload:

1. **Integrated Upload** (`ci.yml`): Debug symbols are uploaded as part of the build process
2. **Separate Upload** (`upload-symbols.yml`): Manual workflow for uploading debug symbols

Both workflows require the following repository secrets:
- `SENTRY_AUTH_TOKEN`: Your Sentry authentication token
- `SENTRY_ORG`: Your Sentry organization slug
- `SENTRY_PROJECT`: Your Sentry project slug

## Third-party assets

* 3D models - [Space Kit](https://kenney.nl/assets/space-kit) by @KenneyNL
* Materials - [Stylized Egypt](https://www.unrealengine.com/marketplace/en-US/product/stylized-egypt) permanently free UE Marketplace collection
* VFX - [FX Variety Pack](https://www.unrealengine.com/marketplace/en-US/product/a36bac8b05004e999dd4b1d332501f49) permanently free UE Marketplace collection
* Sounds - [Sci-fi Sounds](https://kenney.nl/assets/sci-fi-sounds) by @KenneyNL