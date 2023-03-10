using cads_gui.Data;

namespace Fluxor.Blazor.Store
{
	public class BeltAction
	{
    public readonly Scan Belt;

		public BeltAction(Scan b)
		{
			Belt = b;
		}
	}

  public class YAction
	{
    public readonly double Y;

		public YAction(double y)
		{
			Y = y;
		}
	}
}