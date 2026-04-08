using FloodForge.Popups;
using Silk.NET.SDL;
using Stride.Core.Extensions;

namespace FloodForge.World;

public class SubregionPopup : Popup {
	protected readonly HashSet<Room> rooms;
	protected float scroll = 0f;
	protected float targetScroll = 0f;

	public SubregionPopup(IEnumerable<Room> rooms) {
		if (rooms.IsNullOrEmpty()) throw new NotImplementedException("SubregionPopup must have at least 1 room");

		this.rooms = [..rooms];
		Main.Scroll += this.Scroll;
	}

	protected void Scroll(float deltaX, float deltaY) {
		if (!this.hovered || this.minimized) return;

		this.targetScroll += deltaY * 0.1f;
		this.ClampScroll();
	}

	protected void ClampScroll() {
		float maxScroll = (WorldWindow.region.subregions.Count - 3) * -0.075f;
		if (this.targetScroll < maxScroll) {
			this.targetScroll = maxScroll;
			if (this.scroll <= maxScroll + 0.06f) {
				this.scroll = maxScroll - 0.03f;
			}
		}
		if (this.targetScroll > 0f) {
			this.targetScroll = 0f;
			if (this.scroll >= -0.06f) {
				this.scroll = 0.03f;
			}
		}
	}

	protected void DrawSubregionButton(int idx, string subregion, float centerX, float y) {
		Rect rect = new Rect(-0.325f + centerX, y, 0.325f + centerX, y - 0.05f);
		bool selected = false;
		if (this.rooms.Count == 1) {
			selected = this.rooms.First().data.subregion == idx;
		}

		if (UI.TextButton(subregion, rect, new UI.TextButtonMods { selected = selected })) {
			if (idx == -1) {
				this.SetSubregion(-1);
				this.Close();
			}
			else if (idx == -2) {
				PopupManager.Add(new SubregionNewPopup(this.rooms));
				this.Close();
			}
			else if (idx <= WorldWindow.region.subregions.Count) {
				this.SetSubregion(idx);
				this.Close();
			}
		}

		if (idx >= 0) {
			if (UI.TextButton("Edit", new Rect(this.bounds.x0 + 0.01f, y, -0.335f + centerX, y - 0.05f))) {
				PopupManager.Add(new SubregionNewPopup(this.rooms, idx));
				this.Close();
			}

			if (UI.TextButton("X", new Rect(0.335f + centerX, y, 0.385f + centerX, y - 0.05f))) {
				if (Keys.Modifier(Keymod.Shift)) {
					SubregionChange change = new SubregionChange(idx);

					foreach (Room otherRoom in WorldWindow.region.rooms) {
						if (otherRoom.data.subregion == idx) {
							change.AddRoom(otherRoom, -1);
						}
						else if (otherRoom.data.subregion > idx) {
							change.AddRoom(otherRoom, otherRoom.data.subregion - 1);
						}
					}

					History.Apply(change);
				}
				else {
					bool canRemove = !WorldWindow.region.rooms.Any(r => r.data.subregion == idx);
	
					if (canRemove) {
						SubregionChange change = new SubregionChange(idx);

						WorldWindow.region.rooms.Where(r => r.data.subregion == idx)
							.ForEach(r => change.AddRoom(r, r.data.subregion - 1));

						History.Apply(change);
					}
					else {
						PopupManager.Add(new InfoPopup("Cannot remove subregion if assigned to rooms\n(Hold shift to force)"));
					}
				}
			}
		}

		if (idx >= -1) {
			Color[] colors = Settings.SubregionColors.value;
			Color subregionColor = Settings.NoSubregionColor;
			if (colors.Length != 0 && idx != -1) {
				subregionColor = colors[idx % colors.Length];
			}
			bool exists = WorldWindow.region.overrideSubregionColors.TryGetValue(idx, out Color col);
			if (exists) subregionColor = col;
			UVRect buttonUv = new UVRect(0.395f + centerX, y - 0.05f, 0.445f + centerX, y).UV(exists ? 0.75f : 0.5f, 0.25f, exists ? 1f : 0.75f, 0.5f);
			if (UI.TextureButton(buttonUv, new UI.TextureButtonMods { textureColor = subregionColor })) {
				if (!exists) {
					History.Apply(new OverrideSubregionColorChange(idx, subregionColor));
				}
				PopupManager.Add(new ColorEditPopup(WorldWindow.region.overrideSubregionColors[idx], (col) => {
					History.Apply(new OverrideSubregionColorChange(idx, WorldWindow.region.overrideSubregionColors[idx], col));
				}));
			}
			if (exists) {
				if (UI.TextureButton(UVRect.FromSize(0.455f + centerX, y - 0.04f, 0.03f, 0.03f).UV(0.5f, 0.25f, 0.75f, 0f))) {
					History.Apply(new OverrideSubregionColorChange(idx));
				}
			}
		}
	}

	public override void Draw() {
		base.Draw();
		if (this.minimized) return;

		this.scroll += (this.targetScroll - this.scroll) * (1f - MathF.Pow(1f - Settings.PopupScrollSpeed, Program.Delta * 60f));

		float centerX = this.bounds.CenterX;
		Immediate.Color(Themes.Text);
		string title = this.rooms.Count == 1 ? this.rooms.First().name : "Selected Rooms";
		UI.font.Write(title, centerX, this.bounds.y1 - 0.1f, 0.04f, Font.Align.MiddleCenter);

		Program.gl.Enable(EnableCap.ScissorTest);
		float clipBottom = (this.bounds.y0 + 0.01f + Main.screenBounds.y) * 0.5f * MathF.Min(Program.window.FramebufferSize.X, Program.window.FramebufferSize.Y);
		float clipTop = (this.bounds.y1 - 0.14f + Main.screenBounds.y) * 0.5f * MathF.Min(Program.window.FramebufferSize.X, Program.window.FramebufferSize.Y);
		Program.gl.Scissor(0, (int) clipBottom, (uint) Program.window.FramebufferSize.X, (uint) (clipTop - clipBottom));

		float y = this.bounds.y1 - 0.15f - this.scroll;
		this.DrawSubregionButton(-1, "None", centerX, y);
		y -= 0.075f;

		int idx = 0;
		for (int i = 0; i < WorldWindow.region.subregions.Count; i++) {
			string subregion = WorldWindow.region.subregions[i];
			this.DrawSubregionButton(idx, subregion, centerX, y);
			y -= 0.075f;
			idx++;
		}

		this.DrawSubregionButton(-2, "+ new subregion +", centerX, y);
		Program.gl.Disable(EnableCap.ScissorTest);
	}

	public override void Close() {
		base.Close();

		Main.Scroll -= this.Scroll;
	}

	protected void SetSubregion(int subregion) {
		GeneralRoomChange<int> change = new GeneralRoomChange<int>(r => r.data.subregion, (r, i) => r.data.subregion = i);
		this.rooms.ForEach(r => change.AddRoom(r, subregion));
		History.Apply(change);
	}
}