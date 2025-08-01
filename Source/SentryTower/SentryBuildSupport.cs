// Copyright (c) 2024 Sentry. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;
using System.Text;
using System.Collections.Generic;

namespace SentryTower
{
	/// <summary>
	/// Provides build-time support for Sentry symbol upload integration
	/// </summary>
	public static class SentryBuildSupport
	{
		/// <summary>
		/// Adds Sentry PostBuildSteps to the target if Sentry is enabled
		/// </summary>
		public static void AddSentryPostBuildSteps(ReadOnlyTargetRules Target, List<string> PostBuildSteps)
		{
			// Skip for editor targets
			if (Target.Type == TargetType.Editor)
			{
				return;
			}

			// Check if Sentry plugin is available
			if (!IsSentryPluginAvailable(Target))
			{
				return;
			}

			// Check if symbol upload is enabled
			if (!IsSymbolUploadEnabled(Target))
			{
				Log.TraceInformation("Sentry: Symbol upload is disabled. Skipping PostBuildStep.");
				return;
			}

			// Get Sentry configuration
			var config = GetSentryConfiguration(Target);
			if (config == null)
			{
				Log.TraceWarning("Sentry: Configuration not found. Skipping symbol upload.");
				return;
			}

			// Create and add the PostBuildStep
			string postBuildCommand = CreatePostBuildCommand(Target, config);
			PostBuildSteps.Add(postBuildCommand);
			
			Log.TraceInformation("Sentry: Added PostBuildStep for symbol upload.");
		}

		private static bool IsSentryPluginAvailable(ReadOnlyTargetRules Target)
		{
			string pluginPath = Path.Combine(PluginDirectory, "Plugins", "Sentry");
			return Directory.Exists(pluginPath);
		}

		private static bool IsSymbolUploadEnabled(ReadOnlyTargetRules Target)
		{
			// Check environment variable first
			string envUpload = Environment.GetEnvironmentVariable("SENTRY_UPLOAD_SYMBOLS_AUTOMATICALLY");
			if (!string.IsNullOrEmpty(envUpload))
			{
				return envUpload.ToLower() == "true";
			}

			// Check project settings
			string configPath = Path.Combine(PluginDirectory, "Config", "DefaultEngine.ini");
			if (File.Exists(configPath))
			{
				string[] lines = File.ReadAllLines(configPath);
				foreach (string line in lines)
				{
					if (line.Contains("UploadSymbolsAutomatically=True"))
					{
						return true;
					}
				}
			}

			return false;
		}

		private static SentryConfig GetSentryConfiguration(ReadOnlyTargetRules Target)
		{
			// Try properties file first
			string propertiesPath = Path.Combine(PluginDirectory, "sentry.properties");
			if (File.Exists(propertiesPath))
			{
				return ParsePropertiesFile(propertiesPath);
			}

			// Fall back to environment variables
			return ParseEnvironmentVariables();
		}

		private static SentryConfig ParsePropertiesFile(string propertiesPath)
		{
			var config = new SentryConfig();
			string[] lines = File.ReadAllLines(propertiesPath);

			foreach (string line in lines)
			{
				if (line.StartsWith("defaults.project="))
				{
					config.Project = line.Substring("defaults.project=".Length).Trim();
				}
				else if (line.StartsWith("defaults.org="))
				{
					config.Org = line.Substring("defaults.org=".Length).Trim();
				}
				else if (line.StartsWith("auth.token="))
				{
					config.AuthToken = line.Substring("auth.token=".Length).Trim();
				}
			}

			return config.IsValid ? config : null;
		}

		private static SentryConfig ParseEnvironmentVariables()
		{
			var config = new SentryConfig
			{
				Project = Environment.GetEnvironmentVariable("SENTRY_PROJECT"),
				Org = Environment.GetEnvironmentVariable("SENTRY_ORG"),
				AuthToken = Environment.GetEnvironmentVariable("SENTRY_AUTH_TOKEN")
			};

			return config.IsValid ? config : null;
		}

		private static string CreatePostBuildCommand(ReadOnlyTargetRules Target, SentryConfig config)
		{
			string sentryCliPath = GetSentryCliPath(Target.Platform);
			string binariesDir = Path.Combine(PluginDirectory, "Binaries", Target.Platform.ToString());
			string pluginBinariesDir = Path.Combine(PluginDirectory, "Plugins", "Sentry", "Source", "ThirdParty", Target.Platform.ToString());

			// Build the command with proper escaping
			var commandBuilder = new StringBuilder();
			
			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				commandBuilder.AppendFormat("\"{0}\" debug-files upload", sentryCliPath);
				commandBuilder.AppendFormat(" --org \"{0}\"", config.Org);
				commandBuilder.AppendFormat(" --project \"{0}\"", config.Project);
				commandBuilder.AppendFormat(" --auth-token \"{0}\"", config.AuthToken);
				commandBuilder.Append(" --include-sources");
				commandBuilder.AppendFormat(" \"{0}\"", binariesDir);
				commandBuilder.AppendFormat(" \"{0}\"", pluginBinariesDir);
			}
			else
			{
				commandBuilder.AppendFormat("\"{0}\" debug-files upload", sentryCliPath);
				commandBuilder.AppendFormat(" --org \"{0}\"", config.Org);
				commandBuilder.AppendFormat(" --project \"{0}\"", config.Project);
				commandBuilder.AppendFormat(" --auth-token \"{0}\"", config.AuthToken);
				commandBuilder.Append(" --include-sources");
				commandBuilder.AppendFormat(" \"{0}\"", binariesDir);
				commandBuilder.AppendFormat(" \"{0}\"", pluginBinariesDir);
			}

			return commandBuilder.ToString();
		}

		private static string GetSentryCliPath(UnrealTargetPlatform Platform)
		{
			string pluginDir = Path.Combine(PluginDirectory, "Plugins", "Sentry");
			
			switch (Platform)
			{
				case UnrealTargetPlatform.Win64:
					return Path.Combine(pluginDir, "Source", "ThirdParty", "CLI", "sentry-cli-Windows-x86_64.exe");
				case UnrealTargetPlatform.Mac:
					// Try universal binary first, then x86_64
					string universalPath = Path.Combine(pluginDir, "Source", "ThirdParty", "CLI", "sentry-cli-Darwin-universal");
					if (File.Exists(universalPath))
					{
						return universalPath;
					}
					return Path.Combine(pluginDir, "Source", "ThirdParty", "CLI", "sentry-cli-Darwin-x86_64");
				case UnrealTargetPlatform.Linux:
				case UnrealTargetPlatform.LinuxArm64:
					return Path.Combine(pluginDir, "Source", "ThirdParty", "CLI", "sentry-cli-Linux-x86_64");
				default:
					return null;
			}
		}

		private static string PluginDirectory => Path.GetDirectoryName(Path.GetDirectoryName(Path.GetDirectoryName(Path.GetDirectoryName(typeof(SentryBuildSupport).Assembly.Location))));
	}

	/// <summary>
	/// Configuration for Sentry symbol upload
	/// </summary>
	public class SentryConfig
	{
		public string Project { get; set; }
		public string Org { get; set; }
		public string AuthToken { get; set; }

		public bool IsValid => !string.IsNullOrEmpty(Project) && !string.IsNullOrEmpty(Org) && !string.IsNullOrEmpty(AuthToken);
	}
} 