using System.Diagnostics;
using FloodForge;
using FloodForge.World;

public static class Profiler {
	static Stack<(Stopwatch contextSegment, ProfilerContext context)> contextStack = [];
	static ProfilerContext rootContext = null!;
	public static ProfilerContext finalContext = null!;
	static Stopwatch segmentStopwatch = new Stopwatch();
	static Stopwatch sumStopwatch = new Stopwatch();
	static Dictionary<string, (ProfilerSum, Stopwatch)> AllSumStopwatches = [];
	static readonly float[] FPSHistory = new float[100];
	static int currentIndex = 0;
	static int FPSHistoryFillLevel = 0;
	public static bool enableProfiler = false;
	public static void InitProfiler() {
		if (Main.mode == Main.Mode.World && WorldWindow.EnableProfilerScreen) {
			enableProfiler = true;
			contextStack = [];
			segmentStopwatch = Stopwatch.StartNew();
			sumStopwatch = Stopwatch.StartNew();
			AllSumStopwatches = [];
		}
		else {
			enableProfiler = false;
		}
	}

	public static void EndProfiler() {
		segmentStopwatch.Stop();
		sumStopwatch.Stop();
		finalContext = rootContext;
		rootContext = null!;
	}

	public static float GetAVGFPS() {
		if (!enableProfiler) {
			return 0f;
		}
		else {
			if (FPSHistory.Length == 0)
				return 0f;
			if (FPSHistoryFillLevel == currentIndex) {
				return FPSHistory.Average();
			}
			else {
				if (FPSHistoryFillLevel == 0)
					return 0f;
				float total = 0f;
				for (int i = 0; i < FPSHistoryFillLevel; i++) {
					total += FPSHistory[i];
				}
				return total / FPSHistoryFillLevel;
			}
		}
	}

	public static (float, float) GetMinMaxFPS() {
		if (!enableProfiler) {
			return (0f, 0f);
		}
		else {
			if (FPSHistory.Length == 0)
				return (0f, 0f);
			float min;
			if (FPSHistoryFillLevel == FPSHistory.Length) {
				min = FPSHistory.Min();
			}
			else {
				min = float.PositiveInfinity;
				for (int i = 0; i < FPSHistoryFillLevel; i++) {
					min = float.Min(min, FPSHistory[i]);
				}
			}
			return (min, FPSHistory.Max());
		}
	}

	public static void AddFPSDataPoint(float FPS) {
		if (FPSHistory.Length != 0) {
			if (currentIndex >= FPSHistory.Length)
				currentIndex = 0;
			FPSHistory[currentIndex] = FPS;
			FPSHistoryFillLevel = int.Max(currentIndex, FPSHistoryFillLevel);
			currentIndex++;
		}
	}

	/// <summary>
	/// Marks a segment in the profiler timeline.<br />Unless specifically used to exit a context, use the other overload instead.
	/// </summary>
	public static void MarkPoint(int navigateContext) => MarkPoint("", navigateContext: navigateContext);

	/// <summary>
	/// Marks a segment in the profiler timeline.<br />Use <c>navigateContext</c> to profile timings within a method and around it.
	/// </summary>
	public static void MarkPoint(string key, int navigateContext = 0, bool sumContext = false) {
		if (!enableProfiler) {
			return;
		}
		if (sumContext) {
			switch (navigateContext) {
				case 2:
					AllSumStopwatches.Add(key, (new ProfilerSum(key), new Stopwatch()));
					contextStack.Peek().context.AddItem(AllSumStopwatches[key].Item1);
					break;
				case 1:
					AllSumStopwatches[key].Item2.Restart();
					break;
				case 0:
					AllSumStopwatches[key].Item1.AddSummedTime(AllSumStopwatches[key].Item2.Elapsed);
					AllSumStopwatches[key].Item2.Stop();
					break;
			}
		}
		else {
			switch (navigateContext) {
				case 1:
					if (rootContext == null) {
						rootContext = new ProfilerContext(key, TimeSpan.Zero);
						InitProfiler();
						contextStack.Push((sumStopwatch, rootContext));
					}
					else {
						ProfilerContext newContext = new ProfilerContext(key, segmentStopwatch.Elapsed);
						contextStack.Peek().context.AddItem(newContext);
						contextStack.Push((Stopwatch.StartNew(), newContext));
						segmentStopwatch.Restart();
					}
					break;
				case 0:
					contextStack.Peek().context.AddItem(new ProfilerItem(key, segmentStopwatch.Elapsed, sumStopwatch.Elapsed));
					segmentStopwatch.Restart();
					break;
				case -1:
					if (contextStack.Count == 0) {
						rootContext.EndContext(sumStopwatch.Elapsed, sumStopwatch.Elapsed);
						EndProfiler();
					}
					else {
						(Stopwatch contextSegment, ProfilerContext context) = contextStack.Pop();
						context.EndContext(contextSegment.Elapsed, sumStopwatch.Elapsed);
						contextSegment.Stop();
						segmentStopwatch.Restart();
					}
					break;
				default:
					if (contextStack.Count != 0 && rootContext != null) {
						MarkPoint(-1);
						if (contextStack.Count == 0) {
							MarkPoint(-1);
						}
						else {
							MarkPoint(-2);
						}
					}
					break;
			}
		}
	}

