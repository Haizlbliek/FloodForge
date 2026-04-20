using System.Diagnostics;
using System.Text;
using FloodForge.Popups;
using FloodForge.World;
using Silk.NET.GLFW;
using Silk.NET.Input;
using StbImageWriteSharp;
using Stride.Core.Extensions;
using static FloodForge.Main;

namespace FloodForge.Droplet;

public static class DropletWindow {
	private static readonly DropletMenuItems menuItems = new DropletMenuItems();

	private static Texture GeometryTexture = null!;
	private static bool showObjects;
	private static bool showMousePosition = false;
	public static bool showResize;
	public static Vector2i resizeSize;
	public static Vector2i resizeOffset;
	public static Room Room { get; private set; } = null!;

	private enum EditorTab {
		Details,
		Geometry,
		Camera,
	}
	private static readonly string[] TabNames = [ "Environment", "Geometry", "Cameras" ];

	private enum GeometryTool {
		Wall,
		Slope,
		Platform,
		BackgroundWall,
		HorizontalPole,
		VerticalPole,
		Spear,
		Rock,
		Shortcut,
		RoomExit,
		CreatureDen,
		WackAMoleHole,
		ScavengerDen,
		GarbageWorm,
		Wormgrass,
		BatflyHive,
	}
	private static readonly string[] GeometryToolNames = [ "Wall", "Slope", "Platform", "Background Wall", "Horizontal Pole", "Vertical Pole", "Spear", "Rock", "Shortcut", "Room Exit", "Creature Den", "Wack a Mole Hole", "Scavenger Den", "Garbage Worm", "Wormgrass", "Batfly Hive" ];

	private static Vector2 transformedMouse;
	private static Rect roomRect;
	private static Vector2i mouseTile;
	private static Vector2i lastMouseTile;
	private static bool lastMouseDrawing;
	private static bool blockMouse;

	private static EditorTab currentTab;
	private static GeometryTool selectedTool = GeometryTool.Wall;

	private static RoomData.Camera? selectedCamera;

	private static uint[]? backupGeometry = null;
	private static int backupWaterHeight;
	private static bool backupWaterInFront;
	private static bool backupEnclosedRoom;
	private static int backupWidth;
	private static int backupHeight;
	private static RoomData.Camera[] backupCameras = [];
	private static List<DevObject> backupObjects = [];

	private static Vector2 cameraOffset;
	private static float cameraScale = 40f;
	private static float targetCameraScale = 40f;
	private static bool cameraPanning = false;
	private static bool cameraPanningBlocked = false;
	private static Vector2 cameraPanStartMouse;
	private static Vector2 cameraPanStart;
	private static Vector2 targetCameraPan;

	private static string hoverText = "";

	private static int trashCanState = 0;

	private static Rect trashCanRect;

	public static void Initialize() {
		GeometryTexture = Texture.Load(Themes.GetPath("geometry.png"));
	}

	private static void UpdateCamera() {
		bool isHoveringPopup = Mouse.Disabled;
		float scrollY = -Mouse.Scroll;
		float zoom = MathF.Pow(1.25f, scrollY);

		Vector2 previousWorldMouse = Mouse.Pos * cameraScale + cameraOffset;
		targetCameraScale *= zoom;
		targetCameraScale = Mathf.Clamp(targetCameraScale, 2.5f, 1f * MathF.Max(Room.width, Room.height));
		cameraScale += (targetCameraScale - cameraScale) * (1f - MathF.Pow(1f - Settings.CameraZoomSpeed, Program.Delta * 60f));
		Vector2 worldMouse = Mouse.Pos * cameraScale + cameraOffset;

		cameraOffset += previousWorldMouse - worldMouse;
		targetCameraPan += previousWorldMouse - worldMouse;

		if (Mouse.Middle) {
			if (!cameraPanningBlocked && !cameraPanning) {
				if (isHoveringPopup) cameraPanningBlocked = true;
				else {
					cameraPanStart = cameraOffset;
					cameraPanStartMouse = Main.GlobalMouse;
					cameraPanning = true;
				}
			}

			if (cameraPanning && !cameraPanningBlocked) {
				targetCameraPan = cameraPanStart + cameraScale * (cameraPanStartMouse - Main.GlobalMouse);
			}
		}
		else {
			cameraPanning = false;
			cameraPanningBlocked = false;
		}

		targetCameraPan.x = Mathf.Clamp(targetCameraPan.x, -(Main.screenBounds.x - 0.41f) * cameraScale, Main.screenBounds.x * cameraScale + Room.width);
		targetCameraPan.y = Mathf.Clamp(targetCameraPan.y, -(Main.screenBounds.y - 0.12f) * cameraScale - Room.height, Main.screenBounds.y * cameraScale);

		cameraOffset += (targetCameraPan - cameraOffset) * (1f - MathF.Pow(1f - Settings.CameraPanSpeed, Program.Delta * 60f));
	}

	private static Node? movingNode = null;

	private static void UpdateDetailsTab() {
		if (Room.visuals.hasTerrain && Room.visuals.terrain.Count >= 2) {
			Immediate.Color(0f, 1f, 0f);
			Immediate.Begin(Immediate.PrimitiveType.LINE_STRIP);
			foreach (Vector2 point in Room.visuals.terrain) {
				Immediate.Vertex(roomRect.x0 + point.x / 20f, roomRect.y0 + point.y / 20f);
			}
			Immediate.End();
		}

		Node? hoveringNode = null;
		Vector2 nodeMouse = new Vector2(transformedMouse.x, transformedMouse.y + (roomRect.y1 - roomRect.y0)) * 20f;
		float mouseDistance = 0.3f * cameraScale;
		foreach (DevObject devObject in Room.data.objects) {
			if (!devObject.ShowInDroplet) continue;

			devObject.Draw(new Vector2(roomRect.x0, roomRect.y0));

			foreach (Node node in devObject.nodes) {
				Vector2 nodePos = node.GlobalPosition;

				if ((nodePos - nodeMouse).Length < mouseDistance) {
					Immediate.Color(Color.White);
					hoveringNode = node;
				}
				else {
					Immediate.Color(Color.Grey);
				}

				Vector2 nodeRenderPos = new Vector2(roomRect.x0 + nodePos.x / 20f, roomRect.y0 + nodePos.y / 20f);
				UI.FillCircle(nodeRenderPos.x, nodeRenderPos.y, 0.01f * cameraScale, 8);
				Immediate.Color(Color.Black);
				UI.StrokeCircle(nodeRenderPos.x, nodeRenderPos.y, 0.01f * cameraScale, 8);
			}
		}

		if (Mouse.JustLeft && hoveringNode != null) {
			movingNode = hoveringNode;
		}

		if (movingNode != null) {
			bool needsDeleted = false;

			if (movingNode.parent == null) {
				trashCanState = 1;
				movingNode.position = nodeMouse;

				if (trashCanRect.Inside(Mouse.Pos)) {
					trashCanState = 2;

					if (!Mouse.Left) {
						needsDeleted = true;
					}
				}
			}
			else {
				trashCanState = 0;
				movingNode.position = nodeMouse - movingNode.parent.position;
			}

			if (movingNode.devObject is TerrainHandleObject) {
				Room.visuals.terrainNeedsRefresh = true;
			}
			if (movingNode.devObject is AirPocketObject) {
				Room.visuals.waterNeedsRefresh = true;
			}

			if (!Mouse.Left) {
				if (needsDeleted) {
					Room.data.objects.Remove(movingNode.devObject);
				}

				hoveringNode = null;
				movingNode = null;
			}
		}
		else {
			trashCanState = 0;

			if (!blockMouse && Keys.Pressed(Key.W) && Room.data.waterHeight != -1) {
				Room.data.waterHeight = Room.height - mouseTile.y - 1;
				if (Room.data.waterHeight < 0) Room.data.waterHeight = 0;
				Room.visuals.waterNeedsRefresh = true;
			}
		}

		Room.visuals.Refresh();
	}

	private static void UpdateNotDetailsTab() {
		if (!showObjects) return;

		if (Room.visuals.hasTerrain && Room.visuals.terrain.Count >= 2) {
			Immediate.Color(0f, 1f, 0f);
			Immediate.Begin(Immediate.PrimitiveType.LINE_STRIP);
			foreach (Vector2 point in Room.visuals.terrain) {
				Immediate.Vertex(roomRect.x0 + point.x / 20f, roomRect.y0 + point.y / 20f);
			}
			Immediate.End();
		}

		foreach (DevObject devObject in Room.data.objects) {
			if (!devObject.ShowInDroplet) continue;

			devObject.Draw(new Vector2(roomRect.x0, roomRect.y0));
		}
	}

	private static bool ShortcutAir(uint v) {
		uint w = v % 16;
		return w == 0 || w == 2 || w == 3;
	}

	private static void VerifyShortcut(int x, int y) {
		if ((Room.GetTile(x, y) & 128) == 0) return;

		bool shortcutEntrance = false;
		if (
			(Room.GetTile(x - 1, y - 1) % 16) == 1 && (Room.GetTile(x + 1, y - 1) % 16) == 1 &&
			(Room.GetTile(x - 1, y + 1) % 16) == 1 && (Room.GetTile(x + 1, y + 1) % 16) == 1
		) {
			int dir = 0;
			dir += ShortcutAir(Room.GetTile(x - 1, y)) ? 1 : 0;
			dir += ShortcutAir(Room.GetTile(x + 1, y)) ? 2 : 0;
			dir += ShortcutAir(Room.GetTile(x, y - 1)) ? 4 : 0;
			dir += ShortcutAir(Room.GetTile(x, y + 1)) ? 8 : 0;
			dir += ((Room.GetTile(x - 1, y) & 128) > 0) ? 16 : 0;
			dir += ((Room.GetTile(x + 1, y) & 128) > 0) ? 32 : 0;
			dir += ((Room.GetTile(x, y - 1) & 128) > 0) ? 64 : 0;
			dir += ((Room.GetTile(x, y + 1) & 128) > 0) ? 128 : 0;

			if (dir == 1 + 32 || dir == 2 + 16 || dir == 4 + 128 || dir == 8 + 64) {
				shortcutEntrance = true;
			}
		}

		if (shortcutEntrance) {
			Room.geometry[x * Room.height + y] = (Room.geometry[x * Room.height + y] & 0xFFFFFFF0) | 4;
		}
		else {
			if ((Room.geometry[x * Room.height + y] % 16) == 4) {
				Room.geometry[x * Room.height + y] = (Room.geometry[x * Room.height + y] & 0xFFFFFFF0) | 1;
			}
		}
	}

