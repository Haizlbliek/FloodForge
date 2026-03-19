public static class Logger {
	private static readonly StreamWriter logFile;

	static Logger() {
		logFile = new StreamWriter("log.txt");
	}

	private static void Write(string value) {
		Console.WriteLine(value);
		logFile.WriteLine(value);
		logFile.Flush();
	}

	public static void Info(params object[] args) {
		Console.ResetColor();
		Write("[INFO] " + string.Join("", args));
	}

	public static void Note(params object[] args) {
		Console.ForegroundColor = ConsoleColor.Gray;
		Write("[NOTE] " + string.Join("", args));
	}

	public static void Error(params object[] args) {
		Console.ForegroundColor = ConsoleColor.Red;
		Write("[ERROR] " + string.Join("", args));
	}

	public static void Warn(params object[] args) {
		Console.ForegroundColor = ConsoleColor.Yellow;
		Write("[WARN] " + string.Join("", args));
	}
}