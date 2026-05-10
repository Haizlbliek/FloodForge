namespace FloodForge.World;

public class Timeline {
    public TimelineType timelineType;
    public HashSet<string> timelines;

    public Timeline() {
        this.timelineType = TimelineType.All;
        this.timelines = [];
    }

    public Timeline(Timeline timeline) {
        this.timelineType = timeline.timelineType;
        this.timelines = timeline.timelines;
    }
    
    public Timeline (TimelineType type, HashSet<string> timelines) {
        this.timelineType = type;
        this.timelines = timelines;
    }

    public bool Match (Timeline other) {
        if (this.timelineType != other.timelineType) return false;
        if (this.timelineType == TimelineType.All) return true;

        return this.timelines.SetEquals(other.timelines);
    }

	public override string ToString() {
		if (this.timelineType == TimelineType.All) {
			return "ALL";
		}
		else {
            bool first = true;
			string timelineText = "";
			foreach (string timelineEntry in this.timelines) {
				timelineText += (first ? "" : ",") + timelineEntry;
			}
			if (this.timelineType == TimelineType.Except)
				timelineText = "X-" + timelineText;
			return timelineText;
		}
	}

	public Timeline And(Timeline timeline) {
		if (this.timelineType == TimelineType.All)
			return new (timeline);
		if (timeline.timelineType == TimelineType.All)
			return new (this);
		if (this.timelineType == TimelineType.Only) {
			if (timeline.timelineType == TimelineType.Only) {
				Timeline newTimeline = new(TimelineType.Only, []);
				foreach (string line in timeline.timelines) {
					if (this.timelines.Contains(line))
						newTimeline.timelines.Add(line);
				}
				return newTimeline;
			}
			if (timeline.timelineType == TimelineType.Except) {
				Timeline newTimeline = new(TimelineType.Only, [..this.timelines]);
				foreach (string line in timeline.timelines) {
					newTimeline.timelines.Remove(line);
				}
				return newTimeline;
			}
		}
		if (this.timelineType == TimelineType.Except) {
			if (timeline.timelineType == TimelineType.Only) {
				Timeline newTimeline = new(TimelineType.Only, [..timeline.timelines]);
				foreach (string line in this.timelines) {
					newTimeline.timelines.Remove(line);
				}
				return newTimeline;
			}
			if (timeline.timelineType == TimelineType.Except) {
				Timeline newTimeline = new(TimelineType.Except, [..this.timelines]);
				foreach (string line in timeline.timelines) {
					newTimeline.timelines.Add(line);
				}
				return newTimeline;
			}
		}
		return new (this);
	}

	public bool OverlapsWith(Timeline timeline) {
		if (this.timelineType == TimelineType.All || timeline.timelineType == TimelineType.All)
			return true;
		switch (this.timelineType) {
			case TimelineType.Except:
				switch (timeline.timelineType) {
					case TimelineType.Except:
						// theoretically this should return false if timeline.timelines excludes every scug not excluded by this.timelines
						// but that does not take custom scugs into account, so there's always a possible overlap
						return true;
					case TimelineType.Only:
						// if any timeline.timelines isn't excluded by this.timelines they can overlap
						foreach (string timelineEntry in timeline.timelines) {
							if (!this.timelines.Contains(timelineEntry)) {
								return true;
							}
						}
						return false;
				}
			break;
			case TimelineType.Only:
				switch (timeline.timelineType) {
					case TimelineType.Except:
						// if any this.timelines isn't excluded by timeline.timelines they can overlap
						foreach (string timelineEntry in this.timelines) {
							if (!timeline.timelines.Contains(timelineEntry)) {
								return true;
							}
						}
						return false;
					case TimelineType.Only:
						// if this.timelines includes any timeline.timelines they can overlap
						foreach (string timelineEntry in this.timelines) {
							if (timeline.timelines.Contains(timelineEntry)) {
								return true;
							}
						}
						return false;
				}
			break;
		}
		return false;
	}

}

public enum TimelineType {
	All,
	Only,
	Except
}