json = require "json"

gocatorFps = 984.0

sqlitegocatorConfig = {
  Range = {169812, 2166792},
  Fps = gocatorFps,
  Forever = true,
  Source = "../../profiles/rawprofile_cv001_2.db",
  TypicalSpeed = 6.187,
  Sleep = true
}

laserConf = {
  Trim = true,
  TypicalResolution = 6.0
}


function main(sendmsg)

  local gocator_luamain = BlockingReaderWriterQueue()
  
  local decimate = profile_decimation(420,1000,gocator_luamain)
  --local laser = sqlitegocator(sqlitegocatorConfig,decimate) 
  local laser = gocator(laserConf,decimate) 

  if laser == nil then
    sendmsg("caas." .. DeviceSerial .. "." .. "error","","Unable to start gocator")
    return
  end

  if laser:Start(gocatorFps) then
    sendmsg("caas." .. DeviceSerial .. "." .. "error","","Unable to start gocator")
    return
  end

  unloop = false
  repeat
    local is_value,msg_id,data = wait_for(gocator_luamain)

    if is_value then
      if msg_id == 2 then break 
      --elseif msg_id == 5 then break
      elseif msg_id == 10 then
        local m,profile = table.unpack(data)
        sendmsg("caas." .. DeviceSerial .. "." .. m,"",profile)
      elseif msg_id == 12 then
        sendmsg("caas." .. DeviceSerial .. "." .. "error","",data)
      break
      end
    end

    unloop = coroutine.yield(0)
  until unloop

  laser:Stop()
  
  if laser:Align() then
    sendmsg("caas." .. DeviceSerial .. "." .. "error","","Unable to align gocator")
  end
  
end

mainco = coroutine.create(main)