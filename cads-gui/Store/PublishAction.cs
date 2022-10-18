using cads_gui.Data;

namespace Fluxor.Blazor.Store
{
	public class PublishAction
	{
    public readonly Belt Belt;
    private PublishAction() { }
		public PublishAction(Belt b)
		{
			Belt = b;
		}
	}
}