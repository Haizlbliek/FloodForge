using System.Text.RegularExpressions;
using System.Threading.Tasks;
using FloodForge.Droplet;
using FloodForge.Popups;
using Silk.NET.Input;
using Stride.Core;
using Stride.Core.Extensions;
using TextCopy;

namespace FloodForge.World;

internal static partial class Regexs {
	[GeneratedRegex(@"world_([^._-]+)\.txt", RegexOptions.IgnoreCase)]
	public static partial Regex WorldFileRegex();
}

public static class WorldWindow {
	private static readonly WorldMenuItems menuItems = new WorldMenuItems();

	public static bool VisibleDevItems { get; private set; } = false;
	public static bool VisibleCreatures { get; private set; } = true;
	public static RoomPosition PositionType { get; private set; } = RoomPosition.Canon;
	public static RoomColors ColorType { get; private set; } = RoomColors.None;
	public static readonly bool[] VisibleLayers = [true, true, true];

	public static TimelineType VisibleTimelineType;
	public static HashSet<string> VisibleTimelines = [];

	// REVIEW: Auto-mode? Which basically chooses whichever looks better for any given connection?
	// (I.E. choose the one that's closest, but preferably one that does not invert (for example, CC_S01))
	public static bool changeConnectBehaviour = true;

	public static Region region = null!;
	public static bool ValidRegionLoaded => !(WorldWindow.region == null || WorldWindow.region.acronym.IsNullOrEmpty() || WorldWindow.region.exportPath.IsNullOrEmpty());
	public static bool invalidCreaturesEncountered = false;
	public static bool ExportFinished = true;
	public static Vector2 cameraOffset;
	private static bool cameraPanning = false;
	private static bool cameraPanningBlocked = false;
	private static Vector2 cameraPanTo = Vector2.Zero;
	private static Vector2 cameraPanStart = Vector2.Zero;
	private static Vector2 cameraPanStartMouse = Vector2.Zero;
	public static float cameraScale = 32f;
	private static float cameraScaleTo = 32f;
	private static Rect camBound;
	public static float SelectorScale { get; private set; } = 1f;
	public static Vector2 worldMouse;

	public static HashSet<Room> selectedRooms = []; // REVIEW - HashSet?
	public static HashSet<Room> oldSelection = [];
	public static Room? roomPossibleSelect = null;
	private static SelectingState selectingState = SelectingState.None;
	public static Vector2 selectionStart;
	public static Vector2 selectionEnd;

	private static bool roomSnap;
	public static bool placingRoom = false;
	public static Vector2 placingRoomPos;
	public static Vector2i placingRoomSize;

	public static Room? holdingRoom = null;
	public static Vector2? holdingStart = null;
	public static int holdingType = 0;
	public static bool continueDrag = false;
	public static Room? highlightRoom;

	public static Connection? CurrentConnection;
	public static Vector2? ConnectionStart;
	public static Vector2? ConnectionEnd;
	public static bool CurrentConnectionValid;
	private static ConnectionState connectionState;

	public static bool EnableProfilerScreen = false;

	private enum ConnectionState {
		None,
		NoConnection,
		Connection
	}

	private enum SelectingState {
		None,
		Selecting,
		PendingDrag,
		Dragging,
		Panning,
	}

	public static Room? HoveringRoom => region.rooms.LastOrDefault(r => r.Visible && r.Inside(worldMouse));

	public static Connection? HoveringConnection => region.connections?.LastOrDefault(c => {
		return c.roomA.Visible && c.roomB.Visible && c.Hovered;
	});

	public static bool HoveringOrSelectedRooms(out HashSet<Room> rooms) {
		rooms = [];
		if (selectedRooms.Count >= 1) {
			rooms = [.. selectedRooms];
			return true;
		}
		else {
			Room? room = HoveringRoom;
			if (room != null && room is not OffscreenRoom) {
				rooms = [room];
				return true;
			}
		}
		return false;
	}

	public static void Initialize() {
		CreatureTextures.Initialize();
		ConditionalTimelineTextures.Initialize();
		RecentFiles.Initialize();
	}

	public static void Reset() {
		selectedRooms.Clear();
		roomPossibleSelect = null;
		selectingState = SelectingState.None;
		region = new Region();
	}

	private static float r = new Random().NextSingle() * 18000f + 16000f;
	private static void UpdateCamera() {
		if (r <= 0f && Main.AprilFools) {
			r = new Random().NextSingle() * 18000f + 16000f;
			Sfx.Play("assets/objects/hum.wav");
		}
		bool isHoveringPopup = PopupManager.Windows.Any(x => x.InteractBounds().Inside(Mouse.Pos));

		// LATER: Refactor into new UI system with ScrollArea
		float scrollY = -Mouse.Scroll;
		if (isHoveringPopup)
			scrollY = 0f;
		if (scrollY < -10f)
			scrollY = -10f;
		float zoom = MathF.Pow(1.25f, scrollY);
		if (MathF.Abs(zoom - 1f) > 0.1f)
			highlightRoom = null;
		if (float.IsNaN(cameraOffset.x) || float.IsNaN(cameraOffset.y)) { // find a way to maintain position when tabbing out and into fullscreen
			cameraOffset = Vector2.Zero;
			cameraPanTo = Vector2.Zero;
			cameraPanStart = Vector2.Zero;
			cameraPanStartMouse = Vector2.Zero;
		}
		Vector2 previousWorldMouse = Mouse.Pos * cameraScale + cameraOffset;
		cameraScaleTo *= zoom;
		cameraScale += (cameraScaleTo - cameraScale) * (1f - MathF.Pow(1f - Settings.CameraZoomSpeed, Program.Delta * 60f));
		worldMouse = Mouse.Pos * cameraScale + cameraOffset;

		if (highlightRoom == null) {
			cameraOffset += previousWorldMouse - worldMouse;
			cameraPanTo += previousWorldMouse - worldMouse;
		}

		if (Mouse.Middle) {
			highlightRoom = null;
			if (!cameraPanningBlocked && !cameraPanning) {
				if (isHoveringPopup)
					cameraPanningBlocked = true;

				if (!cameraPanningBlocked) {
					cameraPanStart = cameraOffset;
					cameraPanStartMouse = Main.GlobalMouse;
					cameraPanning = true;
				}
			}

			if (cameraPanning && !cameraPanningBlocked) {
				cameraPanTo = cameraPanStart + cameraScale * (cameraPanStartMouse - Main.GlobalMouse);
			}
		}
		else {
			cameraPanning = false;
			cameraPanningBlocked = false;
		}

		cameraOffset += (cameraPanTo - cameraOffset) * (1f - MathF.Pow(1f - Settings.CameraPanSpeed, Program.Delta * 60f));
	}

	public static void FocusCameraOn(Room room) {
		Vector2 focus = room.Position + new Vector2(room.width / 2f, room.height / -2f);
		cameraPanTo = focus;
		cameraScaleTo = MathF.Max(room.width, room.height * 1.1f) / 2f + 8f;
		highlightRoom = room;
	}

