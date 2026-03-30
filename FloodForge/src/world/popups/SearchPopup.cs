using FloodForge.Popups;
using Stride.Core.Extensions;

namespace FloodForge.World;

public class SearchPopup : Popup {
	protected UI.TextInputEditable Search;

	public SearchPopup() {
		this.bounds = new Rect(-0.25f, -0.08f, 0.25f, 0.25f);
		this.Search = new UI.TextInputEditable(UI.TextInputEditable.Type.Text, "");
		this.Search.GrabFocus();
	}

	public override void Accept() {
		if (this.Search.Focused)
			return;

		this.Submit(this.Search.value);
		this.Search.value = "";
		this.Search.Submit();
	}

	public override void Close() {
		base.Close();
		UI.Delete(this.Search);
	}

	public override void Draw() {
		base.Draw();
		if (this.minimized)
			return;

		float centerX = this.bounds.CenterX;

		Immediate.Color(Themes.Text);
		UI.font.Write("Find room", centerX, this.bounds.y1 - 0.1f, 0.04f, Font.Align.MiddleCenter);

		UI.TextInput(new Rect(this.bounds.x0 + 0.01f, this.bounds.y1 - 0.14f, this.bounds.x1 - 0.01f, this.bounds.y1 - 0.19f), this.Search, new UI.TextInputMods() {
			placeholder = "A01",
		});

		if (UI.TextButton("Cancel", new Rect(centerX - 0.2f, this.bounds.y1 - 0.28f, centerX - 0.05f, this.bounds.y1 - 0.22f))) {
			this.Reject();
			this.Search.value = "";
			this.Search.Submit();
		}

		if (UI.TextButton("Confirm", new Rect(centerX + 0.05f, this.bounds.y1 - 0.28f, centerX + 0.2f, this.bounds.y1 - 0.22f))) {
			this.Submit(this.Search.value);
			this.Search.value = "";
			this.Search.Submit();
		}
	}

	protected virtual void Submit(string search) {
		if (search.IsNullOrEmpty())
			return;

		this.Close();

		// TODO: Search for closest and show all options
		Room? room = WorldWindow.region.rooms.FirstOrDefault(r => {
			return
				r.name[(WorldWindow.region.acronym.Length + 1)..].Equals(search, StringComparison.InvariantCultureIgnoreCase) ||
				r.name.Equals(search, StringComparison.InvariantCultureIgnoreCase);
		});
		if (room == null)
			return;

		WorldWindow.FocusCameraOn(room);
	}
}