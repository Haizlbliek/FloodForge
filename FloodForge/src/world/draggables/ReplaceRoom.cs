namespace FloodForge.World;

public class ReplaceRoom : MapDraggable {
	public Room replacingRoom;
	public Room replacedRoom;
	public Timeline timeline;
	public string[] preProcessorConditions;

    public Vector2i size;

	public override bool IsVisible() {
		return WorldWindow.VisibleTimeline.OverlapsWith(this.timeline);
	}

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
    public void Draw(WorldWindow.RoomPosition positionType) {
		if (Settings.DEBUGRoomWireframe) {
			Program.gl.PolygonMode(GLEnum.FrontAndBack, GLEnum.Line);
		}
		Vector2 renderedPosition = positionType == WorldWindow.RoomPosition.Canon ? this.CanonPosition : this.DevPosition;

		Immediate.Color(Themes.RoomSolid);
		UI.FillRect(renderedPosition.x, renderedPosition.y - this.size.y, renderedPosition.x + this.size.x, renderedPosition.y);

		// REVIEW - change room mesh drawing to display correct layer information
		this.replacingRoom.DrawRoomMeshes(renderedPosition, positionType);

		Immediate.Color(Themes.Layer2Color);
		UI.Line(renderedPosition, this.replacedRoom.Position);
		if (!this.replacingRoom.isVirtualRoom){
			Immediate.Color(Themes.Layer0Color);
			UI.Line(renderedPosition, this.replacingRoom.Position);
		}

		this.DrawReplaceRoomShortcuts(renderedPosition);

		Vector2 o = WorldWindow.worldMouse - renderedPosition;
		bool hovered = o.x >= 0f && o.y <= 0f && o.x <= this.size.x && o.y >= -this.size.y;
		Immediate.Color(hovered ? Themes.RoomBorderHighlight : Themes.RoomBorder);
		UI.StrokeRect(renderedPosition.x, renderedPosition.y, renderedPosition.x + this.size.x, renderedPosition.y - this.size.y);

		for (int j = 0; j < this.replacingRoom.denShortcutEntrances.Count && j < this.replacedRoom.dens.Count; j++) {
			Vector2i denPosition = this.replacingRoom.denShortcutEntrances[j];
			Room.DrawDen(this.replacedRoom.dens[j], renderedPosition.x + denPosition.x, renderedPosition.y - denPosition.y, WorldWindow.denRoom == this.replacedRoom && WorldWindow.hoveredDen == j, WorldWindow.HoveringDraggable == this, this.timeline);
		}

		int i = 0;
		Immediate.Color(1f, 1f, 1f);
		foreach (string timeline in this.timeline.timelines) {
			UI.CenteredTexture(Mods.GetTimelineTexture(timeline), (float) (renderedPosition.x + (i * WorldWindow.SelectorScale) + 1.5f), (float) (renderedPosition.y - 1.5f), WorldWindow.SelectorScale);
			i++;
		}

		if (this.timeline.timelines.Count > 0 && this.timeline.timelineType == TimelineType.Except) {
			Immediate.Color(1f, 0f, 0f);
			UI.Line(renderedPosition.x + 2f - WorldWindow.SelectorScale * 0.5f, renderedPosition.y - 2f, renderedPosition.x + 2f + WorldWindow.SelectorScale * 0.5f + (this.timeline.timelines.Count - 1) * WorldWindow.SelectorScale, renderedPosition.y - 2f, WorldWindow.SelectorScale * 4f);
		}

		if (this.preProcessorConditions.Length != 0) {
			Immediate.Color(1f, 1f, 0f);
			float x0 = renderedPosition.x + 2f - WorldWindow.SelectorScale * 0.5f;
			float y0 = renderedPosition.y - 2f - WorldWindow.SelectorScale * 0.5f;
			float y1 = renderedPosition.y - 2f + WorldWindow.SelectorScale * 0.5f;
			UI.Line(x0, y0, x0, y1, WorldWindow.SelectorScale * 3f);
		}
    }