	private static void UpdateConnectionControls() {
		Room? hoveringRoom = null;
		uint hoveringConnection = 0;
		int hoveringShortcutEntrance = -1;
		float maxSqrDist = SelectorScale * SelectorScale;
		foreach (Room room in WorldWindow.region.rooms) {
			room.hoveredRoomExit = -1;
			if (!WorldWindow.VisibleLayers[room.data.layer])
				continue;

			for (uint i = 0; i < room.roomExits.Count; i++) {
				Vector2 spot = new();
				float sqrDist = 0;
				if (room.roomExitPaths[room.roomExits[(int) i]].endType == Room.RoomPathEndType.shortcutEntrance) {
					spot = room.GetShortcutEntranceWorldPoint(i); // if the roomPath has a shortcutExit, first check that
					sqrDist = (worldMouse - spot).SqrLength;
					if (sqrDist < maxSqrDist) {
						maxSqrDist = sqrDist;
						hoveringRoom = room;
						hoveringConnection = i;
						hoveringShortcutEntrance = -1;
					}
				}
				spot = room.GetConnectionConnectPoint(i); // then check the roomExit
				sqrDist = (worldMouse - spot).SqrLength;
				if (sqrDist < maxSqrDist) {
					maxSqrDist = sqrDist;
					hoveringRoom = room;
					hoveringConnection = i;
					hoveringShortcutEntrance = -1;
				}
			}
			for (uint i = 0; i < room.allShortcutEntrancePoints.Count; i++) {
				Vector2 spot = new();
				float sqrDist = 0;
				(Room.RoomConnection connection, bool matchesWithRoomExitPath) = room.shortcutEntrancePaths[room.allShortcutEntrancePoints[(int) i]];
				if (!matchesWithRoomExitPath && connection.endType == Room.RoomPathEndType.roomExit) {
					spot = room.RoomPositionToWorldPosition(connection.path.StartPosition);
					sqrDist = (worldMouse - spot).SqrLength;
					if (sqrDist < maxSqrDist) {
						maxSqrDist = sqrDist;
						hoveringRoom = room;
						hoveringConnection = room.GetRoomExitIDFromShortcut(i);
						hoveringShortcutEntrance = (int) i;
					}
				}
			}
		}
		hoveringRoom?.hoveredRoomExit = (int) hoveringConnection;
		hoveringRoom?.hoveredShortcutEntrance = hoveringShortcutEntrance;

		if (Input.Connection) {
			if (connectionState == ConnectionState.None) {
				if (hoveringRoom == null) {
					connectionState = ConnectionState.NoConnection;
					return;
				}

				if (hoveringRoom.Visible) {
					ConnectionStart = hoveringRoom.GetConnectionConnectPoint(hoveringConnection);
					ConnectionEnd = ConnectionStart;
					CurrentConnection = new Connection(hoveringRoom, hoveringConnection, null!, 0);
					connectionState = ConnectionState.Connection;
				}
			}
			else if (connectionState == ConnectionState.Connection && CurrentConnection != null) {
				if (hoveringRoom != null && hoveringRoom.Visible) {
					ConnectionEnd = hoveringRoom.GetConnectionConnectPoint(hoveringConnection);
					CurrentConnection.roomB = hoveringRoom;
					CurrentConnection.roomBExitID = hoveringConnection;
					CurrentConnectionValid = true;

					if (CurrentConnection.roomA == CurrentConnection.roomB) {
						CurrentConnectionValid = false;
					}
					else {
						foreach (Connection other in CurrentConnection.roomB.connections) {
							if (other.roomA == CurrentConnection.roomB && other.roomAExitID == CurrentConnection.roomBExitID &&
								other.roomB == CurrentConnection.roomA && other.roomBExitID == CurrentConnection.roomAExitID
							) {
								CurrentConnectionValid = false;
								break;
							}
						}
					}
				}
				else {
					ConnectionEnd = worldMouse;
					CurrentConnection.roomB = null!;
					CurrentConnection.roomBExitID = 0;
					CurrentConnectionValid = false;
				}
			}
		}
		else {
			if (CurrentConnection != null) {
				if (CurrentConnectionValid) {
					RoomAndConnectionChange change = new RoomAndConnectionChange(true);
					change.AddConnection(CurrentConnection);
					History.Apply(change);
				}

				CurrentConnection = null;
			}

			ConnectionStart = null;
			ConnectionEnd = null;
			connectionState = ConnectionState.None;
		}
	}

	private static void UpdateControls() {
		if (Mouse.Disabled)
			return;

		bool isOriginal = Settings.OriginalControls;

		if (Mouse.Left) {
			if (!Mouse.LastLeft) {
				if (selectingState == SelectingState.None) {
					Room? room = HoveringRoom;

					if (room != null && room.Visible) {
						holdingRoom = room;
						holdingStart = worldMouse;
						roomPossibleSelect = room;
						selectingState = SelectingState.PendingDrag;
					}
				}

				if (selectingState == SelectingState.None) {
					bool isPanning = isOriginal && !Keys.Modifier(Keys.Modifiers.Shift);

					selectingState = isPanning ? SelectingState.Panning : SelectingState.Selecting;
					selectionStart = isPanning ? Mouse.Pos : worldMouse;
					selectionEnd = selectionStart;

					bool isAdditive = (!isOriginal && Keys.Modifier(Keys.Modifiers.Shift)) || Keys.Modifier(Keys.Modifiers.Control);
					if (!isAdditive && !isPanning)
						selectedRooms.Clear();
				}
			}
			else {
				if ((selectingState == SelectingState.PendingDrag && Mouse.Moved || selectingState == SelectingState.Dragging) && roomPossibleSelect != null && holdingStart != null) {
					if (selectingState == SelectingState.PendingDrag) {
						HandleSelectionLogic(roomPossibleSelect);
						selectingState = SelectingState.Dragging;
					}

					ApplyMovement();
				}

				if (selectingState == SelectingState.Selecting)
					selectionEnd = worldMouse;

				if (selectingState == SelectingState.Panning) {
					selectionEnd = Mouse.Pos;
					cameraPanTo += (selectionStart - selectionEnd) * cameraScale;
					selectionStart = selectionEnd;
				}
			}
		}
		else {
			if (selectingState == SelectingState.PendingDrag && roomPossibleSelect != null) {
				HandleSelectionLogic(roomPossibleSelect);
				if (roomSnap) {
					foreach (Room room in selectedRooms)
						room.Position = room.Position.Rounded();
				}
			}

			if (selectingState == SelectingState.Selecting) {
				foreach (Room room in region.rooms) {
					if (room.Intersects(selectionStart, selectionEnd) && room.Visible)
						selectedRooms.Add(room);
				}
			}

			holdingRoom = null;
			continueDrag = false;
			selectingState = SelectingState.None;
		}
	}

	private static void HandleSelectionLogic(Room room) {
		region.rooms.Remove(room);
		region.rooms.Add(room);

		bool isAdditive = Keys.Modifier(Keys.Modifiers.Shift) || Keys.Modifier(Keys.Modifiers.Control);
		if (isAdditive) {
			if (!selectedRooms.Remove(room))
				selectedRooms.Add(room);
		}
		else {
			if (!selectedRooms.Contains(room)) {
				selectedRooms.Clear();
				selectedRooms.Add(room);
			}
		}
	}

	private static void ApplyMovement() {
		if (holdingStart == null)
			return;

		MoveChange change = new MoveChange();
		Vector2 offset = worldMouse - (Vector2) holdingStart;
		if (roomSnap)
			offset.Round();

		foreach (Room room in selectedRooms) {
			Vector2 newPos = room.Position;
			if (roomSnap)
				newPos.Round();
			newPos += offset;

			Vector2 diff = newPos - room.Position;
			Vector2 dev = Vector2.Zero, canon = Vector2.Zero;

			bool moveBoth = Keys.Modifier(Keys.Modifiers.Alt) || PositionType == RoomPosition.Both;

			if (PositionType == RoomPosition.Canon) {
				canon = diff;
				if (moveBoth)
					dev = canon - room.DevPosition + room.CanonPosition;
			}
			else {
				dev = diff;
				if (moveBoth)
					canon = dev - room.CanonPosition + room.DevPosition;
			}
			room.MoveUpdate();
			change.AddRoom(room, dev, canon);
		}

		holdingStart += offset;
		if (continueDrag && History.Last is MoveChange moveChange) {
			change.Redo();
			moveChange.Merge(change);
		}
		else {
			History.Apply(change);
			continueDrag = true;
		}
	}

