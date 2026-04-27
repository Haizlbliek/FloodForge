using FloodForge.Popups;

namespace FloodForge.World;

public class AcronymPopup : Popup {
	protected string? setTo = null;

	protected virtual int MinLength => 1;
	protected virtual string BanLetters => "_/\\ ";

	protected UI.TextInputEditable Acronym;

	public AcronymPopup() {
		this.bounds = new Rect(-0.25f, -0.08f, 0.25f, 0.25f);
		this.Acronym = new UI.TextInputEditable(UI.TextInputEditable.Type.Text, "") { bannedLetters = this.BanLetters };
		this.Acronym.GrabFocus();
	}

	public override void Accept() {
		if (this.Acronym.Focused || this.Acronym.value.Length < this.MinLength)
			return;

		this.Submit(this.Acronym.value);
		this.Acronym.value = "";
		this.Acronym.Submit();
	}

	public override void Close() {
		base.Close();
		UI.Delete(this.Acronym);
	}

	public override void Draw() {
		base.Draw();
		if (this.collapsed) return;

		float centerX = this.bounds.CenterX;

		if (this.setTo != null) {
			this.Acronym.value = this.setTo;
		}

		UI.TextInput(new Rect(this.bounds.x0 + 0.01f, this.bounds.y1 - 0.14f, this.bounds.x1 - 0.01f, this.bounds.y1 - 0.19f), this.Acronym);

		if (UI.TextButton("Cancel", new Rect(centerX - 0.2f, this.bounds.y1 - 0.28f, centerX - 0.05f, this.bounds.y1 - 0.22f))) {
			this.Reject();
			this.Acronym.value = "";
			this.Acronym.Submit();
		}

		if (UI.TextButton("Confirm", new Rect(centerX + 0.05f, this.bounds.y1 - 0.28f, centerX + 0.2f, this.bounds.y1 - 0.22f), new UI.TextButtonMods() { disabled = this.Acronym.value.Length < this.MinLength })) {
			this.Submit(this.Acronym.value);
			this.Acronym.value = "";
			this.Acronym.Submit();
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