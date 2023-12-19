using cads_gui.Data;

namespace Fluxor.Blazor.Store
{
	[FeatureState]
	public class GuiState
	{
		public Scan? scan { get; }
    public double Y { get; }
    private GuiState(){}
		public GuiState(Scan? b, double y)
		{
			scan = b;
      Y = y;
		}
	}

  public static class Reducers
	{
		[ReducerMethod]
		public static GuiState ReduceBeltAction(GuiState state, BeltAction action) => new (b: action.Belt, y:state.Y);

    [ReducerMethod]
		public static GuiState ReduceYAction(GuiState state, YAction action) => new (b: state.scan, y:action.Y);
	}
}