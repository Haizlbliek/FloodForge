using FloodForge.Popups;
using Silk.NET.Input;
using Stride.Core.Extensions;

namespace FloodForge.World;

// TODO: Resize main section width based on popup size
public class DenPopup : Popup {
	protected const float buttonSize = 1f / 14f;
	protected const float buttonPadding = 0.01f;
	protected const int CreatureRows = 7;

	private float scrollCreatures = 0f;
	private float scrollCreaturesTo = 0f;
	private float scrollTags = 0f;
	private float scrollTagsTo = 0f;
	private float scrollLineages = 0f;
	private float scrollLineagesTo = 0f;
	private int scrollLineagesMax = 0;
	private int scrollTagsMax = 0;
	private readonly Den den;
	private int mouseSection;
	private int selectedCreature;
	private DenCreature? selectedLineage;
	private float editingLineageChance;
	private DenCreature? selectedLineageChance;
	private string hoverText = "";

	private List<CreatureTags.Tag> editableTags = [];
	private readonly List<UI.Editable?> editables = [];

	protected static void CheckTags(DenCreature? creature) {
		if (creature == null)
			return;

		if (creature.type == "") {
			creature.tags.Clear();
			return;
		}

		for (int i = creature.tags.Count - 1; i >= 0; i--) {
			DenCreature.Tag tag = creature.tags[i];

			if (tag.id.Supports(creature.type)) {
				tag.id.displayType.Clamp(tag);
			}
			else {
				creature.tags.Remove(tag);
			}
		}
	}

	protected void UpdateEditables(DenCreature? creature) {
		if (creature == null)
			return;

		this.editableTags = [..CreatureTags.Tags(creature.type)];

		foreach (UI.Editable? editable in this.editables) {
			if (editable == null)
				continue;

			UI.Delete(editable);
		}
		this.editables.Clear();

		foreach (CreatureTags.Tag tag in this.editableTags) {
			DenCreature.Tag? creatureTag = creature.tags.FirstOrDefault(t => t.id == tag);

			if (tag.displayType == CreatureTags.DisplayType.InputString) {
				this.editables.Add(new UI.TextInputEditable(UI.TextInputEditable.Type.Text, (creatureTag as DenCreature.StringTag)?.data ?? ""));
			}
			else if (tag.displayType == CreatureTags.DisplayType.InputUnsignedInteger) {
				this.editables.Add(new UI.TextInputEditable(UI.TextInputEditable.Type.UnsignedInteger, ((creatureTag as DenCreature.IntegerTag)?.data ?? 0).ToString()));
			}
			else if (tag.displayType == CreatureTags.DisplayType.InputSignedInteger) {
				this.editables.Add(new UI.TextInputEditable(UI.TextInputEditable.Type.SignedInteger, ((creatureTag as DenCreature.IntegerTag)?.data ?? 0).ToString()));
			}
			else if (tag.displayType == CreatureTags.DisplayType.InputUnsignedFloat) {
				this.editables.Add(new UI.TextInputEditable(UI.TextInputEditable.Type.UnsignedFloat, ((creatureTag as DenCreature.IntegerTag)?.data ?? 0).ToString()));
			}
			else if (tag.displayType == CreatureTags.DisplayType.InputSignedFloat) {
				this.editables.Add(new UI.TextInputEditable(UI.TextInputEditable.Type.SignedFloat, ((creatureTag as DenCreature.IntegerTag)?.data ?? 0).ToString()));
			}
			else if (tag.displayType is CreatureTags.DisplayType.FloatSlider floatSlider) {
				this.editables.Add(new UI.SliderFloatEditable(floatSlider.min, floatSlider.max));
			}
			else if (tag.displayType is CreatureTags.DisplayType.IntSlider intSlider) {
				this.editables.Add(new UI.SliderIntEditable(intSlider.min, intSlider.max));
			}
			else {
				this.editables.Add(null);
			}
		}
	}

