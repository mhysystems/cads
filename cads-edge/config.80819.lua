gocator = {
  Fps = 984.0
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
  LengthN = conveyor.TypicalSpeed / gocator.Fps, 
  Splices = 1,
  Conveyor = 3
}

sqlitegocatorConfig = {
  Range = {4,99999999999},
  Fps = gocator.Fps,
  Forever = true,
  Delay = 98,
  Source = "../../profiles/rawprofile_cv912.db",
  TypicalSpeed = conveyor.TypicalSpeed
}

y_res_mm = 1000 * conveyor.TypicalSpeed / gocator.Fps -- In mm

anomaly = {
  WindowSize = 3 * 1000 / y_res_mm,
  BeltPartitionSize = 1000 * 1000 / y_res_mm,
  BeltSize = belt.Length / y_res_mm,
  MinPosition = (belt.Length - 10000) / y_res_mm,
  MaxPosition = (belt.Length + 10000) / y_res_mm,
  ConveyorName = conveyor.Name
}

profileConfig = {
  Width = belt.Width,
  WidthN = conveyor.WidthN,
  NaNPercentage = 0.15,
  ClipHeight = 35.0
}

function main()

  local gocator_cads = BlockingReaderWriterQueue()
  local cads_origin = BlockingReaderWriterQueue()
  local origin_savedb = BlockingReaderWriterQueue()
  local savedb_luamain = BlockingReaderWriterQueue()
  
  local laser = sqlitegocator(sqlitegocatorConfig,gocator_cads) 
  local thread_process_profile = process_profile(profileConfig,gocator_cads,cads_origin)
  --local belt_loop = anomaly_detection_thread(anomaly,cads_origin,origin_savedb)
  local belt_loop = loop_beltlength_thread(conveyor,cads_origin,origin_savedb)
  local thread_send_save = save_send_thread(conveyor,origin_savedb,savedb_luamain)

  laser:Start(gocator.Fps)

  unloop = false
  repeat
    local is_value,msg_id = wait_for(savedb_luamain)

    if is_value then
      print(msg_id)
      if msg_id == 2 then break 
      --elseif msg_id == 5 then break
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

-- Meaurement Interface --

json = require "json"

measurements = {}

function timeToString(time) -- overwritten externally
  return tostring(time)
end

function out(sub, json) -- overwritten externally
end

function encode(category, msg) 
  out(conveyor.Org,category,json.encode(msg))
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
  
  measurements = m
end

function send(name,quality,time,...)

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
      encode(measurements[name].category, msg)
      m.time0 = time
    end

  end

end

function sendHack(serial) 
  out("caas." .. serial .. ".scancomplete","","")
end
