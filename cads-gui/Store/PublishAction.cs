using cads_gui.Data;

namespace Fluxor.Blazor.Store
{
	public class BeltAction
	{
    public readonly Belt Belt;
    private BeltAction() { }
		public BeltAction(Belt b)
		{
			Belt = b;
		}
	}

  public class YAction
	{
    public readonly double Y;
    private YAction() { }
		public YAction(double y)
		{
			Y = y;
		}
	}
}