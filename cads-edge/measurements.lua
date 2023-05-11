json = require "json"

measurements = {}

function timeToString(time) -- overwritten externally
  return tostring(time)
end

function out(sub, json) -- overwritten externally
end

function encode(category, msg) 
  out("bhp",category,json.encode(msg))
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
  
  measurements = m
end

function send(name,quality,time,...)

  if measurements[name] then

    local m = measurements[name]
    local elapsedTime = time - m.time0
    
    if elapsedTime >= m.period then 
    
      local msg = {
        measurement = name, 
        site = "jimblebar", 
        conveyor = "cv001", 
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

