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
  local term = BlockingReaderWriterQueue()
  local gocator = mk_gocator(gocator_cads)
  local thread = process_identity(gocator_cads,term)

  gocator:Start()

  repeat
    local is_value,msg_id = wait_for(term)

    if is_value then
      print(msg_id)
      if msg_id == 1 then break end
    end

  until false

  print("stopping")
  gocator:Stop()

  join_threads({thread})
  
end