	public static void SetFlag(int x, int y, uint mask, bool remove) {
		if (remove) Room.geometry[x * Room.height + y] &= ~mask;
		else Room.geometry[x * Room.height + y] |= mask;
	}

	public static void SetDenFlag(int x, int y, uint mask, bool remove) {
		int index = x * Room.height + y;
		bool hasFlag = (Room.geometry[index] & mask) != 0;

		if (remove) {
			if (!hasFlag) return;
			Room.geometry[index] &= ~(mask | 128);
		}
		else {
			Room.geometry[index] |= mask | 128;
			VerifyShortcut(x, y);
		}

		VerifyShortcut(x - 1, y);
		VerifyShortcut(x + 1, y);
		VerifyShortcut(x, y - 1);
		VerifyShortcut(x, y + 1);
	}

	private static void ApplyTool(int x, int y, bool right) {
		if (x < 0 || y < 0 || x >= Room.width || y >= Room.height) return;

		if (selectedTool == GeometryTool.Wall) {
			Room.geometry[x * Room.height + y] = right ? 0u : 1u;
		}
		else if (selectedTool == GeometryTool.Slope) {
			if (right) {
				Room.geometry[x * Room.height + y] = 0u;
			}
			else {
				int bits = 0;
				bits += ((Room.GetTile(x - 1, y) & 15) == 1) ? 1 : 0;
				bits += ((Room.GetTile(x + 1, y) & 15) == 1) ? 2 : 0;
				bits += ((Room.GetTile(x, y - 1) & 15) == 1) ? 4 : 0;
				bits += ((Room.GetTile(x, y + 1) & 15) == 1) ? 8 : 0;

				if ((Room.GetTile(x - 1, y) & 15) == 2 || (Room.GetTile(x + 1, y) & 15) == 2 ||
					(Room.GetTile(x, y - 1) & 15) == 2 || (Room.GetTile(x, y + 1) & 15) == 2) {
					bits = -1;
				}

				uint type = 4;
				if (bits == 1 + 4) type = 0;
				else if (bits == 1 + 8) type = 1;
				else if (bits == 2 + 4) type = 2;
				else if (bits == 2 + 8) type = 3;

				if (type != 4) {
					Room.geometry[x * Room.height + y] = 2 + 1024 * type;
				}
			}
		}
		else if (selectedTool == GeometryTool.Platform) Room.geometry[x * Room.height + y] = right ? 0u : 3u;
		else if (selectedTool == GeometryTool.VerticalPole) SetFlag(x, y, 16, right);
		else if (selectedTool == GeometryTool.HorizontalPole) SetFlag(x, y, 32, right);
		else if (selectedTool == GeometryTool.BackgroundWall) SetFlag(x, y, 512, right);
		else if (selectedTool == GeometryTool.RoomExit) SetDenFlag(x, y, 64, right);
		else if (selectedTool == GeometryTool.CreatureDen) SetDenFlag(x, y, 256, right);
		else if (selectedTool == GeometryTool.ScavengerDen) SetDenFlag(x, y, 4096, right);
		else if (selectedTool == GeometryTool.WackAMoleHole) SetDenFlag(x, y, 8192, right);
		else if (selectedTool == GeometryTool.GarbageWorm) SetFlag(x, y, 16384, right);
		else if (selectedTool == GeometryTool.Wormgrass) SetFlag(x, y, 32768, right);
		else if (selectedTool == GeometryTool.BatflyHive) SetFlag(x, y, 65536, right);
		else if (selectedTool == GeometryTool.Rock) SetFlag(x, y, 262144, right);
		else if (selectedTool == GeometryTool.Spear) SetFlag(x, y, 524288, right);
		else if (selectedTool == GeometryTool.Shortcut) {
			if (!right) {
				Room.geometry[x * Room.height + y] |= 128;
				VerifyShortcut(x, y);
				VerifyShortcut(x - 1, y);
				VerifyShortcut(x + 1, y);
				VerifyShortcut(x, y - 1);
				VerifyShortcut(x, y + 1);
			}
			else if ((Room.geometry[x * Room.height + y] & 128) > 0) {
				Room.geometry[x * Room.height + y] &= ~(64u | 128u | 256u | 4096u | 8192u);
				if ((Room.geometry[x * Room.height + y] % 16) == 4) {
					Room.geometry[x * Room.height + y] = (Room.geometry[x * Room.height + y] & ~15u) | 1;
				}

				VerifyShortcut(x - 1, y);
				VerifyShortcut(x + 1, y);
				VerifyShortcut(x, y - 1);
				VerifyShortcut(x, y + 1);
			}
		}
	}

	private static int rectDrawing = -1;
	private static Vector2i rectStart;
	private static void UpdateGeometryTab() {
		if (!(Mouse.Left || Mouse.Right)) {
			int tool = (int)selectedTool;

			if (Keys.JustPressed(Key.A)) {
				tool = ((tool + 3) % 4) + (tool & 0b1100);
			}

			if (Keys.JustPressed(Key.D)) {
				tool = ((tool + 1) % 4) + (tool & 0b1100);
			}

			if (Keys.JustPressed(Key.W)) {
				tool = (tool + 12) % 16;
			}

			if (Keys.JustPressed(Key.S)) {
				tool = (tool + 4) % 16;
			}

			selectedTool = (GeometryTool)tool;
		}

		if (showMousePosition) {
			Immediate.Color(Color.White);
			UI.font.Write($"x:{mouseTile.x} y:{mouseTile.y}", mouseTile.x, -mouseTile.y + 1, 0.6f);
		}

		if (!blockMouse) {
			if (rectDrawing != -1) {
				if ((rectDrawing == 0 && !Mouse.Left) || (rectDrawing == 1 && !Mouse.Right)) {
					for (int x = Math.Min(rectStart.x, mouseTile.x); x <= Math.Max(rectStart.x, mouseTile.x); x++) {
						for (int y = Math.Min(rectStart.y, mouseTile.y); y <= Math.Max(rectStart.y, mouseTile.y); y++) {
							ApplyTool(x, y, rectDrawing == 1);
						}
					}
					rectDrawing = -1;
				}
			}
			else if (Keys.Modifier(Keys.Modifiers.Shift) && (Mouse.Left || Mouse.Right)) {
				rectDrawing = Mouse.Left ? 0 : 1;
				rectStart = mouseTile;
			}
			else if (selectedTool == GeometryTool.Wall && Keys.Pressed(Key.Q) && (Mouse.Left || Mouse.Right) && Room.Inside(mouseTile)) {
				Stack<Vector2i> items = new Stack<Vector2i>();
				HashSet<Vector2i> visited = [];

				uint setTo = Mouse.Left ? 1u : 0u;
				uint geoType = Room.GetTile(mouseTile.x, mouseTile.y) % 16;

				if (geoType == 1 || geoType == 0) {
					bool solid = geoType == 1;
					items.Push(mouseTile);

					while (items.Count > 0) {
						Vector2i tile = items.Pop();

						if (!visited.Add(tile)) continue;
						if (!Room.Inside(tile)) continue;

						uint geo = Room.GetTile(tile);
						uint currentType = geo % 16;

						if (currentType != 1 && currentType != 0) continue;
						if (currentType == 1 != solid) continue;

						Room.geometry[tile.x * Room.height + tile.y] = (byte)((geo & ~15) | setTo);

						items.Push(new Vector2i(tile.x - 1, tile.y));
						items.Push(new Vector2i(tile.x + 1, tile.y));
						items.Push(new Vector2i(tile.x, tile.y - 1));
						items.Push(new Vector2i(tile.x, tile.y + 1));
					}
				}
			}
			else if ((Mouse.Left || Mouse.Right) && (lastMouseTile != mouseTile)) {
				List<Vector2i> drawLine = LevelUtils.Line(lastMouseTile.x, lastMouseTile.y, mouseTile.x, mouseTile.y);
				foreach (Vector2i point in drawLine) {
					if (Room.Inside(point)) {
						ApplyTool(point.x, point.y, Mouse.Right);
					}
				}
			}
			else if (Mouse.JustLeft || Mouse.JustRight) {
				if (Room.Inside(mouseTile)) {
					ApplyTool(mouseTile.x, mouseTile.y, Mouse.Right);
				}
			}

			Immediate.Color(Themes.RoomBorderHighlight);
			if (rectDrawing != -1) {
				int sx = Math.Min(rectStart.x, mouseTile.x);
				int sy = Math.Min(rectStart.y, mouseTile.y);
				int ex = Math.Max(rectStart.x, mouseTile.x);
				int ey = Math.Max(rectStart.y, mouseTile.y);
				UI.StrokeRect(new Rect(roomRect.x0 + sx, roomRect.y1 - sy, roomRect.x0 + ex + 1, roomRect.y1 - ey - 1));
			}
			else {
				UI.StrokeRect(Rect.FromSize(roomRect.x0 + mouseTile.x, roomRect.y1 - mouseTile.y - 1, 1f, 1f));
			}
		}

		lastMouseDrawing = Mouse.Left || Mouse.Right;
	}

