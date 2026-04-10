public static class Logger {
	private static readonly StreamWriter logFile;

	static Logger() {
		if (File.Exists("log.txt")) {
			File.Copy("log.txt", "log.old.txt", true);
		}

		logFile = new StreamWriter("log.txt");
	}

	private static void Write(string value) {
		Console.WriteLine(value);
		logFile.WriteLine(value);
		logFile.Flush();
	}

	public static void Note(params object[] args) {
		Console.ForegroundColor = ConsoleColor.Gray;
		Write("[NOTE] " + string.Join("", args));
		Profiler.Debug.AddLogMessage(string.Join("", args), 0);
	}

	public static void Info(params object[] args) {
		Console.ResetColor();
		Write("[INFO] " + string.Join("", args));
		Profiler.Debug.AddLogMessage(string.Join("", args), 1);
	}

	public static void Warn(params object[] args) {
		Console.ForegroundColor = ConsoleColor.Yellow;
		Write("[WARN] " + string.Join("", args));
		Profiler.Debug.AddLogMessage(string.Join("", args), 2);
	}

	public static void Error(params object[] args) {
		Console.ForegroundColor = ConsoleColor.Red;
		Write("[ERROR] " + string.Join("", args));
		Profiler.Debug.AddLogMessage(string.Join("", args), 3);
	}
}