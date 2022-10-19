using cads_gui.Data;

namespace Fluxor.Blazor.Store
{
	[FeatureState]
	public class GuiState
	{
		public Belt Belt { get; }
    public double Y { get; }

		private GuiState() { }
		public GuiState(Belt b, double y)
		{
			Belt = b;
      Y = y;
		}
	}

  public static class Reducers
	{
		[ReducerMethod]
		public static GuiState ReduceBeltAction(GuiState state, BeltAction action) => new (b: action.Belt, y:state.Y);

    [ReducerMethod]
		public static GuiState ReduceYAction(GuiState state, YAction action) => new (b: state.Belt, y:action.Y);
	}
}