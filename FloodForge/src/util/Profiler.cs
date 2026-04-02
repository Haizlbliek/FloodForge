using System.Diagnostics;
using FloodForge;
using FloodForge.World;

public static class Profiler{
    static Stack<(Stopwatch contextSegment, ProfilerContext context)> contextStack = [];
    static ProfilerContext rootContext = null!;
    public static ProfilerContext finalContext = null!;
	static Stopwatch segmentStopwatch = new();
	static Stopwatch sumStopwatch = new();
    public static bool enableProfiler = false;
	public static void InitProfiler() {
        if (Main.mode == Main.Mode.World && WorldWindow.EnableProfilerScreen) {
            enableProfiler = true;
        contextStack = [];
		segmentStopwatch = Stopwatch.StartNew();
		sumStopwatch = Stopwatch.StartNew();
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


/// <summary>
/// Marks a segment in the profiler timeline.<br />Unless specifically used to exit a context, use the other overload instead.
/// </summary>
    public static void MarkPoint(int navigateContext) => MarkPoint("", navigateContext: navigateContext);

/// <summary>
/// Marks a segment in the profiler timeline.<br />Use <c>navigateContext</c> to profile timings within a method and around it.
/// </summary>
	public static void MarkPoint(string key, int navigateContext = 0) {
        if (!enableProfiler) {
            return;
        }
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
                if(contextStack.Count == 0) {
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
                if(contextStack.Count != 0 && rootContext != null) {
                    MarkPoint(-1);
                    if(contextStack.Count == 0) {
                        MarkPoint(-1);
                    }
                    else {
                        MarkPoint(-2);
                    }
                }
            break;
        }
	}

    public class ProfilerItem (string name, TimeSpan segment, TimeSpan sum){
        public string itemName = name;
        public TimeSpan segmentSpan = segment;
        public TimeSpan sumSpan = sum;
        public virtual string ToString(int contextDepth = 0) {
            string spacer = "";
            for(int i = 0; i < contextDepth; i++) spacer += ".  ";
            return $"{spacer + this.itemName}: {Math.Floor(this.segmentSpan.TotalMilliseconds * 1000)/1000}ms - ({Math.Floor(this.sumSpan.TotalMilliseconds * 1000)/1000}ms)";
        }
    }

	public class ProfilerContext : ProfilerItem {
        public TimeSpan startSegmentSpan;
        List<ProfilerItem> itemsInContext;
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
            for(int i = 0; i < contextDepth; i++) spacer += ".  ";
            string result = $"{spacer + this.itemName}: {Math.Floor(this.startSegmentSpan.TotalMilliseconds * 1000)/1000}ms";
            foreach(ProfilerItem profilerItem in this.itemsInContext) {
                result += $"\n{profilerItem.ToString(contextDepth + 1)}";
            }
            result += $"\n{spacer + this.itemName}>totalTime: {Math.Floor(this.segmentSpan.TotalMilliseconds*1000)/1000}ms";
            return result;
		}   
	}
}

//  mark point:
//  if navigateContext == -1, move out of the context and mark this as the total span of said context (for example, first mark ("MAIN", 1) and then finally mark (-1))
//      if the list of contexts is empty, instead end profiling.
//  if navigateContext == 0, add the current things to the most recent Context
//  if navigateContext == 1, create new ProfilerContext in current Context, then start a Stopwatch for said context so the relevant Contexttimer isn't interrupted
//  if navigateContext == anything else, force back out of every context and end profiling.