gocator = {
  Fps = 1000.0
}

conveyor = {
  Org  = "gen",
  Site = "demo",
  Name = "cv001",
  Timezone = "Australia/Perth",
  PulleyCircumference = 1000.0,
  TypicalSpeed = 6.0
}

belt = {
  Installed = "2023-01-14T00:00:00Z",
  PulleyCover = 7.0,
  CordDiameter = 9.1,
  TopCover = 14.0,
  Width = 1600,
  WidthN = 1750,
  Length = 9600,
  LengthN = conveyor.TypicalSpeed / gocator.Fps, 
  Splices = 1
}

anomaly = {
  WindowLength = 20 * 6, -- In mm
  BeltPartitionLength = 500 * 6 -- In mm
}

function main()

  local gocator_cads = BlockingReaderWriterQueue()
  local ede_origin = BlockingReaderWriterQueue()
  local origin_anomaly = BlockingReaderWriterQueue()
  local anomaly_savedb = BlockingReaderWriterQueue()
  local savedb_luamain = BlockingReaderWriterQueue()
  
  local hh = conveyor.TypicalSpeed / gocator.Fps
  local gocator = mk_gocator(gocator_cads) 
  local ede = encoder_distance_estimation(ede_origin,hh)
  local thread_process_profile = process_profile(gocator_cads,ede)
  local window_processing = splice_detection_thread(ede_origin,origin_anomaly)
  local dynamic_processing = dynamic_processing_thread(origin_anomaly,anomaly_savedb)
  local thread_send_save = save_send_thread(anomaly_savedb,savedb_luamain)

  gocator:Start()

  unloop = false
  repeat
    local is_value,msg_id = wait_for(savedb_luamain)

    if is_value then
      print(msg_id)
      if msg_id == 1 then break end
    end

    unloop = coroutine.yield(0)
  until unloop

  gocator:Stop()

  join_threads({thread_process_profile,window_processing,dynamic_processing,upload_scan})
  
end

mainco = coroutine.create(main)