json = require "json"
wNbD-eZ9RtqKCSQ9Ft63HWwn8jlfvaLKmjIFHfLEPrB_ZB2Jg-7fEAjmGl1V6Mec5g-4-7cAqh9MlMG84z0N8Q==
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
    pulleyLevel = {period = p, time0 = now}
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