	private static bool DrawCameraAngle(Vector2 pos, ref Vector2 angle, bool dragging) {
		bool hovered = !blockMouse && ((transformedMouse - pos).SqrLength < 16f || (transformedMouse - (pos + angle * 4f)).Length < 0.05f * cameraScale);
		Immediate.Color(0f, 1f, 0f, hovered ? 1f : 0.5f);
		UI.StrokeCircle(pos, 4f, 20);
		UI.Line(pos, pos + angle * 4f);
		UI.StrokeCircle(pos, angle.Length * 4f, 20);
		UI.FillCircle(pos + angle * 4f, 0.01f * cameraScale, 8);

		if (dragging) {
			angle = (transformedMouse - pos) / 4f;
			if (!Keys.Modifier(Keys.Modifiers.Shift)) {
				float len = angle.Length;
				if (len > 1f) angle /= len;
			}
			return true;
		}

		if (hovered && Mouse.JustLeft) {
			angle = (transformedMouse - pos) / 4f;
			return true;
		}

		if (hovered && Mouse.JustRight) {
			angle = Vector2.Zero;
		}

		return false;
	}

	private static readonly Vector2 cameraSizeTiles = new Vector2(70, 40);
	private static readonly Vector2 cameraSizeLarge = new Vector2(68.3f, 38.4f);
	private static readonly Vector2 cameraSizeSmall = new Vector2(51.2f, 38.4f);
	private static RoomData.Camera? draggingCamera = null;
	private static int draggingCameraAngle = -1;
	private static Vector2 dragStart;
	private static void UpdateCameraTab() {
		if (!Mouse.Left) {
			draggingCamera = null;
			draggingCameraAngle = -1;
		}

		Program.gl.Enable(EnableCap.Blend);
		int i = 1;
		bool newSelectedCamera = false;
		foreach (RoomData.Camera camera in Room.data.cameras) {
			bool selected = selectedCamera == camera;
			Vector2 center = camera.position / 20f + cameraSizeTiles * 0.5f;
			Immediate.Color(0f, 1f, 0f, selected ? 0.25f : 0.15f);
			Immediate.Begin(Immediate.PrimitiveType.QUADS);
			Immediate.Vertex(camera.position.x / 20f + camera.angles[0].x * 4f, -camera.position.y / 20f + camera.angles[0].y * 4f);
			Immediate.Vertex(camera.position.x / 20f + camera.angles[1].x * 4f + cameraSizeTiles.x, -camera.position.y / 20f + camera.angles[1].y * 4f);
			Immediate.Vertex(camera.position.x / 20f + camera.angles[2].x * 4f + cameraSizeTiles.x, -camera.position.y / 20f + camera.angles[2].y * 4f - cameraSizeTiles.y);
			Immediate.Vertex(camera.position.x / 20f + camera.angles[3].x * 4f, -camera.position.y / 20f + camera.angles[3].y * 4f - cameraSizeTiles.y);
			Immediate.End();

			Immediate.Color(0f, 0f, 0f);
			UI.StrokeRect(Rect.FromSize(center.x - cameraSizeLarge.x * 0.5f, -center.y - cameraSizeLarge.y * 0.5f, cameraSizeLarge.x, cameraSizeLarge.y));
			UI.Line(camera.position.x / 20f, -center.y, camera.position.x / 20f + cameraSizeTiles.x, -center.y);
			UI.Line(center.x, -camera.position.y / 20f, center.x, -camera.position.y / 20f - cameraSizeTiles.y);
			Immediate.Color(0.0f, 1.0f, 0.0f);
			UI.StrokeRect(Rect.FromSize(center.x - cameraSizeSmall.x * 0.5f, -center.y - cameraSizeSmall.y * 0.5f, cameraSizeSmall.x, cameraSizeSmall.y));
			Immediate.Color(1f, 1f, 1f);
			UI.font.Write(i.ToString(), center.x, -center.y, 0.0625f * cameraScale, Font.Align.MiddleCenter);
			i++;

			if (selected) {
				if (DrawCameraAngle(camera.position / 20f * Vector2.NegY, ref camera.angles[0], draggingCamera == camera && draggingCameraAngle == 0)) {
					newSelectedCamera = true; draggingCamera = camera; draggingCameraAngle = 0;
				}
				if (DrawCameraAngle(camera.position / 20f * Vector2.NegY + new Vector2(cameraSizeTiles.x, 0f), ref camera.angles[1], draggingCamera == camera && draggingCameraAngle == 1)) {
					newSelectedCamera = true; draggingCamera = camera; draggingCameraAngle = 1;
				}
				if (DrawCameraAngle(camera.position / 20f * Vector2.NegY + new Vector2(cameraSizeTiles.x, -cameraSizeTiles.y), ref camera.angles[2], draggingCamera == camera && draggingCameraAngle == 2)) {
					newSelectedCamera = true; draggingCamera = camera; draggingCameraAngle = 2;
				}
				if (DrawCameraAngle(camera.position / 20f * Vector2.NegY + new Vector2(0f, -cameraSizeTiles.y), ref camera.angles[3], draggingCamera == camera && draggingCameraAngle == 3)) {
					newSelectedCamera = true; draggingCamera = camera; draggingCameraAngle = 3;
				}
			}

			if (draggingCamera == camera && draggingCameraAngle == -1) {
				camera.position += (transformedMouse - dragStart) * Vector2.NegY * 20f;
				dragStart = transformedMouse;
			}

			if (!newSelectedCamera && !blockMouse && Mouse.JustLeft && Rect.FromSize(camera.position.x / 20f, -camera.position.y / 20f, cameraSizeTiles.x, -cameraSizeTiles.y).Inside(transformedMouse)) {
				newSelectedCamera = true;
				selectedCamera = camera;
				draggingCamera = camera;
				draggingCameraAngle = -1;
				dragStart = transformedMouse;
			}
		}
		Program.gl.Disable(EnableCap.Blend);

		if (Mouse.JustLeft && !newSelectedCamera) {
			selectedCamera = null;
		}

		if (Keys.JustPressed(Key.C)) {
			Room.data.cameras.Add(new RoomData.Camera() {
				position = (transformedMouse * Vector2.NegY - cameraSizeTiles * 0.5f) * 20f,
			});
			selectedCamera = Room.data.cameras[^1];
		}

		if (Keys.JustPressed(Key.X) && selectedCamera != null) {
			if (Room.data.cameras.Count == 1) {
				PopupManager.Add(new InfoPopup("Cannot delete last camera"));
			}
			else {
				Room.data.cameras.Remove(selectedCamera);
				selectedCamera = null;
			}
		}
	}

	private static void UpdateNotCameraTab() {
		if (!showObjects) return;

		Program.gl.Enable(EnableCap.Blend);
		foreach (RoomData.Camera camera in Room.data.cameras) {
			Vector2 center = camera.position / 20f + cameraSizeTiles * 0.5f;
			Immediate.Color(0.0f, 1.0f, 0.0f);
			UI.StrokeRect(Rect.FromSize(camera.position.x / 20f, -camera.position.y / 20f, cameraSizeTiles.x, -cameraSizeTiles.y));
			UI.StrokeRect(Rect.FromSize(center.x - cameraSizeSmall.x * 0.5f, -center.y - cameraSizeSmall.y * 0.5f, cameraSizeSmall.x, cameraSizeSmall.y));
		}
		Program.gl.Disable(EnableCap.Blend);
	}

	private static void DrawWater(bool border) {
		if (border) {
			Rect water = new Rect(roomRect.x0, roomRect.y0, roomRect.x1, roomRect.y0 + Room.data.waterHeight + 0.5f);
			Immediate.Color(0f, 0f, 0.5f, 1f);
			UI.Line(water.x0, water.y1, water.x1, water.y1);
			return;
		}

		Program.gl.Enable(EnableCap.Blend);
		Immediate.Color(0f, 0f, 0.5f, 0.5f);
		foreach (RoomVisuals.WaterSpot spot in Room.visuals.water) {
			Rect waterRect = Rect.FromSize(roomRect.x0 + spot.pos.x / 20f, roomRect.y0 + spot.pos.y / 20f, spot.size.x / 20f, spot.size.y / 20f);
			UI.FillRect(waterRect);
		}
		Program.gl.Disable(EnableCap.Blend);
	}

	private static void SetToolUv(ref UVRect rect, int tool) {
		if (tool == 0) {
			rect.UV(0.5f, 0.125f, 0.625f, 0.0f);
		}
		else if (tool == 1) {
			rect.UV(0.625f, 0.125f, 0.75f, 0.0f);
		}
		else if (tool == 2) {
			rect.UV(0.75f, 0.125f, 0.875f, 0.0f);
		}
		else if (tool == 3) {
			rect.UV(0.875f, 0.125f, 1.0f, 0.0f);

		}
		else if (tool == 4) {
			rect.UV(0.5f, 0.25f, 0.625f, 0.125f);
		}
		else if (tool == 5) {
			rect.UV(0.625f, 0.25f, 0.75f, 0.125f);
		}
		else if (tool == 6) {
			rect.UV(0.0f, 0.375f, 0.125f, 0.25f);
		}
		else if (tool == 7) {
			rect.UV(0.125f, 0.375f, 0.25f, 0.25f);

		}
		else if (tool == 8) {
			rect.UV(0.75f, 0.25f, 0.875f, 0.125f);
		}
		else if (tool == 9) {
			rect.UV(0.0f, 0.5f, 0.125f, 0.375f);
		}
		else if (tool == 10) {
			rect.UV(0.125f, 0.5f, 0.25f, 0.375f);
		}
		else if (tool == 11) {
			rect.UV(0.375f, 0.5f, 0.5f, 0.375f);

		}
		else if (tool == 12) {
			rect.UV(0.25f, 0.5f, 0.375f, 0.375f);
		}
		else if (tool == 13) {
			rect.UV(0.25f, 0.25f, 0.375f, 0.125f);
		}
		else if (tool == 14) {
			rect.UV(0.25f, 0.375f, 0.375f, 0.25f);
		}
		else if (tool == 15) {
			rect.UV(0.375f, 0.375f, 0.5f, 0.25f);
		}
	}

