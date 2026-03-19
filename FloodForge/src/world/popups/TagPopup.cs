using FloodForge.Popups;
using Silk.NET.SDL;
using Stride.Core.Extensions;

namespace FloodForge.World;

// LATER: Add scrolling
public class TagPopup : Popup {
	protected readonly HashSet<Room> rooms;
	protected float scroll = 0f;
	protected float targetScroll = 0f;

	public TagPopup(IEnumerable<Room> rooms) {
		if (rooms.IsNullOrEmpty()) throw new NotImplementedException("TagPopup must have at least 1 room");

		this.rooms = [..rooms];
		Main.Scroll += this.Scroll;
	}

	protected void Scroll(float deltaX, float deltaY) {
		if (!this.hovered || this.minimized) return;

		this.targetScroll += deltaY * 0.1f;
		this.ClampScroll();
	}

	protected void ClampScroll() {
		float maxScroll = (RoomTags.Count - 3) * -0.075f;
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

	protected void SetTag(string tag) {
		TagChange change = new TagChange();
		HashSet<string> tags = tag.IsNullOrEmpty() ? [] : [ tag ];
		this.rooms.Where(r => r is not OffscreenRoom).ForEach(r => change.AddRoom(r, tags));
		History.Apply(change);
	}

	protected void ToggleTag(string tag) {
		TagChange change = new TagChange();
		foreach (Room room in this.rooms) {
			if (room is OffscreenRoom) continue;

			HashSet<string> tags = [ ..room.data.tags ];
			tags.Toggle(tag);
			change.AddRoom(room, tags);
		}
		History.Apply(change);
	}

	protected void DrawTagButton(string tag, string tagId, float y) {
		Rect rect = new Rect(this.bounds.x0 + 0.1f, y, this.bounds.x1 - 0.1f, y - 0.05f);
		HashSet<string> roomTags = this.rooms.First().data.tags;
		bool selected = tagId == "" && roomTags.Count == 0 || roomTags.Contains(tagId);

		if (UI.TextButton(tag, rect, new UI.TextButtonMods { selected = selected })) {
			if (Keys.Modifier(Keymod.Shift)) {
				if (!tagId.IsNullOrEmpty()) this.ToggleTag(tagId);
			}
			else {
				this.SetTag(tagId);
			}
		}
	}

	public override void Draw() {
		base.Draw();
		if (this.minimized) return;

		this.scroll += (this.targetScroll - this.scroll) * (1f - MathF.Pow(1f - Settings.PopupScrollSpeed, Program.Delta * 60f));

		float centerX = this.bounds.CenterX;
		Immediate.Color(Themes.Text);
		string title = this.rooms.Count == 1 ? this.rooms.First().Name : "Selected Rooms";
		UI.font.Write(title, centerX, this.bounds.y1 - 0.1f, 0.04f, Font.Align.MiddleCenter);

		Program.gl.Enable(EnableCap.ScissorTest);
		float clipBottom = (this.bounds.y0 + 0.01f + Main.screenBounds.y) * 0.5f * Program.window.FramebufferSize.Y;
		float clipTop = (this.bounds.y1 - 0.14f + Main.screenBounds.y) * 0.5f * Program.window.FramebufferSize.Y;
		Program.gl.Scissor(0, (int) clipBottom, (uint) Program.window.FramebufferSize.X, (uint) (clipTop - clipBottom));

		float y = this.bounds.y1 - 0.15f - this.scroll;
		this.DrawTagButton("None", "", y);
		y -= 0.075f;

		for (int i = 0; i < RoomTags.Count; i++) {
			this.DrawTagButton(RoomTags.Names[i], RoomTags.Ids[i], y);
			y -= 0.075f;
		}

		Program.gl.Disable(EnableCap.ScissorTest);
	}
}