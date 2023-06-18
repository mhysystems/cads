gocator = {
  Fps = 984.0
}

conveyor = {
  Org  = "bhp",
  Site = "jimblebar",
  Name = "cv001",
  Timezone = "Australia/Perth",
  PulleyCircumference = 4178.32,
  TypicalSpeed = 6.09
}

belt = {
  Installed = "2023-01-14T00:00:00Z",
  PulleyCover = 7.0,
  CordDiameter = 9.1,
  TopCover = 14.0,
  Width = 1600,
  WidthN = 1890,
  Length = 12545200,
  LengthN = conveyor.TypicalSpeed / gocator.Fps, 
  Splices = 1
}

anomaly = {
  WindowLength = 3 * 1000, -- In mm
  BeltPartitionLength = 500 * 1000 -- In mm
}

function main()

  local gocator_cads = BlockingReaderWriterQueue()
  local ede_origin = BlockingReaderWriterQueue()
  local origin_anomaly = BlockingReaderWriterQueue()
  local anomaly_savedb = BlockingReaderWriterQueue()
  local savedb_upload = BlockingReaderWriterQueue()
  local upload_luamain = BlockingReaderWriterQueue()
  
  local hh = conveyor.TypicalSpeed / gocator.Fps
  local gocator = mk_gocator(gocator_cads) 
  local ede = encoder_distance_estimation(ede_origin,hh)
  local thread_process_profile = process_profile(gocator_cads,ede_origin)
  local window_processing = window_processing_thread(ede_origin,origin_anomaly)
  local dynamic_processing = dynamic_processing_thread(origin_anomaly,anomaly_savedb)
  local thread_send_save = save_send_thread(anomaly_savedb,savedb_upload)
  local upload_scan = upload_scan_thread(savedb_upload,upload_luamain)

  gocator:Start()

  repeat
    print("GO!!!!")
    local val = coroutine.yield(55)
    print("I am back", val)
    coroutine.yield(66)
    print("I am back2", val)
    local is_value,msg_id = wait_for(upload_luamain)

    if is_value then
      print(msg_id)
      if msg_id == 1 then break end
    end

  until false

  gocator:Stop()

  join_threads({thread_process_profile,window_processing,dynamic_processing,upload_scan})
  
end

mainco = coroutine.create(main)