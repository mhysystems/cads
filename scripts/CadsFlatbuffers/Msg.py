# automatically generated by the FlatBuffers compiler, do not modify

# namespace: CadsFlatbuffers

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class Msg(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = Msg()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsMsg(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # Msg
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # Msg
    def ContentsType(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Uint8Flags, o + self._tab.Pos)
        return 0

    # Msg
    def Contents(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            from flatbuffers.table import Table
            obj = Table(bytearray(), 0)
            self._tab.Union(obj, o)
            return obj
        return None

def MsgStart(builder): builder.StartObject(2)
def Start(builder):
    return MsgStart(builder)
def MsgAddContentsType(builder, contentsType): builder.PrependUint8Slot(0, contentsType, 0)
def AddContentsType(builder, contentsType):
    return MsgAddContentsType(builder, contentsType)
def MsgAddContents(builder, contents): builder.PrependUOffsetTRelativeSlot(1, flatbuffers.number_types.UOffsetTFlags.py_type(contents), 0)
def AddContents(builder, contents):
    return MsgAddContents(builder, contents)
def MsgEnd(builder): return builder.EndObject()
def End(builder):
    return MsgEnd(builder)