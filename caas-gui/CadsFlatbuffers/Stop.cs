// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

namespace CadsFlatbuffers
{

using global::System;
using global::System.Collections.Generic;
using global::Google.FlatBuffers;

public struct Stop : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_23_3_3(); }
  public static Stop GetRootAsStop(ByteBuffer _bb) { return GetRootAsStop(_bb, new Stop()); }
  public static Stop GetRootAsStop(ByteBuffer _bb, Stop obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public Stop __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }


  public static void StartStop(FlatBufferBuilder builder) { builder.StartTable(0); }
  public static Offset<CadsFlatbuffers.Stop> EndStop(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    return new Offset<CadsFlatbuffers.Stop>(o);
  }
}


}