	protected static void SetCreature(DenCreature creature, string creatureType) {
		DenCreature newCreature = new DenCreature(creature);

		if (creatureType == CreatureTextures.CLEAR) {
			newCreature.type = "";
			newCreature.count = 0;
		}
		else if (newCreature.type == creatureType || creatureType == CreatureTextures.UNKNOWN) {
			if (Keys.Modifier(Keys.Modifiers.Shift)) {
				newCreature.count--;
				if (newCreature.count <= 0) {
					newCreature.type = "";
					newCreature.count = 0;
				}
			}
			else {
				newCreature.count++;
			}
		}
		else {
			newCreature.type = creatureType;
			newCreature.count = 1;
		}

		CheckTags(newCreature);
		History.Apply(new CreatureDataChange(creature, newCreature.type, newCreature.count, newCreature.tags));
	}

	protected static void ToggleTag(DenCreature creature, CreatureTags.Tag creatureTag) {
		DenCreature newCreature = new DenCreature(creature);
		if (newCreature.tags.Any(x => x.id == creatureTag)) {
			newCreature.tags.Remove(newCreature.tags.First(x => x.id == creatureTag));
		}
		else {
			newCreature.tags.Add(DenCreature.CreateFrom(creatureTag));
		}
		CheckTags(newCreature);
		History.Apply(new CreatureDataChange(creature, newCreature.type, newCreature.count, newCreature.tags));
	}

	public DenPopup(Den den) {
		this.den = den;
		this.bounds = new Rect(-0.35f, -0.35f, 0.475f, 0.35f);
		Main.Scroll += this.Scroll;
		Main.KeyPress += this.KeyPress;
		this.selectedCreature = 0;
		if (this.den.creatures.Count == 0) {
			History.Apply(new LineageChange(this.den));
		}
		this.selectedLineage = this.den.creatures[0];
		this.selectedLineageChance = null;
		CheckTags(this.selectedLineage);
		this.UpdateEditables(this.selectedLineage);
	}

	protected void DrawCreatures(float mainX, bool unknown, DenCreature? creature) {
		float centerX = mainX + 0.305f;

		Program.gl.Disable(EnableCap.ScissorTest);
		Immediate.Color(Themes.Text);
		UI.font.Write(this.selectedLineage == null ? "No lineages" : "Creature type:", centerX, this.bounds.y1 - 0.07f, 0.035f, Font.Align.TopCenter);
		Program.gl.Enable(EnableCap.ScissorTest);

		if (this.selectedLineage == null) return;
		if (creature == null) return;

		int count = CreatureTextures.creatureOrder.Count;
		if (!unknown) count--;

		float countX = 0f;
		float countY = 0f;
		for (int y = 0; y <= (count / CreatureRows); y++) {
			for (int x = 0; x < CreatureRows; x++) {
				int id = x + y * CreatureRows;
				if (id >= count) break;

				string type = CreatureTextures.creatureOrder[id];
				bool selected = creature.type == type || (unknown && type == CreatureTextures.UNKNOWN);
				UVRect rect = UVRect.FromSize(
					centerX + (x - 0.5f * CreatureRows) * (buttonSize + buttonPadding) + buttonPadding * 0.5f,
					this.bounds.y1 - 0.1f - buttonPadding * 0.5f - (y + 1) * (buttonSize + buttonPadding) - this.scrollCreatures,
					buttonSize, buttonSize
				);
				Texture texture = CreatureTextures.GetTexture(type);
				UI.CenteredUV(texture, ref rect);
				UI.ButtonResponse response = UI.TextureButton(rect, new UI.TextureButtonMods { selected = selected, texture = texture, textureColor = selected ? Color.White : Color.Grey });

				if (response.clicked) {
					SetCreature(creature, type);
					CheckTags(creature);
					this.UpdateEditables(creature);
					selected = true;
				}

				if (response.hovered) {
					this.hoverText = (unknown && type == CreatureTextures.UNKNOWN) ? creature.type : CreatureTextures.ExportName(type);
				}

				if (selected) {
					countX = rect.x1;
					countY = rect.y0;
				}
			}
		}

		if (this.selectedCreature >= 0 && this.selectedCreature <= this.den.creatures.Count && !creature.type.IsNullOrEmpty() && this.den.creatures[this.selectedCreature].lineageTo == null) {
			Immediate.Color(Themes.Text);
			UI.font.Write(creature.count.ToString(), countX, countY, 0.04f, Font.Align.MiddleCenter);
		}
	}

