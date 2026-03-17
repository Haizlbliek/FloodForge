using FloodForge.Popups;
using Silk.NET.SDL;
using Stride.Core.Extensions;

namespace FloodForge.World;

// LATER: Add scrolling
public class TagPopup : Popup {
	protected readonly HashSet<Room> rooms;

	public TagPopup(IEnumerable<Room> rooms) {
		if (rooms.IsNullOrEmpty()) throw new NotImplementedException("TagPopup must have at least 1 room");

		this.rooms = [..rooms];
	}

	protected void SetTag(string tag) {
		TagChange change = new TagChange();
		HashSet<string> tags = tag.IsNullOrEmpty() ? [] : [ tag ];
		this.rooms.Where(r => r is not OffscreenRoom).ForEach(r => change.AddRoom(r, tags));
		// REVIEW - Do I need to set room.data.tags = []; ?
		History.Apply(change);
	}

	protected void ToggleTag(string tag) {
		TagChange change = new TagChange();
		foreach (Room room in this.rooms) {
			if (room is OffscreenRoom) continue;

			HashSet<string> tags = [ ..room.data.tags ];
			tags.Toggle(tag);
			change.AddRoom(room, tags);
			// REVIEW - Do I need to set room.data.tags = []; ?
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

		float centerX = this.bounds.CenterX;

		Immediate.Color(Themes.Text);
		string title = this.rooms.Count == 1 ? this.rooms.First().Name : "Selected Rooms";
		UI.font.Write(title, centerX, this.bounds.y1 - 0.1f, 0.04f, Font.Align.MiddleCenter);

		float y = this.bounds.y1 - 0.15f;
		this.DrawTagButton("None", "", y);
		y -= 0.075f;

		for (int i = 0; i < RoomTags.Count; i++) {
			this.DrawTagButton(RoomTags.Names[i], RoomTags.Ids[i], y);
			y -= 0.075f;
		}
	}
}