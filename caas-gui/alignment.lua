json = require "json"

gocatorFps = 984.0

sqlitegocatorConfig = {
  Range = {0,99999999999},
  Fps = gocatorFps,
  Forever = true,
  Delay = 98,
  Source = "../../profiles/rawprofile_cv912.db",
  TypicalSpeed = 6.0
}

laserConf = {
  Trim = true,
  TypicalResolution = 6.0
}

function timeToString(time) -- overwritten externally
  return tostring(time)
end

function main(sendmsg)

  local gocator_luamain = BlockingReaderWriterQueue()
  
  local decimate = profile_decimation(420,1000,gocator_luamain)
  --local laser = sqlitegocator(sqlitegocatorConfig,decimate) 
  local laser = gocator(laserConf,decimate) 

  laser:Start(gocatorFps)

  unloop = false
  repeat
    local is_value,msg_id,data = wait_for(gocator_luamain)

    if is_value then
      if msg_id == 2 then break 
      --elseif msg_id == 5 then break
      elseif msg_id == 10 then
        local m,profile = table.unpack(data)
        sendmsg("caas." .. DeviceSerial .. "." .. m,"",profile)
      end
    end

    unloop = coroutine.yield(0)
  until unloop

  laser:Stop()
  
end

mainco = coroutine.create(main)