	protected void DrawTags(float mainX, DenCreature? creature) {
		Program.gl.Disable(EnableCap.ScissorTest);
		Immediate.Color(Themes.Text);
		UI.font.Write("Tags:", mainX + 0.6f + 0.125f, this.bounds.y1 - 0.07f, 0.035f, Font.Align.TopCenter);
		Program.gl.Enable(EnableCap.ScissorTest);

		Immediate.Color(this.hovered ? Themes.BorderHighlight : Themes.Border);
		UI.Line(mainX + 0.6f, this.bounds.y0, mainX + 0.6f, this.bounds.y1);

		if (creature == null) return;

		int y = 0;
		foreach (CreatureTags.Tag tag in this.editableTags) {
			bool selected = creature.tags.Any(t => t.id == tag);
			UVRect rect = UVRect.FromSize(
				mainX + 0.6f + buttonPadding,
				this.bounds.y1 - 0.1f - buttonPadding * 0.5f - (y + 1) * (buttonSize + buttonPadding) - this.scrollTags,
				buttonSize,
				buttonSize
			);

			UI.ButtonResponse response = UI.TextureButton(rect, new UI.TextureButtonMods { selected = selected, texture = CreatureTextures.GetTexture(tag.id, false), textureColor = selected ? Color.White : Color.Grey});
			if (response.clicked) {
				ToggleTag(creature, tag);
				this.UpdateEditables(creature);
			}

			if (response.hovered)
				this.hoverText = tag.id;

			UI.Editable? editable = this.editables[y];
			DenCreature.Tag? creatureTag = creature.tags.FirstOrDefault(t => t.id == tag);

			if (editable == null) {
			}
			else if (editable is UI.TextInputEditable textInput) {
				UI.TextInputResponse inputResponse = UI.TextInput(Rect.FromSize(rect.x1 + buttonPadding, rect.CenterY - 0.025f, 0.2f, 0.05f), textInput, new UI.TextInputMods() { disabled = !selected });
				if (inputResponse.submitted) {
					if (creatureTag is DenCreature.StringTag stringTag) {
						stringTag.data = textInput.value;
					}
					else if (creatureTag is DenCreature.IntegerTag intTag && int.TryParse(textInput.value, out int value)) {
						intTag.data = value;
					}
					else if (creatureTag is DenCreature.FloatTag floatTag && float.TryParse(textInput.value, out float value2)) {
						floatTag.data = value2;
					}
				}
			}
			else if (editable is UI.SliderIntEditable intEditable && creatureTag is DenCreature.IntegerTag intTag) {
				UI.SliderResponse slider = UI.Slider(Rect.FromSize(rect.x1 + buttonPadding, rect.CenterY - 0.025f, 0.2f, 0.05f), intEditable, ref intTag.data, new UI.SliderMods() { disabled = !selected });
				Immediate.Color(Themes.Text);
				bool swap = slider.sliderPos.x > rect.x1 + 0.1f + buttonPadding;
				float x = slider.sliderPos.x + (swap ? -0.01f : 0.01f);
				UI.font.Write($"{intTag.data}", x, slider.sliderPos.y, 0.03f, swap ? Font.Align.MiddleRight : Font.Align.MiddleLeft);
			}
			else if (editable is UI.SliderFloatEditable floatEditable && creatureTag is DenCreature.FloatTag floatTag) {
				UI.SliderResponse slider = UI.Slider(Rect.FromSize(rect.x1 + buttonPadding, rect.CenterY - 0.025f, 0.2f, 0.05f), floatEditable, ref floatTag.data, new UI.SliderMods() { disabled = !selected });
				Immediate.Color(Themes.Text);
				bool swap = slider.sliderPos.x > rect.x1 + 0.1f + buttonPadding;
				float x = slider.sliderPos.x + (swap ? -0.01f : 0.01f);
				UI.font.Write($"{floatTag.data:F3}", x, slider.sliderPos.y, 0.03f, swap ? Font.Align.MiddleRight : Font.Align.MiddleLeft);
			}

			y++;
		}
		this.scrollTagsMax = y;
	}