	private static void KeybindDelete() {
		Connection? connection = region.connections.FirstOrDefault(c => c.roomA.Visible && c.roomB.Visible && c.Hovered);
		if (connection != null) {
			RoomAndConnectionChange change1 = new RoomAndConnectionChange(false);
			change1.AddConnection(connection);
			History.Apply(change1);
			return;
		}

		Room? room = HoveringRoom;
		if (room != null && room is OffscreenRoom)
			return;

		RoomAndConnectionChange change = new RoomAndConnectionChange(false);

		if (selectedRooms.Count != 0) {
			foreach (Room room1 in selectedRooms) {
				if (room1 is OffscreenRoom)
					continue;

				change.AddRoom(room1);
				region.connections.Where(c => c.roomA == room1 || c.roomB == room1)
					.ForEach(change.AddConnection);
			}
			selectedRooms.Clear();
		}
		if (room != null) {
			change.AddRoom(room);
			region.connections.Where(c => c.roomA == room || c.roomB == room)
				.ForEach(change.AddConnection);
		}

		History.Apply(change);
		return;
	}

	private static void UpdateKeybinds() {
		if (Keys.JustPressed(Key.F3)) {
			EnableProfilerScreen = !EnableProfilerScreen;
		}

		if (Keys.JustPressed(Key.F)) {
			PopupManager.Add(new SearchPopup());
		}

		if (Keys.JustPressed(Key.I)) {
			if (HoveringOrSelectedRooms(out HashSet<Room> rooms)) {
				History.Apply(new MoveToBackChange(rooms));
			}
		}

		if (Keys.JustPressed(Key.X)) {
			KeybindDelete();
		}

		if (Keys.JustPressed(Key.S)) {
			if (HoveringOrSelectedRooms(out HashSet<Room> rooms)) {
				PopupManager.Add(new SubregionPopup(rooms));
			}
		}

		if (Keys.JustPressed(Key.T)) {
			if (HoveringOrSelectedRooms(out HashSet<Room> rooms)) {
				PopupManager.Add(new TagPopup(rooms));
			}
		}

		if (Keys.JustPressed(Key.L)) {
			if (HoveringOrSelectedRooms(out HashSet<Room> rooms)) {
				int minimumLayer = 3;
				foreach (Room room in rooms) {
					minimumLayer = Math.Min(minimumLayer, room.data.layer);
				}

				minimumLayer = (minimumLayer + 1) % 3;

				GeneralRoomChange<int> change = new GeneralRoomChange<int>(r => r.data.layer, (r, i) => r.data.layer = i);
				rooms.ForEach(r => change.AddRoom(r, minimumLayer));
				History.Apply(change);
			}
		}

		if (Keys.JustPressed(Key.G)) {
			if (HoveringOrSelectedRooms(out HashSet<Room> rooms)) {
				bool setMerge = !rooms.Any(r => r.data.merge);

				GeneralRoomChange<bool> change = new GeneralRoomChange<bool>(r => r.data.merge, (r, i) => r.data.merge = i);
				rooms.ForEach(r => change.AddRoom(r, setMerge));
				History.Apply(change);
			}
		}

		if (Keys.JustPressed(Key.H)) {
			if (HoveringOrSelectedRooms(out HashSet<Room> rooms)) {
				bool setHidden = !rooms.Any(r => r.data.hidden);

				GeneralRoomChange<bool> change = new GeneralRoomChange<bool>(r => r.data.hidden, (r, i) => r.data.hidden = i);
				rooms.ForEach(r => change.AddRoom(r, setHidden));
				History.Apply(change);
			}
		}

		if (VisibleCreatures && Keys.JustPressed(Key.C)) {
			bool found = false;

			for (int i = region.rooms.Count - 1; i >= 0; i--) {
				Room room = region.rooms[i];
				Vector2 roomMouse = worldMouse - room.Position;
				Vector2 shortcutPosition;

				if (room is OffscreenRoom offscreenRoom) {
					for (int j = 0; j <= room.dens.Count; j++) {
						shortcutPosition = new Vector2(room.width * 0.5f - room.dens.Count * 2f + i * 4f + 2.5f, -room.height * 0.25f - 0.5f);
						if ((roomMouse - shortcutPosition).Length < SelectorScale) {
							PopupManager.Add(new DenPopup(room.GetDen01(j)));
							found = true;
							break;
						}
					}
				}
				else {
					for (int j = 0; j < room.denShortcutEntrances.Count; j++) {
						Vector2i shortcut = room.denShortcutEntrances[j];
						shortcutPosition = new Vector2(shortcut.x + 0.5f, -1f - shortcut.y + 0.5f);
						if ((roomMouse - shortcutPosition).Length < SelectorScale) {
							PopupManager.Add(new DenPopup(room.GetDen01(j)));
							found = true;
							break;
						}
					}
				}

				if (found)
					break;
			}

			if (!found) {
				Room? room = HoveringRoom;
				if (room is OffscreenRoom offscreen) {
					PopupManager.Add(new DenPopup(offscreen.GetDen()));
				}
			}
		}

		if (Keys.JustPressed(Key.A)) {
			if (HoveringOrSelectedRooms(out HashSet<Room> rooms)) {
				PopupManager.Add(new RoomAttractivenessPopup(rooms));
			}
		}

		if (Keys.JustPressed(Key.D)) {
			Connection? connection = HoveringConnection;
			if (connection != null) {
				connection.conditionalPopup = PopupManager.Add(new ConditionalPopup(connection));
			}
			else if (HoveringOrSelectedRooms(out HashSet<Room> rooms)) {
				ConditionalPopup? conditionalPopup = PopupManager.Add(new ConditionalPopup(rooms));
				rooms.ForEach((room) => { room.conditionalPopup = conditionalPopup; });
			}
		}

		if (Keys.JustPressed(Key.R)) {
			if (region.acronym.IsNullOrEmpty()) {
				PopupManager.Add(new InfoPopup("You must create or import your region\nbefore creating or editing a room."));
			}
			else if (region.exportPath.IsNullOrEmpty()) {
				PopupManager.Add(new InfoPopup("You must export your region\nbefore creating or editing a room."));
			}
			else {
				if (HoveringRoom == null || HoveringRoom is OffscreenRoom) {
					PopupManager.Add(new CreateRoomPopup());
					placingRoom = true;
					placingRoomPos = worldMouse;
				}
				else {
					Main.mode = Main.Mode.Droplet;
					DropletWindow.LoadRoom(HoveringRoom);
				}
			}
		}

		{
			bool found = false;
			for (int i = region.rooms.Count - 1; i >= 0; i--) {
				Room room = region.rooms[i];
				room.hoveredDen = -1;
				if (found)
					continue;
				Vector2 roomMouse = worldMouse - room.Position;
				Vector2 shortcutPosition;
				float closestDistance = SelectorScale;

				if (room is OffscreenRoom offscreenRoom) {
					for (int j = 0; j < room.dens.Count; j++) {
						shortcutPosition = new Vector2(room.width * 0.5f - room.dens.Count * 2f + i * 4f + 2.5f, -room.height * 0.25f - 0.5f);
						float dist = (roomMouse - shortcutPosition).Length;
						if (dist < closestDistance) {
							room.hoveredDen = i;
							closestDistance = dist;
							found = true;
						}
					}
					continue;
				}

				foreach (Vector2i shortcut in room.denShortcutEntrances) {
					shortcutPosition = new Vector2(shortcut.x + 0.5f, -1f - shortcut.y + 0.5f);
					float dist = (roomMouse - shortcutPosition).Length;
					if (dist < closestDistance) {
						room.hoveredDen = room.GetDenId01(shortcut);
						closestDistance = dist;
						found = true;
					}
				}
			}
		}
	}

