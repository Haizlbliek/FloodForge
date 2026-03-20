using System.Text.RegularExpressions;
using FloodForge.Droplet;
using FloodForge.Popups;
using Silk.NET.Input;
using Silk.NET.SDL;
using Stride.Core;
using Stride.Core.Extensions;

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

	public static Region region = null!;
	public static Vector2 cameraOffset;
	private static bool cameraPanning = false;
	private static bool cameraPanningBlocked = false;
	private static Vector2 cameraPanTo = Vector2.Zero;
	private static Vector2 cameraPanStart = Vector2.Zero;
	private static Vector2 cameraPanStartMouse = Vector2.Zero;
	public static float cameraScale = 32f;
	private static float cameraScaleTo = 32f;
	public static float SelectorScale { get; private set; } = 1f;
	public static Vector2 worldMouse;

	public static List<Room> selectedRooms = []; // REVIEW - HashSet?
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
		float maxSqrDist = SelectorScale * SelectorScale;
		foreach (Room room in WorldWindow.region.rooms) {
			room.hoveredRoomExit = -1;
			if (!WorldWindow.VisibleLayers[room.data.layer])
				continue;

			for (uint i = 0; i < room.roomShortcutEntrances.Count; i++) {
				Vector2 spot = room.GetRoomEntrancePosition(i);
				float sqrDist = (worldMouse - spot).SqrLength;
				if (sqrDist < maxSqrDist) {
					maxSqrDist = sqrDist;
					hoveringRoom = room;
					hoveringConnection = i;
				}
			}
		}
		hoveringRoom?.hoveredRoomExit = (int) hoveringConnection;

		if (Input.Connection) {
			if (connectionState == ConnectionState.None) {
				if (hoveringRoom == null) {
					connectionState = ConnectionState.NoConnection;
					return;
				}

				ConnectionStart = hoveringRoom.GetRoomEntrancePosition(hoveringConnection);
				ConnectionEnd = ConnectionStart;
				CurrentConnection = new Connection(hoveringRoom, hoveringConnection, null!, 0);
				connectionState = ConnectionState.Connection;
			}
			else if (connectionState == ConnectionState.Connection && CurrentConnection != null) {
				if (hoveringRoom != null) {
					ConnectionEnd = hoveringRoom.GetRoomEntrancePosition(hoveringConnection);
					CurrentConnection.roomB = hoveringRoom;
					CurrentConnection.connectionB = hoveringConnection;
					CurrentConnectionValid = true;

					if (CurrentConnection.roomA == CurrentConnection.roomB) {
						CurrentConnectionValid = false;
					}
					else {
						foreach (Connection other in CurrentConnection.roomB.connections) {
							if (other.roomA == CurrentConnection.roomB && other.connectionA == CurrentConnection.connectionB &&
								other.roomB == CurrentConnection.roomA && other.connectionB == CurrentConnection.connectionA
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
					CurrentConnection.connectionB = 0;
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
		if (Mouse.Disabled) return;

		bool isOriginal = Settings.OriginalControls;

		if (Mouse.Left) {
			if (!Mouse.LastLeft) {
				if (selectingState == SelectingState.None) {
					Room? room = HoveringRoom;

					if (room != null) {
						holdingRoom = room;
						holdingStart = worldMouse;
						roomPossibleSelect = room;
						selectingState = SelectingState.PendingDrag;
						if (!isOriginal && Main.AprilFools) Sfx.Play($"assets/objects/click{new Random().Next(1, 3)}.wav");
					}
				}

				if (selectingState == SelectingState.None) {
					bool isPanning = isOriginal && !Keys.Modifier(Keymod.Shift);
					
					selectingState = isPanning ? SelectingState.Panning : SelectingState.Selecting;
					selectionStart = isPanning ? Mouse.Pos : worldMouse;
					selectionEnd = selectionStart;

					bool isAdditive = (!isOriginal && Keys.Modifier(Keymod.Shift)) || Keys.Modifier(Keymod.Ctrl);
					if (!isAdditive && !isPanning) selectedRooms.Clear();
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

				if (selectingState == SelectingState.Selecting) selectionEnd = worldMouse;
				
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
					foreach (Room room in selectedRooms) room.Position = room.Position.Rounded();
				}
			}

			if (selectingState == SelectingState.Selecting) {
				foreach (Room room in region.rooms) {
					if (room.Intersects(selectionStart, selectionEnd)) selectedRooms.Add(room);
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

		bool isAdditive = Keys.Modifier(Keymod.Shift) || Keys.Modifier(Keymod.Ctrl);
		if (isAdditive) {
			if (selectedRooms.Contains(room)) selectedRooms.Remove(room);
			else selectedRooms.Add(room);
		} else {
			if (!selectedRooms.Contains(room)) {
				selectedRooms.Clear();
				selectedRooms.Add(room);
			}
		}
	}

	private static void ApplyMovement() {
		if (holdingStart == null) return;

		MoveChange change = new MoveChange();
		Vector2 offset = worldMouse - (Vector2) holdingStart;
		if (roomSnap) offset.Round();

		foreach (Room room in selectedRooms) {
			Vector2 newPos = room.Position;
			if (roomSnap) newPos.Round();
			newPos += offset;

			Vector2 diff = newPos - room.Position;
			Vector2 dev = Vector2.Zero, canon = Vector2.Zero;

			bool moveBoth = Keys.Modifier(Keymod.Alt) || PositionType == RoomPosition.Both;
			
			if (PositionType == RoomPosition.Canon) {
				canon = diff;
				if (moveBoth) dev = canon - room.DevPosition + room.CanonPosition;
			} else {
				dev = diff;
				if (moveBoth) canon = dev - room.CanonPosition + room.DevPosition;
			}
			change.AddRoom(room, dev, canon);
		}

		holdingStart += offset;
		if (continueDrag && History.Last is MoveChange moveChange) {
			change.Redo();
			moveChange.Merge(change);
		} else {
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
		if (room == null || room is OffscreenRoom)
			return;

		RoomAndConnectionChange change = new RoomAndConnectionChange(false);

		if (selectedRooms.Contains(room)) {
			foreach (Room room1 in selectedRooms) {
				if (room1 is OffscreenRoom)
					continue;

				change.AddRoom(room1);
				region.connections.Where(c => c.roomA == room1 || c.roomB == room1)
					.ForEach(change.AddConnection);
			}
			selectedRooms.Clear();
		}
		else {
			change.AddRoom(room);
			region.connections.Where(c => c.roomA == room || c.roomB == room)
				.ForEach(change.AddConnection);
		}

		History.Apply(change);
		return;
	}

	private static void UpdateKeybinds() {
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
				PopupManager.Add(new ConditionalPopup(connection));
			}
			else if (HoveringOrSelectedRooms(out HashSet<Room> rooms)) {
				PopupManager.Add(new ConditionalPopup(rooms));
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

		if (Keys.Modifier(Keymod.Ctrl) && Keys.JustPressed(Key.Z)) {
			if (Keys.Modifier(Keymod.Shift)) {
				History.Redo();
			}
			else {
				History.Undo();
			}
		}
		if (Keys.Modifier(Keymod.Ctrl) && Keys.JustPressed(Key.Y)) {
			History.Redo();
		}

		roomSnap = !Keys.Modifier(Keymod.Alt);

		UpdateConnectionControls();

		UpdateControls();

		if (PopupManager.Windows.Count != 0)
			return;

		UpdateKeybinds();
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

		if (CurrentConnectionValid) {
			Immediate.Color(1f, 1f, 0f);
		}
		else {
			Immediate.Color(1f, 0f, 0f);
		}

		int segments = Mathf.RoundToInt((ConnectionStart - ConnectionEnd).Value.Length / 2f);
		segments = Math.Clamp(segments, 4, 100);
		float directionStrength = (ConnectionStart - ConnectionEnd).Value.Length;
		if (directionStrength > 300f)
			directionStrength = (directionStrength - 300f) * 0.5f + 300f;

		if (Settings.ConnectionType.value == Settings.STConnectionType.Linear) {
			UI.Line(ConnectionStart.Value, ConnectionEnd.Value, cameraScale / 4f);
		}
		else {
			Vector2 directionA = CurrentConnection.roomA.GetRoomEntranceDirectionVector(CurrentConnection.connectionA);
			Vector2 directionB = CurrentConnection.roomB?.GetRoomEntranceDirectionVector(CurrentConnection.connectionB) ?? Vector2.Zero;

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

	private static void ResolveCollision(ref Vector2 pos1, ref Vector2 vel1, float w1, float h1, ref Vector2 pos2, ref Vector2 vel2, float w2, float h2) {
		float halfW1 = w1 / 2f, halfH1 = h1 / 2f;
		float halfW2 = w2 / 2f, halfH2 = h2 / 2f;

		Vector2 center1 = new Vector2(pos1.x + halfW1, pos1.y - halfH1);
		Vector2 center2 = new Vector2(pos2.x + halfW2, pos2.y - halfH2);

		float diffX = center1.x - center2.x;
		float diffY = center1.y - center2.y;

		float minDistanceX = halfW1 + halfW2;
		float minDistanceY = halfH1 + halfH2;

		float overlapX = minDistanceX - Math.Abs(diffX);
		float overlapY = minDistanceY - Math.Abs(diffY);

		if (overlapX > 0 && overlapY > 0) {
			if (overlapX < overlapY) {
				float dir = diffX > 0 ? 1 : -1;

				pos1.x += overlapX * 0.5f * dir;
				pos2.x -= overlapX * 0.5f * dir;

				float relativeVelX = vel1.x - vel2.x;
				if (relativeVelX * dir < 0) {
					float v1Next = vel2.x * 0.8f;
					float v2Next = vel1.x * 0.8f;
					vel1.x = v1Next;
					vel2.x = v2Next;
				}
			} else {
				float dir = diffY > 0 ? 1 : -1;

				pos1.y += overlapY * 0.5f * dir;
				pos2.y -= overlapY * 0.5f * dir;

				float relativeVelY = vel1.y - vel2.y;
				if (relativeVelY * dir < 0) {
					float v1Next = vel2.y * 0.8f;
					float v2Next = vel1.y * 0.8f;
					vel1.y = v1Next;
					vel2.y = v2Next;
				}
			}
			
			if (Math.Abs(vel1.Length + vel2.Length) > 5f) {
				Sfx.Play($"assets/objects/bump{Random.Shared.Next(1, 6)}.wav");
			}
		}
	}

	private static void DrawEditor() {
		if (WorldWindow.region == null)
			return;

		Immediate.LoadIdentity();
		Immediate.Ortho(cameraOffset.x, cameraOffset.y, cameraScale * Main.screenBounds.x, cameraScale * Main.screenBounds.y);
		DrawGrid();

		Program.gl.Enable(EnableCap.Blend);
		foreach (Room room in WorldWindow.region.rooms) {
			if (!room.data.merge)
				continue;
			if (!VisibleLayers[room.data.layer])
				continue;

			if (PositionType == RoomPosition.Both) {
				room.DrawBlack(RoomPosition.Canon);
				room.DrawBlack(RoomPosition.Dev);
			}
			else {
				room.DrawBlack(PositionType);
			}
		}
		foreach (Room room in WorldWindow.region.rooms) {
			if (Main.AprilFools) {
				room.CanonPosition += room.CanonVel * 0.1f;
				room.DevPosition += room.DevVel * 0.1f;
				room.CanonVel *= 0.95f;
				room.DevVel *= 0.95f;
				foreach (Room room2 in WorldWindow.region.rooms) {
					if (room == room2) continue;
					ResolveCollision(ref room.DevPosition, ref room.DevVel, room.width, room.height, ref room2.DevPosition, ref room2.DevVel, room2.width, room2.height);
					ResolveCollision(ref room.CanonPosition, ref room.CanonVel, room.width, room.height, ref room2.CanonPosition, ref room2.CanonVel, room2.width, room2.height);
				}
			}

			if (!VisibleLayers[room.data.layer])
				continue;

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
				if (Keys.Modifier(Keymod.Alt)) {
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

		if (placingRoom) {
			Immediate.Color(1f, 1f, 1f, 0.5f);
			UI.FillRect(new Rect(
				placingRoomPos.x - placingRoomSize.x * 0.5f, placingRoomPos.y - placingRoomSize.y * 0.5f,
				placingRoomPos.x + placingRoomSize.x * 0.5f, placingRoomPos.y + placingRoomSize.y * 0.5f
			));
		}
		Program.gl.Disable(EnableCap.Blend);

		foreach (Connection connection in WorldWindow.region.connections) {
			connection.Draw();
		}

		DrawCurrentConnection();

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
		debugText.Add($"Den: {room.Name}");
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

		List<string> debugText = [];
		debugText.Add("    Count:");
		debugText.Add($"Rooms: {region.rooms.Count}");
		debugText.Add($"Screens: {screenCount}");
		debugText.Add($"Connections: {region.connections.Count}");

		if (hoveringConnection != null) {
			debugText.Add("");
			debugText.Add("    Connection:");
			debugText.Add($"Room A: {hoveringConnection.roomA.Name}");
			debugText.Add($"Connection A: {hoveringConnection.connectionA}");
			debugText.Add($"Room B: {hoveringConnection.roomB.Name}");
			debugText.Add($"Connection B: {hoveringConnection.connectionB}");
		}

		if (hoveringRoom != null) {
			debugText.Add("");
			debugText.Add("    Room:");
			if (!hoveringRoom.valid) {
				debugText.Add($"INVALID - Check {region.acronym}-rooms");
				debugText.Add($"Name: {hoveringRoom.Name}");
			}
			else {
				debugText.Add($"Name: {hoveringRoom.Name}");
				debugText.Add($"Tags: {string.Join(" ", hoveringRoom.data.tags)}");
				debugText.Add($"Size: {hoveringRoom.width}x{hoveringRoom.height}");
				debugText.Add($"Dens: {hoveringRoom.dens.Count}");
				debugText.Add($"Subregion: {(hoveringRoom.data.subregion == -1 ? "<<NONE>>" : region.subregions[hoveringRoom.data.subregion])}");
				debugText.Add($"Layer: {hoveringRoom.data.layer}");
				if (!hoveringRoom.data.merge)
					debugText.Add("No Merge");
				if (hoveringRoom.data.hidden)
					debugText.Add("Hidden");
			}
		}

		if (VisibleCreatures) {
			for (int r = region.rooms.Count - 1; r >= 0; r--) {
				Room room = region.rooms[r];
				Vector2 roomMouse = worldMouse - room.Position;
				Vector2 shortcutPosition;

				if (room is OffscreenRoom offscreenRoom) {
					for (int j = 0; j <= room.dens.Count; j++) {
						shortcutPosition = new Vector2(room.width * 0.5f - room.dens.Count * 2f + r * 4f + 2.5f, -room.height * 0.25f - 0.5f);
						if ((roomMouse - shortcutPosition).Length < SelectorScale) {
							DebugDen(offscreenRoom.GetDen(), offscreenRoom, ref debugText);
						}
					}
				}
				else {
					for (int j = 0; j < room.denShortcutEntrances.Count; j++) {
						Vector2i shortcut = room.denShortcutEntrances[j];
						shortcutPosition = new Vector2(shortcut.x + 0.5f, -1f - shortcut.y + 0.5f);
						if ((roomMouse - shortcutPosition).Length < SelectorScale) {
							DebugDen(room.GetDen01(j), room, ref debugText);
						}
					}
				}
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
		if (Keys.Modifier(Keymod.Alt)) {
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
		DrawEditor();

		Immediate.LoadIdentity();
		Immediate.Ortho(-1f * Main.screenBounds.x, 1f * Main.screenBounds.x, -1f * Main.screenBounds.y, 1f * Main.screenBounds.y, 0f, 1f);

		DrawDebugData();

		menuItems.Draw();
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

	private static Room? CopyRoom(string fromFilePath, string toFilePath) {
		if (File.Exists(toFilePath)) {
			return null;
		}

		FileInfo fromFile = new FileInfo(fromFilePath);
		FileInfo toFile = new FileInfo(toFilePath);
		string fromRoom = Path.GetFileNameWithoutExtension(fromFile.Name);
		string toRoom = Path.GetFileNameWithoutExtension(toFile.Name);

		File.Copy(fromFilePath, toFilePath);
		bool initial = Settings.WarnMissingImages;
		Settings.WarnMissingImages.value = false;
		Room room = new Room(fromFilePath, toRoom) {
			CanonPosition = WorldWindow.cameraOffset,
			DevPosition = WorldWindow.cameraOffset
		};
		Settings.WarnMissingImages.value = initial;

		RoomAndConnectionChange change = new RoomAndConnectionChange(true);
		change.AddRoom(room);
		History.Apply(change);

		for (int i = 0; i < room.data.cameras.Count; i++) {
			string imageSuffix = $"_{i + 1}.png";
			string imagePath = fromRoom + imageSuffix;

			string? sourceImage = PathUtil.FindFile(fromFile.DirectoryName!, imagePath);

			if (sourceImage != null) {
				string destImage = Path.Combine(toFile.DirectoryName!, toRoom + imageSuffix);
				File.Copy(sourceImage, destImage);
			}
		}

		return room;
	}

	private static void CreateAndAddRoom(string path, string name, string tag = "") {
		RoomAndConnectionChange change = new RoomAndConnectionChange(true);
		Room room = new Room(path, name);
		if (tag.Length > 0)
			room.data.tags = [tag];
		room.CanonPosition = room.DevPosition = WorldWindow.cameraOffset;
		change.AddRoom(room);
		History.Apply(change);
	}

	private static void HandleRoomFilesSelected(string[] paths) {
		if (paths.Length == 0)
			return;

		foreach (string path in paths) {
			if (!path.EndsWith(".txt")) {
				PopupManager.Add(new InfoPopup("File must be .txt: " + path));
				return;
			}

			string filename = Path.GetFileNameWithoutExtension(path);
			string acronym = Path.GetFileNameWithoutExtension(PathUtil.Parent(path));
			acronym = acronym[0..acronym.IndexOfReverse('-')];

			if (acronym.ToLowerInvariant() == "gates") {
				HandleGateFile(path, filename);
			}
			else {
				HandleStandardFile(path, filename, acronym);
			}
		}
	}

	private static void HandleGateFile(string path, string filename) {
		string[] names = filename.Split('_');
		string regAcro = WorldWindow.region.acronym.ToLowerInvariant();

		if (names[1].ToLowerInvariant() == regAcro || names[2].ToLowerInvariant() == regAcro) {
			CreateAndAddRoom(path, filename, "GATE");
		}
		else {
			PopupManager.Add(
				new ConfirmPopup("Change which acronym?")
					.SetOkay(names[2])
					.Okay(() => {
						string newName = $"gate_{names[1]}_{WorldWindow.region.acronym}.txt";
						CopyRoom(path, PathUtil.Combine(path, $"../{newName}"))?.data.tags = ["GATE"];
					})
					.SetCancel(names[1])
					.Cancel(() => {
						string newName = $"gate_{WorldWindow.region.acronym}_{names[2]}.txt";
						CopyRoom(path, PathUtil.Combine(path, $"../{newName}"))?.data.tags = ["GATE"];
					})
			);
		}
	}

	private static void HandleStandardFile(string path, string filename, string acronym) {
		if (acronym.Equals(WorldWindow.region.acronym, StringComparison.InvariantCultureIgnoreCase) || WorldWindow.region.exportPath.IsNullOrEmpty()) {
			CreateAndAddRoom(path, filename);
		}
		else {
			PopupManager.Add(
				new ConfirmPopup($"Copy room to {WorldWindow.region.acronym}-rooms?")
					.SetCancel("Just Add")
					.Cancel(() => {
						CreateAndAddRoom(path, filename);
					})
					.SetOkay("Yes")
					.Okay(() => {
						string filename = Path.GetFileName(path);
						string newName = $"{WorldWindow.region.acronym}{filename[filename.IndexOf('_')..]}.txt";
						CopyRoom(path, PathUtil.Combine(path, $"../{newName}"))?.data.tags = ["GATE"];
					})
			);
		}
	}

	public class WorldMenuItems : MenuItems {
		public WorldMenuItems() {
			this.buttons = [
				new Button("New", button => {
					PopupManager.Add(new AcronymPopup());
				}),

				new Button("Add Room", button => {
					if (WorldWindow.region.acronym.IsNullOrEmpty() || WorldWindow.region.exportPath.IsNullOrEmpty()) {
						PopupManager.Add(new InfoPopup("You must create or import a region\nbefore adding rooms."));
						return;
					}

					PopupManager.Add(
						new FilesystemPopup(HandleRoomFilesSelected, 1)
							.Filter(new Regex("((?!.*_settings)(?=.+_.+).+\\.txt)|(gate_([^._-]+)_([^._-]+)\\.txt)"))
							.Multiple()
							.Hint("xx_a01.txt")
					);
				}),

				new Button("Import", button => {
					PopupManager.Add(new FilesystemPopup(selection => {
						if (selection.Length == 0) return;

						WorldParser.ImportWorldFile(selection[0]);
					}, 0).Filter(Regexs.WorldFileRegex()).Hint("world_xx.txt"));
				}),

				new Button("Export", button => {
					string lastExportDirectory = WorldWindow.region.exportPath;

					if (!Settings.UpdateWorldFiles) {
						WorldWindow.region.exportPath = PathUtil.FindOrAssumeDirectory("worlds", WorldWindow.region.acronym);
						Logger.Info($"Special exporting to directory: {WorldWindow.region.exportPath}");

						if (!Directory.Exists(WorldWindow.region.exportPath)) {
							Directory.CreateDirectory(WorldWindow.region.exportPath);
						}
					}

					if (!string.IsNullOrEmpty(WorldWindow.region.exportPath)) {
						WorldExporter.ExportMapFile();
						WorldExporter.ExportWorldFile();

						string imagePath1 = PathUtil.FindOrAssumeFile(WorldWindow.region.exportPath, $"map_{WorldWindow.region.acronym}.png");
						WorldExporter.ExportImageFile(imagePath1);

						WorldExporter.ExportPropertiesFile(PathUtil.FindOrAssumeFile(WorldWindow.region.exportPath, "properties.txt"));
						PopupManager.Add(new InfoPopup("Exported successfully!"));
						if (Main.AprilFools) Sfx.Play("assets/objects/yay.wav");
					}
					else {
						if (string.IsNullOrEmpty(WorldWindow.region.acronym)) {
							PopupManager.Add(new InfoPopup("You must create or import a region\nbefore exporting."));
							return;
						}

						PopupManager.Add(
							new FilesystemPopup((pathStrings) => {
								if (pathStrings == null || pathStrings.Length == 0) return;

								string selectedPath = pathStrings[0];
								WorldWindow.region.exportPath = PathUtil.FindOrAssumeDirectory(selectedPath, WorldWindow.region.acronym);
								WorldWindow.region.roomsPath = PathUtil.FindOrAssumeDirectory(selectedPath, $"{WorldWindow.region.acronym}-rooms");

								Directory.CreateDirectory(WorldWindow.region.exportPath);
								Directory.CreateDirectory(WorldWindow.region.roomsPath);

								WorldExporter.ExportMapFile();
								WorldExporter.ExportWorldFile();

								string imagePath1 = PathUtil.FindOrAssumeFile(WorldWindow.region.exportPath, $"map_{WorldWindow.region.acronym}.png");
								WorldExporter.ExportImageFile(imagePath1);

								WorldExporter.ExportPropertiesFile(PathUtil.FindOrAssumeFile(WorldWindow.region.exportPath, "properties.txt"));
								PopupManager.Add(new InfoPopup("Exported successfully!"));
								if (Main.AprilFools) Sfx.Play("assets/objects/yay.wav");
							}, 0)
							.Filter(FilesystemPopup.SelectionType.Folder)
							.Hint("YOUR_MOD/world/")
						);
					}

					WorldWindow.region.exportPath = lastExportDirectory;
				}),

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
				}),

				new LayerButton(0),
				new LayerButton(1),
				new LayerButton(2),

				new Button("Dev Items: Hidden", button => {
					VisibleDevItems = !VisibleDevItems;
					button.text = VisibleDevItems ? "Dev Items: Shown" : "Dev Items: Hidden";
				}),

				new Button("Creatures: Shown", button => {
					VisibleCreatures = !VisibleCreatures;
					button.text = VisibleCreatures ? "Creatures: Shown" : "Creatures: Hidden";
				}),

				new Button("Refresh Region", button => {
					if (WorldWindow.region.acronym.IsNullOrEmpty() || WorldWindow.region.exportPath.IsNullOrEmpty()) {
						PopupManager.Add(new InfoPopup("You must create or import a region\nbefore refreshing"));
						return;
					}

					string? path = PathUtil.FindFile(WorldWindow.region.exportPath, $"world_{WorldWindow.region.acronym}.txt");
					if (path == null) {
						PopupManager.Add(new InfoPopup("Could not find world_xx.txt file!"));
						return;
					}

					WorldParser.ImportWorldFile(path);
				}),

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
				}),
			];
		}

		private class LayerButton : Button {
			private readonly int layer;

			public override bool Dark => !VisibleLayers[this.layer];

			public LayerButton(int layer) : base((layer + 1).ToString(), b => { ((LayerButton) b).Click(); }) {
				this.layer = layer;
			}

			private void Click() {
				if (!Keys.Modifier(Keymod.Shift)) {
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