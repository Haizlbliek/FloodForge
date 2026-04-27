using FloodForge.Popups;
using FloodForge.History;
using Stride.Core.Extensions;

namespace FloodForge.World;

public class RoomAttractivenessPopup : Popup {
	protected const float buttonSize = 1f / 14f;
	protected const float buttonPadding = 0.01f;
	protected const int CreatureRows = 7;
	public static readonly RoomAttractiveness[] AttractivenessIds = [ RoomAttractiveness.Default, RoomAttractiveness.Neutral, RoomAttractiveness.Forbidden, RoomAttractiveness.Avoid, RoomAttractiveness.Like, RoomAttractiveness.Stay ];
	public static readonly Color[] AttractivenessColors = [ new Color(0.5f, 0.5f, 0.5f), new Color(1f, 1f, 1f), new Color(1f, 0f, 0f), new Color(1f, 1f, 0f), new Color(0f, 1f, 0f), new Color(0f, 1f, 1f) ];
	public static readonly string[] AttractivenessNames = [ "DEFAULT", "NEUTRAL", "FORBIDDEN", "AVOID", "LIKE", "STAY" ];

	protected HashSet<Room> rooms;
	protected RoomAttractiveness selectedAttractiveness = RoomAttractiveness.Neutral;
	protected float scroll;
	protected float targetScroll;

	public RoomAttractivenessPopup(IEnumerable<Room> rooms) {
		this.rooms = [ ..rooms ];
		this.bounds = new Rect(-0.35f, -0.35f, 0.475f, 0.35f);

		Main.Scroll += this.Scroll;
	}

	public override void Close() {
		base.Close();

		Main.Scroll -= this.Scroll;
	}

	public override void Draw() {
		base.Draw();

		if (this.minimized) return;

		Immediate.Color(this.hovered ? Themes.BorderHighlight : Themes.Border);
		UI.Line(this.bounds.x0 + 0.6f, this.bounds.y0, this.bounds.x0 + 0.6f, this.bounds.y1);

		this.scroll += (this.targetScroll - this.scroll) * (1f - MathF.Pow(1f - Settings.PopupScrollSpeed, Program.Delta * 60f));

		float centerX = this.bounds.x0 + 0.305f;
		Immediate.Color(Themes.Text);
		UI.font.Write("Creature type:", centerX, this.bounds.y1 - 0.07f, 0.035f, Font.Align.TopCenter);
		UI.font.Write("Attract:", this.bounds.x0 + 0.72f, this.bounds.y1 - 0.07f, 0.035f, Font.Align.TopCenter);

		string hover = "";

		Program.gl.Enable(EnableCap.ScissorTest);
		float clipBottom = (this.bounds.y0 + 0.01f + buttonPadding + Main.screenBounds.y) * 0.5f * Program.window.FramebufferSize.Y;
		float clipTop = (this.bounds.y1 - 0.1f - buttonPadding + Main.screenBounds.y) * 0.5f * Program.window.FramebufferSize.Y;
		Program.gl.Scissor(0, (int) clipBottom, (uint) Program.window.FramebufferSize.X, (uint) (clipTop - clipBottom));
		UI.Clip(new Rect(float.NegativeInfinity, this.bounds.y0 + 0.01f + buttonPadding, float.PositiveInfinity, this.bounds.y1 - 0.1f));
		int countA = CreatureTextures.creatures.Count - 2;
		Room? room = this.rooms.FirstOrDefault();

		for (int y = 0; y <= (countA / CreatureRows); y++) {
			for (int x = 0; x < CreatureRows; x++) {
				int id = x + y * CreatureRows + 1;
				if (id >= countA) break;

				string type = CreatureTextures.creatures[id];
				float rectX = centerX + (x - 0.5f * CreatureRows) * (buttonSize + buttonPadding) + buttonPadding * 0.5f;
				float rectY = this.bounds.y1 - 0.1f - buttonPadding * 0.5f - (y + 1) * (buttonSize + buttonPadding) - this.scroll;
				UVRect rect = UVRect.FromSize(rectX, rectY, buttonSize, buttonSize);
				Texture texture = CreatureTextures.GetTexture(CreatureTextures.creatures[id]);
				UI.CenteredUV(texture, ref rect);

				UI.ButtonResponse response = UI.TextureButton(rect, new UI.TextureButtonMods { texture = texture });
				if (response.hovered) hover = type;
				if (response.clicked) this.SetAllTo(this.selectedAttractiveness, type);

				Color color;
				bool inherit = false;
				if (room == null || !room.data.attractiveness.ContainsKey(type)) {
					inherit = WorldWindow.region.defaultAttractiveness.ContainsKey(type);
					color = AttractivenessColors[inherit ? (int) WorldWindow.region.defaultAttractiveness[type] : 0];
				}
				else {
					color = AttractivenessColors[(int) room.data.attractiveness[type]];
				}
				Immediate.Color(color);
				if (inherit && room != null) {
					UI.FillRect(rectX - 0.01f, rectY - 0.01f, rectX + 0.02f, rectY + 0.02f);
					Immediate.Color(AttractivenessColors[0]);
				}
				UI.FillRect(rectX - 0.005f, rectY - 0.005f, rectX + 0.015f, rectY + 0.015f);
			}
		}

		Program.gl.Disable(EnableCap.ScissorTest);
		UI.ClearClip();

		for (int i = 0; i < 6; i++) {
			float y = this.bounds.y1 - 0.165f - i * 0.09f;
			Rect rect = new Rect(this.bounds.x0 + 0.61f, y - 0.02f, this.bounds.x1 - 0.01f, y + 0.02f);
			if (UI.TextButton(AttractivenessNames[i], rect, new UI.TextButtonMods { selected = this.selectedAttractiveness == AttractivenessIds[i] })) {
				this.selectedAttractiveness = AttractivenessIds[i];
			}
		}

		if (!hover.IsNullOrEmpty() && this.hovered) {
			float width = UI.font.Measure(hover, 0.04f).x + 0.02f;
			Rect rect = Rect.FromSize(Mouse.X, Mouse.Y, width, 0.06f);
			Immediate.Color(Themes.Popup);
			UI.FillRect(rect);
			Immediate.Color(Themes.Border);
			UI.StrokeRect(rect);
			Immediate.Color(Themes.Text);
			UI.font.Write(hover, Mouse.X + 0.01f, Mouse.Y + 0.03f, 0.04f, Font.Align.MiddleLeft);
		}
	}

	protected void SetAllTo(RoomAttractiveness attr, string creature) {
		AttractivenessChange change = new AttractivenessChange(attr, creature);
		this.rooms.ForEach(change.AddRoom);
		WorldWindow.worldHistory.Apply(change);
	}

	protected void ClampScroll() {
		int items = CreatureTextures.creatures.Count / CreatureRows - 1;
		float size = items * (buttonSize + buttonPadding);

		if (this.targetScroll < -size) {
			this.targetScroll = -size;
			if (this.scroll <= -size + 0.06f) {
				this.scroll = -size - 0.03f;
			}
		}

		if (this.targetScroll > 0f) {
			this.targetScroll = 0f;
			if (this.scroll >= -0.06f) {
				this.scroll = 0.03f;
			}
		}
	}

	protected void Scroll(float deltaX, float deltaY) {
		if (!this.hovered || this.minimized) return;

		this.targetScroll += deltaY * 0.06f;
		this.ClampScroll();
	}
}