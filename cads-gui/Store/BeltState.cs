using cads_gui.Data;

namespace Fluxor.Blazor.Store
{
	[FeatureState]
	public class GuiState
	{
		public Belt Belt { get; }

		private GuiState() { }
		public GuiState(Belt b)
		{
			Belt = b;
		}
	}

  public static class Reducers
	{
		[ReducerMethod]
		public static GuiState ReducePublishAction(GuiState state, PublishAction action) =>
			new (b: action.Belt);
	}
}