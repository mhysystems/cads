import CadsFlatbuffers.Msg as cm
import CadsFlatbuffers.MsgContents as cmc
import CadsFlatbuffers.Start as cs
import CadsFlatbuffers.Stop as cst
import flatbuffers
import sys
import argparse

parser = argparse.ArgumentParser(description='Create flatbuffer cads messages')
parser.add_argument("--stop", help="Send Stop", action='store_true')
args = parser.parse_args()


builder = flatbuffers.Builder(1024)

if args.stop:
  cst.Start(builder)
  stopMsg = cs.End(builder)
  cm.MsgStart(builder)
  cm.AddContentsType(builder,cmc.MsgContents().Stop)
  cm.AddContents(builder,stopMsg)
  msg = cm.End(builder)
  builder.Finish(msg)
else:
  code = sys.stdin.read()
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


