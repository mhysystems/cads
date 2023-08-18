// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

namespace CadsFlatbuffers
{

using global::System;
using global::System.Collections.Generic;
using global::Google.FlatBuffers;

public struct Msg : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_23_3_3(); }
  public static Msg GetRootAsMsg(ByteBuffer _bb) { return GetRootAsMsg(_bb, new Msg()); }
  public static Msg GetRootAsMsg(ByteBuffer _bb, Msg obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public Msg __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public CadsFlatbuffers.MsgContents ContentsType { get { int o = __p.__offset(4); return o != 0 ? (CadsFlatbuffers.MsgContents)__p.bb.Get(o + __p.bb_pos) : CadsFlatbuffers.MsgContents.NONE; } }
  public TTable? Contents<TTable>() where TTable : struct, IFlatbufferObject { int o = __p.__offset(6); return o != 0 ? (TTable?)__p.__union<TTable>(o + __p.bb_pos) : null; }
  public CadsFlatbuffers.Start ContentsAsStart() { return Contents<CadsFlatbuffers.Start>().Value; }
  public CadsFlatbuffers.Stop ContentsAsStop() { return Contents<CadsFlatbuffers.Stop>().Value; }

  public static Offset<CadsFlatbuffers.Msg> CreateMsg(FlatBufferBuilder builder,
      CadsFlatbuffers.MsgContents contents_type = CadsFlatbuffers.MsgContents.NONE,
      int contentsOffset = 0) {
    builder.StartTable(2);
    Msg.AddContents(builder, contentsOffset);
    Msg.AddContentsType(builder, contents_type);
    return Msg.EndMsg(builder);
  }

  public static void StartMsg(FlatBufferBuilder builder) { builder.StartTable(2); }
  public static void AddContentsType(FlatBufferBuilder builder, CadsFlatbuffers.MsgContents contentsType) { builder.AddByte(0, (byte)contentsType, 0); }
  public static void AddContents(FlatBufferBuilder builder, int contentsOffset) { builder.AddOffset(1, contentsOffset, 0); }
  public static Offset<CadsFlatbuffers.Msg> EndMsg(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    return new Offset<CadsFlatbuffers.Msg>(o);
  }
  public static void FinishMsgBuffer(FlatBufferBuilder builder, Offset<CadsFlatbuffers.Msg> offset) { builder.Finish(offset.Value); }
  public static void FinishSizePrefixedMsgBuffer(FlatBufferBuilder builder, Offset<CadsFlatbuffers.Msg> offset) { builder.FinishSizePrefixed(offset.Value); }
}


}