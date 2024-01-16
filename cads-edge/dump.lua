json = require "json"

gocatorFps = 984.0

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
  MinPoints = 12,
  ZMergeRadius = 10,
  XMergeRadius = 50,
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

function timeToString(time) -- overwritten externally
  return tostring(time)
end

function main(sendmsg)

  local gocator_cads = BlockingReaderWriterQueue()
  local origin_savedb = BlockingReaderWriterQueue()
  local savedb_luamain = BlockingReaderWriterQueue()
  
  local laser = gocator(laserConf,gocator_cads)

  if laser == nil then
    return
  end

  local belt_loop = loop_beltlength_thread(conveyor,gocator_cads,origin_savedb)
  local thread_send_save = save_send_thread(conveyor,false,origin_savedb,savedb_luamain)

  if laser:Start(gocatorFps) then
    return
  end

  local beltprogress = 0

  unloop = false
  repeat
    local is_value,msg_id,data = wait_for(savedb_luamain)

    if is_value then
      if msg_id == 2 then break 
      elseif msg_id == 5 then break
      elseif msg_id == 12 then
        break
      end
    end

    unloop = coroutine.yield(0)
  until unloop

  laser:Stop(true)
  join_threads({belt_loop,thread_send_save})

end

mainco = coroutine.create(main)