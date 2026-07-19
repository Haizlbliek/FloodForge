namespace FloodForge.World;

/// <summary>
/// A WorldDraggable with split canon & dev positions
/// </summary>
public class MapDraggable : WorldDraggable {
	public Vector2 CanonPosition;
	public Vector2 DevPosition;

	public override Vector2 GetPosition() {
		return WorldWindow.PositionType == WorldWindow.RoomPosition.Canon ? this.CanonPosition : this.DevPosition;
	}

	public override void SetPosition(Vector2 value) {
		if (WorldWindow.PositionType == WorldWindow.RoomPosition.Canon) {
			this.CanonPosition = value;
		}
		else {
			this.DevPosition = value;
		}
	}

	public Vector2 InactivePosition {
		get {
			return WorldWindow.PositionType == WorldWindow.RoomPosition.Canon ? this.DevPosition : this.CanonPosition;
		}

		set {
			if (WorldWindow.PositionType == WorldWindow.RoomPosition.Canon) {
				this.DevPosition = value;
			}
			else {
				this.CanonPosition = value;
			}
		}
	}

	public virtual void MoveUpdate() {
		
	}
}