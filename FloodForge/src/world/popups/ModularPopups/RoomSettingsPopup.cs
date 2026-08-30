using FloodForge.Popups;
using FloodForge.History;
using FloodForge.Droplet;
using StbImageWriteSharp;
using static FloodForge.Main;

namespace FloodForge.World;

public class RoomSettingsPopup : ModularPopup {
	public Room relevantRoom;
	private BoolSettingContainer enclosedRoomToggle;
	private IntSliderSettingContainer waterLevelSlider;
	private BoolSettingContainer waterInFrontToggle;
	private ButtonContainer editCreatures;
	private ButtonContainer renderRoomButton;
	private ButtonContainer renameRoomButton;
	private ButtonContainer createTimelineRoomButton;
	private CreateTimelineRoomPopup? createTimelineRoomPopup;

	private ButtonContainer editReplaceRooms;
	private EditReplaceRoomPopup? editReplaceRoomPopup;

	public RoomSettingsPopup(Room relevantRoom) {
		this.relevantRoom = relevantRoom;

		this.enclosedRoomToggle = new BoolSettingContainer("Enclosed Room", this.relevantRoom.data.enclosedRoom, this.UpdateEnclosedRoom);
		this.AddToQueue(this.enclosedRoomToggle);
		this.waterLevelSlider = new IntSliderSettingContainer("Water Height", this.relevantRoom.data.waterHeight, -2, this.relevantRoom.height, this.UpdateWaterHeight).UpdateWhileDragging(true);
		this.AddToQueue(this.waterLevelSlider);
		this.waterInFrontToggle = new BoolSettingContainer("Water In Front", this.relevantRoom.data.waterInFront, b => {
			WorldWindow.worldHistory.Apply(new VariableChange<bool>(this.relevantRoom.data.waterInFront, b, bRedo => this.relevantRoom.data.waterInFront = bRedo));
		});
		this.AddToQueue(this.waterInFrontToggle);
		this.editCreatures = new ButtonContainer("Edit Creatures", this.EditCreatures);
		this.AddToQueue(this.editCreatures);
		this.renderRoomButton = new ButtonContainer("Render Room", this.RenderRoom);
		this.AddToQueue(this.renderRoomButton);
		this.renameRoomButton = new ButtonContainer("Rename Room", this.RenameRoom);
		this.AddToQueue(this.renameRoomButton);
		this.createTimelineRoomButton = new ButtonContainer("Create Timeline Room", this.AddCreateTimelineRoomPopup);
		this.AddToQueue(this.createTimelineRoomButton);
		this.editReplaceRooms = new ButtonContainer("Edit ReplaceRooms", this.EditReplaceRooms);
		this.AddToQueue(this.editReplaceRooms);
		this.AddQueuedSettings();
	}

	private void UpdateEnclosedRoom(bool enclosed) {
		WorldWindow.worldHistory.Apply(new VariableChange<bool>(this.relevantRoom.data.enclosedRoom, enclosed, enclosedRedo => 
			this.relevantRoom.data.enclosedRoom = enclosedRedo
		));
	}

	private void UpdateWaterHeight(int newHeight) {
		WorldWindow.worldHistory.Apply(new VariableChange<int>(this.relevantRoom.data.waterHeight, newHeight, hRedo => {
			this.relevantRoom.data.waterHeight = hRedo;
			this.relevantRoom.RegenerateWater();
		}));
	}

	private void EditCreatures() {
		PopupManager.Add(new EditCreaturesPopup(this.relevantRoom));
	}

	private void RenderRoom() {
		PopupManager.Add(new ConfirmPopup($"Render Room {this.relevantRoom}?\nThis will overwrite existing images.")).SetOkay("Render").Okay(() => {
			DropletWindow.LoadRoom(this.relevantRoom, Vector2.Zero);
			if (DropletWindow.Render(out string errorMessage, out (string name, string path, byte[] image)[] images)) {
				foreach ((string name, string path, byte[] image) in images) {
					FloodForge.Backup.File(path);

					using Stream stream = File.OpenWrite(path);
					ImageWriter writer = new();
					writer.WritePng(image, CameraTextureWidth, CameraTextureHeight, ColorComponents.RedGreenBlue, stream);
				}
				PopupManager.Add(new InfoPopup($"Render complete.\nBackups made."));
			}
			else {
				PopupManager.Add(new InfoPopup($"Error while rendering {this.relevantRoom.name}\n{errorMessage}\nview log.txt for more info"));
			}
		});
	}

	private void RenameRoom() {
		if (this.relevantRoom.data.tags.Contains("GATE") || this.relevantRoom.name.StartsWith("GATE")) {
			PopupManager.Add(new InfoPopup("Cannot rename GATE rooms!"));
		}
		else {
			PopupManager.Add(new RenameRoomPopup(this.relevantRoom, name => {
				NameChanger.ChangeRoomName(this.relevantRoom, name);
			}).Translate(Mouse.Pos, true).Title("Rename Room"));
		}
	}

	private void EditReplaceRooms() {
		this.editReplaceRoomPopup = (EditReplaceRoomPopup)new EditReplaceRoomPopup(this.relevantRoom).Translate(Mouse.Pos, false);
		PopupManager.Add(this.editReplaceRoomPopup);
	}

	private void AddCreateTimelineRoomPopup() {
		this.createTimelineRoomPopup = (CreateTimelineRoomPopup)new CreateTimelineRoomPopup(this).SetSize(new(0.7f, 0f)).Translate(Mouse.Pos, false).Title("Create Timeline Room");
		PopupManager.Add(this.createTimelineRoomPopup);
	}
}