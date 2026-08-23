namespace Custom;

public class UVAtlas {
	private UVAtlasElement[] elements;

	public UVAtlas(UVAtlasElement[] elements) {
		this.elements = elements;
	}

	public UVCoordinates UV(string ID) {
		foreach (UVAtlasElement atlasElement in this.elements) {
			if (atlasElement.elementID.Equals(ID, StringComparison.InvariantCultureIgnoreCase)) {
				return atlasElement.elementUVs;
			}
		}
		Logger.Info($"Failed to get UV's for ID: {ID}");
		return new(0, 0, 1, 1);
	}

	public class UVAtlasElement {
		public string elementID;
		public UVCoordinates elementUVs;
		public UVAtlasElement(string elementID, UVCoordinates elementUVs) {
			this.elementID = elementID;
			this.elementUVs = elementUVs;
		}
	}
		
	public struct UVCoordinates {
		/// <summary> TOPLEFT </summary>
		public Vector2 uv0;
		/// <summary> TOPRIGHT </summary>
		public Vector2 uv1;
		/// <summary> BOTTOMRIGHT </summary>
		public Vector2 uv2;
		/// <summary> BOTTOMLEFT </summary>
		public Vector2 uv3;

		public UVCoordinates(float u0, float v0, float u1, float v1) {
			this.uv0 = new Vector2(u0, v0);
			this.uv1 = new Vector2(u1, v0);
			this.uv2 = new Vector2(u1, v1);
			this.uv3 = new Vector2(u0, v1);
		}

		public UVCoordinates(Vector2 uv0, Vector2 uv1, Vector2 uv2, Vector2 uv3) {
			this.uv0 = uv0;
			this.uv1 = uv1;
			this.uv2 = uv2;
			this.uv3 = uv3;
		}
	}
}