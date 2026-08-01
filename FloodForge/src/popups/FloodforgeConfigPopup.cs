using Silk.NET.GLFW;
using static FloodForge.UI;

namespace FloodForge.Popups;

public class FloodforgeConfigPopup : Popup {
	protected float scroll;
	protected float targetScroll;
	protected float maxScroll;

	public FloodforgeConfigPopup() {
		this.popupTitle = "Floodforge Settings - assets/settings.cfg";
		this.bounds = Rect.FromSize(new (-0.4f, -0.5f), new (0.8f, 1f));
	}

	public override void Open() {
		Main.Scroll += this.Scroll;
	}

	private readonly SettingEditableReferrer<float, SliderFloatEditable> CameraPanSpeed = new (Settings.CameraPanSpeed, new (0, 1));
	private readonly SettingEditableReferrer<float, SliderFloatEditable> CameraZoomSpeed = new (Settings.CameraZoomSpeed, new (0, 1));
	private readonly SettingEditableReferrer<float, SliderFloatEditable> PopupScrollSpeed = new (Settings.PopupScrollSpeed, new (0, 1));
	// ToAdd: ConnectionType
	// ToAdd: ConnectionPoint
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> OriginalControls = new (Settings.OriginalControls, new (Settings.OriginalControls.value));
	private readonly SettingEditableReferrer<float, TextInputEditable> WorldIconScale = new (Settings.WorldIconScale, new (TextInputEditable.Type.SignedFloat, Settings.WorldIconScale.value.ToString()));
	private readonly SettingEditableReferrer<string, TextInputEditable> DefaultFilePath = new (Settings.DefaultFilePath, new (TextInputEditable.Type.Text, Settings.DefaultFilePath.value));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> KeepFilesystemPath = new (Settings.KeepFilesystemPath, new (Settings.KeepFilesystemPath));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> WarnMissingImages = new (Settings.WarnMissingImages, new (Settings.WarnMissingImages));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> HideTutorial = new (Settings.HideTutorial, new (Settings.HideTutorial));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> HideTutorialOnLoadWorld = new (Settings.HideTutorialOnLoadWorld, new (Settings.HideTutorialOnLoadWorld));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> UpdateWorldFiles = new (Settings.UpdateWorldFiles, new (Settings.UpdateWorldFiles));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> UpdateRoomImagesOnRender = new (Settings.UpdateRoomImagesOnRender, new (Settings.UpdateRoomImagesOnRender));
	// ToAdd: NoSubregionColor
	private readonly SettingEditableReferrer<float, SliderFloatEditable> RoomTintStrength = new (Settings.RoomTintStrength, new (0, 1));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> DropdownOnHover = new (Settings.DropdownOnHover, new (Settings.DropdownOnHover));
	// ToAdd: DisabledButtonsMode
	// ToAdd: ForceExportCasing
	// ToAdd: DropletGridVisibility
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> DropletKeepRelativePosition = new (Settings.DropletKeepRelativePosition, new (Settings.DropletKeepRelativePosition));
	private readonly SettingEditableReferrer<float, SliderFloatEditable> ConnectionOpacity = new (Settings.ConnectionOpacity, new (0, 1));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> DisableAprilFoolsUpdates = new (Settings.DisableAprilFoolsUpdates, new (Settings.DisableAprilFoolsUpdates.value));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> DiscordRichPresence = new (Settings.DiscordRichPresence, new (Settings.DiscordRichPresence.value));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> RoundedUI = new (Settings.RoundedUI, new (Settings.RoundedUI.value));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> DisableUpdater = new (Settings.DisableUpdater, new (Settings.DisableUpdater.value));
	private readonly SettingEditableReferrer<string, TextInputEditable> RainedPath = new (Settings.RainedPath, new (TextInputEditable.Type.Text, Settings.RainedPath.value));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> ExportPsdFiles = new (Settings.ExportPsdFiles, new (Settings.ExportPsdFiles.value));
	