	protected void DrawLineages() {
		this.scrollLineagesMax = 0;

		Program.gl.Disable(EnableCap.ScissorTest);
		Immediate.Color(Themes.Text);
		UI.font.Write("Lineages:", this.bounds.x0 + 0.11f, this.bounds.y1 - 0.07f, 0.035f, Font.Align.TopCenter);
		Program.gl.Enable(EnableCap.ScissorTest);

		Immediate.Color(this.hovered ? Themes.BorderHighlight : Themes.Border);
		UI.Line(this.bounds.x0 + 0.22f, this.bounds.y0, this.bounds.x0 + 0.22f, this.bounds.y1);

		float dotsCenterX = this.bounds.x0 + 0.11f - (this.den.creatures.Count - 1) * 0.015f;
		float dotsCenterY = this.bounds.y1 - 0.13f;
		for (int i = 0; i < this.den.creatures.Count; i++) {
			float x = dotsCenterX + i * 0.03f;
			if (i == this.selectedCreature) {
				Immediate.Color(Themes.BorderHighlight);
				UI.FillRect(x - 0.01f, dotsCenterY - 0.01f, x + 0.01f, dotsCenterY + 0.01f);
			}
			else {
				Immediate.Color(Themes.Border);
				UI.FillRect(x - 0.0075f, dotsCenterY - 0.0075f, x + 0.0075f, dotsCenterY + 0.0075f);
			}
		}

		// Left
		if (UI.TextureButton(
			UVRect.FromSize(this.bounds.x0 + 0.01f, this.bounds.y1 - 0.19f, 0.04f, 0.04f).UV(0.5f, 0.5f, 0.75f, 0.75f),
			new UI.TextureButtonMods { disabled = this.selectedCreature == 0, textureColor = (this.selectedCreature <= 0) ? Color.Grey : Color.White }
		)) {
			this.selectedCreature--;
			if (this.selectedCreature < 0) {
				this.selectedCreature = this.den.creatures.Count;
			}
			this.selectedLineage = this.den.creatures.Count == 0 ? null : this.den.creatures[this.selectedCreature];
			CheckTags(this.selectedLineage);
			this.UpdateEditables(this.selectedLineage);
		}

		// Delete
		if (UI.TextureButton(
			UVRect.FromSize(this.bounds.x0 + 0.06f, this.bounds.y1 - 0.19f, 0.04f, 0.04f).UV(0.0f, 0.0f, 0.25f, 0.25f),
			new UI.TextureButtonMods { disabled = this.den.creatures.Count == 0, textureColor = this.den.creatures.Count == 0 ? Color.Grey : Color.White }
		)) {
			History.Apply(new LineageChange(this.den, this.selectedCreature));

			this.selectedCreature--;
			if (this.selectedCreature < 0) {
				this.selectedCreature = 0;
			}
			this.selectedLineage = this.den.creatures.Count == 0 ? null : this.den.creatures[this.selectedCreature];
			CheckTags(this.selectedLineage);
			this.UpdateEditables(this.selectedLineage);
		}

		// Add
		if (UI.TextureButton(
			UVRect.FromSize(this.bounds.x0 + 0.11f, this.bounds.y1 - 0.19f, 0.04f, 0.04f).UV(0.25f, 0.5f, 0.5f, 0.75f)
		)) {
			History.Apply(new LineageChange(this.den));

			this.selectedCreature = this.den.creatures.Count - 1;
			this.selectedLineage = this.den.creatures[this.selectedCreature];
			CheckTags(this.selectedLineage);
			this.UpdateEditables(this.selectedLineage);
		}

		// Right
		if (UI.TextureButton(
			UVRect.FromSize(this.bounds.x0 + 0.16f, this.bounds.y1 - 0.19f, 0.04f, 0.04f).UV(0.75f, 0.5f, 1f, 0.75f),
			new UI.TextureButtonMods { disabled = this.selectedCreature >= this.den.creatures.Count - 1, textureColor = (this.selectedCreature >= this.den.creatures.Count - 1) ? Color.Grey : Color.White }
		)) {
			this.selectedCreature++;
			if (this.selectedCreature >= this.den.creatures.Count) {
				this.selectedCreature = 0;
			}
			this.selectedLineage = this.den.creatures[this.selectedCreature];
			CheckTags(this.selectedLineage);
			this.UpdateEditables(this.selectedLineage);
		}

		float clipBottom = (this.bounds.y0 + 0.01f + buttonPadding + Main.screenBounds.y) * 0.5f * Program.window.FramebufferSize.Y;
		float clipTop = (this.bounds.y1 - 0.185f - buttonPadding + Main.screenBounds.y) * 0.5f * Program.window.FramebufferSize.Y;
		// LATER Make/use UI.ClipGl
		Program.gl.Scissor(0, (int) clipBottom, (uint) Program.window.FramebufferSize.X, (uint) (clipTop - clipBottom));
		UI.Clip(new Rect(float.NegativeInfinity, this.bounds.y0 + 0.01f + buttonPadding, float.PositiveInfinity, this.bounds.y1 - 0.185f));

		if (this.den.creatures.Count == 0 || this.selectedCreature == -1) return;

		DenCreature? lastCreature = null;
		DenCreature creature = this.den.creatures[this.selectedCreature];
		int j = 0;
		while (true) {
			this.scrollLineagesMax = Math.Max(this.scrollLineagesMax, j);
			UVRect creatureRect = UVRect.FromSize(this.bounds.x0 + 0.01f, this.bounds.y1 - this.scrollLineages - 0.19f - (j + 1) * (buttonSize + buttonPadding), buttonSize, buttonSize);
			bool selected = creature == this.selectedLineage;

			UI.ButtonResponse response;

			Texture texture = CreatureTextures.GetTexture(creature.type);
			if (texture == CreatureTextures.UnknownCreature) {
				response = UI.Button(new Rect(creatureRect), new UI.ButtonMods { selected = selected });
			}
			else {
				UI.CenteredUV(texture, ref creatureRect);
				response = UI.TextureButton(creatureRect, new UI.TextureButtonMods { texture = texture, selected = selected, textureColor = selected ? Color.White : Color.Grey });
			}

			if (response.clicked) {
				this.selectedLineage = creature;
				CheckTags(this.selectedLineage);
				this.UpdateEditables(this.selectedLineage);
			}

			if (creature.lineageTo != null) {
				Rect inputRect = Rect.FromSize(creatureRect.x0 + buttonSize + buttonPadding, creatureRect.y0 - (buttonPadding + buttonSize * 0.5f) * 0.5f, buttonSize, buttonSize * 0.5f);
				bool lineageSelected = creature == this.selectedLineageChance;
				float chance = (lineageSelected ? this.editingLineageChance : creature.lineageChance) * 100f;
				string text = lineageSelected ? $"{chance:0.#}" : $"{chance:0.#}%";

				if (UI.TextButton(text, inputRect, new UI.TextButtonMods { selected = lineageSelected })) {
					lineageSelected = !lineageSelected;
					if (lineageSelected) {
						this.selectedLineageChance = creature;
						this.editingLineageChance = this.selectedLineageChance.lineageChance;
					}
					else {
						this.SubmitChance();
					}
				}
			}

			if (UI.TextureButton(
				UVRect.FromSize(creatureRect.x0 + buttonSize + buttonPadding, creatureRect.y0 + buttonSize * 0.25f, buttonSize * 0.5f, buttonSize * 0.5f).UV(0f, 0f, 0.25f, 0.25f)
			)) {
				if (lastCreature == null) {
					if (creature.lineageTo == null) {
						History.Apply(new CreatureDataChange(creature, "", 0, []));
					}
					else {
						if (this.selectedLineage == creature.lineageTo) {
							this.selectedLineage = creature;
						}
						History.Apply(new CreatureDeleteChange(creature, lastCreature));
					}
				}
				else {
					if (this.selectedLineage == creature) {
						this.selectedLineage = lastCreature;
					}
					History.Apply(new CreatureDeleteChange(creature, lastCreature));
				}
			}

			lastCreature = creature;
			j++;
			if (creature.lineageTo == null) {
				break;
			}
			creature = creature.lineageTo;
		}

		if (UI.TextureButton(
			UVRect.FromSize(this.bounds.x0 + 0.01f, this.bounds.y1 - 0.19f - this.scrollLineages - (j + 1) * (buttonSize + buttonPadding), buttonSize, buttonSize).UV(0.25f, 0.5f, 0.5f, 0.75f)
		)) {
			History.Apply(new CreatureLineageChange(creature));
			this.selectedLineage = creature.lineageTo;
		}

		UI.ButtonResponse response1 = UI.TextureButton(
			UVRect.FromSize(this.bounds.x0 + 0.01f + buttonSize + buttonPadding, this.bounds.y1 - 0.19f - this.scrollLineages - (j + 1) * (buttonSize + buttonPadding), buttonSize, buttonSize).UV(0.75f, 0f, 1f, 0.25f)
		);
		if (response1.hovered) {
			this.hoverText = "Edit conditionals";
		}
		if (response1.clicked) {
			this.den.creatures[this.selectedCreature].conditionalPopup = PopupManager.Add(new ConditionalPopup(this.den.creatures[this.selectedCreature])) as ConditionalPopup;
		}
	}

