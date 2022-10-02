
using Microsoft.AspNetCore.SignalR;
using System.Threading.Tasks;

namespace cads_gui.Data;

public class RealtimeHub : Hub
{
    public async Task JoinGroup(string group)
    {
      await Groups.AddToGroupAsync(Context.ConnectionId,group);
    }

    public async Task LeaveGroup(string group)
    {
      await Groups.RemoveFromGroupAsync(Context.ConnectionId,group);
    }
}