	private static void UpdateMain() {
		if (WorldWindow.region == null)
			return;

		UpdateCamera();
		UI.Update();

		float scale = Settings.WorldIconScale;
		SelectorScale = (scale < 0f) ? MathF.Max(cameraScale / 16f, 1f) : scale;

		if (renderRoomsTask == null || renderRoomsTask.IsCompleted) {
			if (Keys.Modifier(Keys.Modifiers.Control) && Keys.JustPressed(Key.Z)) {
				if (Keys.Modifier(Keys.Modifiers.Shift)) {
					History.Redo();
				}
				else {
					History.Undo();
				}
			}
			if (Keys.Modifier(Keys.Modifiers.Control) && Keys.JustPressed(Key.Y)) {
				History.Redo();
			}

			roomSnap = !Keys.Modifier(Keys.Modifiers.Alt);

			UpdateConnectionControls();

			UpdateControls();

			if (PopupManager.Windows.Count != 0)
				return;

			UpdateKeybinds();
		}
	}

	private static void DrawGrid() {
		float gridStep = MathF.Max(cameraScale / 16f, 1f);
		gridStep = MathF.Pow(2f, MathF.Ceiling(MathF.Log2(gridStep - 0.01f)));
		Vector2 offset = (cameraOffset / gridStep).Rounded() * gridStep;
		Vector2 extraOffset = new Vector2((Main.screenBounds.x - 1f) * gridStep * 16f % gridStep, 0f);
		Vector2 gridScale = Main.screenBounds * (gridStep * 16f);

		Immediate.Color(Themes.Grid);
		Immediate.Begin(Immediate.PrimitiveType.LINES);
		for (float x = -gridScale.x + offset.x; x < gridScale.x + offset.x; x += gridStep) {
			Immediate.Vertex(x + extraOffset.x, -cameraScale * Main.screenBounds.y + offset.y + extraOffset.y - gridStep);
			Immediate.Vertex(x + extraOffset.x, cameraScale * Main.screenBounds.y + offset.y + extraOffset.y + gridStep);
		}
		for (float y = -gridScale.y + offset.y; y < gridScale.y + offset.y; y += gridStep) {
			Immediate.Vertex(-cameraScale * Main.screenBounds.x + offset.x + extraOffset.x - gridStep, y + extraOffset.y);
			Immediate.Vertex(cameraScale * Main.screenBounds.x + offset.x + extraOffset.x + gridStep, y + extraOffset.y);
		}
		Immediate.End();
	}

	private static void DrawCurrentConnection() {
		if (ConnectionStart == null || ConnectionEnd == null || CurrentConnection == null)
			return;

		Immediate.Color(CurrentConnectionValid ? Themes.RoomConnectionHover : Themes.RoomConnectionInvalid);

		int segments = Mathf.RoundToInt((ConnectionStart - ConnectionEnd).Value.Length / 2f);
		segments = Math.Clamp(segments, 4, 100);
		float directionStrength = (ConnectionStart - ConnectionEnd).Value.Length;
		if (directionStrength > 300f)
			directionStrength = (directionStrength - 300f) * 0.5f + 300f;

		if (Settings.ConnectionType.value == Settings.STConnectionType.Linear) {
			UI.Line(ConnectionStart.Value, ConnectionEnd.Value, cameraScale / 4f);
		}
		else {
			Vector2 directionA = CurrentConnection.roomA.GetConnectionConnectDirection(CurrentConnection.roomAExitID);
			Vector2 directionB = CurrentConnection.roomB?.GetConnectionConnectDirection(CurrentConnection.roomBExitID) ?? Vector2.Zero;

			if (directionA.x == -directionB.x || directionA.y == -directionB.y) {
				directionStrength *= 0.3333f;
			}
			else {
				directionStrength *= 0.6666f;
			}

			directionA *= directionStrength;
			directionB *= directionStrength;

			Vector2 lastPoint = MathUtil.BezierCubic(0f, ConnectionStart.Value, ConnectionStart.Value + directionA, ConnectionEnd.Value + directionB, ConnectionEnd.Value);
			for (float t = 1f / segments; t <= 1.01f; t += 1f / segments) {
				Vector2 point = MathUtil.BezierCubic(t, ConnectionStart.Value, ConnectionStart.Value + directionA, ConnectionEnd.Value + directionB, ConnectionEnd.Value);
				UI.Line(lastPoint, point, cameraScale / 4f);
				lastPoint = point;
			}
		}
	}

	private static void DrawEditor() {
		if (WorldWindow.region == null)
			return;

		camBound = new Rect(cameraOffset - Main.screenBounds * WorldWindow.cameraScale, cameraOffset + Main.screenBounds * WorldWindow.cameraScale);
		Profiler.Debug.AddProfilerMessage($"camScale: {WorldWindow.cameraScale}; camPos: x={WorldWindow.cameraOffset.x},y={WorldWindow.cameraOffset.y};");

		Immediate.LoadIdentity();
		Immediate.Ortho(cameraOffset.x, cameraOffset.y, cameraScale * Main.screenBounds.x, cameraScale * Main.screenBounds.y);
		DrawGrid();
		Profiler.MarkPoint("DrawGrid");

		Program.gl.Enable(EnableCap.Blend);
		foreach (Room room in WorldWindow.region.rooms) {
			if (!room.data.merge)
				continue;
			if (!VisibleLayers[room.data.layer] || !CheckVisibleTimeline(room.TimelineType, room.Timelines))
				continue;

			if (PositionType == RoomPosition.Both) {
				room.DrawBlack(RoomPosition.Canon);
				room.DrawBlack(RoomPosition.Dev);
			}
			else {
				room.DrawBlack(PositionType);
			}
		}
		Profiler.MarkPoint("rooms", 2, true);
		foreach (Room room in WorldWindow.region.rooms) {
			Profiler.MarkPoint("rooms", 1, true);

			if (!VisibleLayers[room.data.layer] || !CheckVisibleTimeline(room.TimelineType, room.Timelines))
				continue;

			if (WorldWindow.CullTest(new Rect(room.Position.x, room.Position.y - room.height, room.Position.x + room.width, room.Position.y))) {
				if (!room.data.merge) {
					if (PositionType == RoomPosition.Both) {
						room.DrawBlack(RoomPosition.Canon);
						room.DrawBlack(RoomPosition.Dev);
					}
					else {
						room.DrawBlack(PositionType);
					}
				}

				if (PositionType == RoomPosition.Both) {
					room.Draw(RoomPosition.Canon);
					room.Draw(RoomPosition.Dev);
				}
				else {
					room.Draw(PositionType);
					if (Keys.Modifier(Keys.Modifiers.Alt)) {
						room.Draw((PositionType == RoomPosition.Canon) ? RoomPosition.Dev : RoomPosition.Canon);
					}
				}

				if (selectedRooms.Contains(room)) {
					Immediate.Color(Themes.SelectionBorder);
					if (PositionType == RoomPosition.Dev || PositionType == RoomPosition.Both) {
						UI.StrokeRect(Rect.FromSize(room.DevPosition.x, room.DevPosition.y, room.width, -room.height), cameraScale / 4f);
					}
					if (PositionType == RoomPosition.Canon || PositionType == RoomPosition.Both) {
						UI.StrokeRect(Rect.FromSize(room.CanonPosition.x, room.CanonPosition.y, room.width, -room.height), cameraScale / 4f);
					}
				}
			}
			Profiler.MarkPoint("rooms", 0, true);
		}
		Profiler.MarkPoint("DrawRooms");

		if (placingRoom) {
			Immediate.Color(1f, 1f, 1f, 0.5f);
			UI.FillRect(new Rect(
				placingRoomPos.x - placingRoomSize.x * 0.5f, placingRoomPos.y - placingRoomSize.y * 0.5f,
				placingRoomPos.x + placingRoomSize.x * 0.5f, placingRoomPos.y + placingRoomSize.y * 0.5f
			));
		}
		Program.gl.Disable(EnableCap.Blend);

		foreach (Connection connection in WorldWindow.region.connections) {
			Rect connectionAABB = connection.fittedAABB;
			if (Settings.DEBUGVisibleConnectionBounds) {
				Immediate.Color(Color.Cyan);
				UI.StrokeRect(connectionAABB);
			}
			connection.Draw();
		}

		DrawCurrentConnection();
		Profiler.MarkPoint("DrawConnections");

		if (selectingState == SelectingState.Selecting) {
			Program.gl.Enable(EnableCap.Blend);
			Immediate.Color(0.1f, 0.1f, 0.1f, 0.125f);
			UI.FillRect(selectionStart.x, selectionStart.y, selectionEnd.x, selectionEnd.y);
			Program.gl.Disable(EnableCap.Blend);
			Immediate.Color(Themes.SelectionBorder);
			UI.StrokeRect(selectionStart.x, selectionStart.y, selectionEnd.x, selectionEnd.y, cameraScale / 4f);
		}
	}