	public override void Draw() {
		this.hoverText = "";
		this.mouseSection = (Mouse.X > (this.bounds.x0 + 0.6f + 0.22f)) ? 2 : (Mouse.X > this.bounds.x0 + 0.22f) ? 1 : 0;

		int lastSelectedCreature = this.selectedCreature;
		this.selectedCreature = Math.Clamp(this.selectedCreature, 0, Math.Max(this.den.creatures.Count - 1, 0));
		if (this.selectedCreature != lastSelectedCreature) {
			this.selectedLineage = this.den.creatures.Count == 0 ? null : this.den.creatures[this.selectedCreature];
		}
		else if (this.den.creatures.Count == 0) {
			this.selectedLineage = null;
		}
		else {
			this.selectedLineage ??= this.den.creatures[this.selectedCreature];
		}

		DenCreature? creature = this.selectedLineage;
		bool unknown = creature != null && !CreatureTextures.Known(creature.type);

		this.minimumBounds.x1 = this.minimumBounds.x0 + 0.6f + 0.3f + 0.22f;
		if (this.bounds.x1 - this.bounds.x0 < this.minimumBounds.x1 - this.minimumBounds.x0) {
			this.bounds.x1 = this.bounds.x0 + this.minimumBounds.x1 - this.minimumBounds.x0;
		}

		base.Draw();

		if (this.minimized) return;

		this.scrollLineages += (this.scrollLineagesTo - this.scrollLineages) * (1f - MathF.Pow(1f - Settings.PopupScrollSpeed, Program.Delta * 60f));
		this.scrollCreatures += (this.scrollCreaturesTo - this.scrollCreatures) * (1f - MathF.Pow(1f - Settings.PopupScrollSpeed, Program.Delta * 60f));
		this.scrollTags += (this.scrollTagsTo - this.scrollTags) * (1f - MathF.Pow(1f - Settings.PopupScrollSpeed, Program.Delta * 60f));

		float mainX = this.bounds.x0 + 0.22f;
		Program.gl.Enable(EnableCap.ScissorTest);
		float clipBottom = (this.bounds.y0 + 0.01f + buttonPadding + Main.screenBounds.y) * 0.5f * Program.window.FramebufferSize.Y;
		float clipTop = (this.bounds.y1 - 0.1f - buttonPadding + Main.screenBounds.y) * 0.5f * Program.window.FramebufferSize.Y;
		Program.gl.Scissor(0, (int) clipBottom, (uint) Program.window.FramebufferSize.X, (uint) (clipTop - clipBottom));
		UI.Clip(new Rect(float.NegativeInfinity, this.bounds.y0 + 0.01f + buttonPadding, float.PositiveInfinity, this.bounds.y1 - 0.1f));

		this.DrawCreatures(mainX, unknown, creature);
		this.DrawTags(mainX, creature);
		this.DrawLineages();

		Program.gl.Disable(EnableCap.ScissorTest);
		UI.ClearClip();

		// if (this.hasSlider && this.mouseClickSlider && creature != null) {
		// 	if (!Mouse.Left) {
		// 		this.mouseClickSlider = false;
		// 	}

		// 	float P = Mathf.Clamp01((Mouse.Y - this.bounds.y0 - 0.075f) / (this.bounds.y1 - this.bounds.y0 - 0.2f));
		// 	float val = P * (this.sliderMax - this.sliderMin) + this.sliderMin;
		// 	if (!this.sliderFloat) val = MathF.Round(val);

		// 	if (!this.lastMouseClickSlider || History.Last is not CreatureDataChange change) {
		// 		History.Apply(new CreatureDataChange(creature, creature.type, creature.count, creature.tag, val));
		// 	}
		// 	else {
		// 		creature.data = val;
		// 		change.redoData = val;
		// 	}
		// }

		if (!this.hoverText.IsNullOrEmpty() && this.hovered) {
			float width = UI.font.Measure(this.hoverText, 0.04f).x + 0.02f;
			Rect rect = Rect.FromSize(Mouse.X, Mouse.Y, width, 0.06f);
			Immediate.Color(Themes.Popup);
			UI.FillRect(rect);
			Immediate.Color(Themes.Border);
			UI.StrokeRect(rect);
			Immediate.Color(Themes.Text);
			UI.font.Write(this.hoverText, Mouse.X + 0.01f, Mouse.Y + 0.03f, 0.04f, Font.Align.MiddleLeft);
		}
	}

