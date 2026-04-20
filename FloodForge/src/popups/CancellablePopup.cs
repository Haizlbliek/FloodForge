namespace FloodForge.Popups;

public class CancellablePopup : Popup {
	protected string[] text;

    protected string cancel = "Cancel";
    protected bool closeOnCancel = true;
    protected bool hideButton = false;
    protected event Action<CancellablePopup> OnCancel = (CancellablePopup) => {};

    public CancellablePopup(string text) {
		this.text = text.Split(['\n', '\r'], StringSplitOptions.RemoveEmptyEntries);
		this.UpdateText(text);
    }

    public void UpdateText(string text = "") {
        if(text != "")
		    this.text = text.Split(['\n', '\r'], StringSplitOptions.RemoveEmptyEntries);
		float height = MathF.Max(0.2f, this.text.Length * 0.05f + 0.07f + (this.hideButton ? 0f : 0.06f));
		float textWidth = this.text.Length > 0 ? this.text.Max(line => UI.font.Measure(line, 0.04f).x) : 0f;
		float width = MathF.Max(0.4f, textWidth + 0.05f);
		this.bounds = new Rect(width * -0.5f + this.bounds.CenterX, height * -0.5f + this.bounds.CenterY, width * 0.5f + this.bounds.CenterX, height * 0.5f + this.bounds.CenterY);
	}

	public string GetText() {
		string returnString = "";
		foreach(string item in this.text) {
			if(returnString != "") {
				returnString += "\n";
			}
			returnString += item;
		}
		return returnString;
	}

	public override void Draw() {
        Rect buttonRect = new Rect(this.bounds.x0 + 0.01f, this.bounds.y0 + 0.01f, this.bounds.x1 - 0.01f, this.bounds.y0 + 0.06f);
		if (!this.minimized && buttonRect.Inside(Mouse.Pos) &! this.hideButton) {
            this.cursorOverButton = true;
        }

		base.Draw();
        
		if (this.minimized) return;
		
		for (int idx = 0; idx < this.text.Length; idx++) {
			string lineToWrite = this.text[idx];
			float y = this.bounds.y1 - 0.08f - 0.05f * idx;
			WriteText(lineToWrite, this.bounds.CenterX, y, 0.04f, Font.Align.TopCenter | Font.Align.MiddleLeft);
		}

		if (!this.hideButton && UI.TextButton(this.cancel, buttonRect)) {
			this.Accept();
		}
	}

    public CancellablePopup SetCancel (string cancel) {
        this.cancel = cancel;
        return this;
    }

    public CancellablePopup Cancel (Action<CancellablePopup> callback) {
        this.OnCancel += callback;
        return this;
    }

    public CancellablePopup CloseOnCancel (bool closeOnCancel) {
        this.closeOnCancel = closeOnCancel;
        return this;
    }

    public CancellablePopup HideButton (bool hideButton) {
        this.hideButton = hideButton;
        this.UpdateText();
        return this;
    }

	public override void Accept() {
		if(this.closeOnCancel) base.Accept();
        else this.OnCancel(this);
	}
}