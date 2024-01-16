json = require "json"

gocatorFps = 984.0

sqlitegocatorConfig = {
  Range = {0,99999999999999},
  Fps = gocatorFps,
  Forever = true,
  --Source = "../../profiles/rawprofile_cv001_2023-08-30.db",
  Source = "../../profiles/rawprofile_cv211_2023-11-08.db",
  TypicalSpeed = 6.0,
  Sleep = false
}

dbscan = {
  InClusterRadius = 12,
  MinPoints = 12,
  ZMergeRadius = 10,
  XMergeRadius = 50,
  MaxClusters = 2
}


function main(sendmsg)

  local gocator_luamain = BlockingReaderWriterQueue()



  local toCSV = teeMsg(
    filterMsgs(13,extractPartition(0,fileCSV("cv211L.csv"))),
    filterMsgs(13,extractPartition(2,fileCSV("cv211R.csv")))
  )

  local tee = teeMsg(
    gocator_luamain,
    toCSV
  )
  local partition_profile = partitionProfile(dbscan,tee)
  local laser = sqlitegocator(sqlitegocatorConfig,partition_profile) 

  laser:Start(gocatorFps)

  unloop = false
  repeat
    local is_value,msg_id,data = wait_for(gocator_luamain)

    if is_value then
      if msg_id == 2 then break 
      elseif msg_id == 5 then break
      elseif msg_id == 12 then
        cadsLog(data)
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
-- profile_partitioned = 13