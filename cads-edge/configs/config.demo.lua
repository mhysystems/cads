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
  Id = 1,
  Org  = "MHY",
  Site = "site1",
  Name = "belt1",
  Timezone = "Australia/Perth",
  PulleyCircumference = 4197.696,
  TypicalSpeed = 6.09,
  Belt = 1,
  Length = 12565200,
  WidthN = 1890
}

belt = {
  Id = 1,
  Installed = "2023-01-14T00:00:00Z",
  PulleyCover = 7.0,
  CordDiameter = 9.1,
  TopCover = 14.0,
  Width = 1600,
  Length = 12565200,
  LengthN = conveyor.TypicalSpeed / gocatorFps, 
  Splices = 1,
  Conveyor = 1
}

dbscan = {
  InClusterRadius = 12,
  MinPoints = 11
}

revolutionsensor = {
  Source = "raw",
  TriggerDistance = conveyor.PulleyCircumference / 1,
  Bias = -32.0,
  Threshold = 0.05,
  Bidirectional = false,
  Skip = math.floor((conveyor.PulleyCircumference / (1000 * conveyor.TypicalSpeed)) * gocatorFps * 0.9)
}

y_res_mm = 1000 * conveyor.TypicalSpeed / gocatorFps -- In mm


sqlitegocatorConfig = {
  Range = {86300 - iirfilter.Skip - 2*math.floor(revolutionsensor.TriggerDistance / y_res_mm)
          ,2040734 + 86300 - iirfilter.Skip - 2*math.floor(revolutionsensor.TriggerDistance / y_res_mm)},
  Fps = gocatorFps,
  Forever = true,
  Source = "../../profiles/rawprofile_cv001_2023-08-30.db",
  TypicalSpeed = conveyor.TypicalSpeed,
  Sleep = true
}



laserConf = {
  Trim = true,
  TypicalResolution = y_res_mm,
  Fov = 250.0 --mm
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
  Enable = true
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

fiducialOriginConfig = {
  BeltLength = {12555200,12575200},
  CrossCorrelationThreshold = 0.23,
  DumpMatch = false,
  Fiducial = {
    FiducialDepth = 2,
    FiducialX = 25,
    FiducialY = 40,
    FiducialGap = 40,
    EdgeHeight = 30.1
  },
  Conveyor = conveyor
}

function process(width,height)
  local sum = 0
  for i = 1+50,width-50 do
    for j = 0,height-1 do
      sum = sum + ((win[j*width + i] <= 23.5 and win[j*width + i] > 10) and 1 or 0)
    end
  end
  return (sum / (width * height)) > 0.005 and sum or 0
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

  local p = 5
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
      out("demo",measurements[name].category,json.encode(msg))
      m.time0 = time
    end

  end

end

function main(sendmsg)

  local run = true

  while run do
    local measurements = make(getNow())

    local gocator_cads = BlockingReaderWriterQueue()
    local cads_origin = BlockingReaderWriterQueue()
    local origin_dynamic = BlockingReaderWriterQueue()
    local dynamic_savedb = BlockingReaderWriterQueue()
    local luamain = BlockingReaderWriterQueue()
    --local laser = sqlitegocator(sqlitegocatorConfig,decimate) 
    local laser = sqlitegocator(sqlitegocatorConfig,gocator_cads) 

    if laser == nil then
      return
    end

    local ede = encoder_distance_estimation(cads_origin,y_res_mm)
    --local ede = prsToScan(cads_origin)
    local thread_profile = process_profile(profileConfig,gocator_cads,ede)
    --local thread_origin = loop_beltlength_thread(conveyor,cads_origin,origin_dynamic)
    local thread_origin = fiducial_origin_thread(fiducialOriginConfig,cads_origin,origin_dynamic)
    local thread_dynamic = dynamic_processing_thread(dynamicProcessingConfig,origin_dynamic,dynamic_savedb)
    local thread_send_save = save_send_thread(conveyor,dynamic_savedb,luamain)

    laser:Start(gocatorFps)

    repeat
      local is_value,msg_id,data = wait_for(luamain)

      if is_value then
        if msg_id == 2 then 
          run = false
          break 
        elseif msg_id == 7 then break
        elseif msg_id == 11 then
          send(sendmsg,measurements,table.unpack(data))
        end
      end

      run = not coroutine.yield(0)
    until not run
    
    laser:Stop()
    join_threads({thread_profile,thread_origin,thread_dynamic,thread_send_save})

    if run then
      sleep_ms(30000)
    end
  end
end

mainco = coroutine.create(main)