	public override void Accept() {}

	public override void Close() {
		if (this.selectedLineageChance != null) {
			this.selectedLineageChance = null;
			return;
		}

		base.Close();
		Main.Scroll -= this.Scroll;
		Main.KeyPress -= this.KeyPress;
	}

	protected void ClampScroll() {
		{
			int count = Mathf.CeilToInt(CreatureTextures.creatureOrder.Count / (float) CreatureRows) - 2;
			float size = count * (buttonSize + buttonPadding);
			if (this.scrollCreaturesTo < -size) {
				this.scrollCreaturesTo = -size;
				if (this.scrollCreatures <= -size + 0.06f) {
					this.scrollCreatures = -size - 0.03f;
				}
			}

			if (this.scrollCreaturesTo > 0) {
				this.scrollCreaturesTo = 0;
				if (this.scrollCreatures >= -0.06f) {
					this.scrollCreatures = 0.03f;
				}
			}
		}

		{
			int count = this.scrollTagsMax - 2;
			float size = count * (buttonSize + buttonPadding);
			if (this.scrollTagsTo < -size) {
				this.scrollTagsTo = -size;
				if (this.scrollTags <= -size + 0.06f) {
					this.scrollTags = -size - 0.03f;
				}
			}

			if (this.scrollTagsTo > 0) {
				this.scrollTagsTo = 0;
				if (this.scrollTags >= -0.06f) {
					this.scrollTags = 0.03f;
				}
			}
		}

		{
			int count = this.scrollLineagesMax + 1;
			float size = count * (buttonSize + buttonPadding);
			if (this.scrollLineagesTo < -size) {
				this.scrollLineagesTo = -size;
				if (this.scrollLineages <= -size + 0.06f) {
					this.scrollLineages = -size - 0.03f;
				}
			}

			if (this.scrollLineagesTo > 0) {
				this.scrollLineagesTo = 0;
				if (this.scrollLineages >= -0.06f) {
					this.scrollLineages = 0.03f;
				}
			}
		}
	}