	private static void DrawGrid() {
		float gridStep = MathF.Max(cameraScale / 32f, 1f);
		gridStep = MathF.Pow(2, MathF.Ceiling(MathF.Log2(gridStep - 0.01f)));

		Immediate.Begin(Immediate.PrimitiveType.LINES);
		for (float x = roomRect.x0; x < roomRect.x1; x += gridStep) {
			Immediate.Vertex(x, roomRect.y0);
			Immediate.Vertex(x, roomRect.y1);
		}
		for (float y = roomRect.y0; y < roomRect.y1; y += gridStep) {
			Immediate.Vertex(roomRect.x0, y);
			Immediate.Vertex(roomRect.x1, y);
		}
		Immediate.End();
	}

	public static void Draw() {
		if (Keys.Modifier(Keys.Modifiers.Alt)) {
			if (Keys.JustPressed(Key.T)) {
				PopupManager.Add(new MarkdownPopup("docs/TutorialDroplet.md"));
				return;
			}
		}

		hoverText = "";

		if (!Mouse.Left && !Mouse.Right) {
			if (Keys.JustPressed(Key.Number1)) currentTab = EditorTab.Details;
			if (Keys.JustPressed(Key.Number2)) currentTab = EditorTab.Geometry;
			if (Keys.JustPressed(Key.Number3)) currentTab = EditorTab.Camera;
		}

		UpdateCamera();

		Immediate.LoadIdentity();
		Immediate.Ortho(cameraOffset.x, cameraOffset.y, cameraScale * Main.screenBounds.x, cameraScale * Main.screenBounds.y);

		trashCanRect = Rect.FromSize(-Main.screenBounds.x + 0.01f, -Main.screenBounds.y + 0.01f, 0.1f, 0.1f);

		{
			Immediate.Color(Themes.Grid);
			float gridStep = MathF.Max(cameraScale / 16f, 1f);
			gridStep = MathF.Pow(2f, MathF.Ceiling(MathF.Log2(gridStep - 0.01f)));
			Vector2 offset = (cameraOffset / gridStep).Rounded() * gridStep;
			Vector2 extraOffset = new Vector2((Main.screenBounds.x - 1f) * gridStep * 16f % gridStep, 0f);
			Vector2 gridScale = gridStep * 16f * Main.screenBounds;

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

		roomRect = Rect.FromSize(0f, 0f, Room.width, -Room.height);
		Immediate.Color(Themes.RoomAir);
		UI.FillRect(roomRect);

		Immediate.Color(Color.Lerp(Themes.RoomAir, Themes.RoomSolid, 0.25f));

		if (Settings.DropletGridVisibility.value == Settings.STDropletGridVisibility.Air) {
			DrawGrid();
		}

		Immediate.Begin(Immediate.PrimitiveType.QUADS);
		for (int x = 0; x < Room.width; x++) {
			for (int y = 0; y < Room.height; y++) {
				float x0 = roomRect.x0 + x;
				float y0 = roomRect.y1 - y;
				float x1 = x0 + 1;
				float y1 = y0 - 1;

				if ((Room.GetTile(x, y) & 512) > 0) {
					Immediate.Vertex(x0, y0);
					Immediate.Vertex(x1, y0);
					Immediate.Vertex(x1, y1);
					Immediate.Vertex(x0, y1);
				}
			}
		}
		Immediate.End();

		if (!Room.data.waterInFront && Room.data.waterHeight != -1) {
			DrawWater(false);
		}

		Immediate.Begin(Immediate.PrimitiveType.QUADS);
		for (int x = 0; x < Room.width; x++) {
			for (int y = 0; y < Room.height; y++) {
				float x0 = roomRect.x0 + x;
				float y0 = roomRect.y1 - y;
				float x1 = x0 + 1;
				float y1 = y0 - 1;
				uint geo = Room.GetTile(x, y);

				// Solid Blocks
				if ((geo % 16) == 1) {
					Immediate.Color(Themes.RoomSolid);
					Immediate.Vertex(x0, y0);
					Immediate.Vertex(x1, y0);
					Immediate.Vertex(x1, y1);
					Immediate.Vertex(x0, y1);
				}
				// Platforms
				else if ((geo % 16) == 3) {
					Immediate.Color(Themes.RoomPlatform);
					Immediate.Vertex(x0, y0);
					Immediate.Vertex(x1, y0);
					Immediate.Vertex(x1, y0 - 0.5f);
					Immediate.Vertex(x0, y0 - 0.5f);
				}
				// Slopes
				else if ((geo % 16) == 2) {
					uint type = (Room.GetTile(x, y) & (1024 + 2048)) / 1024;
					Immediate.Color(Themes.RoomSolid);

					if (type == 0) {
						Immediate.Vertex(x0, y1);
						Immediate.Vertex(x1, y0);
						Immediate.Vertex(x0, y0);
						Immediate.Vertex(x0, y1);
					}
					else if (type == 1) {
						Immediate.Vertex(x0, y0);
						Immediate.Vertex(x1, y1);
						Immediate.Vertex(x0, y1);
						Immediate.Vertex(x0, y0);
					}
					else if (type == 2) {
						Immediate.Vertex(x0, y0);
						Immediate.Vertex(x1, y0);
						Immediate.Vertex(x1, y1);
						Immediate.Vertex(x0, y0);
					}
					else if (type == 3) {
						Immediate.Vertex(x0, y1);
						Immediate.Vertex(x1, y0);
						Immediate.Vertex(x1, y1);
						Immediate.Vertex(x0, y1);
					}
				}
				// Shortcut Entrances
				else if ((geo % 16) == 4) {
					Immediate.Color(Themes.RoomShortcutEntrance);
					Immediate.Vertex(x0, y0);
					Immediate.Vertex(x1, y0);
					Immediate.Vertex(x1, y1);
					Immediate.Vertex(x0, y1);
				}

				// Vertical Poles
				if ((geo & 16) > 0) {
					Immediate.Color(Themes.RoomPole);
					Immediate.Vertex(x0 + 0.4f, y0);
					Immediate.Vertex(x1 - 0.4f, y0);
					Immediate.Vertex(x1 - 0.4f, y1);
					Immediate.Vertex(x0 + 0.4f, y1);
				}
				// Horizontal Poles
				if ((geo & 32) > 0) {
					Immediate.Color(Themes.RoomPole);
					Immediate.Vertex(x0, y0 - 0.4f);
					Immediate.Vertex(x1, y0 - 0.4f);
					Immediate.Vertex(x1, y1 + 0.4f);
					Immediate.Vertex(x0, y1 + 0.4f);
				}
			}
		}
		Immediate.End();

		Program.gl.Enable(EnableCap.Blend);
		Immediate.UseTexture(GeometryTexture);

		for (int x = 0; x < Room.width; x++) {
			for (int y = 0; y < Room.height; y++) {
				float x0 = roomRect.x0 + x;
				float y0 = roomRect.y1 - y - 1;
				float x1 = x0 + 1;
				float y1 = y0 + 1;
				uint geo = Room.GetTile(x, y);

				if ((geo & 15) == 4) {
					Immediate.Color(Themes.RoomShortcutRoom);
					if ((Room.GetTile(x, y + 1) & 128) > 0) {
						UI.FillRect(new UVRect(x0, y0, x1, y1).UV(0f, 0f, 0.125f, 0.125f));
					}
					else if ((Room.GetTile(x - 1, y) & 128) > 0) {
						UI.FillRect(new UVRect(x0, y0, x1, y1).UV(0.125f, 0f, 0.25f, 0.125f));
					}
					else if ((Room.GetTile(x + 1, y) & 128) > 0) {
						UI.FillRect(new UVRect(x0, y0, x1, y1).UV(0.25f, 0f, 0.375f, 0.125f));
					}
					else if ((Room.GetTile(x, y - 1) & 128) > 0) {
						UI.FillRect(new UVRect(x0, y0, x1, y1).UV(0.375f, 0f, 0.5f, 0.125f));
					}
					else {
						UI.FillRect(new UVRect(x0, y0, x1, y1).UV(0.125f, 0.125f, 0.25f, 0.25f));
					}
				}
				else if ((geo & 64) > 0) {
					Immediate.Color(Themes.RoomShortcutRoom);
					UI.FillRect(new UVRect(x0, y0, x1, y1).UV(0f, 0.375f, 0.125f, 0.5f));
				}
				else if ((geo & 128) > 0) {
					Immediate.Color(Themes.RoomShortcutDot);
					UI.FillRect(new UVRect(x0, y0, x1, y1).UV(0f, 0.125f, 0.125f, 0.25f));
				}

				if ((geo & 256) > 0) {
					Immediate.Color(Themes.RoomShortcutDen);
					UI.FillRect(new UVRect(x0, y0, x1, y1).UV(0.125f, 0.375f, 0.25f, 0.5f));
				}
				if ((geo & 4096) > 0) {
					Immediate.Color(Themes.RoomShortcutDen);
					UI.FillRect(new UVRect(x0, y0, x1, y1).UV(0.25f, 0.375f, 0.375f, 0.5f));
				}
				if ((geo & 8192) > 0) {
					Immediate.Color(Themes.RoomShortcutDen);
					UI.FillRect(new UVRect(x0, y0, x1, y1).UV(0.375f, 0.375f, 0.5f, 0.5f));
				}
				if ((geo & 16384) > 0) {
					Immediate.Color(Themes.RoomShortcutDot);
					UI.FillRect(new UVRect(x0, y0, x1, y1).UV(0.25f, 0.125f, 0.375f, 0.25f));
				}
				if ((geo & 32768) > 0) {
					Immediate.Color(Themes.RoomShortcutDot);
					UI.FillRect(new UVRect(x0, y0, x1, y1).UV(0.25f, 0.25f, 0.375f, 0.375f));
				}
				if ((geo & 65536) > 0) {
					Immediate.Color(Themes.RoomShortcutDot);
					UI.FillRect(new UVRect(x0, y0, x1, y1).UV(0.375f, 0.25f, 0.5f, 0.375f));
				}
				if ((geo & 262144) > 0) {
					Immediate.Color(Themes.RoomShortcutDot);
					UI.FillRect(new UVRect(x0, y0, x1, y1).UV(0.125f, 0.25f, 0.25f, 0.375f));
				}
				if ((geo & 524288) > 0) {
					Immediate.Color(Themes.RoomShortcutDot);
					UI.FillRect(new UVRect(x0, y0, x1, y1).UV(0f, 0.25f, 0.125f, 0.375f));
				}
			}
		}
		Immediate.UseTexture(null);

		if (Room.data.waterHeight != -1) {
			if (Room.data.waterInFront) {
				DrawWater(false);
			}

			DrawWater(true);
		}

		Program.gl.Disable(EnableCap.Blend);

		if (Settings.DropletGridVisibility.value == Settings.STDropletGridVisibility.All) {
			Immediate.Color(Color.Lerp(Themes.RoomAir, Themes.RoomSolid, 0.75f));
			DrawGrid();
		}

		Immediate.Color(Themes.RoomBorder);
		UI.StrokeRect(roomRect);
		if (showResize) {
			UI.StrokeRect(Rect.FromSize(resizeOffset.x, -resizeOffset.y, resizeSize.x, -resizeSize.y));
		}

		transformedMouse = Mouse.Pos * cameraScale + cameraOffset;
		lastMouseTile = mouseTile;
		mouseTile = new Vector2i(
			Mathf.FloorToInt(Mouse.X * cameraScale + cameraOffset.x),
			-Mathf.CeilToInt(Mouse.Y * cameraScale + cameraOffset.y)
		);

		blockMouse = Mouse.Y >= Main.screenBounds.y - 0.12f || Mouse.X >= Main.screenBounds.x - 0.41f || Mouse.Disabled;

		if (currentTab == EditorTab.Details) {
			UpdateDetailsTab();
		}
		else {
			UpdateNotDetailsTab();
		}
		if (currentTab == EditorTab.Geometry) {
			UpdateGeometryTab();
		}
		if (currentTab == EditorTab.Camera) {
			UpdateCameraTab();
		}
		else {
			UpdateNotCameraTab();
		}

		Immediate.LoadIdentity();
		Immediate.Ortho(-1f * Main.screenBounds.x, 1f * Main.screenBounds.x, -1f * Main.screenBounds.y, 1f * Main.screenBounds.y, 0f, 1f);

		Rect sidebar = new Rect(Main.screenBounds.x - 0.41f, Main.screenBounds.y - 0.12f, Main.screenBounds.x, -Main.screenBounds.y);
		Immediate.Color(Themes.Popup);
		UI.FillRect(sidebar);
		Immediate.Color(Themes.Border);
		UI.Line(sidebar.x0, sidebar.y0, sidebar.x0, sidebar.y1);

		if (currentTab == EditorTab.Geometry) {
			for (int y = 0; y < 4; y++) {
				for (int x = 0; x < 4; x++) {
					int i = x + y * 4;
					UVRect toolRect = UVRect.FromSize(sidebar.x0 + 0.01f + x * 0.1f, sidebar.y1 - (y + 1) * 0.1f, 0.09f, 0.09f);
					SetToolUv(ref toolRect, i);

					bool selected = selectedTool == (GeometryTool)i;
					UI.ButtonResponse response = UI.TextureButton(toolRect, new UI.TextureButtonMods() {
						texture = GeometryTexture,
						textureScale = new Vector2(0.75f, 0.75f),
						selected = selected,
						textureColor = selected ? Color.White : Color.Grey
					});

					if (response.clicked) {
						selectedTool = (GeometryTool)i;
					}
					if (response.hovered) {
						hoverText = GeometryToolNames[i];
					}
				}
			}
		}
		else if (currentTab == EditorTab.Details) {
			bool hasWater = Room.data.waterHeight != -1;

			UI.CheckBox(Rect.FromSize(sidebar.x0 + 0.01f, sidebar.y1 - 0.06f, 0.05f, 0.05f), ref Room.data.enclosedRoom);
			UI.CheckBox(Rect.FromSize(sidebar.x0 + 0.01f, sidebar.y1 - 0.12f, 0.05f, 0.05f), ref hasWater);
			UI.CheckBox(Rect.FromSize(sidebar.x0 + 0.01f, sidebar.y1 - 0.18f, 0.05f, 0.05f), ref Room.data.waterInFront);

			if (!hasWater) {
				Room.data.waterHeight = -1;
			}
			else if (Room.data.waterHeight == -1) {
				Room.data.waterHeight = Room.height / 2;
			}

			Immediate.Color(Color.White);
			UI.font.Write("Enclosed Room", sidebar.x0 + 0.07f, sidebar.y1 - 0.035f, 0.03f, Font.Align.MiddleLeft);
			UI.font.Write("Water", sidebar.x0 + 0.07f, sidebar.y1 - 0.095f, 0.03f, Font.Align.MiddleLeft);
			UI.font.Write("Water in Front", sidebar.x0 + 0.07f, sidebar.y1 - 0.155f, 0.03f, Font.Align.MiddleLeft);

			float barY = sidebar.y1 - 0.2f;
			Immediate.Color(Themes.Border);
			UI.Line(sidebar.x0, barY, sidebar.x1, sidebar.y1 - 0.2f);

			if (UI.TextButton("Add TerrainHandle", Rect.FromSize(sidebar.x0 + 0.01f, barY - 0.06f, 0.39f, 0.05f))) {
				Room.data.objects.Add(new TerrainHandleObject());
				Room.visuals.terrainNeedsRefresh = true;
			}
			if (UI.TextButton("Add MudPit", Rect.FromSize(sidebar.x0 + 0.01f, barY - 0.12f, 0.39f, 0.05f))) {
				Room.data.objects.Add(new MudPitObject());
			}
			if (UI.TextButton("Add AirPocket", Rect.FromSize(sidebar.x0 + 0.01f, barY - 0.18f, 0.39f, 0.05f))) {
				Room.data.objects.Add(new AirPocketObject());
				Room.visuals.waterNeedsRefresh = true;
			}

			if (trashCanState > 0) {
				Immediate.Color(trashCanState == 2 ? Color.Red : Color.White);
				UI.StrokeRect(trashCanRect);
				UI.font.Write("Trash", -Main.screenBounds.x + 0.06f, -Main.screenBounds.y + 0.13f, 0.03f, Font.Align.MiddleCenter);
			}
		}

		Rect tabPositions = new Rect(-Main.screenBounds.x, Main.screenBounds.y - 0.06f, Main.screenBounds.x, Main.screenBounds.y - 0.12f);
		Immediate.Color(Themes.Popup);
		UI.FillRect(tabPositions);
		Immediate.Color(Themes.Border);
		UI.Line(tabPositions.x0, tabPositions.y0, tabPositions.x1, tabPositions.y0);

		Vector2 tabPosition = new Vector2(-Main.screenBounds.x + 0.01f, Main.screenBounds.y - 0.12f);
		float tabHeight = 0.05f;
		for (int i = 0; i < 3; i++) {
			float tabWidth = MathF.Max(0.15f, UI.font.Measure(TabNames[i], 0.03f).x + 0.04f);
			Rect tab = Rect.FromSize(tabPosition, new Vector2(tabWidth, tabHeight));
			bool hovered = tab.Inside(Mouse.Pos);
			bool selected = i == (int) currentTab;

			if (selected || hovered) {
				Immediate.Color(Themes.PopupHeader);
				UI.FillRect(tab);
			}

			Immediate.Color(selected ? Themes.BorderHighlight : Themes.Border);
			UI.StrokeRect(tab);

			if (selected || hovered) {
				Immediate.Color(Themes.PopupHeader);
				UI.Line(tab.x0, tab.y0, tab.x1, tab.y0);
			}

			Immediate.Color(selected ? Themes.TextHighlight : Themes.Text);
			UI.font.Write(TabNames[i], tabPosition.x + 0.02f, tabPosition.y + 0.04f, 0.03f);

			if (hovered && Mouse.JustLeft) {
				currentTab = (EditorTab) i;
			}

			tabPosition.x += tabWidth + 0.01f;
		}

		if (hoverText.Length != 0) {
			float width = UI.font.Measure(hoverText, 0.03f).x;
			Rect hoverRect = Rect.FromSize(Mouse.X, Mouse.Y, width + 0.02f, 0.05f);
			if (hoverRect.x1 > Main.screenBounds.x) {
				hoverRect += new Vector2(Main.screenBounds.x - hoverRect.x1, 0f);
			}
			if (hoverRect.y1 > Main.screenBounds.y) {
				hoverRect += new Vector2(0f, Main.screenBounds.y - hoverRect.y1);
			}

			Immediate.Color(Themes.Background);
			UI.FillRect(hoverRect);
			Immediate.Color(Themes.Text);
			UI.font.Write(hoverText, hoverRect.x0 + 0.01f, hoverRect.CenterY, 0.03f, Font.Align.MiddleLeft);
			Immediate.Color(Themes.Border);
			UI.StrokeRect(hoverRect);
		}

		menuItems.Draw();
	}

	public static void Backup() {
		backupGeometry = new uint[Room.width * Room.height];
		for (int i = 0; i < Room.width * Room.height; i++) {
			backupGeometry[i] = Room.geometry[i];
		}
		backupWaterHeight = Room.data.waterHeight;
		backupWaterInFront = Room.data.waterInFront;
		backupEnclosedRoom = Room.data.enclosedRoom;
		backupWidth = Room.width;
		backupHeight = Room.height;
		backupCameras = [.. Room.data.cameras.Select(x => new RoomData.Camera() { position = x.position, angles = [ ..x.angles ] })];
		backupObjects = [.. Room.data.objects.Select(x => x.Clone())];
	}

	public static void Reset() {
		if (backupGeometry == null) return;

		if (Room.width != backupWidth || Room.height != backupHeight) {
			Room.geometry = new uint[backupWidth * backupHeight];
		}
		Room.width = backupWidth;
		Room.height = backupHeight;
		Room.data.waterHeight = backupWaterHeight;
		Room.data.waterInFront = backupWaterInFront;
		Room.data.enclosedRoom = backupEnclosedRoom;
		Room.data.cameras = [.. backupCameras.Select(x => new RoomData.Camera() { position = x.position, angles = [ ..x.angles ] })];
		Room.data.objects = [.. backupObjects.Select(x => x.Clone())];
		Room.valid = true;

		for (int i = 0; i < backupWidth * backupHeight; i++) {
			Room.geometry[i] = backupGeometry[i];
		}

		backupGeometry = null;
		Room.RegenerateGeometry();
		Room.visuals.terrainNeedsRefresh = true;
		Room.visuals.waterNeedsRefresh = true;
		Room.visuals.Refresh();
	}

	public static void LoadRoom(Room room) {
		Room = room;

		if (!File.Exists(Room.path)) {
			Logger.Error("Failed to open droplet room file: " + Room.path);
			return;
		}

		Backup();
		Room.visuals.terrainNeedsRefresh = true;
		Room.visuals.waterNeedsRefresh = true;
	}

	private static void ExportGeometry() {
		string geoPath = PathUtil.FindFile(WorldWindow.region.roomsPath, Room.name + ".txt")!;
		FloodForge.Backup.File(geoPath);

		StringBuilder geo = new StringBuilder();

		geo.AppendLine(Room.name);
		geo.Append($"{Room.width}*{Room.height}");
		if (Room.data.waterHeight != -1) {
			geo.Append($"|{Room.data.waterHeight}|{(Room.data.waterInFront ? "1" : "0")}");
		}
		geo.Append("\n");
		geo.AppendLine("0.0000*1.0000|0|0");

		for (int i = 0; i < Room.data.cameras.Count; i++) {
			if (i > 0) geo.Append("|");
			geo.Append($"{Mathf.RoundToInt(Room.data.cameras[i].position.x)},{Mathf.RoundToInt(Room.data.cameras[i].position.y)}");
		}
		geo.Append("\n");

		geo.AppendLine($"Border: {(Room.data.enclosedRoom ? "Solid" : "Passable")}");

		for (int x = 0; x < Room.width; x++) {
			for (int y = 0; y < Room.height; y++) {
				uint tile = Room.GetTile(x, y);
				if ((tile & 262144) > 0) geo.Append($"0,{x + 1},{y + 1}|");
				if ((tile & 524288) > 0) geo.Append($"1,{x + 1},{y + 1}|");
			}
		}
		geo.Append("\n\n\n\n0\n\n");

		for (int x = 0; x < Room.width; x++) {
			for (int y = 0; y < Room.height; y++) {
				uint tile = Room.GetTile(x, y);
				geo.Append(tile % 16);

				if ((tile & 16) > 0)     geo.Append(",1");
				if ((tile & 32) > 0)     geo.Append(",2");
				if ((tile & 64) > 0)     geo.Append(",4");
				if ((tile & 128) > 0)    geo.Append(",3");
				if ((tile & 256) > 0)    geo.Append(",5");
				if ((tile & 512) > 0)    geo.Append(",6");
				if ((tile & 4096) > 0)   geo.Append(",12");
				if ((tile & 8192) > 0)   geo.Append(",9");
				if ((tile & 16384) > 0)  geo.Append(",10");
				if ((tile & 32768) > 0)  geo.Append(",11");
				if ((tile & 65536) > 0)  geo.Append(",7");
				if ((tile & 131072) > 0) geo.Append(",8");

				geo.Append("|");
			}
		}
		geo.Append("\n");

		geo.Append("camera angles:");
		for (int i = 0; i < Room.data.cameras.Count; i++) {
			if (i > 0) geo.Append("|");
			RoomData.Camera c = Room.data.cameras[i];

			static string FormatAngle(Vector2 ang) {
				return $"{MathF.Atan2(ang.x, ang.y) * (180f / Mathf.PI):F4},{ang.Length:F4}";
			}

			geo.Append($"{FormatAngle(c.angles[0])};{FormatAngle(c.angles[1])};{FormatAngle(c.angles[2])};{FormatAngle(c.angles[3])}");
		}
		geo.Append("\n");

		File.WriteAllText(geoPath, geo.ToString());

		Backup();
		Room.visuals.terrainNeedsRefresh = true;
		Room.visuals.waterNeedsRefresh = true;
		Room.visuals.Refresh();

		string? settingsPath = PathUtil.FindFile(WorldWindow.region.roomsPath, Room.name + "_settings.txt");
		FloodForge.Backup.File(settingsPath);

		string before = "";
		string placedObjectsLine = "";
		string after = "";

		if (settingsPath != null) {
			bool isBefore = true;
			foreach (string line in File.ReadLines(settingsPath)) {
				if (line.StartsWith("PlacedObjects:")) {
					placedObjectsLine = line;
					isBefore = false;
				}
				else {
					if (isBefore) before += line + "\n";
					else after += line + "\n";
				}
			}
		}

		{
			StringBuilder output = new StringBuilder();
			string data = placedObjectsLine.Contains(' ') ? placedObjectsLine[(placedObjectsLine.IndexOf(' ') + 1)..] : "";
			string[] poData = data.Split([", "], StringSplitOptions.RemoveEmptyEntries);

			int terrainIdx = 0, mudIdx = 0, airIdx = 0;
			TerrainHandleObject[] terrainHandleObjects = [.. Room.data.objects.OfType<TerrainHandleObject>()];
			MudPitObject[] mudPitObjects = [.. Room.data.objects.OfType<MudPitObject>()];
			AirPocketObject[] airPocketObjects = [.. Room.data.objects.OfType<AirPocketObject>()];

			foreach (string po in poData) {
				if (string.IsNullOrWhiteSpace(po)) continue;
				string currentOutputState = output.ToString();
				try {
					int start = po.IndexOf('<');
					int next = po.IndexOf('>', start);
					int end = po.IndexOf('>', next + 1);
					string last = po.Substring(end + 2);
					string[] splits = last.Split('~');

					if (po.StartsWith("TerrainHandle>")) {
						if (terrainIdx >= terrainHandleObjects.Length) continue;
						TerrainHandleObject h = terrainHandleObjects[terrainIdx++];
						string height = splits.Length >= 5 ? splits[4] : "20";
						output.Append($"TerrainHandle><{h.nodes[0].position.x}><{h.nodes[0].position.y}><{h.nodes[1].position.x}~{h.nodes[1].position.y}~{h.nodes[2].position.x}~{h.nodes[2].position.y}~{height}");
					}
					else if (po.StartsWith("MudPit>")) {
						if (mudIdx >= mudPitObjects.Length) continue;
						MudPitObject m = mudPitObjects[mudIdx++];
						string size = splits.Length >= 3 ? splits[2] : "15.0";
						output.Append($"MudPit><{m.nodes[0].position.x}><{m.nodes[0].position.y}><{m.nodes[1].position.x}~{m.nodes[1].position.y}~{size}");
					}
					else if (po.StartsWith("AirPocket>")) {
						if (airIdx >= airPocketObjects.Length) continue;
						AirPocketObject p = airPocketObjects[airIdx++];
						string px = "30.0", py = "30.0", flood = "Y";
						if (splits.Length >= 6) { px = splits[2]; py = splits[3]; flood = splits[4]; }
						output.Append($"AirPocket><{p.nodes[0].position.x}><{p.nodes[0].position.y}><{p.nodes[1].position.x}~{p.nodes[1].position.y}~{px}~{py}~{flood}~{p.nodes[2].position.y}");
					}
					else {
						output.Append(po);
					}
					output.Append(", ");
				} catch {
					output.Clear();
					output.Append(currentOutputState).Append(po).Append(", ");
					Logger.Info($"Failed when outputting po '{po}'");
				}
			}

			while (terrainIdx < terrainHandleObjects.Length) {
				TerrainHandleObject h = terrainHandleObjects[terrainIdx++];
				output.Append($"TerrainHandle><{h.nodes[0].position.x}><{h.nodes[0].position.y}><{h.nodes[1].position.x}~{h.nodes[1].position.y}~{h.nodes[2].position.x}~{h.nodes[2].position.y}~20, ");
			}
			while (mudIdx < mudPitObjects.Length) {
				MudPitObject m = mudPitObjects[mudIdx++];
				output.Append($"MudPit><{m.nodes[0].position.x}><{m.nodes[0].position.y}><{m.nodes[1].position.x}~{m.nodes[1].position.y}~15, ");
			}
			while (airIdx < airPocketObjects.Length) {
				AirPocketObject p = airPocketObjects[airIdx++];
				output.Append($"AirPocket><{p.nodes[0].position.x}><{p.nodes[0].position.y}><{p.nodes[1].position.x}~{p.nodes[1].position.y}~30.0~30.0~Y~{p.nodes[2].position.y}, ");
			}

			placedObjectsLine = output.ToString();
		}

		if (before != null && before != "" || after != null && after != "" || !string.IsNullOrWhiteSpace(placedObjectsLine)) {
			settingsPath ??= PathUtil.Combine(WorldWindow.region.roomsPath, Room.name + "_settings.txt");
			using StreamWriter sw = new StreamWriter(settingsPath);
			sw.Write(before);
			if (!string.IsNullOrWhiteSpace(placedObjectsLine)) {
				if (!placedObjectsLine.EndsWith(", "))
					placedObjectsLine += ", ";
				sw.Write("PlacedObjects: " + placedObjectsLine + "\n");
			}
			sw.Write(after);
		}
	}

	private static bool ValidSlopePos(uint geo, Vector2 tp) { // something in this function is broken, which is why the other overload doesn't simply make a new vector2 to call this one with.
		uint type = (geo & (1024 | 2048)) / 1024;
		float x = (tp.x - 0.5f) % 1f;
		float y = (tp.y - 0.5f) % 1f;
		return type switch {
			0 => 1f - x > y,
			1 => 1f - x > 1f - y,
			2 => x > y,
			3 => x > 1f - y,
			_ => false,
		};
	}

	private static bool ValidSlopePos(uint geo, int xInTile, int yInTile) {
		if (xInTile < 0 || xInTile > 19 || yInTile < 0 || yInTile > 19)
			return false;
		uint slopeType = (geo & (1024 | 2048)) / 1024;
		return slopeType switch {
			0 => 19 - xInTile > yInTile,
			1 => 19 - xInTile > 19 - yInTile,
			2 => xInTile > yInTile,
			3 => xInTile > 19 - yInTile,
			_ => false
		};
	}

	private const int CameraTextureWidth = 1400;
	private const int CameraTextureHeight = 800;
	private static void SetPixel(byte[] data, int index, byte r, byte g, byte b) {
		data[index] = r;
		data[index + 1] = g;
		data[index + 2] = b;
	}

	// NOTE - Shortcutdots don't seem to line up properly - this may be a result of cangles
	private static byte[] RenderCamera(RoomData.Camera camera) {
		byte[] image = new byte[CameraTextureWidth * CameraTextureHeight * 3];

		for(int i = 0; i < image.Length; i++) {
			image[i] = 255;
		}

		int firstTilePositionX = Mathf.FloorToInt(camera.position.x / 20) - 1;
		float TopLeftOffsetX = firstTilePositionX * 20 - camera.position.x;
		int firstTilePositionY = Mathf.FloorToInt(camera.position.y / 20) - 1;
		float TopLeftOffsetY = firstTilePositionY * 20 - camera.position.y;
		int lastTilePositionX = Mathf.CeilToInt((camera.position.x + CameraTextureWidth) / 20);
		int lastTilePositionY = Mathf.CeilToInt((camera.position.y + CameraTextureHeight) / 20);

		for(int y = firstTilePositionY; y <= lastTilePositionY; y++) {
			for(int x = firstTilePositionX; x <= lastTilePositionX; x++) {
				uint tileType = Room.GetTile(x, y);
				float offsetX = camera.position.x;// - (TopLeftOffsetX % 20);
				float offsetY = camera.position.y;//  (TopLeftOffsetY % 20);
				int pixelOffsetX = Mathf.FloorToInt(offsetX);
				int pixelOffsetY = Mathf.FloorToInt(offsetY);

				if ((tileType % 16) == 1) {
					for (int tileY = 0; tileY < 20; tileY++) {
						int pixelYPos = y * 20 - pixelOffsetY + tileY;
						for (int tileX = 0; tileX < 20; tileX++) {
							int pixelXPos = x * 20 - pixelOffsetX + tileX;

							if(pixelXPos >= 0 && pixelXPos < CameraTextureWidth && pixelYPos >= 0 && pixelYPos < CameraTextureHeight) {
								int index = (pixelXPos + pixelYPos * CameraTextureWidth) * 3;
								if (tileX == 0 || tileY == 0) {
									SetPixel(image, index, 151, 0, 0);
								}
								else if (tileX == 19 || tileY == 19) {
									SetPixel(image, index, 91, 0, 0);
								}
								else {
									SetPixel(image, index, 121, 0, 0);
								}
							}
						}
					}
				}
				else if ((tileType % 16) == 4) {
					for (int tileY = 0; tileY < 20; tileY++) {
						int pixelYPos = y * 20 - pixelOffsetY + tileY;
						for (int tileX = 0; tileX < 20; tileX++) {
							int pixelXPos = x * 20 - pixelOffsetX + tileX;

							if (pixelXPos >= 0 && pixelXPos < CameraTextureWidth && pixelYPos >= 0 && pixelYPos < CameraTextureHeight) {
								int index = (pixelXPos + pixelYPos * CameraTextureWidth) * 3;
								if (tileX == 0 || tileY == 0) {
									SetPixel(image, index, 155, 0, 0);
								}
								else if (tileX == 19 || tileY == 19) {
									SetPixel(image, index, 95, 0, 0);
								}
								else if (tileX < 8 || tileX > 12 || tileY < 8 || tileY > 12) {
									SetPixel(image, index, 125, 0, 0);
								}
							}
						}
					}
				}
				else if((tileType & 512) > 0) {
					for (int tileY = 0; tileY < 20; tileY++) {
						int pixelYPos = y * 20 - pixelOffsetY + tileY;
						for (int tileX = 0; tileX < 20; tileX++) {
							int pixelXPos = x * 20 - pixelOffsetX + tileX;

							if (pixelXPos >= 0 && pixelXPos < CameraTextureWidth && pixelYPos >= 0 && pixelYPos < CameraTextureHeight) {
								int index = (pixelXPos + pixelYPos * CameraTextureWidth) * 3;
								if (tileX == 0 || tileY == 0) {
									SetPixel(image, index, 161, 0, 0);
								}
								else if (tileX == 19 || tileY == 19) {
									SetPixel(image, index, 101, 0, 0);
								}
								else {
									SetPixel(image, index, 131, 0, 0);
								}
							}
						}
					}
				}
				if ((tileType % 16) == 2) {
					for (int tileY = 0; tileY < 20; tileY++) {
						for (int tileX = 0; tileX < 20; tileX++) {
							int pixelYPos = y * 20 - pixelOffsetY + tileY;
							int pixelXPos = x * 20 - pixelOffsetX + tileX;
							if (pixelXPos >= 0 && pixelXPos < CameraTextureWidth && pixelYPos >= 0 && pixelYPos < CameraTextureHeight) {
								int index = (pixelXPos + pixelYPos * CameraTextureWidth) * 3;
								if (ValidSlopePos(tileType, tileX, tileY)) {
									if (!ValidSlopePos(tileType, tileX, tileY - 1) || !ValidSlopePos(tileType, tileX - 1, tileY)) {
										SetPixel(image, index, 151, 0, 0);
									}
									else if (!ValidSlopePos(tileType, tileX, tileY + 1) || !ValidSlopePos(tileType, tileX + 1, tileY)) {
										SetPixel(image, index, 91, 0, 0);
									}
									else {
										SetPixel(image, index, 121, 0, 0);
									}
								}
							}
						}
					}
				}
				if ((tileType & 128) > 0 && (tileType % 16) != 4) {
					bool isBackgroundShortcut = (tileType & 512) > 0 && (tileType % 16) != 1;
					for (int tileY = 8; tileY < 12; tileY++) {
						for (int tileX = 8; tileX < 12; tileX++) {
							int pixelYPos = y * 20 - pixelOffsetY + tileY;
							int pixelXPos = x * 20 - pixelOffsetX + tileX;

							if (pixelXPos >= 0 && pixelXPos < CameraTextureWidth && pixelYPos >= 0 && pixelYPos < CameraTextureHeight) {
								int index = (pixelXPos + pixelYPos * CameraTextureWidth) * 3;
								if (!isBackgroundShortcut) {
									SetPixel(image, index, 31, 8, 0);
								}
								else {
									SetPixel(image, index, 51, 8, 0);
								}
							}
						}
					}
				}
				if ((tileType & 16) > 0) {
					for (int tileY = 0; tileY < 20; tileY++) {
						int pixelYPos = y * 20 - pixelOffsetY + tileY;
						for (int tileX = 8; tileX < 12; tileX++) {
							int pixelXPos = x * 20 - pixelOffsetX + tileX;

							if (pixelXPos >= 0 && pixelXPos < CameraTextureWidth && pixelYPos >= 0 && pixelYPos < CameraTextureHeight) {
								int index = (pixelXPos + pixelYPos * CameraTextureWidth) * 3;
								SetPixel(image, index, 95, 0, 0);
							}
						}
					}
				}
				if ((tileType & 32) > 0) {
					for (int tileY = 8; tileY < 12; tileY++) {
						int pixelYPos = y * 20 - pixelOffsetY + tileY;
						for (int tileX = 0; tileX < 20; tileX++) {
							int pixelXPos = x * 20 - pixelOffsetX + tileX;

							if (pixelXPos >= 0 && pixelXPos < CameraTextureWidth && pixelYPos >= 0 && pixelYPos < CameraTextureHeight) {
								int index = (pixelXPos + pixelYPos * CameraTextureWidth) * 3;
								SetPixel(image, index, 95, 0, 0);
							}
						}
					}
				}
				if ((tileType % 16) == 3) {
					for (int tileY = 0; tileY < 10; tileY++) {
						int pixelYPos = y * 20 - pixelOffsetY + tileY;
						for (int tileX = 0; tileX < 20; tileX++) {
							int pixelXPos = x * 20 - pixelOffsetX + tileX;

							if (pixelXPos >= 0 && pixelXPos < CameraTextureWidth && pixelYPos >= 0 && pixelYPos < CameraTextureHeight) {
								int index = (pixelXPos + pixelYPos * CameraTextureWidth) * 3;
								SetPixel(image, index, 95, 0, 0);
							}
						}
					}
				}
			}
		}
		return image;
	}

	public static bool Render(out string errorMessage, out (string, string, byte[])[] images) {
		ExportGeometry();
		bool success = true;
		errorMessage = "";
		string updateBaseText = "";
		if (WorldWindow.renderStatusPopup != null) {
			updateBaseText = WorldWindow.renderStatusPopup.GetText();
		}

		if(Room.data.cameras.Count <= 0) {
			errorMessage += "No cameras in room!\n";
			success = false;
		}
		List<(string, string, byte[])> renderedRooms = [];
		for (int i = 0; i < Room.data.cameras.Count; i++) {
			string imageName = $"{Room.name}_{i + 1}";
			string outputPath = Path.Combine(WorldWindow.region.roomsPath, $"{imageName}.png");
			Logger.Info(outputPath.Split("Rain World")[^1]);
			if(WorldWindow.renderStatusPopup != null) WorldWindow.renderStatusPopup?.UpdateText(updateBaseText + " " + (i + 1) + "/" + Room.data.cameras.Count);
			try {
				renderedRooms.Add(new (imageName, outputPath, RenderCamera(Room.data.cameras[i])));
			}
			catch (Exception e) {
				success = false;
				errorMessage += $"{e.Message}\n";
				break;
			}
		}
		images = [..renderedRooms];
		return success;
	}

	private static void ExportProject(string path) {
		string fileName = Path.Combine(path, Room.name + ".txt");
		StringBuilder project = new StringBuilder();

		project.Append("[");
		for (int x = -12; x < Room.width + 12; x++) {
			if (x != -12) project.Append(", ");

			project.Append("[");
			for (int y = -3; y < Room.height + 5; y++) {
				if (y != -3) project.Append(", ");

				uint geo = Room.GetTile(x, y);
				int solidA = 0;
				List<string> flags = [];

				if ((geo % 16) == 1) solidA = 1;
				else if ((geo % 16) == 3) solidA = 6;
				else if ((geo % 16) == 2) {
					solidA = 2 + ((geo & 2048) > 0 ? 1 : 0) + ((geo & 1024) > 0 ? 0 : 2);
				}
				else if ((geo % 16) == 4) {
					solidA = 7;
					flags.Add("4");
				}

				if ((geo & 16) > 0) flags.Add("2");
				if ((geo & 32) > 0) flags.Add("1");
				if ((geo & 64) > 0) flags.Add("6");
				if ((geo & 128) > 0) flags.Add("5");
				if ((geo & 256) > 0) flags.Add("7");
				if ((geo & 4096) > 0) flags.Add("21");
				if ((geo & 8192) > 0) flags.Add("19");
				if ((geo & 16384) > 0) flags.Add("13");
				if ((geo & 32768) > 0) flags.Add("20");
				if ((geo & 65536) > 0) flags.Add("3");
				if ((geo & 131072) > 0) flags.Add("18");
				if ((geo & 262144) > 0) flags.Add("9");
				if ((geo & 524288) > 0) flags.Add("10");

				project.Append($"[[{solidA}, [{string.Join(", ", flags)}]], ");
				project.Append($"[{((geo & 512) > 0 ? "1" : "0")}, []], ");
				project.Append("[0, []]]");
			}
			project.Append("]");
		}
		project.Append("]\n");

		project.AppendLine("[#lastKeys: [], #Keys: [], #workLayer: 1, #lstMsPs: point(0, 0), #tlMatrix: [], #defaultMaterial: \"Standard\", #toolType: \"material\", #toolData: \"Big Metal\", #tmPos: point(1, 1), #tmSavPosL: [], #specialEdit: 0]");
		project.AppendLine("[#lastKeys: [], #Keys: [], #lstMsPs: point(0, 0), #effects: [], #emPos: point(1, 1), #editEffect: 0, #selectEditEffect: 0, #mode: \"createNew\", #brushSize: 5]");
		project.AppendLine($"[#pos: point(0, 0), #rot: 0, #sz: point({Room.width}, {Room.height}), #col: 1, #Keys: [#m1: 0, #m2: 0, #w: 0, #a: 0, #s: 0, #d: 0, #r: 0, #f: 0, #z: 0, #m: 0], #lastKeys: [#m1: 0, #m2: 0, #w: 0, #a: 0, #s: 0, #d: 0, #r: 0, #f: 0, #z: 0, #m: 0], #lastTm: 0, #lightAngle: 180, #flatness: 1, #lightRect: rect(1000, 1000, -1000, -1000), #paintShape: \"pxl\"]");
		project.AppendLine($"[#timeLimit: 4800, #defaultTerrain: {(Room.data.enclosedRoom ? "1" : "0")}, #maxFlies: 10, #flySpawnRate: 50, #lizards: [], #ambientSounds: [], #music: \"NONE\", #tags: [], #lightType: \"Static\", #waterDrips: 1, #lightRect: rect(0, 0, 1040, 800), #Matrix: []]");
		project.AppendLine($"[#mouse: 1, #lastMouse: 0, #mouseClick: 0, #pal: 1, #pals: [[#detCol: color( 255, 0, 0 )]], #eCol1: 1, #eCol2: 2, #totEcols: 5, #tileSeed: 225, #colGlows: [0, 0], #size: point({Room.width + 24}, {Room.height + 8}), #extraTiles: [12, 3, 12, 5], #light: 1]");

		project.Append("[#cameras: [");
		for (int i = 0; i < Room.data.cameras.Count; i++) {
			if (i > 0) project.Append(", ");
			RoomData.Camera c = Room.data.cameras[i];
			project.Append($"point({(c.position.x + 12.0f) * 20.0f:F1}, {(c.position.y + 3.0f) * 20.0f:F1})");
		}
		project.Append("], #selectedCamera: 0, #quads: [");

		for (int i = 0; i < Room.data.cameras.Count; i++) {
			if (i > 0) project.Append(", ");
			RoomData.Camera c = Room.data.cameras[i];

			static string AngLen(Vector2 a) {
				return $"[{Math.Atan2(a.x, a.y) * (180.0 / Math.PI):F4}, {a.Length:F4}]";
			}

			project.Append($"[{AngLen(c.angles[0])}, {AngLen(c.angles[1])}, {AngLen(c.angles[2])}, {AngLen(c.angles[3])}]");
		}
		project.AppendLine("], #Keys: [#n: 0, #d: 0, #e: 0, #p: 0], #lastKeys: [#n: 0, #d: 0, #e: 0, #p: 0]]");

		project.AppendLine($"[#waterLevel: {Room.data.waterHeight}, #waterInFront: {(Room.data.waterInFront ? "1" : "0")}, #waveLength: 60, #waveAmplitude: 5, #waveSpeed: 10]");
		project.AppendLine("[#props: [], #lastKeys: [#w: 0, #a: 0, #s: 0, #d: 0, #L: 0, #n: 0, #m1: 0, #m2: 0, #c: 0, #z: 0], #Keys: [#w: 0, #a: 0, #s: 0, #d: 0, #L: 0, #n: 0, #m1: 0, #m2: 0, #c: 0, #z: 0], #workLayer: 1, #lstMsPs: point(0, 0), #pmPos: point(1, 1), #pmSavPosL: [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1], #propRotation: 0, #propStretchX: 1, #propStretchY: 1, #propFlipX: 1, #propFlipY: 1, #depth: 0, #color: 0]");

		File.WriteAllText(fileName, project.ToString());

		if (!Settings.RainedPath.value.IsNullOrEmpty() && File.Exists(Settings.RainedPath)) {
			PopupManager.Add(new ConfirmPopup("Open in Rained?").Okay(() => {
				// TODO: Don't open a new exe when Rained is already running

				ProcessStartInfo startInfo = new ProcessStartInfo {
					FileName = Settings.RainedPath,
					UseShellExecute = false,
					CreateNoWindow = false,
					WorkingDirectory = Path.GetDirectoryName(Settings.RainedPath),
					Arguments = $"\"{fileName}\""
				};

				try {
					Process.Start(startInfo);
				}
				catch (Exception ex) {
					Logger.Error($"Error launching Rained: {ex.Message}");
				}
			}));
		}
	}

	public static void ResizeRoom(int width, int height, bool stretch) {
		Backup();

		Room.width = width;
		Room.height = height;
		Room.geometry = new uint[width * height];

		if (stretch) {
			for (int x = 0; x < width; x++) {
				float xRatio = (width > 1) ? (float)x / (width - 1) : 0;
				int bx = (int)(xRatio * (backupWidth - 1));

				for (int y = 0; y < height; y++) {
					float yRatio = (height > 1) ? (float)y / (height - 1) : 0;
					int by = (int)(yRatio * (backupHeight - 1));

					int i = x * height + y;
					Room.geometry[i] = backupGeometry![bx * backupHeight + by];
				}
			}
		}
		else {
			for (int x = 0; x < width; x++) {
				int bx = x + resizeOffset.x;
				for (int y = 0; y < height; y++) {
					int by = y + resizeOffset.y;
					int i = x * height + y;

					if (bx < 0 || by < 0 || bx >= backupWidth || by >= backupHeight) {
						Room.geometry[i] = 0;
					}
					else {
						Room.geometry[i] = backupGeometry![bx * backupHeight + by];
					}
				}
			}
		}
	}

	private class DropletMenuItems : MenuItems {
		public DropletMenuItems() {
			this.buttons = [
				new Button("Export Geometry", b => {
					ExportGeometry();
					PopupManager.Add(new InfoPopup("Exported successfully"));
				}),
				new Button("Render", b => {
					bool success = true;
					if(Render(out string message, out (string name, string outputPath, byte[] image)[] images)){
						for(int i = 0; i < images.Length; i++){
							FloodForge.Backup.File(images[i].outputPath);

							try {
								using Stream stream = File.OpenWrite(images[i].outputPath);
								ImageWriter writer = new ImageWriter();
								writer.WritePng(images[i].image, CameraTextureWidth, CameraTextureHeight, ColorComponents.RedGreenBlue, stream);
								Logger.Info("Screen exported");
								success = true;
							}
							catch (Exception ex) {
								Logger.Error("Exporting screen failed: " + ex.Message);
								message = ex.Message;
								success = false;
							}
						}
					}
					else{
						success = false;
					}
					if(success){
						PopupManager.Add(new InfoPopup("Rendered successfully"));
					}
					else
						PopupManager.Add(new InfoPopup($"Render failed!\nMessage: \n{message}View log.txt for more information."));
				}),
				new Button("Export Leditor Project", b => {
					PopupManager.Add(
						new FilesystemPopup(paths => {
							if (paths.Length != 1) return;

							ExportProject(paths[0]);
						}).Filter(FilesystemPopup.SelectionType.Folder).Hint("Data/LevelEditorProjects")
					);
				}),
				new Button("Show Objects", b => {
					showObjects = !showObjects;
					b.text = showObjects ? "Hide objects" : "Show objects";
				}),
				new Button("Show Position", b => {
					showMousePosition = !showMousePosition;
					b.text = showMousePosition ? "Hide Position" : "Show Position";
				}, button => {
					return currentTab == EditorTab.Geometry;
				}),
				new Button("Resize", b => {
					PopupManager.Add(new ResizeLevelPopup());
				}),
				new Button("Exit Droplet", b =>{
					PopupManager.Add(new ConfirmPopup("Exit Droplet?\nUnsaved changes will be lost").Okay(() => {
						mode = Mode.World;
						DropletWindow.Reset();
					}));
				}),
			];
		}
	}
}