	// DEBUG
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> DEBUGVisibleOutputPadding = new (Settings.DEBUGVisibleOutputPadding, new (Settings.DEBUGVisibleOutputPadding.value));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> DEBUGVisiblePopupVisuals = new (Settings.DEBUGVisiblePopupVisuals, new (Settings.DEBUGVisiblePopupVisuals.value));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> DEBUGVisibleConnectionBounds = new (Settings.DEBUGVisibleConnectionBounds, new (Settings.DEBUGVisibleConnectionBounds.value));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> DEBUGVisibleShortcutEntranceData = new (Settings.DEBUGVisibleShortcutEntranceData, new (Settings.DEBUGVisibleShortcutEntranceData.value));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> DEBUGRoomWireframe = new (Settings.DEBUGRoomWireframe, new (Settings.DEBUGRoomWireframe.value));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> DEBUGLogInvalidSlopes = new (Settings.DEBUGLogInvalidSlopes, new (Settings.DEBUGLogInvalidSlopes.value));
	private readonly SettingEditableReferrer<bool, BoolToggleEditable> DEBUGVerboseExportLog = new (Settings.DEBUGVerboseExportLog, new (Settings.DEBUGVerboseExportLog.value));
	
	// REVIEW - the cursor overrides behave weirdly for some reason
	public override void Draw() {
		base.Draw();

		if (this.collapsed)
			return;

		float padding = 0.01f;
		int width = Program.window.FramebufferSize.X;
		int height = Program.window.FramebufferSize.Y;
		Program.gl.Enable(EnableCap.ScissorTest);
		Program.gl.Scissor(
			(int) (((this.bounds.x0 + padding) / Main.screenBounds.x + 1f) * 0.5f * width),
			(int) (((this.bounds.y0 + padding) / Main.screenBounds.y + 1f) * 0.5f * height),
			(uint) ((this.bounds.x1 - this.bounds.x0 - padding * 2f) / Main.screenBounds.x * 0.5f * width),
			(uint) ((this.bounds.y1 - this.bounds.y0 - padding * 2f - 0.05f) / Main.screenBounds.y * 0.5f * height)
		);

		this.scroll += (this.targetScroll - this.scroll) * (1f - MathF.Pow(1f - Settings.PopupScrollSpeed, Program.Delta * 60f));

		float y = -0.06f + this.scroll;
		float nameWidth = Math.Min(0.6f, (this.bounds.x1 - this.bounds.x0) * 0.65f);

		Rect settingRect = new (this.bounds.x0 + nameWidth + 0.03f, this.bounds.y1 + y, this.bounds.x1 - 0.01f, this.bounds.y1 + y - 0.05f);
		Rect squareRect;
		void GotoNextSetting() {
			y -= 0.07f;
			settingRect = new (this.bounds.x0 + nameWidth + 0.03f, this.bounds.y1 + y, this.bounds.x1 - 0.01f, this.bounds.y1 + y - 0.05f);
			squareRect = new (settingRect.x0, settingRect.y0, settingRect.x0 + 0.05f, settingRect.y1);
			Immediate.Color(Color.White);
		}

		Immediate.Color(Color.White);
		font.Write(this.CameraPanSpeed.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.Slider(settingRect, this.CameraPanSpeed.editable, ref this.CameraPanSpeed.ValueRef);

		GotoNextSetting();
		font.Write(this.CameraZoomSpeed.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.Slider(settingRect, this.CameraZoomSpeed.editable, ref this.CameraZoomSpeed.ValueRef);
		
		GotoNextSetting();
		font.Write(this.PopupScrollSpeed.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.Slider(settingRect, this.PopupScrollSpeed.editable, ref this.PopupScrollSpeed.ValueRef);

		GotoNextSetting();
		font.Write(this.OriginalControls.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.OriginalControls.ValueRef);

		GotoNextSetting();
		font.Write(this.WorldIconScale.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.TextInput(settingRect, this.WorldIconScale.editable);

		GotoNextSetting();
		font.Write(this.DefaultFilePath.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.TextInput(settingRect, this.DefaultFilePath.editable);
		
		GotoNextSetting();
		font.Write(this.KeepFilesystemPath.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.KeepFilesystemPath.ValueRef);

		GotoNextSetting();
		font.Write(this.WarnMissingImages.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.WarnMissingImages.ValueRef);
		
		GotoNextSetting();
		font.Write(this.HideTutorial.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.HideTutorial.ValueRef);

		GotoNextSetting();
		font.Write(this.HideTutorialOnLoadWorld.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.HideTutorialOnLoadWorld.ValueRef);

		GotoNextSetting();
		font.Write(this.UpdateWorldFiles.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.UpdateWorldFiles.ValueRef);
		
		GotoNextSetting();
		font.Write(this.UpdateRoomImagesOnRender.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.UpdateRoomImagesOnRender.ValueRef);

		GotoNextSetting();
		font.Write(this.RoomTintStrength.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.Slider(settingRect, this.RoomTintStrength.editable, ref this.RoomTintStrength.ValueRef);
		
		GotoNextSetting();
		font.Write(this.DropdownOnHover.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.DropdownOnHover.ValueRef);
		
		GotoNextSetting();
		font.Write(this.DropletKeepRelativePosition.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.DropletKeepRelativePosition.ValueRef);
		
		GotoNextSetting();
		font.Write(this.ConnectionOpacity.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.Slider(settingRect, this.ConnectionOpacity.editable, ref this.ConnectionOpacity.ValueRef);
		
		GotoNextSetting();
		font.Write(this.DisableAprilFoolsUpdates.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.DisableAprilFoolsUpdates.ValueRef);
		
		GotoNextSetting();
		font.Write(this.DiscordRichPresence.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.DiscordRichPresence.ValueRef);
		
		GotoNextSetting();
		font.Write(this.RoundedUI.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.RoundedUI.ValueRef);
		
		GotoNextSetting();
		font.Write(this.DisableUpdater.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.DisableUpdater.ValueRef);
		
		GotoNextSetting();
		font.Write(this.RainedPath.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.TextInput(settingRect, this.RainedPath.editable);
		
		GotoNextSetting();
		font.Write(this.ExportPsdFiles.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.ExportPsdFiles.ValueRef);

		GotoNextSetting();
		font.Write(this.DEBUGVisibleOutputPadding.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.DEBUGVisibleOutputPadding.ValueRef);

		GotoNextSetting();
		font.Write(this.DEBUGVisiblePopupVisuals.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.DEBUGVisiblePopupVisuals.ValueRef);

		GotoNextSetting();
		font.Write(this.DEBUGVisibleConnectionBounds.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.DEBUGVisibleConnectionBounds.ValueRef);

		GotoNextSetting();
		font.Write(this.DEBUGVisibleShortcutEntranceData.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.DEBUGVisibleShortcutEntranceData.ValueRef);

		GotoNextSetting();
		font.Write(this.DEBUGRoomWireframe.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.DEBUGRoomWireframe.ValueRef);

		GotoNextSetting();
		font.Write(this.DEBUGLogInvalidSlopes.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.DEBUGLogInvalidSlopes.ValueRef);

		GotoNextSetting();
		font.Write(this.DEBUGVerboseExportLog.settingName, this.bounds.x0 + 0.01f, settingRect.y1, 0.03f, Font.Align.TopLeft);
		UI.CheckBox(squareRect, ref this.DEBUGVerboseExportLog.ValueRef);

		GotoNextSetting();
		this.maxScroll = -(y - this.scroll) - this.bounds.y1;

		
		Program.gl.Disable(EnableCap.ScissorTest);
	}

	protected void Scroll(float x, float y) {
		if (!this.isHovered || this.collapsed) return;

		this.targetScroll -= y * 0.1f;
		this.ClampScroll();
	}

	protected void ClampScroll() {
		float trueMaxScroll = this.maxScroll + this.bounds.y0;
		if (this.targetScroll >= trueMaxScroll) {
			this.targetScroll = trueMaxScroll;
			if (this.scroll >= trueMaxScroll - 0.03f) {
				this.scroll = trueMaxScroll + 0.03f;
			}
		}

		if (this.targetScroll < 0) {
			this.targetScroll = 0;
			if (this.scroll < 0.03f) {
				this.scroll = -0.03f;
			}
		}
	}

	public override void Close() {
		Main.Scroll -= this.Scroll;

		base.Close();
	}

	public class SettingEditableReferrer<T, E> where T : IParsable<T> where E : Editable {
		public string settingName;
		public E editable;
		public Settings.Setting<T> relevantSetting;

		public SettingEditableReferrer(Settings.Setting<T> setting, E editable) {
			this.relevantSetting = setting;
			this.settingName = setting.id;
			this.editable = editable;
		}

		public ref T ValueRef {
			get => ref this.relevantSetting.value;
		}
	}
}