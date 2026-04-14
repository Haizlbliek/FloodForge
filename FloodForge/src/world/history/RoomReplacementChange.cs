using Stride.Core;

namespace FloodForge.World;

public class RoomReplacementChange : Change {
	protected readonly RoomAndConnectionChange newRoomCreateChange;
	protected readonly Room newRoom;
	protected readonly RoomAndConnectionChange replacedRoomRemoveChange;
	protected readonly Room replacedRoom;

	protected readonly string filePath;
	protected readonly string oldFile;
	protected readonly string newFile;
	protected readonly string imagePathRoot;
	protected readonly string fromImagePathRoot;
	protected readonly byte[][] oldImages;
	protected readonly byte[][] newImages;

	public RoomReplacementChange(Room newRoom, Room replacedRoom, string toFilePath, string fromFilePath, string oldFileData) {
		this.newRoom = newRoom;
		this.replacedRoom = replacedRoom;

		this.filePath = toFilePath;
		this.oldFile = oldFileData;
		this.newFile = File.ReadAllText(toFilePath);
		this.imagePathRoot = toFilePath[..Math.Max(0, toFilePath.IndexOfReverse('.'))];
		this.fromImagePathRoot = toFilePath[..Math.Max(0, fromFilePath.IndexOfReverse('.'))];

		this.oldImages = new byte[this.replacedRoom.data.cameras.Count][];
		for (int i = 0; i < this.replacedRoom.data.cameras.Count; i++) {
			string imageSuffix = $"_{i + 1}.png";
			string imagePath = this.imagePathRoot + imageSuffix;
			this.oldImages[i] = File.ReadAllBytes(imagePath);
		}

		this.newImages = new byte[this.newRoom.data.cameras.Count][];
		for (int i = 0; i < this.newRoom.data.cameras.Count; i++) {
			string imageSuffix = $"_{i + 1}.png";
			string imagePath = this.fromImagePathRoot + imageSuffix;
			this.newImages[i] = [];
			if (File.Exists(imagePath))
				this.newImages[i] = File.ReadAllBytes(imagePath);
		}

		this.newRoomCreateChange = new RoomAndConnectionChange(true);
		this.newRoomCreateChange.AddRoom(this.newRoom);
		this.replacedRoomRemoveChange = new RoomAndConnectionChange(false);
		this.replacedRoomRemoveChange.AddRoom(this.replacedRoom);
	}

	public override void Redo() {
		this.newRoomCreateChange.Redo();

		for (int i = 0; i < this.newRoom.data.cameras.Count; i++) {
			string imageSuffix = $"_{i + 1}.png";
			string imagePath = this.imagePathRoot + imageSuffix;
			File.WriteAllBytes(imagePath, this.newImages[i]);
		}
		File.WriteAllText(this.filePath, this.newFile);

		List<Connection> connectionsToRemove = [];
		foreach (Connection connection in this.replacedRoom.connections) {
			this.newRoom.connections.Add(connection);
			connectionsToRemove.Add(connection);
			if (connection.roomA == this.replacedRoom)
				connection.roomA = this.newRoom;
			if (connection.roomB == this.replacedRoom)
				connection.roomB = this.newRoom;
		}
		foreach (Connection connectionToRemove in connectionsToRemove) {
			this.replacedRoom.connections.Remove(connectionToRemove);
		}
		foreach (Vector2i denPos in this.replacedRoom.denShortcutEntrances) {
			int id = this.replacedRoom.GetDenId(denPos);
			if (this.newRoom.HasDen(id))
				this.newRoom.dens[id - this.newRoom.nonDenExitCount] = this.replacedRoom.GetDen(id);
		}
		this.newRoom.DevPosition = this.replacedRoom.DevPosition;
		this.newRoom.CanonPosition = this.replacedRoom.CanonPosition;

		this.replacedRoomRemoveChange.Redo();
	}

	public override void Undo() {
		this.replacedRoomRemoveChange.Undo();

		List<Connection> connectionsToRemove = [];
		foreach (Connection connection in this.newRoom.connections) {
			this.replacedRoom.connections.Add(connection);
			if (connection.roomA == this.newRoom)
				connection.roomA = this.replacedRoom;
			connectionsToRemove.Add(connection);
			if (connection.roomB == this.newRoom)
				connection.roomB = this.replacedRoom;
		}
		foreach (Connection connectionToRemove in connectionsToRemove) {
			this.newRoom.connections.Remove(connectionToRemove);
		}
		this.newRoomCreateChange.Undo();

		for (int i = 0; i < this.replacedRoom.data.cameras.Count; i++) {
			string imageSuffix = $"_{i + 1}.png";
			string imagePath = this.imagePathRoot + imageSuffix;
			File.WriteAllBytes(imagePath, this.oldImages[i]);
		}
		File.WriteAllText(this.filePath, this.oldFile);
	}
}