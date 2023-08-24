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
  TypicalSpeed = conveyor.TypicalSpeed,
  Sleep = false
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
  PulleyEstimatorInit = -15.0,
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

function msgAppendList(root, keys, values)
  
  for i,v in ipairs(values) do 
    root[keys[i]] = (type(v) == "function") and v() or v
  end
  
  return root
end

function msgAppendTable(root, t)
  
  for k,v in pairs(t) do 
    root[k] = v
  end
  
  return root
end

function make(now)

  local p = 50000
  local tag = {revision = 0}
  local field = {"value"}
  local cat = "all"

  local m = {
    pulleyspeed = {category = "measure", period = p, time0 = now, tags = tag, fields = field},
    pulleylevel = {category = cat, period = p, time0 = now, tags = tag, fields = field},
    beltlength  = {category = "measure", period = p, time0 = now, tags = tag, fields = field},
    cadstoorigin = {category = "measure", period = p, time0 = now ,tags = tag, fields = field},
    beltrotationperiod = {category = "measure", period = p, time0 = now ,tags = tag, fields = field},
    beltedgeposition = {category = cat, period = p, time0 = now, tags = tag, fields = field},
    pulleyoscillation = {category = "measure", period = p, time0 = now, tags = tag, fields = field},
    nancount = {category = cat, period = p, time0 = now, tags = tag, fields = field},
    anomaly = {category = "anomaly", period = 1, time0 = now, tags = tag, fields = {"value", "location"}}
  }

  local cnt = 0
  for _ in pairs(m) do cnt = cnt + 1 end

  local s = p / cnt
  for k,v in pairs(m) do
    m[k].time0 = m[k].time0 + s * (cnt - 1)
    cnt = cnt - 1
  end
  
  return m
end

function send(out,measurements,name,quality,time,...)

  if measurements[name] then

    local m = measurements[name]
    local elapsedTime = time - m.time0
    
    if elapsedTime >= m.period then 
    
      local msg = {
        measurement = name, 
        site = conveyor.Site, 
        conveyor = conveyor.Name, 
        timestamp = timeToString(time),
        quality = quality
      }
      
      msgAppendTable(msg,measurements[name].tags)
      msgAppendList(msg,measurements[name].fields, {...})
      out(conveyor.Org,measurements[name].category,json.encode(msg))
      m.time0 = time
    end

  end

end

function main(sendmsg)

  local measurements = make(getNow())

  local gocator_cads = BlockingReaderWriterQueue()
  local cads_origin = BlockingReaderWriterQueue()
  local origin_savedb = BlockingReaderWriterQueue()
  local savedb_luamain = BlockingReaderWriterQueue()
  
  --local laser = gocator(laserConf,gocator_cads)
  local laser = sqlitegocator(sqlitegocatorConfig,gocator_cads) 
  local thread_process_profile = dynamic_processing_thread(dynamicProcessingConfig,gocator_cads,cads_origin)
  --local belt_loop = anomaly_detection_thread(anomaly,cads_origin,origin_savedb)
  local belt_loop = loop_beltlength_thread(conveyor,cads_origin,origin_savedb)
  local thread_send_save = save_send_thread(conveyor,origin_savedb,savedb_luamain)

  laser:Start(gocatorFps)
  local beltprogress = 0

  unloop = false
  repeat
    local is_value,msg_id,data = wait_for(savedb_luamain)

    if is_value then
      if msg_id == 2 then break 
      --elseif msg_id == 5 then break
      elseif msg_id == 11 then
        send(sendmsg,measurements,table.unpack(data))
      elseif msg_id == 10 then
        local m,progress = table.unpack(data)
        if progress ~= beltprogress then
          print("caas." .. DeviceSerial .. "." .. m,"",progress)
          sendmsg("caas." .. DeviceSerial .. "." .. m,"",progress)
          beltprogress = progress
        end
      end
    end

    unloop = coroutine.yield(0)
  until unloop

  print("stopping")
  laser:Stop()
  join_threads({thread_process_profile,window_processing,dynamic_processing,upload_scan})
  print("stopped")
  
end

mainco = coroutine.create(main)