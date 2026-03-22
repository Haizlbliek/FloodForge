using DiscordRPC;
using DiscordRPC.Logging;

namespace FloodForge;

// TODO: Displayname
public static class RichPresenceManager {
	private static DiscordRpcClient? client = null;
	private static string acronym = "";
	private static int roomCount = 0;
	private static int screenCount = 0;
	private static int connectionCount = 0;

	private static System.Timers.Timer? debounceTimer;
	private static readonly Timestamps sessionStart = Timestamps.Now;

	public static string Acronym {
		set { if (acronym != value) { acronym = value; RequestRefresh(); } }
	}

	public static int RoomCount {
		set { if (roomCount != value) { roomCount = value; RequestRefresh(); } }
	}

	public static int ScreenCount {
		set { if (screenCount != value) { screenCount = value; RequestRefresh(); } }
	}

	public static int ConnectionCount {
		set { if (connectionCount != value) { connectionCount = value; RequestRefresh(); } }
	}

	private static void RequestRefresh() {
		if (debounceTimer == null) {
			debounceTimer = new System.Timers.Timer(2000);
			debounceTimer.Elapsed += (s, e) => Refresh();
			debounceTimer.AutoReset = false;
		}

		debounceTimer.Stop();
		debounceTimer.Start();
	}

	private static void Refresh() {
		if (acronym == "") {
			Set("Waiting...", "");
			return;
		}

		Set("Editing " + acronym.ToUpperInvariant(), $"{roomCount} rooms, {screenCount} screens, {connectionCount} connections");
	}

	public static void Initialize() {
		if (!Settings.DiscordRichPresence) return;

		client = new DiscordRpcClient("1484944143980560544") {
			Logger = new DiscordLoggerBridge() { Level = LogLevel.Warning }
		};
		client.OnReady += (sender, e) => {
			Logger.Info($"Received Ready from user {e.User.Username}");
		};
		client.Initialize();
		Refresh();
	}

	public static void Set(string details, string state) {
		client?.SetPresence(new RichPresence() {
			Details = details,
			State = state,
			Timestamps = sessionStart
		});
	}

	public static void Cleanup() {
		debounceTimer?.Dispose();
		if (client == null) return;

		client.ClearPresence();
		client.Dispose();
		client = null;
	}

	public class DiscordLoggerBridge : ILogger {
		public LogLevel Level { get; set; }

		public void Info(string message, params object[] args) {
			// Logger.Info(string.Format(message, args));
		}

		public void Warning(string message, params object[] args) {
			Logger.Warn(string.Format(message, args));
		}

		public void Error(string message, params object[] args) {
			Logger.Error(string.Format(message, args));
		}

		public void Trace(string message, params object[] args) {
			// Logger.Note(string.Format(message, args));
		}
	}
}