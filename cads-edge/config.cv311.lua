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
  Site = "GOLD",
  Name = "JIG",
  Timezone = "Australia/Perth",
  PulleyCircumference = 624 * math.pi,
  TypicalSpeed = 2.63
}

belt = {
  Serial = "SerialBelt",
  PulleyCover = 7.0,
  CordDiameter = 5.6,
  TopCover = 20.0,
  Width = 1200.0,
  Length = 39190.0,
  WidthN = 1500.0
}

dbscan = {
  InClusterRadius = 12,
  MinPoints = 20,
  MergeRadius = 10,
  MaxClusters = 2
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
  NaNPercentage = 1,
  ClipHeight = 35.0,
  ClampToZeroHeight = 10.0,
  PulleyEstimatorInit = -15.0,
  PulleyCircumference = conveyor.PulleyCircumference,
  TypicalSpeed = conveyor.TypicalSpeed,
  WidthN = belt.WidthN,
  IIRFilter = iirfilter,
  RevolutionSensor = revolutionsensor,
  Measures = measures
}

sqlitegocatorConfig = {
    Range = {0,99999999999999},
    Fps = gocatorFps,
    Forever = true,
    --Source = "../../profiles/rawprofile_cv311_2023-11-08.db",
    Source = "../../profiles/scan-1703728368588917648.sqlite",
    TypicalSpeed = conveyor.TypicalSpeed,
    Sleep = false
  }


function timeToString(time) -- overwritten externally
  return tostring(time)
end

function main(sendmsg)

  local gocator_cads = BlockingReaderWriterQueue()
  local cads_origin = BlockingReaderWriterQueue()
  local origin_savedb = BlockingReaderWriterQueue()
  local savedb_luamain = BlockingReaderWriterQueue()
  
  local align_profile = alignProfile(gocator_cads)
  local partition_profile = partitionProfile(dbscan,align_profile)
  --local laser = gocator(laserConf,partition_profile)
  local laser = sqlitegocator(sqlitegocatorConfig,partition_profile) 

  if laser == nil then
    sendmsg("caas." .. DeviceSerial .. "." .. "error","","Unable to start gocator")
    return
  end

  local type_conversion = prsToScan(cads_origin)
  local thread_process_profile = process_profile(profileConfig,gocator_cads,type_conversion)
  local belt_loop = loop_beltlength_thread(belt.Length,cads_origin,origin_savedb)
  local thread_send_save = save_send_thread(conveyor,belt,origin_savedb,savedb_luamain)

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
      end
    end

    unloop = coroutine.yield(0)
  until unloop

  laser:Stop(true)
  join_threads({thread_process_profile,belt_loop,thread_send_save})

end

mainco = coroutine.create(main)