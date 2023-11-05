json = require "json"

gocatorFps = 984.0

sqlitegocatorConfig = {
  Range = {0,99999999999999},
  Fps = gocatorFps,
  Forever = true,
  Source = "../../profiles/rawprofile_cv001_2023-08-30.db",
  TypicalSpeed = 6.0,
  Sleep = false
}

dbscan = {
  InClusterRadius = 2.5,
  MinPoints = 11
}


function main(sendmsg)

  local gocator_luamain = BlockingReaderWriterQueue()
  local scan_only = teeMsg(
    filterMsgs(1,pulleyValues(dbscan,-30.0,true,scanZ(fileCSV("fileL.csv")))),
    filterMsgs(1,pulleyValues(dbscan,-30.0,false,scanZ(fileCSV("fileR.csv"))))
  )
  local laser = sqlitegocator(sqlitegocatorConfig,scan_only) 

  laser:Start(gocatorFps)

  unloop = false
  repeat
    local is_value,msg_id,data = wait_for(gocator_luamain)

    if is_value then
      if msg_id == 2 then 
        print("finished")
        break 
      end
    end

    unloop = coroutine.yield(0)
  until unloop

  laser:Stop(true)
  
end

mainco = coroutine.create(main)

-- gocator_properties = 0
-- scan = 1
-- finished = 2
-- begin_sequence =3 
-- end_sequence = 4
-- complete_belt = 5
-- pulley_revolution_scan = 6 
-- stopped = 7
-- nothing = 8
-- select = 9
-- caas_msg = 10
-- measure = 11
-- error = 12