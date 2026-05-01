using FloodForge.Popups;

namespace FloodForge.World;

public class RenameRoomPopup : Popup {
	protected bool init = true;
	protected UI.TextInputEditable RoomName = new UI.TextInputEditable(UI.TextInputEditable.Type.Text, "");
    protected Room roomToRename;
    protected int screenCount;
	protected string accept = "Accept";
	protected string cancel = "Cancel";
    protected Action<string> callback;

	protected static string GenerateRoomName(int screens) {
		string prefix = "";
		for (screens--; screens >= 0; screens = (screens / 26) - 1) {
			prefix = (char)('A' + (screens % 26)) + prefix;
		}

		int i = 1;
		while (PathUtil.FindFile(WorldWindow.region.roomsPath, $"{WorldWindow.region.acronym}_{prefix}{i:D2}.txt") != null) {
			i++;
		}

		return $"{prefix}{i:D2}";
	}

	public RenameRoomPopup(Room room, Action<string> callback) {
        this.callback = callback;
        this.roomToRename = room;
        this.screenCount = room.data.cameras.Count;
		this.bounds = new Rect(-0.4f, -0.15f, 0.4f, 0.15f);
	}

	public override void Draw() {
		Rect left = new Rect(this.bounds.x0 + 0.01f, this.bounds.y0 + 0.01f, this.bounds.CenterX - 0.005f, this.bounds.y0 + 0.06f);
		Rect right = new Rect(this.bounds.CenterX + 0.005f, this.bounds.y0 + 0.01f, this.bounds.x1 - 0.01f, this.bounds.y0 + 0.06f);
		if (left.Inside(Mouse.Pos) || right.Inside(Mouse.Pos)) {
			this.cursorOverButton = true;
		}

		base.Draw();

		if (this.collapsed) return;

		float y = this.bounds.y1 - 0.05f;

		y -= 0.06f;
		Immediate.Color(Themes.Text);
		UI.font.Write($"Rename room {this.roomToRename.name}", this.bounds.x0 + 0.01f, y + 0.025f, 0.03f, Font.Align.MiddleLeft);

		y -= 0.06f;
		Immediate.Color(Themes.Text);
		UI.font.Write($"{WorldWindow.region.acronym}_", this.bounds.x0 + 0.01f, y + 0.025f, 0.03f, Font.Align.MiddleLeft);
		float roomNameX = UI.font.Measure($"{WorldWindow.region.acronym}_", 0.03f).x;
		UI.TextInputResponse roomNameResponse = UI.TextInput(Rect.FromSize(this.bounds.x0 + 0.01f + roomNameX, y, 0.35f, 0.05f), this.RoomName);
		if (UI.TextButton("Generate", Rect.FromSize(this.bounds.x0 + 0.01f + roomNameX + 0.36f, y, 0.2f, 0.05f), new UI.TextButtonMods { disabled = false })) {
			try {
				this.RoomName.value = GenerateRoomName(this.screenCount);
				this.RoomName.submitted = true;
			}
			catch (Exception) {}
        }

		if (UI.TextButton(this.cancel, left)) {
			this.Reject();
		}

		if (UI.TextButton(this.accept, right)) {
            this.callback($"{WorldWindow.region.acronym}_" + this.RoomName.value);
			this.Accept();
		}
	}
}