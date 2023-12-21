json = require "json"

gocatorFps = 984.0

sqlitegocatorConfig = {
  Range = {0,99999999999999},
  Fps = gocatorFps,
  Forever = true,
  Source = "../../profiles/rawprofile_cv311_2023-11-08.db",
  TypicalSpeed = 6.0,
  Sleep = true
}

laserConf = {
  Trim = true,
  TypicalResolution = 6.0
}


function main(sendmsg)

  local gocator_luamain = BlockingReaderWriterQueue()
  
  local decimate = profile_decimation(420,666,gocator_luamain) -- 420 should be the same as caas gui width
  local laser = sqlitegocator(sqlitegocatorConfig,decimate) 
  --local laser = gocator(laserConf,decimate) 

  if laser == nil then
    sendmsg("caas." .. DeviceSerial .. "." .. "error","","Unable to construct gocator")
    return
  end

  if laser:ResetAlign() then
    sendmsg("caas." .. DeviceSerial .. "." .. "error","","Unable to reset align")
    return
  end

  if laser:SetFoV(math.maxinteger) then
    sendmsg("caas." .. DeviceSerial .. "." .. "error","","Unable to set field of view")
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

  laser:Stop(true)
  
  if laser:SetFoV(250.0) then
    sendmsg("caas." .. DeviceSerial .. "." .. "error","","Unable to set field of view")
    return
  end

  if laser:Align() then
    sendmsg("caas." .. DeviceSerial .. "." .. "error","","Unable to align gocator")
  end
  
end

mainco = coroutine.create(main)