	public static void DebugDen(Den den, Room room, ref List<string> debugText) {
		debugText.Add("");
		debugText.Add($"Den: {room.name}");
		foreach (DenLineage lineage in den.creatures) {
			DenCreature creature = lineage;
			string line = "";
			line += $"{CreatureTextures.ExportName(creature.type)} x {creature.count}";
			while (creature.lineageTo != null) {
				creature = creature.lineageTo;
				line += $" --{Mathf.FloorToInt(creature.lineageChance * 100f)}%-> ";
				line += $"{CreatureTextures.ExportName(creature.type)} x {creature.count}";
			}
			debugText.Add(line);
		}
	}

	public static void DrawDebugData() {
		if (region == null)
			return;

		Connection? hoveringConnection = HoveringConnection;
		Room? hoveringRoom = HoveringRoom;
		int screenCount = region.rooms.Aggregate(0, (a, b) => a + b.data.cameras.Count);
		RichPresenceManager.Acronym = region.acronym;
		RichPresenceManager.DisplayName = region.displayName;
		RichPresenceManager.RoomCount = region.rooms.Count;
		RichPresenceManager.ScreenCount = screenCount;
		RichPresenceManager.ConnectionCount = region.connections.Count;

		List<string> debugText = [];
		debugText.Add("    Count:");
		debugText.Add($"Rooms: {region.rooms.Count}");
		debugText.Add($"Screens: {screenCount}");
		debugText.Add($"Connections: {region.connections.Count}");

		if (hoveringConnection != null) {
			debugText.Add("");
			debugText.Add("    Connection:");
			debugText.Add($"Room A: {hoveringConnection.roomA.name}");
			debugText.Add($"Connection A: {hoveringConnection.roomAExitID}");
			debugText.Add($"Room B: {hoveringConnection.roomB.name}");
			debugText.Add($"Connection B: {hoveringConnection.roomBExitID}");
		}

		if (hoveringRoom != null) {
			debugText.Add("");
			debugText.Add("    Room:");
			if (!hoveringRoom.valid) {
				debugText.Add($"INVALID - Check {region.acronym}-rooms");
				debugText.Add($"Name: {hoveringRoom.name}");
			}
			else {
				debugText.Add($"Name: {hoveringRoom.name}");
				if (hoveringRoom.pathOutsideRoomsFolder)
					debugText.Add($" > Room imported from outside {region.acronym}-rooms");
				debugText.Add($"Tags: {string.Join(" ", hoveringRoom.data.tags)}");
				debugText.Add($"Size: {hoveringRoom.width}x{hoveringRoom.height}");
				debugText.Add($"Dens: {hoveringRoom.dens.Count}");
				// CONNECTION DEBUG
				{
					List<string> encounteredConnections = [];
					List<string> connectionStringList = [];
					string connectionList = "";
					for (uint index = 0; index < hoveringRoom.roomExits.Count; index++) {
						if (hoveringRoom.AnyConnectionConnectedTo(index)) {
							foreach (Connection connection in hoveringRoom.connections) {
								string finalString = "";
								bool canHaveArrows = false;
								if (connection.roomA.name != hoveringRoom.name && connection.roomBExitID == index) {
									finalString += connection.roomA.name;
									encounteredConnections.Add(connection.roomA.name);
									canHaveArrows = true;
								}
								else if (connection.roomB.name != hoveringRoom.name && connection.roomAExitID == index) {
									finalString += connection.roomB.name;
									encounteredConnections.Add(connection.roomB.name);
									canHaveArrows = true;
								}
								if (connection == hoveringConnection && canHaveArrows)
									finalString = $">{finalString}<";
								connectionList += finalString;
							}
						}
						else {
							connectionList += "DISCONNECTED";
						}
						if (index + 1 < hoveringRoom.roomExits.Count)
							connectionList += ", ";
						if (connectionList.Length > 75) {
							connectionStringList.Add(connectionList);
							connectionList = "";
						}
						;
					}
					if (connectionList != "")
						connectionStringList.Add(connectionList);
					debugText.Add($"Connections: {hoveringRoom.roomExitPaths.Count}{(hoveringRoom.roomExitPaths.Count > 0 ? " : " + (connectionStringList.Count > 0 ? connectionStringList[0] : "") : "")}");
					if (connectionStringList.Count > 1)
					foreach (string line in connectionStringList[1..]) {
						debugText.Add(line);
					}
					List<string> duplicateConnections = [];
					string duplicateString = "";
					for (int index = 0; index < encounteredConnections.Count; index++) {
						for (int index2 = index + 1; index2 < encounteredConnections.Count; index2++) {
							if (!duplicateConnections.Contains(encounteredConnections[index]) && encounteredConnections[index] == encounteredConnections[index2]) {
								duplicateConnections.Add(encounteredConnections[index]);
								duplicateString += (duplicateString.Length > 0 ? ", " : "") + encounteredConnections[index];
								continue;
							}
						}
					}

					if (duplicateConnections.Count > 0)
						debugText.Add($" > This room has duplicate connections: {duplicateString}");
				}
				// END CONNECTION DEBUG
				debugText.Add($"Subregion: {(hoveringRoom.data.subregion == -1 ? "<<NONE>>" : region.subregions[hoveringRoom.data.subregion])}");
				debugText.Add($"Layer: {hoveringRoom.data.layer}");
				if (!hoveringRoom.data.merge)
					debugText.Add("No Merge");
				if (hoveringRoom.data.hidden)
					debugText.Add("Hidden");
			}
		}

		if (VisibleCreatures) {
			bool debuggedDen = false;

			for (int r = region.rooms.Count - 1; r >= 0; r--) {
				Room room = region.rooms[r];
				Vector2 roomMouse = worldMouse - room.Position;
				Vector2 shortcutPosition;

				if (room is OffscreenRoom offscreenRoom) {
					for (int j = 0; j <= room.dens.Count; j++) {
						shortcutPosition = new Vector2(room.width * 0.5f - room.dens.Count * 2f + r * 4f + 2.5f, -room.height * 0.25f - 0.5f);
						if ((roomMouse - shortcutPosition).Length < SelectorScale) {
							DebugDen(offscreenRoom.GetDen(), offscreenRoom, ref debugText);
							debuggedDen = true;
							break;
						}
					}
				}
				else {
					for (int j = 0; j < room.denShortcutEntrances.Count; j++) {
						Vector2i shortcut = room.denShortcutEntrances[j];
						shortcutPosition = new Vector2(shortcut.x + 0.5f, -1f - shortcut.y + 0.5f);
						if ((roomMouse - shortcutPosition).Length < SelectorScale) {
							DebugDen(room.GetDen01(j), room, ref debugText);
							debuggedDen = true;
							break;
						}
					}
				}

				if (debuggedDen)
					break;
			}
		}

		int i = 1;
		Immediate.Color(0f, 0f, 0f);
		foreach (string line in debugText.AsEnumerable().Reverse()) {
			float yPos = -Main.screenBounds.y + (i * 0.04f);
			UI.font.Write(line, -Main.screenBounds.x + 0f, yPos - 0.003f, 0.03f);
			UI.font.Write(line, -Main.screenBounds.x + 0f, yPos + 0.003f, 0.03f);
			UI.font.Write(line, -Main.screenBounds.x - 0.003f, yPos + 0f, 0.03f);
			UI.font.Write(line, -Main.screenBounds.x + 0.003f, yPos + 0f, 0.03f);
			i++;
		}
		i = 1;
		Immediate.Color(Themes.Text);
		foreach (string line in debugText.AsEnumerable().Reverse()) {
			UI.font.Write(line, -Main.screenBounds.x, -Main.screenBounds.y + (i * 0.04f), 0.03f);
			i++;
		}
	}