	public class ProfilerItem(string name, TimeSpan segment, TimeSpan sum) {
		public string itemName = name;
		public TimeSpan segmentSpan = segment;
		public TimeSpan sumSpan = sum;
		public virtual string ToString(int contextDepth = 0) {
			string spacer = "";
			for (int i = 0; i < contextDepth; i++)
				spacer += ".  ";
			return $"{spacer + this.itemName}: {Math.Floor(this.segmentSpan.TotalMilliseconds * 1000) / 1000}ms - ({Math.Floor(this.sumSpan.TotalMilliseconds * 1000) / 1000}ms)";
		}
	}

	public class ProfilerContext : ProfilerItem {
		public TimeSpan startSegmentSpan;
		readonly List<ProfilerItem> itemsInContext;
		public ProfilerContext(string name, TimeSpan segment) : base(name, TimeSpan.Zero, TimeSpan.Zero) {
			this.startSegmentSpan = segment;
			this.itemsInContext = [];
		}
		public void AddItem(ProfilerItem item) {
			this.itemsInContext.Add(item);
		}
		public void EndContext(TimeSpan segment, TimeSpan sum) {
			this.segmentSpan = segment;
			this.sumSpan = sum;
		}
		public override string ToString(int contextDepth = 0) {
			string spacer = "";
			for (int i = 0; i < contextDepth; i++)
				spacer += ".  ";
			string result = $"{spacer + this.itemName} Start: {Math.Floor(this.startSegmentSpan.TotalMilliseconds * 1000) / 1000}ms";
			foreach (ProfilerItem profilerItem in this.itemsInContext) {
				result += $"\n{profilerItem.ToString(contextDepth + 1)}";
			}
			result += $"\n{spacer + this.itemName} End - totalTime: {Math.Floor(this.segmentSpan.TotalMilliseconds * 1000) / 1000}ms";
			return result;
		}
	}

	public class ProfilerSum : ProfilerItem {
		int sumCount = 0;
		public ProfilerSum(string name) : base(name, TimeSpan.Zero, TimeSpan.Zero) { }
		public void AddSummedTime(TimeSpan segment) {
			this.sumCount++;
			this.sumSpan += segment;
		}
		public override string ToString(int contextDepth = 0) {
			string spacer = "";
			for (int i = 0; i < contextDepth; i++)
				spacer += ".  ";
			string result = $"{spacer + this.itemName} - total time: {Math.Floor(this.sumSpan.TotalMilliseconds * 1000) / 1000}ms; avg time: {Math.Floor(1000 * (this.sumSpan.TotalMilliseconds / this.sumCount)) / 1000}ms";
			return result;
		}
		/// intended ProfilerSum usage:
		/// - method contains repeated operation
		/// - each time this specific operation is performed, measure how long it takes and add the span to the ProfilerSum, incrementing the SumCount (so that avg works)
		/// - Profiler keeps track of all ProfilerSums, and then when the logging happens, reads from these sums to see how much time a specific part contributed
	}

	public class Debug {
		// perhaps this should be unified with DrawDebugInformation(), with every message being able to say which corner it wants to be in? Perhaps even which tab?
		// so, for example, a script says AddMessage(message: "No Region Loaded", corner: 3, tab: "WorldEditor state")'  
		// and then DrawDebugMessages goes through each corner, orders by tab and then renders it all?
		// ofc, there'd also be a way to tell this profiler not to hide the debuginformation in the bottom left, since that's more generally useful
		static List<string> profilerMessagesLeft = [];
		static List<string> profilerMessagesRight = [];
		static readonly List<(string, int)> logMessages = [];
		public static void DrawProfilerMessages() {
			Immediate.Color(Color.White);
			int i = 0;
			foreach (string part in profilerMessagesLeft) {
				foreach (string line in part.Split("\n")) {
					UI.font.Write(line, -Main.screenBounds.x, Main.screenBounds.y - 0.1f - (i * 0.04f), 0.03f);
					i++;
				}
			}
			i = 0;
			foreach (string part in profilerMessagesRight) {
				foreach (string line in part.Split("\n")) {
					UI.font.Write(line, Main.screenBounds.x - UI.font.Measure(line, 0.03f).x, Main.screenBounds.y - 0.1f - (i * 0.04f), 0.03f);
					i++;
				}
			}
			i = 0;
			if(logMessages.Count > 45) {
				logMessages.RemoveRange(0, logMessages.Count - 45);
			}
			foreach((string message, int severity) in logMessages.AsEnumerable().Reverse()) {
				Immediate.Color(severity switch {
					0 => Color.Grey,
					1 => Color.White,
					2 => Color.Yellow,
					3 => Color.Red,
					_ => Color.White
				});
				foreach (string line in message.Split("\n").Reverse()) {
					UI.font.Write(line, Main.screenBounds.x - UI.font.Measure(line, 0.025f).x, -Main.screenBounds.y + 0.1f + (i * 0.03f), 0.025f);
					i++;
				}
			}
			profilerMessagesLeft = [];
			profilerMessagesRight = [];

			UVRect clearButton = new UVRect(Main.screenBounds.x - 0.05f, -Main.screenBounds.y, Main.screenBounds.x, -Main.screenBounds.y + 0.05f).UV(0f, 0f, 0.25f, 0.25f);
			if (UI.TextureButton(clearButton)) {
				logMessages.Clear();
			}
		}
		public static void AddProfilerMessage(string message, bool onRight = false) {
			if (Profiler.enableProfiler) {
				if(onRight) profilerMessagesRight.Add(message); else profilerMessagesLeft.Add(message);
			}
		}

		public static void AddLogMessage(string message, int severity) {
			logMessages.Add((message, severity));
		}
	}
}