	public void DrawReplaceRoomShortcuts(Vector2 renderedPosition) {
		float clippedSelectorScale = Math.Min(WorldWindow.SelectorScale, 10f);
		for (int i = 0; i < this.replacingRoom.roomExits.Count; i++) {
			Vector2 exitPos = this.replacingRoom.RoomPositionToWorldPosition(this.replacingRoom.roomExitPaths[this.replacingRoom.roomExits[i]].path.StartPosition, renderedPosition);
			Vector2 entrancePos = this.replacingRoom.RoomPositionToWorldPosition(this.replacingRoom.roomExitPaths[this.replacingRoom.roomExits[i]].path.EndPosition, renderedPosition);
			bool entranceIsShortcutEntrance = this.replacingRoom.roomExitPaths[this.replacingRoom.roomExits[i]].endType == Room.RoomPathEndType.shortcutEntrance;
			bool connected = this.replacedRoom.AnyConnectionConnectedTo((uint) i);
			bool thisRoomExitHovered = WorldWindow.shortcutRoom == this.replacedRoom && WorldWindow.hoveredRoomExit == i;

			// Shortcut Entrance
			Immediate.Color(connected ? Themes.RoomConnection : Themes.RoomShortcutRoom);
			if (entranceIsShortcutEntrance) {
				if (WorldWindow.changeConnectBehaviour)
					UI.StrokeCircle(entrancePos, clippedSelectorScale * (thisRoomExitHovered ? 1.5f : 1f) * (connected ? 0.5f : 1f) * 0.25f, 8);
				else
					UI.FillCircle(entrancePos, clippedSelectorScale * (thisRoomExitHovered ? 1.5f : 1f) * (connected ? 0.5f : 1f) * 0.25f, 8);
			}

			// Room Exit
			if (WorldWindow.changeConnectBehaviour || !entranceIsShortcutEntrance)
				UI.FillCircle(exitPos, clippedSelectorScale * (thisRoomExitHovered ? 1.5f : 1f) * (connected ? 0.5f : 1f) * 0.25f, 8);
			else
				UI.StrokeCircle(exitPos, clippedSelectorScale * (thisRoomExitHovered ? 1.5f : 1f) * (connected ? 0.5f : 1f) * 0.25f, 8);

			// Find the index of the connection associated with this RoomExit (if it's connected to something)
			int getConnectionIndex = 0;
			bool connectionFound = false;
			if (connected) {
				for (int j = 0; j < this.replacedRoom.connections.Count; j++) {
					int connection = this.replacedRoom.connections[j].roomA == this.replacedRoom ? (int) this.replacedRoom.connections[j].roomAExitID : (int) this.replacedRoom.connections[j].roomBExitID;
					if (connection == i) {
						connectionFound = true;
						getConnectionIndex = j;
						break;
					}
				}
			}

			// Draws shortcutpath if either the associated exit or connection is hovered over.
			bool shouldBeHighlighted = (thisRoomExitHovered || connectionFound && this.replacedRoom.connections[getConnectionIndex].Hovered) && WorldWindow.hoveredShortcutEntrance == -1;
			if (shouldBeHighlighted || Keys.Modifier(Keys.Modifiers.Shift)) {
				if (this.replacingRoom.roomExitPaths.TryGetValue(this.replacingRoom.roomExits[i], out Room.RoomConnection result)) {
					Room.DrawRoomPath(renderedPosition, result, thisRoomExitHovered, shouldBeHighlighted);
				}
			}
		}

		if (Settings.DEBUGVisibleShortcutEntranceData) {
			foreach ((Room.RoomConnection connection, bool isMatchedWithRoomExit) in this.replacingRoom.shortcutEntrancePaths.Values) {
				Immediate.Color(isMatchedWithRoomExit ? Color.Black : connection.endType switch {
					Room.RoomPathEndType.deadend => new Color(1, 0, 0),
					Room.RoomPathEndType.shortcutEntrance => new Color(1, 1, 1),
					Room.RoomPathEndType.den => new Color(1, 1, 0),
					Room.RoomPathEndType.scavengerDen => new Color(0, 1, 0),
					Room.RoomPathEndType.roomExit => new Color(0.5f, 0.5f, 1),
					Room.RoomPathEndType.wackAMoleHole => new Color(0.2f, 0.4f, 0.6f),
					_ => Color.Black
				});
				UI.StrokeCircle(this.replacingRoom.RoomPositionToWorldPosition(connection.path.StartPosition, renderedPosition), isMatchedWithRoomExit ? 0.25f : 2f, 8);
				Immediate.Color(isMatchedWithRoomExit ? Color.Black : Color.Magenta);
				UI.StrokeCircle(this.replacingRoom.RoomPositionToWorldPosition(connection.path.EndPosition, renderedPosition), isMatchedWithRoomExit ? 0.25f : 1f, 8);
			}
		}
		// this bit handles the case where:
		// a shortcut entrance that connects to a roomexit, without said roomexit connecting back to the same entrance
		for (int i = 0; i < this.replacingRoom.allShortcutEntrancePoints.Count; i++) {
			bool thisShortcutEntranceHovered = WorldWindow.shortcutRoom == this.replacedRoom && WorldWindow.hoveredShortcutEntrance == i;
			if (this.replacingRoom.shortcutEntrancePaths.TryGetValue(this.replacingRoom.allShortcutEntrancePoints[i], out (Room.RoomConnection connection, bool isMatchedWithRoomExit) value)) {
				bool entranceConnectedToRoomExit = value.connection.endType == Room.RoomPathEndType.roomExit;
				if (!value.isMatchedWithRoomExit && entranceConnectedToRoomExit) {
					Vector2 entrancePos = this.replacingRoom.RoomPositionToWorldPosition(this.replacingRoom.shortcutEntrancePaths[this.replacingRoom.allShortcutEntrancePoints[i]].Item1.path.StartPosition, renderedPosition);
					Vector2 exitPos = this.replacingRoom.RoomPositionToWorldPosition(this.replacingRoom.shortcutEntrancePaths[this.replacingRoom.allShortcutEntrancePoints[i]].Item1.path.EndPosition, renderedPosition);
					uint exitID = this.replacingRoom.GetRoomExitIDFromShortcut((uint) i);
					bool roomExitIsConnected = this.replacedRoom.AnyConnectionConnectedTo(exitID);

					// Shortcut Entrance
					Immediate.Color(roomExitIsConnected ? Themes.RoomConnection : Themes.RoomShortcutRoom);
					if (WorldWindow.changeConnectBehaviour) {
						UI.StrokeCircle(entrancePos, clippedSelectorScale * (thisShortcutEntranceHovered ? 1.5f : 1f) * (roomExitIsConnected ? 0.5f : 1f) * 0.25f, 8);
						UI.FillCircle(exitPos, clippedSelectorScale * (thisShortcutEntranceHovered ? 1.5f : 1f) * (roomExitIsConnected ? 0.5f : 1f) * 0.25f, 8);
					}
					else {
						UI.FillCircle(entrancePos, clippedSelectorScale * (thisShortcutEntranceHovered ? 1.5f : 1f) * (roomExitIsConnected ? 0.5f : 1f) * 0.25f, 8);
						UI.StrokeCircle(exitPos, clippedSelectorScale * (thisShortcutEntranceHovered ? 1.5f : 1f) * (roomExitIsConnected ? 0.5f : 1f) * 0.25f, 8);
					}

					// Draws shortcutpath if the connection is hovered over. (since a roomexit isn't related to this entrance
					// (otherwise it'd have been drawn with the roomExits), there is no exit to hover over that should highlight this shortcut entrance)
					if (thisShortcutEntranceHovered || Keys.Modifier(Keys.Modifiers.Shift)) {
						Room.DrawRoomPath(renderedPosition, value.connection, thisShortcutEntranceHovered, thisShortcutEntranceHovered);
					}
				}
			}
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