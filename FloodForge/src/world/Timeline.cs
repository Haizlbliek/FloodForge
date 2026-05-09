namespace FloodForge.World;

public class Timeline {
    public TimelineType timelineType;
    public HashSet<string> timelines;

    public Timeline() {
        this.timelineType = TimelineType.All;
        this.timelines = [];
    }
    
    public Timeline (TimelineType type, HashSet<string> timelines) {
        this.timelineType = type;
        this.timelines = timelines;
    }
}

public enum TimelineType {
	All,
	Only,
	Except
}