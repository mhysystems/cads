json = require "json"

gocatorFps = 984.0

iirfilter = {
  Skip = 41706,
  Delay = 334,
  Sos  = {{ 1.66008519e-07,  1.66008519e-07,  0.00000000e+00, 1.00000000e+00, -9.77361093e-01,  0.00000000e+00},
          { 1.00000000e+00, -1.98638977e+00,  1.00000000e+00, 1.00000000e+00, -1.95693573e+00,  9.57429859e-01},
          { 1.00000000e+00, -1.99649383e+00,  1.00000000e+00, 1.00000000e+00, -1.96256904e+00,  9.63016716e-01},
          { 1.00000000e+00, -1.99836739e+00,  1.00000000e+00, 1.00000000e+00, -1.96958098e+00,  9.69971891e-01},
          { 1.00000000e+00, -1.99901953e+00,  1.00000000e+00, 1.00000000e+00, -1.97638907e+00,  9.76726728e-01},
          { 1.00000000e+00, -1.99931660e+00,  1.00000000e+00, 1.00000000e+00, -1.98230743e+00,  9.82601536e-01},
          { 1.00000000e+00, -1.99947217e+00,  1.00000000e+00, 1.00000000e+00, -1.98725122e+00,  9.87512612e-01},
          { 1.00000000e+00, -1.99955888e+00,  1.00000000e+00, 1.00000000e+00, -1.99139129e+00,  9.91629854e-01},
          { 1.00000000e+00, -1.99960633e+00,  1.00000000e+00, 1.00000000e+00, -1.99496619e+00,  9.95190499e-01},
          { 1.00000000e+00, -1.99962752e+00,  1.00000000e+00, 1.00000000e+00, -1.99821287e+00,  9.98430473e-01}}
}

conveyor = {
  Id = 3,
  Org  = "FMG",
  Site = "FMG",
  Name = "demo",
  Timezone = "Australia/Perth",
  PulleyCircumference = 670.0,
  TypicalSpeed = 6.0,
  Belt = 3,
  Length = 19900,
  WidthN = 1880
}

belt = {
  Id = 3,
  Installed = "2023-01-14T00:00:00Z",
  PulleyCover = 7.0,
  CordDiameter = 9.1,
  TopCover = 14.0,
  Width = 1700,
  Length = 19900,
  LengthN = conveyor.TypicalSpeed / gocatorFps, 
  Splices = 1,
  Conveyor = 3
}

dbscan = {
  InClusterRadius = 2.5,
  MinPoints = 11
}

revolutionsensor = {
  Source = "length",
  TriggerDistance = conveyor.PulleyCircumference / 1,
  Bias = 0,
  Threshold = 0.05,
  Bidirectional = false,
  Skip = math.floor((conveyor.PulleyCircumference / (1000 * conveyor.TypicalSpeed)) * gocatorFps * 0.9)
}

sqlitegocatorConfig = {
  Range = {0,99999999999},
  Fps = gocatorFps,
  Forever = true,
  Delay = 98,
  Source = "../../profiles/rawprofile_cv912.db",
  TypicalSpeed = conveyor.TypicalSpeed
}

y_res_mm = 1000 * conveyor.TypicalSpeed / gocatorFps -- In mm

laserConf = {
  Trim = true,
  TypicalResolution = y_res_mm
}

anomaly = {
  WindowSize = 3 * 1000 / y_res_mm,
  BeltPartitionSize = 1000 * 1000 / y_res_mm,
  BeltSize = belt.Length / y_res_mm,
  MinPosition = (belt.Length - 10000) / y_res_mm,
  MaxPosition = (belt.Length + 10000) / y_res_mm,
  ConveyorName = conveyor.Name
}

measures = {
  Enable = false
}

profileConfig = {
  Width = belt.Width,
  NaNPercentage = 0.15,
  ClipHeight = 35.0,
  IIRFilter = iirfilter,
  PulleySamplesExtend = 10,
  RevolutionSensor = revolutionsensor,
  Conveyor = conveyor,
  Dbscan = dbscan,
  Measures = measures
}

dynamicProcessingConfig = {
  WidthN = conveyor.WidthN,
  WindowSize = 2,
  InitValue = 30.0,
  LuaCode = LuaCodeSelfRef,
  Entry = "process"  
}

function timeToString(time) -- overwritten externally
  return tostring(time)
end

function main(sendmsg)

  local gocator_luamain = BlockingReaderWriterQueue()
  
  local decimate = profile_decimation(420,10000,gocator_luamain)
  local laser = sqlitegocator(sqlitegocatorConfig,decimate) 

  laser:Start(gocatorFps)
  local beltprogress = 0

  unloop = false
  repeat
    local is_value,msg_id,data = wait_for(gocator_luamain)

    if is_value then
      if msg_id == 2 then break 
      --elseif msg_id == 5 then break
      elseif msg_id == 10 then
        local m,progress = table.unpack(data)
        if progress ~= beltprogress then
          print("caas." .. DeviceSerial .. "." .. m,"")
          sendmsg("caas." .. DeviceSerial .. "." .. m,"",progress)
          beltprogress = progress
        end
      end
    end

    unloop = coroutine.yield(0)
  until unloop

  print("stopping")
  laser:Stop()
  
end

mainco = coroutine.create(main)