	public static void Draw() {
		if ((renderRoomsTask == null || renderRoomsTask.Status != TaskStatus.Running) && PopupManager.Windows.Count == 0) {
			oldSelection.Clear();
			oldSelection = [.. selectedRooms];
		}
		if (Keys.Modifier(Keys.Modifiers.Alt)) {
			if (Keys.JustPressed(Key.S)) {
				PopupManager.Add(new SplashArtPopup());
				return;
			}
			else if (Keys.JustPressed(Key.T)) {
				PopupManager.Add(new MarkdownPopup("docs/TutorialWorld.md"));
				return;
			}
		}
		UpdateMain();
		Profiler.MarkPoint("UpdateMain");

		Profiler.MarkPoint("DrawEditor", 1);
		DrawEditor();
		Profiler.MarkPoint(-1);

		Immediate.LoadIdentity();
		Immediate.Ortho(-1f * Main.screenBounds.x, 1f * Main.screenBounds.x, -1f * Main.screenBounds.y, 1f * Main.screenBounds.y, 0f, 1f);

		DrawDebugData();
		Profiler.MarkPoint("DrawDebug");

		menuItems.Draw();
		Profiler.MarkPoint("DrawMenuItems");
	}

	public enum RoomPosition {
		Canon,
		Dev,
		Both,
	}

	public enum RoomColors {
		None,
		Layer,
		Subregion,
	}

	private static Room? CopyRoom(string fromFilePath, string toFilePath, bool forceOverwrite = false) {
		if (File.Exists(toFilePath) & !forceOverwrite) {
			return null!;
		}

		FileInfo fromFile = new FileInfo(fromFilePath);
		FileInfo toFile = new FileInfo(toFilePath);
		string fromRoom = Path.GetFileNameWithoutExtension(fromFile.Name);
		string toRoom = Path.GetFileNameWithoutExtension(toFile.Name);

		string oldFile = "";
		if (forceOverwrite) {
			oldFile = File.ReadAllText(toFilePath);
			File.Delete(toFilePath);
		}
		File.Copy(fromFilePath, toFilePath);
		bool initial = Settings.WarnMissingImages;
		Settings.WarnMissingImages.value = false;
		Room room = new Room(fromFilePath, toRoom) {
			CanonPosition = WorldWindow.cameraOffset,
			DevPosition = WorldWindow.cameraOffset
		};
		Settings.WarnMissingImages.value = initial;
		Room? roomToDelete = null;
		if (forceOverwrite) {
			foreach (Room roomToCheck in region.rooms) {
				if (roomToCheck.name == room.name) {
					roomToDelete = roomToCheck;
					break;
				}
			}
		}
		if (roomToDelete == null) {
			RoomAndConnectionChange change = new RoomAndConnectionChange(true);
			change.AddRoom(room);
			History.Apply(change);

			for (int i = 0; i < room.data.cameras.Count; i++) {
				string imageSuffix = $"_{i + 1}.png";
				string imagePath = fromRoom + imageSuffix;

				string? sourceImage = PathUtil.FindFile(fromFile.DirectoryName!, imagePath);

				if (sourceImage != null) {
					string destImage = Path.Combine(toFile.DirectoryName!, toRoom + imageSuffix);
					if (forceOverwrite)
						File.Delete(destImage);
					File.Copy(sourceImage, destImage);
				}
			}
		}
		else {
			RoomReplacementChange change = new RoomReplacementChange(room, roomToDelete, toFilePath, fromFilePath, oldFile);
			History.Apply(change);
		}
		return room;
	}

	private static Room CreateAndAddRoom(string path, string name, string tag = "", bool importFromOutside = false) {
		RoomAndConnectionChange change = new RoomAndConnectionChange(true);
		Room room = new Room(path, name, importFromOutside);
		if (tag.Length > 0)
			room.data.tags = [tag];
		room.CanonPosition = room.DevPosition = WorldWindow.cameraOffset;
		change.AddRoom(room);
		History.Apply(change);
		return room;
	}

	private static void MoveUpdate() {
		if (region == null)
			return;
		foreach (Room room in region.rooms) {
			room.MoveUpdate();
		}
	}

	public static bool CullTest(Rect bounds) {
		return bounds.x0 < camBound.x1 && bounds.x1 > camBound.x0 && bounds.y0 < camBound.y1 && bounds.y1 > camBound.y0;
	}

	public static bool CheckVisibleTimeline(TimelineType timelineType, HashSet<string> timelines) {
		if (VisibleTimelineType == TimelineType.All)
			return true;
		if (timelineType == TimelineType.All && VisibleTimelineType != TimelineType.Only)
			return true;
		if (VisibleTimelineType == TimelineType.Except) {
			if (timelineType == TimelineType.Only) {
				foreach (string timeline in timelines) {
					if (!VisibleTimelines.Contains(timeline)) { return true; }
				}
			}
			else if (timelineType == TimelineType.Except) {
				return true; // should really technically only return true if timelines does not exclude literally every slugcat that isn't excepted by world.
			}
		}
		else if (VisibleTimelineType == TimelineType.Only) {
			if (timelineType == TimelineType.All && VisibleTimelines.Count != 0)
				return true;
			if (timelineType == TimelineType.Only) {
				foreach (string timeline in timelines) {
					if (VisibleTimelines.Contains(timeline)) { return true; }
				}
			}
			else if (timelineType == TimelineType.Except) {
				foreach (string timeline in VisibleTimelines) {
					if (!timelines.Contains(timeline)) { return true; }
				}
			}
		}
		return false;
	}

	private static void HandleRoomFilesSelected(string[] paths) {
		if (paths.Length == 0)
			return;

		if (paths.Length > 1)
			History.StartCollectingChanges([typeof(RoomAndConnectionChange), typeof(RoomReplacementChange)]);
		int pathCount = 0;
		foreach (string path in paths) {
			if (!path.EndsWith(".txt")) {
				PopupManager.Add(new InfoPopup("File must be .txt: " + path));
				return;
			}

			string filename = Path.GetFileNameWithoutExtension(path);
			string acronym = Path.GetFileNameWithoutExtension(PathUtil.Parent(path));
			if (acronym.Equals("gates", StringComparison.InvariantCultureIgnoreCase)) {
				HandleGateFile(path, filename);
			}
			else {
				Room newRoom = HandleStandardFile(path, filename, acronym);
				if (newRoom != null) {
					newRoom.CanonPosition.x += (pathCount - paths.Length / 2) * 15f;
					newRoom.CanonPosition.y -= (pathCount - paths.Length / 2) * 5f;
					selectedRooms.Add(newRoom);
					pathCount++;
				}
			}
		}
		if (paths.Length > 1) {
			Change[] collectedChanges = History.StopCollectingChanges();
			if (collectedChanges.Length != 0) {
				MassChange change = new MassChange(collectedChanges);
				History.Apply(change);
			}
		}
	}

