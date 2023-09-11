json = require "json"

gocatorFps = 984.0

iirfilter = {
  Skip = 10000,
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
  Name = "Demo",
  Timezone = "Australia/Perth",
  PulleyCircumference = 2827.4334,
  TypicalSpeed = 6.0,
  Belt = 3,
  Length = 40000.0,
  WidthN = 1800
}

belt = {
  Id = 3,
  Installed = "2023-01-14T00:00:00Z",
  PulleyCover = 6.0,
  CordDiameter = 5.6,
  TopCover = 30.0,
  Width = 1800,
  Length = 40000.0,
  LengthN = conveyor.TypicalSpeed / gocatorFps, 
  Splices = 1,
  Conveyor = 3
}

dbscan = {
  InClusterRadius = 12,
  MinPoints = 20
}

revolutionsensor = {
  Source = "length",
  TriggerDistance = conveyor.PulleyCircumference / 1,
  Bias = 0,
  Threshold = 0.05,
  Bidirectional = false,
  Skip = math.floor((conveyor.PulleyCircumference / (1000 * conveyor.TypicalSpeed)) * gocatorFps * 0.9)
}

y_res_mm = 1000 * conveyor.TypicalSpeed / gocatorFps -- In mm

laserConf = {
  Trim = true,
  TypicalResolution = y_res_mm
}

measures = {
  Enable = false
}

profileConfig = {
  Width = belt.Width,
  NaNPercentage = 0.15,
  ClipHeight = 35.0,
  PulleyEstimatorInit = -15.0,
  IIRFilter = iirfilter,
  PulleySamplesExtend = 10,
  RevolutionSensor = revolutionsensor,
  Conveyor = conveyor,
  Dbscan = dbscan,
  Measures = measures
}

sqlitegocatorConfig = {
  Range = {169812, 2166792},
  Fps = gocatorFps,
  Forever = true,
  Source = "../../profiles/rawprofile_cv001_2.db",
  TypicalSpeed = conveyor.TypicalSpeed,
  Sleep = true
}

function main(sendmsg)

  local gocator_origin = BlockingReaderWriterQueue()
  local origin_savedb = BlockingReaderWriterQueue()
  local savedb_luamain = BlockingReaderWriterQueue()
  --local laser = sqlitegocator(sqlitegocatorConfig,gocator_cads) 
  local laser = gocator(laserConf,gocator_origin)
  
  if laser == nil then
    sendmsg("caas." .. DeviceSerial .. "." .. "error","","Unable to start gocator")
    return
  end

  local belt_loop = loop_beltlength_thread(conveyor,gocator_origin,origin_savedb)
  local thread_send_save = save_send_thread(conveyor,origin_savedb,savedb_luamain)

  if laser:Start(gocatorFps) then
    sendmsg("caas." .. DeviceSerial .. "." .. "error","","Unable to start gocator")
    return
  end

  local beltprogress = 0

  unloop = false
  repeat
    local is_value,msg_id,data = wait_for(savedb_luamain)

    if is_value then
      if msg_id == 2 then break 
      elseif msg_id == 5 then break
      elseif msg_id == 10 then
        local m,progress = table.unpack(data)
        if progress ~= beltprogress then
          sendmsg("caas." .. DeviceSerial .. "." .. m,"",progress)
          beltprogress = progress
        end
      elseif msg_id == 12 then
        sendmsg("caas." .. DeviceSerial .. "." .. "error","",data)
        break
      end
    end

    unloop = coroutine.yield(0)
  until unloop

  laser:Stop(true)
  join_threads({belt_loop,thread_send_save})

end

mainco = coroutine.create(main)