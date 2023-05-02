json = require "json"

measurements = {}

function timeToString(time)
  tostring(time)
end

function out(sub, json) 
end

function mkjson(name, value, time) 
  local t = timeToString(time)
  out("bhp",json.encode({measurement = name, site = "jimblebar", conveyor = "cv001", value = value, timestamp = t}))
end

function make(now)

  local p = 5
  local m = {
    pulleyspeed = {period = p, time0 = now},
    pulleylevel = {period = p, time0 = now},
    beltlength  = {period = p, time0 = now},
    cadstoorigin = {period = p, time0 = now},
    beltedgeposition = {period = p, time0 = now},
    pulleyoscillation = {period = p, time0 = now},
    nancount = {period = p, time0 = now}
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

function send(name,value,time)
  if(measurements[name]) then
    local m = measurements[name]
    local elapsedTime = time - m.time0

    if(elapsedTime > m.period) then
      mkjson(name,value,time)
      m.time0 = time
    end

  end

end