	private static void HandleGateFile(string path, string filename) {
		string[] names = filename.Split('_');

		if (names[1].Equals(region.acronym, StringComparison.InvariantCultureIgnoreCase) || names[2].Equals(region.acronym, StringComparison.InvariantCultureIgnoreCase)) {
			CreateAndAddRoom(path, filename, "GATE");
		}
		else {
			PopupManager.Add(
				new ConfirmPopup("Change which acronym?")
					.SetOkay(names[2])
					.Okay(() => {
						string newName = $"gate_{names[1]}_{region.acronym}.txt";
						CopyRoom(path, PathUtil.Combine(path, $"../{newName}"))?.data.tags = ["GATE"];
					})
					.SetCancel(names[1])
					.Cancel(() => {
						string newName = $"gate_{region.acronym}_{names[2]}.txt";
						CopyRoom(path, PathUtil.Combine(path, $"../{newName}"))?.data.tags = ["GATE"];
					})
			);
		}
	}

	static Task? renderRoomsTask;
	public static InfoPopup? renderStatusPopup;
	public static ConfirmPopup? confirmRenderPopup;
	private static async Task MassRenderRooms() {
		{
			List<Room> rooms = [];
			foreach (Room room in oldSelection) {
				if (room is not OffscreenRoom) {
					rooms.Add(room);
				}
			}
			if (rooms.Count <= 0) {
				PopupManager.Add(new InfoPopup("Select at least one valid room!"));
			}
			else {
				Logger.Info("Mass-rendering rooms!");
				renderStatusPopup = new InfoPopup("Rendering rooms\n_/_\ninit");
				PopupManager.Add(renderStatusPopup);

				int successCount = 0;
				int finished = 0;
				int totalCount = rooms.Count;
				List<(string, string)> messages = [];
				string errorMessage = "";
				foreach (Room room in rooms) {
					renderStatusPopup.UpdateText("Rendering rooms\n" + (finished + 1) + "/" + totalCount + "\nloading");
					await Task.Run(() => DropletWindow.LoadRoom(room));
					renderStatusPopup.UpdateText("Rendering rooms\n" + (finished + 1) + "/" + totalCount + "\nrendering");
					errorMessage = "";
					if (await Task.Run(() => DropletWindow.Render(out errorMessage))) {
						successCount++;
					}
					else {
						Logger.Warn($"Error while rendering {room.name} - message: {errorMessage}");
						messages.Add((room.name, errorMessage));
					}
					finished++;
				}
				Logger.Info($"Finished mass-render.\nSelection: {totalCount}\nSucceeded: {successCount}/{finished}");
				renderStatusPopup.Close();
				if (successCount == finished) {
					PopupManager.Add(new ConfirmPopup($"Finished mass-render.\n {successCount}/{finished} succeeded.").SetOkay("Copy path").SetCancel("Continue").Okay(() => { ClipboardService.SetText(WorldWindow.region.roomsPath); }));
				}
				else {
					string reportString = $"Finished mass-render.\n {successCount}/{finished} succeeded.\nDetected errors:\n";
					foreach ((string roomName, string message) in messages) {
						reportString += $"{roomName} - {message}\n";
					}
					if (messages.Count == 0) {
						reportString += $"No detected errors!";
					}
					PopupManager.Add(new ConfirmPopup(reportString + "View log.txt for more info."));
				}
			}
		}
	}

	private static Room HandleStandardFile(string path, string filename, string acronym, bool isGateFile = false) {
		if (acronym[Math.Max(acronym.IndexOfReverse('-'), 0)..] == "rooms")
			acronym = acronym[0..acronym.IndexOfReverse('-')];
		else if (filename.Split('_').Length > 0)
			acronym = filename.Split('_')[0];
		else
			acronym = "";
		if ((acronym.Equals(region.acronym, StringComparison.InvariantCultureIgnoreCase) & !acronym.IsNullOrEmpty()) || region.exportPath.IsNullOrEmpty()) {
			return CreateAndAddRoom(path, filename);
		}
		else {
			PopupManager.Add(
				new ConfirmPopup($"Room {filename} isn't located inside {region.acronym}.\nCopy room to {region.acronym}-rooms?")
					.SetCancel("Just Add")
					.Cancel(() => {
						CreateAndAddRoom(path, filename, importFromOutside: true);
					})
					.SetOkay("Yes")
					.Okay(() => {
						string filename = Path.GetFileName(path);
						if (!filename.Contains('_'))
							filename = '_' + filename;
						string newName = $"{region.acronym}{filename[Math.Max(0, filename.IndexOf('_'))..]}";
						string toPath = PathUtil.Combine(region.roomsPath, $"../{region.acronym}-rooms/{newName}");
						if (File.Exists(toPath)) {
							PopupManager.Add(new ConfirmPopup($"File {newName}already exists in\n{toPath[..Math.Max(0, toPath.IndexOfReverse('\\'))].Split("StreamingAssets")[^1]}\nOverwrite existing file?")
							.SetOkay("Overwrite")
							.SetCancel("Cancel")
							.Okay(() => {
								CopyRoom(path, toPath, true);
							}));
						}
						else
							CopyRoom(path, toPath)?.data.tags = isGateFile ? ["GATE"] : [];
					})
			);
		}
		return null!;
	}

	public class WorldMenuItems : MenuItems {
		private static event Action<TimelineType, HashSet<string>>? UpdateVisibleTimelines;
		private static void ExportButton() {
			string lastExportDirectory = WorldWindow.region.exportPath;

			if (!Settings.UpdateWorldFiles) {
				WorldWindow.region.exportPath = PathUtil.FindOrAssumeDirectory("worlds", WorldWindow.region.acronym);
				Logger.Info($"Special exporting to directory: {WorldWindow.region.exportPath}");

				if (!Directory.Exists(WorldWindow.region.exportPath)) {
					Directory.CreateDirectory(WorldWindow.region.exportPath);
				}
			}

			if (!string.IsNullOrEmpty(WorldWindow.region.exportPath)) {
				ExportMap();
			}
			else {
				PopupManager.Add(
					new FilesystemPopup((pathStrings) => {
						if (pathStrings == null || pathStrings.Length == 0)
							return;

						string selectedPath = pathStrings[0];
						WorldWindow.region.exportPath = PathUtil.FindOrAssumeDirectory(selectedPath, WorldWindow.region.acronym);
						WorldWindow.region.roomsPath = PathUtil.FindOrAssumeDirectory(selectedPath, $"{WorldWindow.region.acronym}-rooms");

						Directory.CreateDirectory(WorldWindow.region.exportPath);
						Directory.CreateDirectory(WorldWindow.region.roomsPath);

						ExportMap();
					}, 0)
					.Filter(FilesystemPopup.SelectionType.Folder)
					.Hint("YOUR_MOD/world/")
				);
			}

			WorldWindow.region.exportPath = lastExportDirectory;
		}