	protected void Scroll(float deltaX, float deltaY) {
		if (!this.hovered || this.minimized) return;

		if (this.mouseSection == 0) {
			this.scrollLineagesTo += deltaY * 0.06f;
		}
		else if (this.mouseSection == 1) {
			this.scrollCreaturesTo += deltaY * 0.06f;
		}
		else {
			this.scrollTagsTo += deltaY * 0.06f;
		}

		this.ClampScroll();
	}

	protected void SubmitChance() {
		if (this.selectedLineageChance == null) return;

		History.Apply(new CreatureLineageChange(this.selectedLineageChance, Mathf.Clamp(this.editingLineageChance, 0f, 1f)));
		this.selectedLineageChance = null;
	}

	protected void KeyPress(Key key) {
		if (this.minimized || !this.hovered) return;
		if (this.selectedLineageChance == null) return;

		int chance = Mathf.FloorToInt(this.editingLineageChance * 100f);
		if (key >= Key.Number0 && key <= Key.Number9) {
			int number = (int) key - (int) Key.Number0;
			if (chance > 99) return;
			if (chance == 0) {
				chance = number;
			}
			else {
				chance = (chance * 10) + number;
			}
		}
		else if (key == Key.Enter) {
			this.SubmitChance();
			return;
		}
		else if (key == Key.Backspace) {
			if (chance > 9) {
				chance /= 10;
			}
			else {
				chance = 0;
			}
		}
		this.editingLineageChance = chance / 100f;
	}
}