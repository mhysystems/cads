import CadsFlatbuffers.Msg as cm
import CadsFlatbuffers.MsgContents as cmc
import CadsFlatbuffers.Start as cs
import flatbuffers
import sys

code = sys.stdin.read()

builder = flatbuffers.Builder(1024)
lc = builder.CreateString(code)


cs.Start(builder)
cs.AddLuaCode(builder,lc)
startMsg = cs.End(builder)



cm.MsgStart(builder)
cm.AddContentsType(builder,cmc.MsgContents().Start)
cm.AddContents(builder,startMsg)
msg = cm.End(builder)

builder.Finish(msg)

sys.stdout.buffer.write(builder.Output())