		private static void ExportMap() {
			WorldWindow.invalidCreaturesEncountered = false;
			WorldExporter.ExportMapFile();
			WorldExporter.ExportWorldFile();

			string image = PathUtil.FindOrAssumeFile(WorldWindow.region.exportPath, $"map_{WorldWindow.region.acronym}.png");
			WorldExporter.ExportImageFile(image);

			WorldExporter.ExportPropertiesFile(PathUtil.FindOrAssumeFile(WorldWindow.region.exportPath, "properties.txt"));
			PopupManager.Add(new InfoPopup("Exported successfully!"));
			WorldWindow.ExportFinished = true;
		}

		public WorldMenuItems() {
			this.buttons = [
				new Button("New", button => {
					PopupManager.Add(new AcronymPopup());
				}),

				new Button("Add Room", button => {
					PopupManager.Add(
						new FilesystemPopup(HandleRoomFilesSelected, 1)
							.Filter(new Regex("((?!.*_settings)(?=.+_.+).+\\.txt)|(gate_([^._-]+)_([^._-]+)\\.txt)"))
							.Multiple()
							.Hint("xx_a01.txt")
					);
				}, button => {
					return WorldWindow.ValidRegionLoaded;
				},
				"You must create or import a region\nbefore adding rooms."),

				new Button("Import", button => {
					PopupManager.Add(new FilesystemPopup(selection => {
						if (selection.Length == 0) return;

						WorldParser.ImportWorldFile(selection[0]);
					}, 0).Filter(Regexs.WorldFileRegex()).Hint("world_xx.txt"));
				}),

				new Button("Export", button => {
					if(!invalidCreaturesEncountered){
						WorldWindow.ExportFinished = false;
						ExportButton();
					}
					else{
						PopupManager.Add(new ConfirmPopup("This region contains invalid dens!\nExporting may delete or change these dens.").SetOkay("Export anyway").Okay(ExportButton));
					}
				}, button => {
					return WorldWindow.region != null && WorldWindow.ExportFinished;
				},
				"You must create or import a region\nbefore exporting."),

				new Button("No Colors", button => {
					if (ColorType == RoomColors.None) {
						ColorType = RoomColors.Layer;
						button.text = "Layer Colors";
					}
					else if (ColorType == RoomColors.Layer) {
						ColorType = RoomColors.Subregion;
						button.text = "Subregion Colors";
					}
					else if (ColorType == RoomColors.Subregion) {
						ColorType = RoomColors.None;
						button.text = "No Colors";
					}
				}, button => { return WorldWindow.ValidRegionLoaded; }),

				new LayerButton(0, button => { return WorldWindow.ValidRegionLoaded; }),
				new LayerButton(1, button => { return WorldWindow.ValidRegionLoaded; }),
				new LayerButton(2, button => { return WorldWindow.ValidRegionLoaded; }),

				new Button("Timeline", button => {
					PopupManager.Add(new TimelinePopup(
						WorldWindow.VisibleTimelineType,
						WorldWindow.VisibleTimelines,
						(TimelineType) => {
							WorldWindow.VisibleTimelineType = TimelineType;
							UpdateVisibleTimelines?.Invoke(WorldWindow.VisibleTimelineType, WorldWindow.VisibleTimelines);
							if(VisibleTimelineType == TimelineType.All) button.text = "Timeline";
							else if(VisibleTimelineType == TimelineType.Only) button.text = "<Timeline>";
							else button.text = ">Timeline<";
						},
						(selected, timeline) => {
							if(selected)
								WorldWindow.VisibleTimelines.Remove(timeline);
							else
								WorldWindow.VisibleTimelines.Add(timeline);
							UpdateVisibleTimelines?.Invoke(WorldWindow.VisibleTimelineType, WorldWindow.VisibleTimelines);
							if(VisibleTimelineType == TimelineType.All) button.text = "Timeline";
							else if(VisibleTimelineType == TimelineType.Only) button.text = "<Timeline>";
							else button.text = ">Timeline<";
						},
						ref UpdateVisibleTimelines));
					}, button => {
						return WorldWindow.ValidRegionLoaded;
					}
				),

				new Button("Dev Items: Hidden", button => {
					VisibleDevItems = !VisibleDevItems;
					button.text = VisibleDevItems ? "Dev Items: Shown" : "Dev Items: Hidden";
				}, button => { return WorldWindow.ValidRegionLoaded; }),

				new Button("Creatures: Shown", button => {
					VisibleCreatures = !VisibleCreatures;
					button.text = VisibleCreatures ? "Creatures: Shown" : "Creatures: Hidden";
				}, button => { return WorldWindow.ValidRegionLoaded; }),

				new Button("Refresh Region", button => {
					string? path = PathUtil.FindFile(WorldWindow.region.exportPath, $"world_{WorldWindow.region.acronym}.txt");
					if (path == null) {
						PopupManager.Add(new InfoPopup("Could not find world_xx.txt file!"));
						return;
					}

					WorldParser.ImportWorldFile(path);
				}, button => { return WorldWindow.ValidRegionLoaded; },
				"You must create or import a region\nbefore refreshing."),

				new Button("Canon", button => {
					if (PositionType == RoomPosition.Canon) {
						PositionType = RoomPosition.Dev;
						button.text = "Dev";
					}
					else if (PositionType == RoomPosition.Dev) {
						PositionType = RoomPosition.Both;
						button.text = "Both";
					}
					else if (PositionType == RoomPosition.Both) {
						PositionType = RoomPosition.Canon;
						button.text = "Canon";
					}
					MoveUpdate();
				}, button => { return WorldWindow.ValidRegionLoaded; }),

				new Button("Connect: Path", button => {
					changeConnectBehaviour = !changeConnectBehaviour;
					button.text = changeConnectBehaviour ? "Connect: Path" : "Connect: Default";
					MoveUpdate();
				}, button => { return WorldWindow.ValidRegionLoaded; }),

				new AlignedButton("Mass Render", true, button => {
					confirmRenderPopup = new ConfirmPopup("Render " + oldSelection.Count + " rooms?" + (
						region.roomsPath.Contains(Path.Combine("StreamingAssets", "world")) ? "\nVanilla rooms may be overwritten!" :
						region.roomsPath.Contains(Path.Combine("StreamingAssets", "mods", "moreslugcats")) ? "\nDownpour rooms may be overwritten!" :
						region.roomsPath.Contains(Path.Combine("StreamingAssets", "mods", "watcher")) ? "\nWatcher rooms may be overwritten!" : 
						"\n<s:1>This will overwrite all existing images!")
						).Okay(() => {
							renderRoomsTask = Task.Run(MassRenderRooms);
						});
					PopupManager.Add(confirmRenderPopup);
				}, button => { return oldSelection.Count != 0 && ValidRegionLoaded; },
				"Select at least one valid room\nto render.")
			];
		}

		private class LayerButton : Button {
			private readonly int layer;

			public override bool Dark => !VisibleLayers[this.layer];

			public LayerButton(int layer) : base((layer + 1).ToString(), b => { ((LayerButton) b).Click(); }) {
				this.layer = layer;
			}

			public LayerButton(int layer, Func<Button, bool> contextCallback) : base((layer + 1).ToString(), b => { ((LayerButton) b).Click(); }, contextCallback) {
				this.layer = layer;
			}

			private void Click() {
				if (!Keys.Modifier(Keys.Modifiers.Shift)) {
					VisibleLayers[this.layer] = !VisibleLayers[this.layer];
					return;
				}

				bool alreadySolo = true;
				for (int i = 0; i < 3; i++) {
					if (VisibleLayers[i] != (i == this.layer)) {
						alreadySolo = false;
						break;
					}
				}

				for (int i = 0; i < 3; i++) {
					if (i == this.layer) {
						VisibleLayers[i] = true;
					}
					else {
						VisibleLayers[i] = alreadySolo;
					}
				}
			}
		}
	}
}