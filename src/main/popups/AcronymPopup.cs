using FloodForge.Popups;

namespace FloodForge.World;

public class AcronymPopup : Popup {
	protected string? setTo = null;
	protected bool canClose;

	protected virtual int MinLength => 2;
	protected virtual string BanLetters => "_/\\ ";

	protected UI.TextInputEditable Acronym;

	public AcronymPopup() {
		this.bounds = new Rect(-0.25f, -0.08f, 0.25f, 0.25f);
		this.Acronym = new UI.TextInputEditable(UI.TextInputEditable.Type.Text, "") { bannedLetters = this.BanLetters };
	}

	public override void Close() {
		if (this.canClose) {
			base.Close();
			UI.Delete(this.Acronym);
		}
	}

	public override void Draw() {
		base.Draw();
		if (this.minimized) return;

		float centerX = this.bounds.CenterX;
		if (this.setTo != null) {
			this.Acronym.value = this.setTo;
		}
		UI.TextInputResponse response = UI.TextInput(new Rect(this.bounds.x0 + 0.01f, this.bounds.y1 - 0.15f, this.bounds.x1 - 0.01f, this.bounds.y1 - 0.1f), this.Acronym);
		this.canClose = !response.focused;

		if (UI.TextButton("Cancel", new Rect(centerX - 0.2f, this.bounds.y1 - 0.28f, centerX - 0.05f, this.bounds.y1 - 0.22f), new UI.TextButtonMods() { disabled = response.focused })) {
			this.canClose = true;
			this.Reject();
			this.Acronym.value = "";
		}

		if (UI.TextButton("Confirm", new Rect(centerX + 0.05f, this.bounds.y1 - 0.28f, centerX + 0.2f, this.bounds.y1 - 0.22f), new UI.TextButtonMods() { disabled = response.focused || this.Acronym.value.Length < this.MinLength })) {
			this.canClose = true;
			this.Submit(this.Acronym.value);
			this.Acronym.value = "";
		}
	}

	protected virtual void Submit(string acronym) {
		WorldWindow.Reset();
		WorldWindow.region.offscreenDen = new OffscreenRoom("offscreenden" + acronym.ToLowerInvariant(), "OffscreenDen" + acronym.ToUpperInvariant());
		WorldWindow.region.rooms.Add(WorldWindow.region.offscreenDen);
		WorldWindow.region.acronym = acronym;
		this.Close();
	}
}