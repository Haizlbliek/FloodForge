namespace FloodForge.Popups;

public class ConfirmPopup : Popup {
	protected string[] question;
	protected string okay = "Okay";
	protected string cancel = "Cancel";
	protected bool swap = false;
	protected event Action OnOkay = () => {};
	protected event Action OnCancel = () => {};

	public ConfirmPopup(string text) {
		this.question = text.Split(['\n', '\r'], StringSplitOptions.RemoveEmptyEntries);
		float height = MathF.Max(0.25f, this.question.Length * 0.05f + 0.07f + 0.06f);
		float textWidth = this.question.Length > 0 ? this.question.Max(line => UI.font.Measure(line, 0.04f).x) : 0f;
		float width = MathF.Max(0.4f, textWidth + 0.05f);
		this.bounds = new Rect(width * -0.5f, height * -0.5f, width * 0.5f, height * 0.5f);
	}

	public ConfirmPopup SetOkay(string okay) {
		this.okay = okay;
		return this;
	}

	public ConfirmPopup SetCancel(string cancel) {
		this.cancel = cancel;
		return this;
	}

	public ConfirmPopup Okay(Action callback) {
		this.OnOkay += callback;
		return this;
	}

	public ConfirmPopup Cancel(Action callback) {
		this.OnCancel += callback;
		return this;
	}

	public ConfirmPopup SetButtons(string okay, string cancel) {
		this.okay = okay;
		this.cancel = cancel;
		return this;
	}

	public ConfirmPopup Swap() {
		this.swap = true;
		return this;
	}

	public override void Accept() {
		this.OnOkay();
		base.Accept();
	}

	public override void Reject() {
		this.OnCancel();
		base.Reject();
	}

	public override void Draw() {
		base.Draw();

		if (this.minimized) return;

		Immediate.Color(Themes.Text);

		for (int idx = 0; idx < this.question.Length; idx++) {
			float y = this.bounds.y1 - 0.08f - 0.05f * idx;
			UI.font.Write(this.question[idx], this.bounds.CenterX, y, 0.04f, Font.Align.TopCenter | Font.Align.MiddleLeft);
		}

		Rect left = new Rect(this.bounds.x0 + 0.01f, this.bounds.y0 + 0.01f, this.bounds.CenterX - 0.005f, this.bounds.y0 + 0.06f);
		Rect right = new Rect(this.bounds.CenterX + 0.005f, this.bounds.y0 + 0.01f, this.bounds.x1 - 0.01f, this.bounds.y0 + 0.06f);

		if (UI.TextButton(this.cancel, this.swap ? right : left)) {
			this.Reject();
		}

		if (UI.TextButton(this.okay, this.swap ? left : right)) {
			this.Accept();
		}
	}
}