namespace FloodForge.World;

public class ReplaceRoom : WorldDraggable {
	public Room replacingRoom;
	public Room replacedRoom;
	public Timeline timeline;
	public string[] preProcessorConditions;

    public Vector2i size;

	public ReplaceRoom(Room replacingRoom, Room replacedRoom, Timeline replacingTimeline, string[] preProcessorConditions) {
		this.replacingRoom = replacingRoom;
		this.replacedRoom = replacedRoom;
		this.timeline = replacingTimeline;
		this.preProcessorConditions = preProcessorConditions;
		this.size = new (replacingRoom.width, replacingRoom.height);
	}

	//TODO - add split between CanonPosition and DevPosition to actually match with the replaced room
	//TODO - draw the replaced room's information at the replacingroom's positions (dens)
	//IDEA - run timeline checks when drawing dens so that dens that would only appear on replaceroom are only rendered there
    public void Draw() {
		Immediate.Color(Themes.RoomSolid);
		UI.FillRect(this.position.x, this.position.y - this.size.y, this.position.x + this.size.x, this.position.y);

		this.replacingRoom.DrawRoomMeshes(this.position, WorldWindow.PositionType);

		Immediate.Color(Themes.Layer2Color);
		UI.Line(this.Position, this.replacedRoom.Position);
		if (!this.replacingRoom.isVirtualRoom){
			Immediate.Color(Themes.Layer0Color);
			UI.Line(this.Position, this.replacingRoom.Position);
		}

		// TODO - make sure the hoveredRoomExit behaviour is separate from the replacingroom instance
		this.replacingRoom.DrawRoomShortcuts(this.position);

		Vector2 o = WorldWindow.worldMouse - this.position;
		bool hovered = o.x >= 0f && o.y <= 0f && o.x <= this.size.x && o.y >= -this.size.y;
		Immediate.Color(hovered ? Themes.RoomBorderHighlight : Themes.RoomBorder);
		UI.StrokeRect(this.position.x, this.position.y, this.position.x + this.size.x, this.position.y - this.size.y);

		for (int j = 0; j < this.replacingRoom.denShortcutEntrances.Count && j < this.replacedRoom.dens.Count; j++) {
			Vector2i denPosition = this.replacingRoom.denShortcutEntrances[j];
			Room.DrawDen(this.replacedRoom.dens[j], this.position.x + denPosition.x, this.position.y - denPosition.y, j == this.replacedRoom.hoveredDen, WorldWindow.HoveringDraggable == this, this.timeline);
		}

		int i = 0;
		Immediate.Color(1f, 1f, 1f);
		foreach (string timeline in this.timeline.timelines) {
			UI.CenteredTexture(Mods.GetTimelineTexture(timeline), (float) (this.Position.x + (i * WorldWindow.SelectorScale) + 1.5f), (float) (this.Position.y - 1.5f), WorldWindow.SelectorScale);
			i++;
		}

		if (this.timeline.timelines.Count > 0 && this.timeline.timelineType == TimelineType.Except) {
			Immediate.Color(1f, 0f, 0f);
			UI.Line(this.Position.x + 2f - WorldWindow.SelectorScale * 0.5f, this.Position.y - 2f, this.Position.x + 2f + WorldWindow.SelectorScale * 0.5f + (this.timeline.timelines.Count - 1) * WorldWindow.SelectorScale, this.Position.y - 2f, WorldWindow.SelectorScale * 4f);
		}

		if (this.preProcessorConditions.Length != 0) {
			Immediate.Color(1f, 1f, 0f);
			float x0 = this.Position.x + 2f - WorldWindow.SelectorScale * 0.5f;
			float y0 = this.Position.y - 2f - WorldWindow.SelectorScale * 0.5f;
			float y1 = this.Position.y - 2f + WorldWindow.SelectorScale * 0.5f;
			UI.Line(x0, y0, x0, y1, WorldWindow.SelectorScale * 3f);
		}
    }

	public bool Inside(Vector2 pos) {
		Vector2 position = this.Position;
		return pos.x >= position.x && pos.y >= position.y - this.size.y && pos.x < position.x + this.size.x && pos.y <= position.y;
	}

	/// what does this class need to do?
	/// it needs to:
	/// - know what room it replaces
	/// - know what timeline and preprocessorconditions it replaces it for
	/// - know what room it replaces with
	/// - be able to draw the replacing room's graphics in its place
	/// it doesn't need to:
	/// - keep track of dens, since the replaced room's spawns are used
	/// - keep track of connections, since the replaced room's connections are used
	/// 
	/// important aspect:
	/// - if multiple rooms are replaced by the same replaceroom, that exact same replaceroom needs to be drawn multiple times
	/// so:
	/// - region has a list of replacerooms
	/// - when a replaceroom is parsed, look up whether the replaced room's virtualRoom already exists
	/// - if it does exist, refer to it, if it doesn't, create a new virtualRoom
	/// this way, if a room is used both as a replacement and as a normal room, editing the normal room affects all replacements
	/// 
	/// in short:
	/// - ReplaceRoom : WorldDraggable { Room roomReference; }
	/// - Room { bool isVirtualRoom; //true if a room only exists for replacerooms to use }
}